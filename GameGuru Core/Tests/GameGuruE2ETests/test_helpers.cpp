#include "test_helpers.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <atomic>

// DarkLUA file access hook
extern "C" FILE* GG_fopen(const char* filename, const char* mode)
{
    return fopen(filename, mode);
}

extern "C" FILE* GG_wfopen(const wchar_t* filename, const wchar_t* mode)
{
    return _wfopen(filename, mode);
}

int ImGui_Get_Multi_Viewports_Disabled()
{
    return 0;
}

namespace gg_e2e
{
    std::filesystem::path GetRepoRoot()
    {
        namespace fs = std::filesystem;
        fs::path cur = fs::current_path();
        for (int i = 0; i < 10; ++i)
        {
            if (fs::exists(cur / "GameGuruMAX") || fs::exists(cur / "PROJECT.md"))
            {
                return cur;
            }
            if (!cur.has_parent_path() || cur == cur.parent_path()) break;
            cur = cur.parent_path();
        }
        return fs::path("d:/GitHub-repo/GameGuruMAXRepo");
    }

    std::filesystem::path GetGameGuruCorePath()
    {
        return GetRepoRoot() / "GameGuruMAX" / "GameGuru Core";
    }

    std::filesystem::path GetScriptsRootPath()
    {
        return GetRepoRoot() / "GameGuruMAX" / "Scripts";
    }

    std::vector<std::string> ProjectAudit::GetDependencies(const std::filesystem::path& vcxprojPath, const std::string& configName)
    {
        std::vector<std::string> deps;
        if (!std::filesystem::exists(vcxprojPath)) return deps;

        std::ifstream file(vcxprojPath);
        if (!file.is_open()) return deps;

        std::string line;
        bool inConfig = false;
        while (std::getline(file, line))
        {
            if (line.find("<ItemDefinitionGroup") != std::string::npos && line.find(configName) != std::string::npos)
            {
                inConfig = true;
            }
            else if (inConfig && line.find("</ItemDefinitionGroup>") != std::string::npos)
            {
                inConfig = false;
            }

            if (inConfig)
            {
                size_t depStart = line.find("<AdditionalDependencies>");
                if (depStart != std::string::npos)
                {
                    depStart += 24; // length of tag
                    size_t depEnd = line.find("</AdditionalDependencies>", depStart);
                    if (depEnd != std::string::npos)
                    {
                        std::string raw = line.substr(depStart, depEnd - depStart);
                        std::stringstream ss(raw);
                        std::string item;
                        while (std::getline(ss, item, ';'))
                        {
                            // trim
                            size_t first = item.find_first_not_of(" \t\r\n");
                            size_t last = item.find_last_not_of(" \t\r\n");
                            if (first != std::string::npos && last != std::string::npos)
                            {
                                deps.push_back(item.substr(first, last - first + 1));
                            }
                        }
                    }
                }
            }
        }
        return deps;
    }

    bool ProjectAudit::HasDependency(const std::filesystem::path& vcxprojPath, const std::string& configName, const std::string& libName)
    {
        auto deps = GetDependencies(vcxprojPath, configName);
        std::string targetLower = libName;
        std::transform(targetLower.begin(), targetLower.end(), targetLower.begin(), ::tolower);

        for (const auto& dep : deps)
        {
            std::string curLower = dep;
            std::transform(curLower.begin(), curLower.end(), curLower.begin(), ::tolower);
            if (curLower == targetLower || curLower.find(targetLower) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    bool ProjectAudit::CompilesFile(const std::filesystem::path& vcxprojPath, const std::string& filename)
    {
        if (!std::filesystem::exists(vcxprojPath)) return false;
        std::ifstream file(vcxprojPath);
        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("<ClCompile Include=") != std::string::npos && line.find(filename) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    bool ProjectAudit::SlnContainsProject(const std::filesystem::path& slnPath, const std::string& projectName)
    {
        if (!std::filesystem::exists(slnPath)) return false;
        std::ifstream file(slnPath);
        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("Project(") != std::string::npos && line.find("\"" + projectName + "\"") != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    bool ProjectAudit::FileExists(const std::filesystem::path& path)
    {
        return std::filesystem::exists(path);
    }

    // Thin adapters call the production APIs linked into this test executable.
    std::string_view SafeLua::GetStringView(lua_State* L, int idx, std::string_view fallback)
    { return LuaGetStringView(L, idx, fallback); }
    std::string SafeLua::GetStringSafe(lua_State* L, const std::string& name)
    { return LuaGetStringSafe(L, name); }
    void SafeLua::GetStringBounded(lua_State* L, const std::string& name, char* dest, size_t size)
    { LuaGetString(L, name, dest, size); }
    SafeLuaMessageQueue::SafeLuaMessageQueue() { LuaClearMessageQueue(); }
    SafeLuaMessageQueue::~SafeLuaMessageQueue() { LuaClearMessageQueue(); }
    void SafeLuaMessageQueue::Push(const SafeLuaMessage& msg)
    {
        ScopedLuaState state;
        lua_pushlstring(state, msg.msgDesc.data(), msg.msgDesc.size());
        lua_pushinteger(state, msg.entityId);
        lua_pushlstring(state, msg.msgString.data(), msg.msgString.size());
        LuaSendMessageS(state);
    }
    bool SafeLuaMessageQueue::Pop(SafeLuaMessage& msg)
    {
        if (!LuaNext()) return false;
        msg.msgDesc = std::string(LuaCurrentMessageDescription());
        msg.msgString = std::string(LuaCurrentMessageString());
        msg.entityId = LuaCurrentMessageIndex();
        return true;
    }
    size_t SafeLuaMessageQueue::Size() const { return LuaMessageQueueSize(); }
    void SafeLuaMessageQueue::Clear() { LuaClearMessageQueue(); }

    // Scoped Lua State
    ScopedLuaState::ScopedLuaState()
    {
        L = luaL_newstate();
        if (L)
        {
            luaL_requiref(L, "_G", luaopen_base, 1);
            lua_pop(L, 1);
            luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, 1);
            lua_pop(L, 1);
            luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
            lua_pop(L, 1);
            luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
            lua_pop(L, 1);
            luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
            lua_pop(L, 1);
            luaL_requiref(L, LUA_LOADLIBNAME, luaopen_package, 1);
            lua_pop(L, 1);
            luaL_requiref(L, LUA_IOLIBNAME, luaopen_io, 1);
            lua_pop(L, 1);
            luaL_requiref(L, LUA_BITLIBNAME, luaopen_bit32, 1);
            lua_pop(L, 1);
            luaL_requiref(L, LUA_OSLIBNAME, luaopen_os, 1);
            lua_pop(L, 1);
        }
    }

    ScopedLuaState::~ScopedLuaState()
    {
        if (L)
        {
            lua_close(L);
            L = nullptr;
        }
    }

    bool ScopedLuaState::RunString(const std::string& code, std::string* pErr)
    {
        if (!L) return false;
        int status = luaL_dostring(L, code.c_str());
        if (status != 0)
        {
            if (pErr && lua_isstring(L, -1))
            {
                *pErr = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
            return false;
        }
        return true;
    }
}
