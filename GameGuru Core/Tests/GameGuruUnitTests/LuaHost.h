#pragma once
// Minimal headless host for the product's Lua 5.2 scripts (the same Lua
// source that DarkLUA ships to the game, embedded directly from the vendored
// DarkLUA\lua sources). Product scripts expect engine-provided globals; this
// host registers small, faithful stubs for the handful of engine entry points
// the tested scripts reference, so scripts load and run without the engine.
//
// The stubs record their calls so tests can assert on how a script uses the
// engine API (for example which ray it casts and what eye height it assumes).

#include <string>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

namespace ggtest
{
	// Arguments captured from the most recent IntersectAll() stub call.
	struct RayCastRecord
	{
		int callCount = 0;
		double startX = 0, startY = 0, startZ = 0;
		double endX = 0, endY = 0, endZ = 0;
		int ignore = 0;
		int result = 0;
	};

	// Value returned by the IntersectAll() stub on the next call.
	void SetNextRayCastResult(int objectID);
	RayCastRecord LastRayCast();
	// Zeroes the recorded call count so tests can assert call deltas.
	void ResetRayCastCount();

	// What the GetGamePlayerStatePlayerDucking() stub reports (0 = standing).
	void SetDuckingState(int ducked);

	class LuaHost
	{
	public:
		LuaHost();
		~LuaHost();
		LuaHost(const LuaHost&) = delete;
		LuaHost& operator=(const LuaHost&) = delete;

		lua_State* state() { return m_L; }

		// Loads a script chunk and stores its return value (usually the
		// module table) as a global, so tests can call its functions.
		bool LoadScriptAs(const char* globalName, const std::string& path);

		// Pushes globalName.<functionName> onto the stack, or returns false
		// when the module or function does not exist.
		bool PushModuleFunction(const char* globalName, const char* functionName);

		const std::string& GetLastError() const { return m_lastError; }

	private:
		lua_State* m_L = nullptr;
		std::string m_lastError;
	};

	// Locates the product's "Scripts" directory by walking up from the test
	// executable, or from GGMAX_SCRIPTS_ROOT when the environment variable
	// is set. Returns an empty string when not found.
	std::string FindScriptsRoot();
}
