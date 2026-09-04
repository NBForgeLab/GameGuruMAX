// Behavior tests for the product's Master Interpreter
// (Scripts\scriptbank\masterinterpreter.lua) — the visual behavior engine
// that drives every entity in shipped games. Two contracts are covered:
//
//   1. The behavior bytecode loader (masterinterpreter_load): parses the
//      compiled behavior file format (magic number 42, versions 101/102,
//      states with interrupt flags, instruction list) that the editor
//      exports for every behavior.
//   2. The condition evaluator (masterinterpreter_getconditionresult): the
//      decision brain mapping condition types to boolean results from
//      entity and behavior state. Only conditions whose evaluation is pure
//      state math are exercised; ray/visibility conditions are stubbed.
//
// The condition id constants are read from the script's own globals so the
// tests cannot silently drift from the shipped numbering.
#include "doctest.h"

#include "LuaHost.h"

#include <windows.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
	// Engine stubs the interpreter calls; each records or computes from the
	// state the tests set up.
	int g_timerMs = 0;

	int Stub_Timer(lua_State* L)
	{
		lua_pushnumber(L, g_timerMs);
		return 1;
	}
	int Stub_GetTimer(lua_State* L)
	{
		lua_pushnumber(L, g_timerMs);
		return 1;
	}
	int Stub_GetEntityViewRange(lua_State* L)
	{
		lua_pushnumber(L, 2000);
		return 1;
	}
	int Stub_RayTerrain(lua_State* L)
	{
		lua_pushnumber(L, 0); // no terrain hit
		return 1;
	}
	int Stub_IntersectStaticPerformant(lua_State* L)
	{
		lua_pushnumber(L, 0); // no object hit
		return 1;
	}
	int Stub_GetDistanceTo(lua_State* L)
	{
		// Faithful behavior: distance from the entity to the given point.
		const int e = (int)luaL_checkinteger(L, 1);
		const double x = luaL_checknumber(L, 2);
		const double y = luaL_checknumber(L, 3);
		const double z = luaL_checknumber(L, 4);

		lua_getglobal(L, "g_Entity");
		lua_pushinteger(L, e);
		lua_gettable(L, -2);
		double ex = 0, ey = 0, ez = 0;
		if (lua_istable(L, -1))
		{
			lua_getfield(L, -1, "x"); ex = lua_tonumber(L, -1); lua_pop(L, 1);
			lua_getfield(L, -1, "y"); ey = lua_tonumber(L, -1); lua_pop(L, 1);
			lua_getfield(L, -1, "z"); ez = lua_tonumber(L, -1); lua_pop(L, 1);
		}
		lua_pop(L, 2);

		const double dx = ex - x, dy = ey - y, dz = ez - z;
		lua_pushnumber(L, sqrt(dx * dx + dy * dy + dz * dz));
		return 1;
	}

	// Runs a protected call and surfaces the Lua error message on failure.
	bool ProtectedPCall(lua_State* L, int nargs, int nresults)
	{
		const int rc = lua_pcall(L, nargs, nresults, 0);
		if (rc != LUA_OK)
		{
			std::cout << "LUAERR: " << (lua_isstring(L, -1) ? lua_tostring(L, -1) : "?") << std::endl;
			lua_pop(L, 1);
		}
		return rc == LUA_OK;
	}

	void RegisterInterpreterStubs(lua_State* L)
	{
		struct Entry { const char* name; lua_CFunction fn; };
		const Entry entries[] = {
			{ "Timer", Stub_Timer },
			{ "GetTimer", Stub_GetTimer },
			{ "GetEntityViewRange", Stub_GetEntityViewRange },
			{ "RayTerrain", Stub_RayTerrain },
			{ "IntersectStaticPerformant", Stub_IntersectStaticPerformant },
			{ "GetDistanceTo", Stub_GetDistanceTo },
		};
		for (const Entry& e : entries)
		{
			lua_pushcfunction(L, e.fn);
			lua_setglobal(L, e.name);
		}
	}

	// Reads a numeric global constant out of the loaded interpreter script.
	int InterpreterConstant(ggtest::LuaHost& host, const char* name)
	{
		lua_getglobal(host.state(), name);
		const int value = (int)lua_tointeger(host.state(), -1);
		lua_pop(host.state(), 1);
		return value;
	}

	// Places entity 10 with the given position and health in g_Entity.
	void PlaceEntity(ggtest::LuaHost& host, double x, double y, double z, int health)
	{
		lua_State* L = host.state();
		lua_getglobal(L, "g_Entity");
		lua_pushinteger(L, 10);
		lua_newtable(L);
		lua_pushnumber(L, x);      lua_setfield(L, -2, "x");
		lua_pushnumber(L, y);      lua_setfield(L, -2, "y");
		lua_pushnumber(L, z);      lua_setfield(L, -2, "z");
		lua_pushinteger(L, health); lua_setfield(L, -2, "health");
		lua_pushinteger(L, 100);   lua_setfield(L, -2, "obj");
		lua_pushnumber(L, 0);      lua_setfield(L, -2, "avoid");
		lua_settable(L, -3);
		lua_pop(L, 1);
	}

	// Calls the global condition evaluator: result is 0 or 1, and any
	// mutations the evaluator makes land in the output_e table passed in
	// (rebuilt fresh by the caller per case).
	int CallCondition(ggtest::LuaHost& host, int e, int conditionType, const char* param)
	{
		lua_State* L = host.state();
		lua_getglobal(L, "masterinterpreter_getconditionresult");
		lua_pushinteger(L, e);
		lua_newtable(L); // output_e
		lua_pushinteger(L, conditionType);
		lua_pushstring(L, param);
		REQUIRE(ProtectedPCall(L, 4, 1));
		const int result = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);
		return result;
	}

	// Same, but the output_e table is pre-populated from key/value string
	// pairs so tests can drive behavior state and read mutations back.
	// Numeric values are pushed as numbers: the interpreter compares this
	// state numerically (damagetaken, targete, lasttimerreached, ...).
	void PushOutputEntry(lua_State* L, const char* key, const char* value)
	{
		lua_pushstring(L, key);
		char* end = nullptr;
		const double number = strtod(value, &end);
		if (end != value && *end == 0)
		{
			lua_pushnumber(L, number);
		}
		else
		{
			lua_pushstring(L, value);
		}
		lua_settable(L, -3);
	}

	// Writes the product's behavior bytecode format to a temp file.
	std::string WriteBytecodeFile(const char* versionLine, bool includeDamageState)
	{
		char temp[MAX_PATH] = {};
		GetTempPathA(MAX_PATH, temp);
		static unsigned counter = 0;
		const std::string path = std::string(temp) + "gg_behavior_test_" +
			std::to_string(++counter) + ".byc";

		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		file << "42\n";             // magic number
		file << versionLine << "\n"; // version (101 / 102)
		file << "2\n";              // two states
		file << "4\n";              // four instructions
		file << "PatrolState\n1\n"; // state 0: name + allow-interrupt
		file << "AttackState\n0\n"; // state 1: no interrupt
		file << "---\n";            // state separator
		for (int i = 0; i < 4; ++i)
		{
			file << (100 + i) << "\n";   // uniquecode
			file << "1\n";               // superstate (state 0)
			file << "11\n";              // conditiontype (cond_always)
			file << "0\n";               // conditionparam1
			file << "0\n";               // conditionparam2
			file << "5\n";               // actiontype
			file << "10\n";              // actionparam1
			file << "20\n";              // actionparam2
			file << (i + 1) << "\n";     // actionnewbehaviorindex (raw, -1 applied)
			file << (i + 2) << "\n";     // elsebehaviorindex (raw, -1 applied)
			file << "---\n";             // instruction separator
		}
		if (includeDamageState)
		{
			file << "3\n";              // damage behavior index (raw, -1 applied)
		}
		return path;
	}
}

