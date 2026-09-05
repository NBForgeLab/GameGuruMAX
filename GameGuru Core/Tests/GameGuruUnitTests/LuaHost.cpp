#include "LuaHost.h"

#include <windows.h>

#include <ctime>
#include <cstdio>
#include <filesystem>

// The game routes Lua's script file access through its own GG_fopen hook
// (defined inside the DarkLUA game library). The test host reads repository
// files directly, which is exactly the behavior under test.
extern "C" FILE* GG_fopen(const char* filename, const char* mode)
{
	return fopen(filename, mode);
}

namespace ggtest
{
	namespace
	{
		RayCastRecord g_lastRayCast;
		int g_duckingState = 0;

		int Stub_IntersectAll(lua_State* L)
		{
			g_lastRayCast.callCount++;
			g_lastRayCast.startX = luaL_checknumber(L, 1);
			g_lastRayCast.startY = luaL_checknumber(L, 2);
			g_lastRayCast.startZ = luaL_checknumber(L, 3);
			g_lastRayCast.endX = luaL_checknumber(L, 4);
			g_lastRayCast.endY = luaL_checknumber(L, 5);
			g_lastRayCast.endZ = luaL_checknumber(L, 6);
			g_lastRayCast.ignore = (int)luaL_optinteger(L, 7, 0);
			lua_pushinteger(L, g_lastRayCast.result);
			return 1;
		}

		int Stub_GetGamePlayerStatePlayerDucking(lua_State* L)
		{
			lua_pushinteger(L, g_duckingState);
			return 1;
		}

		int Stub_Prompt(lua_State* L)
		{
			return 0;
		}

		int Stub_PromptLocal(lua_State* L)
		{
			return 0;
		}
	}

	void SetNextRayCastResult(int objectID)
	{
		g_lastRayCast.result = objectID;
	}

	RayCastRecord LastRayCast()
	{
		return g_lastRayCast;
	}

	void SetDuckingState(int ducked)
	{
		g_duckingState = ducked;
	}

	void ResetRayCastCount()
	{
		g_lastRayCast.callCount = 0;
	}

	LuaHost::LuaHost()
	{
		m_L = luaL_newstate();
		if (m_L == nullptr)
		{
			m_lastError = "luaL_newstate failed";
			return;
		}

		// Standard libraries opened for the product scripts: the package
		// library because scripts load each other with require(), and the
		// io library because the master interpreter's behavior-bytecode
		// loader (a file parser under test) reads files with io.open. No OS
		// or C module loading.
		luaL_requiref(m_L, "_G", luaopen_base, 1);
		luaL_requiref(m_L, LUA_COLIBNAME, luaopen_coroutine, 1);
		luaL_requiref(m_L, LUA_MATHLIBNAME, luaopen_math, 1);
		luaL_requiref(m_L, LUA_STRLIBNAME, luaopen_string, 1);
		luaL_requiref(m_L, LUA_TABLIBNAME, luaopen_table, 1);
		luaL_requiref(m_L, LUA_LOADLIBNAME, luaopen_package, 1);
		luaL_requiref(m_L, LUA_IOLIBNAME, luaopen_io, 1);
		lua_pop(m_L, 7);

		// Route require("scriptbank\...") to the product's script banks:
		const std::string scriptsRoot = FindScriptsRoot();
		if (!scriptsRoot.empty())
		{
			lua_getglobal(m_L, "package");
			if (lua_istable(m_L, -1))
			{
				lua_pushstring(m_L, (scriptsRoot + "/?.lua").c_str());
				lua_setfield(m_L, -2, "path");
			}
			lua_pop(m_L, 1);
		}

		// Engine entry points referenced by product scripts:
		lua_pushcfunction(m_L, Stub_IntersectAll);
		lua_setglobal(m_L, "IntersectAll");
		lua_pushcfunction(m_L, Stub_GetGamePlayerStatePlayerDucking);
		lua_setglobal(m_L, "GetGamePlayerStatePlayerDucking");
		lua_pushcfunction(m_L, Stub_Prompt);
		lua_setglobal(m_L, "Prompt");
		lua_pushcfunction(m_L, Stub_PromptLocal);
		lua_setglobal(m_L, "PromptLocal");

		// The engine's entity table starts out empty:
		lua_newtable(m_L);
		lua_setglobal(m_L, "g_Entity");

		// Minimal os table: product scripts seed their randomizers with
		// os.time(); expose only the two harmless time functions.
		lua_newtable(m_L);
		lua_pushcfunction(m_L, [](lua_State* L) -> int {
			lua_pushinteger(L, (lua_Integer)time(nullptr));
			return 1;
		});
		lua_setfield(m_L, -2, "time");
		lua_pushcfunction(m_L, [](lua_State* L) -> int {
			lua_pushnumber(L, (lua_Number)clock() / CLOCKS_PER_SEC);
			return 1;
		});
		lua_setfield(m_L, -2, "clock");
		lua_setglobal(m_L, "os");
	}

