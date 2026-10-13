#pragma once

#include "../../GameGuru/Include/cStr.h"
#include "../../Dark Basic Public Shared/Include/DarkLuaStrings.h"
#include <string>
#include <vector>
#include <string_view>
#include <memory>
#include <cstdint>
#include <filesystem>

// Lua 5.2 headers from DarkLUA
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

namespace gg_e2e
{
    // Path discovery
    std::filesystem::path GetRepoRoot();
    std::filesystem::path GetGameGuruCorePath();
    std::filesystem::path GetScriptsRootPath();

    // Vcxproj and build configuration inspection
    struct ProjectAudit
    {
        static std::vector<std::string> GetDependencies(const std::filesystem::path& vcxprojPath, const std::string& configName);
        static bool HasDependency(const std::filesystem::path& vcxprojPath, const std::string& configName, const std::string& libName);
        static bool CompilesFile(const std::filesystem::path& vcxprojPath, const std::string& filename);
        static bool SlnContainsProject(const std::filesystem::path& slnPath, const std::string& projectName);
        static bool FileExists(const std::filesystem::path& path);
    };

    // Adapters over linked production DarkLUA APIs (R4)
    class SafeLua
    {
    public:
        static std::string_view GetStringView(lua_State* L, int idx, std::string_view fallback = "");
        static void GetStringBounded(lua_State* L, const std::string& globalName, char* pDest, size_t destSize);
        static std::string GetStringSafe(lua_State* L, const std::string& globalName);
    };

    // Adapter over the linked production DarkLUA message queue (R4)
    struct SafeLuaMessage
    {
        std::string msgDesc;
        std::string msgString;
        int entityId = 0;
    };

    class SafeLuaMessageQueue
    {
    public:
        SafeLuaMessageQueue();
        ~SafeLuaMessageQueue();
        SafeLuaMessageQueue(const SafeLuaMessageQueue&) = delete;
        SafeLuaMessageQueue& operator=(const SafeLuaMessageQueue&) = delete;
        void Push(const SafeLuaMessage& msg);
        bool Pop(SafeLuaMessage& outMsg);
        size_t Size() const;
        void Clear();

    };

    // RAII Lua State Helper
    class ScopedLuaState
    {
    public:
        ScopedLuaState();
        ~ScopedLuaState();
        ScopedLuaState(const ScopedLuaState&) = delete;
        ScopedLuaState& operator=(const ScopedLuaState&) = delete;
        lua_State* Get() const { return L; }
        operator lua_State*() const { return L; }
        bool RunString(const std::string& code, std::string* pErr = nullptr);
    private:
        lua_State* L = nullptr;
    };
}