struct MasterInterpreterFixture
{
	ggtest::LuaHost host;

	MasterInterpreterFixture()
	{
		g_timerMs = 0;
		RegisterInterpreterStubs(host.state());
		const std::string scriptsRoot = ggtest::FindScriptsRoot();
		REQUIRE(scriptsRoot.size() > 0);
		// utillib is not needed here; the interpreter loads standalone:
		REQUIRE(host.LoadScriptAs("MASTERINT", scriptsRoot + "/scriptbank/masterinterpreter.lua"));
		// The behavior store is created by the running game and passed INTO
		// the loader as its second argument (the tests do the same).
		// And the entity database the interpreter reads:
		PlaceEntity(host, 0, 0, 0, 40);
	}
};

TEST_CASE("Master interpreter loads and exposes its condition constants")
{
	MasterInterpreterFixture f;
	// The condition numbering is a stored-content contract: compiled
	// behaviors embed these ids, so the shipped numbering must not drift.
	CHECK(InterpreterConstant(f.host, "g_masterinterpreter_cond_always") == 11);
	CHECK(InterpreterConstant(f.host, "g_masterinterpreter_cond_targetwithin") == 12);
	CHECK(InterpreterConstant(f.host, "g_masterinterpreter_cond_checkhealth") == 19);
}