	LuaHost::~LuaHost()
	{
		if (m_L != nullptr)
		{
			lua_close(m_L);
			m_L = nullptr;
		}
	}

	bool LuaHost::LoadScriptAs(const char* globalName, const std::string& path)
	{
		if (m_L == nullptr)
		{
			return false;
		}
		if (luaL_dofile(m_L, path.c_str()) != LUA_OK)
		{
			const char* message = lua_tostring(m_L, -1);
			m_lastError = message != nullptr ? message : "unknown Lua error";
			lua_pop(m_L, 1);
			return false;
		}
		lua_setglobal(m_L, globalName);
		m_lastError.clear();
		return true;
	}

	bool LuaHost::PushModuleFunction(const char* globalName, const char* functionName)
	{
		lua_getglobal(m_L, globalName);
		if (!lua_istable(m_L, -1))
		{
			lua_pop(m_L, 1);
			m_lastError = std::string(globalName) + " was not loaded";
			return false;
		}
		lua_getfield(m_L, -1, functionName);
		if (!lua_isfunction(m_L, -1))
		{
			lua_pop(m_L, 2);
			m_lastError = std::string(globalName) + "." + functionName + " does not exist";
			return false;
		}
		lua_remove(m_L, -2);
		m_lastError.clear();
		return true;
	}

	namespace LuaHostExtensions
	{
		// Body of every generated auto-stub: count the call in GGAUTO_CALLS
		// and return 0 (benign for arithmetic on the caller's side).
		int AutoStubCall(lua_State* L)
		{
			const char* name = lua_tostring(L, lua_upvalueindex(1));
			lua_getglobal(L, "GGAUTO_CALLS");
			if (lua_istable(L, -1))
			{
				lua_pushstring(L, name);
				lua_rawget(L, -2);
				const int n = (int)lua_tointeger(L, -1);
				lua_pop(L, 1);
				lua_pushstring(L, name);
				lua_pushinteger(L, n + 1);
				lua_rawset(L, -3);
			}
			lua_pop(L, 1);
			lua_pushinteger(L, 0);
			return 1;
		}

		// __index metamethod for _G: missing globals become recording stubs.
		int AutoStubIndex(lua_State* L)
		{
			lua_pushvalue(L, 2); // keep the key alive as the closure's upvalue
			lua_pushcclosure(L, AutoStubCall, 1);
			return 1;
		}
	}

	void LuaHostExtensions::InstallAutoStubs(lua_State* L)
	{
		if (L == nullptr)
		{
			return;
		}
		lua_newtable(L); // GGAUTO_CALLS: name -> call count
		lua_setglobal(L, "GGAUTO_CALLS");
		lua_newtable(L);                       // [mt]
		lua_pushcfunction(L, AutoStubIndex);   // [mt, fn]
		lua_setfield(L, -2, "__index");        // [mt]
		lua_pushglobaltable(L);                // [mt, _G]
		lua_pushvalue(L, -2);                  // [mt, _G, mt]
		lua_setmetatable(L, -2);               // [_G] (mt copy consumed, applied to _G)
		lua_pop(L, 2);                         // []
	}

	int LuaHostExtensions::AutoCallCount(lua_State* L, const char* functionName)
	{
		if (L == nullptr)
		{
			return 0;
		}
		lua_getglobal(L, "GGAUTO_CALLS");
		if (!lua_istable(L, -1))
		{
			lua_pop(L, 1);
			return 0;
		}
		lua_pushstring(L, functionName);
		lua_rawget(L, -2);
		const int n = (int)lua_tointeger(L, -1);
		lua_pop(L, 2);
		return n;
	}

	std::string FindScriptsRoot()
	{
		// Explicit override wins:
		char envBuffer[MAX_PATH] = {};
		if (GetEnvironmentVariableA("GGMAX_SCRIPTS_ROOT", envBuffer, MAX_PATH) > 0)
		{
			return envBuffer;
		}

		namespace fs = std::filesystem;
		char exePath[MAX_PATH] = {};
		GetModuleFileNameA(nullptr, exePath, MAX_PATH);
		fs::path dir = fs::path(exePath).parent_path();

		for (int depth = 0; depth < 10 && !dir.empty(); ++depth)
		{
			const fs::path candidate = dir / "Scripts" / "scriptbank" / "utillib.lua";
			if (fs::exists(candidate))
			{
				return (dir / "Scripts").string();
			}
			dir = dir.parent_path();
		}
		return "";
	}
}