TEST_CASE("Behavior bytecode loader rejects files without the magic number")
{
	MasterInterpreterFixture f;

	const std::string badPath = WriteBytecodeFile("101", false);
	{
		std::ofstream file(badPath, std::ios::binary | std::ios::trunc);
		file << "41\n"; // wrong magic
		file << "101\n0\n0\n";
	}

	lua_State* L = f.host.state();
	lua_getglobal(L, "MASTERINT");
	lua_getfield(L, -1, "masterinterpreter_load");
	lua_newtable(L);                       // output_e
	PushOutputEntry(L, "bycfilename", badPath.c_str());
	lua_newtable(L);                       // behavior store (2nd argument)
	REQUIRE(ProtectedPCall(L, 2, 1));
	CHECK(lua_tointeger(L, -1) == -1); // nothing parsed
	lua_pop(L, 2);
	std::remove(badPath.c_str());
}

TEST_CASE("Behavior bytecode loader parses states, instructions and interrupt flags")
{
	MasterInterpreterFixture f;

	const std::string path = WriteBytecodeFile("101", false);
	lua_State* L = f.host.state();
	lua_getglobal(L, "MASTERINT");
	lua_getfield(L, -1, "masterinterpreter_load");
	lua_newtable(L);                       // output_e
	PushOutputEntry(L, "bycfilename", path.c_str());
	lua_newtable(L);                       // behavior store (2nd argument)
	lua_setglobal(L, "TEST_BEHAVIOR");
	lua_getglobal(L, "TEST_BEHAVIOR");
	REQUIRE(ProtectedPCall(L, 2, 1));
	CHECK(lua_tointeger(L, -1) == 3); // four instructions minus the -1 offset
	lua_pop(L, 2);

	// Instruction count global:
	lua_getglobal(L, "g_myscript_behavior_count");
	CHECK(lua_tointeger(L, -1) == 3);
	lua_pop(L, 1);

	// First instruction fields (params stay strings, ids become numbers):
	auto CheckIntField = [&](const char* field, int expected) {
		lua_getfield(L, -1, field);
		CHECK(lua_tointeger(L, -1) == expected);
		lua_pop(L, 1);
	};
	auto CheckStringField = [&](const char* field, const char* expected) {
		lua_getfield(L, -1, field);
		CHECK(std::string(lua_tostring(L, -1)) == expected);
		lua_pop(L, 1);
	};

	lua_getglobal(L, "TEST_BEHAVIOR");
	lua_pushinteger(L, 0);
	lua_gettable(L, -2);
	REQUIRE(lua_istable(L, -1));
	CheckIntField("uniquecode", 100);
	CheckIntField("conditiontype", 11);
	CheckStringField("conditionparam1", "0");
	CheckStringField("actionparam2", "20");
	CheckIntField("actionnewbehaviorindex", 0);  // 1 - 1
	CheckIntField("elsebehaviorindex", 1);       // 2 - 1
	CheckIntField("caninterupt", 1);             // state 0 allows interrupts
	lua_pop(L, 1);                               // instruction table

	// Last instruction points at state 0 too:
	lua_getglobal(L, "TEST_BEHAVIOR");
	lua_pushinteger(L, 3);
	lua_gettable(L, -2);
	REQUIRE(lua_istable(L, -1));
	CheckIntField("uniquecode", 103);
	CheckIntField("caninterupt", 1);
	lua_pop(L, 2);                               // instruction + store

	std::remove(path.c_str());
}

TEST_CASE("Behavior bytecode loader parses the v102 damage state index")
{
	MasterInterpreterFixture f;

	const std::string path = WriteBytecodeFile("102", true);
	lua_State* L = f.host.state();
	lua_getglobal(L, "MASTERINT");
	lua_getfield(L, -1, "masterinterpreter_load");
	lua_newtable(L);                       // output_e
	PushOutputEntry(L, "bycfilename", path.c_str());
	lua_setglobal(L, "TEST_OUTPUT");       // keep a reference for later reads
	lua_getglobal(L, "TEST_OUTPUT");
	lua_newtable(L);                       // behavior store (2nd argument)
	REQUIRE(ProtectedPCall(L, 2, 1));
	CHECK(lua_tointeger(L, -1) == 3);
	lua_pop(L, 1);                         // result

	// The parsed damage state index is stored back into output_e (3 - 1),
	// after being initialized to -1 before the load:
	lua_getglobal(L, "TEST_OUTPUT");
	lua_getfield(L, -1, "damagebehaviorindex");
	CHECK(lua_tointeger(L, -1) == 2);
	lua_pop(L, 2);                         // field value + output_e

	// The count global is written by the loader as well:
	lua_getglobal(L, "g_myscript_behavior_count");
	CHECK(lua_tointeger(L, -1) == 3);
	lua_pop(L, 1);

	std::remove(path.c_str());
}

TEST_CASE("Condition evaluator answers the always and unknown conditions")
{
	MasterInterpreterFixture f;
	const int always = InterpreterConstant(f.host, "g_masterinterpreter_cond_always");

	CHECK(CallCondition(f.host, 10, always, "0") == 1);
	CHECK(CallCondition(f.host, 10, 9999, "0") == 0);
}

TEST_CASE("Condition evaluator resolves plain and =variable parameters")
{
	MasterInterpreterFixture f;
	const int checkDamage = InterpreterConstant(f.host, "g_masterinterpreter_cond_checkdamage");
	lua_State* L = f.host.state();

	// Plain numeric parameter:
	lua_getglobal(L, "masterinterpreter_getconditionresult");
	lua_pushinteger(L, 10);
	lua_newtable(L);
	PushOutputEntry(L, "damagetaken", "60");
	lua_pushinteger(L, checkDamage);
	lua_pushstring(L, "50");
	REQUIRE(lua_pcall(L, 4, 1, 0) == LUA_OK);
	CHECK(lua_tointeger(L, -1) == 1); // 60 damage > 50 threshold
	lua_pop(L, 1);

	lua_getglobal(L, "masterinterpreter_getconditionresult");
	lua_pushinteger(L, 10);
	lua_newtable(L);
	PushOutputEntry(L, "damagetaken", "60");
	lua_pushinteger(L, checkDamage);
	lua_pushstring(L, "70");
	REQUIRE(lua_pcall(L, 4, 1, 0) == LUA_OK);
	CHECK(lua_tointeger(L, -1) == 0); // 60 damage is not > 70
	lua_pop(L, 1);

	// "=name" parameters read the behavior variable from output_e:
	lua_getglobal(L, "masterinterpreter_getconditionresult");
	lua_pushinteger(L, 10);
	lua_newtable(L);
	PushOutputEntry(L, "damagetaken", "60");
	PushOutputEntry(L, "mythreshold", "50");
	lua_pushinteger(L, checkDamage);
	lua_pushstring(L, "=mythreshold");
	REQUIRE(lua_pcall(L, 4, 1, 0) == LUA_OK);
	CHECK(lua_tointeger(L, -1) == 1);
	lua_pop(L, 1);
}

TEST_CASE("Condition evaluator checks entity health against the threshold")
{
	MasterInterpreterFixture f; // entity 10 has health 40
	const int checkHealth = InterpreterConstant(f.host, "g_masterinterpreter_cond_checkhealth");

	CHECK(CallCondition(f.host, 10, checkHealth, "50") == 1);  // 40 < 50
	CHECK(CallCondition(f.host, 10, checkHealth, "30") == 0);  // 40 not < 30
}

TEST_CASE("Condition evaluator isvaluezero tests the parameter itself")
{
	MasterInterpreterFixture f;
	const int isZero = InterpreterConstant(f.host, "g_masterinterpreter_cond_isvaluezero");

	CHECK(CallCondition(f.host, 10, isZero, "0") == 1);
	CHECK(CallCondition(f.host, 10, isZero, "5") == 0);
}

TEST_CASE("Condition evaluator timers fire once per threshold")
{
	MasterInterpreterFixture f;
	const int checkTimer = InterpreterConstant(f.host, "g_masterinterpreter_cond_checktimer");
	lua_State* L = f.host.state();

	g_timerMs = 1500;

	// First evaluation above the threshold fires:
	lua_getglobal(L, "masterinterpreter_getconditionresult");
	lua_pushinteger(L, 10);
	lua_newtable(L);
	PushOutputEntry(L, "lasttimerreached", "0");
	lua_pushinteger(L, checkTimer);
	lua_pushstring(L, "1000");
	REQUIRE(lua_pcall(L, 4, 1, 0) == LUA_OK);
	CHECK(lua_tointeger(L, -1) == 1);
	lua_pop(L, 1);

	// Re-evaluating the same threshold does not fire again, but a higher
	// threshold still can:
	lua_getglobal(L, "masterinterpreter_getconditionresult");
	lua_pushinteger(L, 10);
	lua_newtable(L);
	PushOutputEntry(L, "lasttimerreached", "1000");
	lua_pushinteger(L, checkTimer);
	lua_pushstring(L, "1000");
	REQUIRE(lua_pcall(L, 4, 1, 0) == LUA_OK);
	CHECK(lua_tointeger(L, -1) == 0);
	lua_pop(L, 1);

	lua_getglobal(L, "masterinterpreter_getconditionresult");
	lua_pushinteger(L, 10);
	lua_newtable(L);
	PushOutputEntry(L, "lasttimerreached", "1000");
	lua_pushinteger(L, checkTimer);
	lua_pushstring(L, "2000");
	REQUIRE(lua_pcall(L, 4, 1, 0) == LUA_OK);
	CHECK(lua_tointeger(L, -1) == 0); // timer (1500) below the threshold
	lua_pop(L, 1);
}

TEST_CASE("Condition evaluator measures target distance for within and beyond")
{
	MasterInterpreterFixture f;
	// Entity 10 stands at the origin; the behavior's start position sits
	// 100 units south. GetDistanceTo computes the real distance.
	const int within = InterpreterConstant(f.host, "g_masterinterpreter_cond_targetwithin");
	const int beyond = InterpreterConstant(f.host, "g_masterinterpreter_cond_targetbeyond");
	lua_State* L = f.host.state();

	auto CallWithStart = [&](int conditionType, const char* param) -> int {
		lua_getglobal(L, "masterinterpreter_getconditionresult");
		lua_pushinteger(L, 10);
		lua_newtable(L);
		PushOutputEntry(L, "target", "start");
		PushOutputEntry(L, "startpositionx", "0");
		PushOutputEntry(L, "startpositiony", "0");
		PushOutputEntry(L, "startpositionz", "100");
		PushOutputEntry(L, "targete", "-1");
		lua_pushinteger(L, conditionType);
		lua_pushstring(L, param);
		REQUIRE(ProtectedPCall(L, 4, 1));
		const int result = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);
		return result;
	};

	CHECK(CallWithStart(within, "150") == 1);   // 100 <= 150
	CHECK(CallWithStart(within, "50") == 0);    // 100 > 50
	CHECK(CallWithStart(beyond, "50") == 1);    // 100 > 50
	CHECK(CallWithStart(beyond, "150") == 0);   // 100 not > 150
}

TEST_CASE("Condition evaluator random condition is deterministic at range zero")
{
	MasterInterpreterFixture f;
	const int random = InterpreterConstant(f.host, "g_masterinterpreter_cond_random");

	// math.random(0, 0) always returns 0, so the condition always fires:
	CHECK(CallCondition(f.host, 10, random, "0") == 1);
}
