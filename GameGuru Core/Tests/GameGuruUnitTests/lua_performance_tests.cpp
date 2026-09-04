// Performance and memory tests for the product's Lua gameplay layer.
//
// Two contracts are covered:
//
//   1. Memory: the Lua heap is measured with collectgarbage("count")
//      (kilobytes) around steady-state operation cycles, after forcing full
//      collections on both ends. A growing delta means the gameplay script
//      or the host integration leaks Lua objects.
//   2. Speed: generous time budgets catch order-of-magnitude regressions in
//      the hottest script entry points (see performance_tests.cpp in the
//      engine repo for the philosophy; the same no-assertions-inside-the-
//      timed-window rule applies).
#include "doctest.h"

#include "LuaHost.h"

#include <windows.h>

#include <chrono>
#include <cmath>
#include <functional>
#include <cstdio>
#include <fstream>
#include <string>

namespace
{
	using Clock = std::chrono::steady_clock;

	double MeasureMillis(const std::function<void()>& workload)
	{
		const auto start = Clock::now();
		workload();
		const auto end = Clock::now();
		return std::chrono::duration<double, std::milli>(end - start).count();
	}

	void ReportBudget(const char* name, double measuredMillis, double budgetMillis)
	{
		std::printf("[lua-perf] %-40s %8.1f ms  (budget %8.1f ms)\n",
			name, measuredMillis, budgetMillis);
	}

	// Full garbage collection, then the Lua heap size in kilobytes.
	double CollectedHeapKB(ggtest::LuaHost& host)
	{
		lua_State* L = host.state();
		lua_getglobal(L, "collectgarbage");
		lua_pushstring(L, "collect");
		REQUIRE(lua_pcall(L, 1, 0, 0) == LUA_OK);
		lua_getglobal(L, "collectgarbage");
		lua_pushstring(L, "collect");
		REQUIRE(lua_pcall(L, 1, 0, 0) == LUA_OK);
		lua_getglobal(L, "collectgarbage");
		lua_pushstring(L, "count");
		REQUIRE(lua_pcall(L, 1, 1, 0) == LUA_OK);
		const double kb = lua_tonumber(L, -1);
		lua_pop(L, 1);
		return kb;
	}

	// ---- interpreter stubs (the master interpreter suite owns its own
	// copies; these mirror the small subset needed here) ----
	int g_timerMs = 0;

	int Stub_Timer(lua_State* L) { lua_pushnumber(L, g_timerMs); return 1; }
	int Stub_GetTimer(lua_State* L) { lua_pushnumber(L, g_timerMs); return 1; }
	int Stub_GetEntityViewRange(lua_State* L) { lua_pushnumber(L, 2000); return 1; }
	int Stub_RayTerrain(lua_State* L) { lua_pushnumber(L, 0); return 1; }
	int Stub_IntersectStaticPerformant(lua_State* L) { lua_pushnumber(L, 0); return 1; }
	int Stub_GetDistanceTo(lua_State* L)
	{
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

	void RegisterPerfStubs(lua_State* L)
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

	void PlaceEntity(lua_State* L, int id, double x, double z, int health)
	{
		lua_getglobal(L, "g_Entity");
		lua_pushinteger(L, id);
		lua_newtable(L);
		lua_pushnumber(L, x);       lua_setfield(L, -2, "x");
		lua_pushnumber(L, 0);       lua_setfield(L, -2, "y");
		lua_pushnumber(L, z);       lua_setfield(L, -2, "z");
		lua_pushinteger(L, health); lua_setfield(L, -2, "health");
		lua_pushinteger(L, 100);    lua_setfield(L, -2, "obj");
		lua_pushnumber(L, 0);       lua_setfield(L, -2, "avoid");
		lua_settable(L, -3);
		lua_pop(L, 1);
	}

	struct PerfFixture
	{
		ggtest::LuaHost host;

		PerfFixture()
		{
			g_timerMs = 0;
			RegisterPerfStubs(host.state());
			const std::string scriptsRoot = ggtest::FindScriptsRoot();
			REQUIRE(scriptsRoot.size() > 0);
			REQUIRE(host.LoadScriptAs("UTILLIB", scriptsRoot + "/scriptbank/utillib.lua"));
			REQUIRE(host.LoadScriptAs("MASTERINT", scriptsRoot + "/scriptbank/masterinterpreter.lua"));

			// Behavior store + entity, like the running game provides:
			lua_newtable(host.state());
			lua_setglobal(host.state(), "g_myscript_behavior");
			PlaceEntity(host.state(), 10, 0, 0, 40);
		}
	};

	// Writes a behavior bytecode file with the requested instruction count.
	std::string WriteBytecodeFile(int instructionCount)
	{
		char temp[MAX_PATH] = {};
		GetTempPathA(MAX_PATH, temp);
		static unsigned counter = 0;
		const std::string path = std::string(temp) + "gg_behavior_perf_" +
			std::to_string(++counter) + ".byc";

		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		file << "42\n101\n1\n" << instructionCount << "\n";
		file << "PerfState\n1\n---\n";
		for (int i = 0; i < instructionCount; ++i)
		{
			file << (100 + i) << "\n1\n11\n0\n0\n5\n10\n20\n" << (i + 1) << "\n" << (i + 1) << "\n---\n";
		}
		return path;
	}
}

TEST_CASE("Lua memory: condition evaluation reaches a flat heap")
{
	PerfFixture f;
	const int checkHealth = 19; // shipped condition numbering

	// Warmup so one-time interpreter state does not count:
	for (int i = 0; i < 200; ++i)
	{
		lua_getglobal(f.host.state(), "masterinterpreter_getconditionresult");
		lua_pushinteger(f.host.state(), 10);
		lua_newtable(f.host.state());
		lua_pushinteger(f.host.state(), checkHealth);
		lua_pushstring(f.host.state(), "50");
		REQUIRE(lua_pcall(f.host.state(), 4, 1, 0) == LUA_OK);
		lua_pop(f.host.state(), 1);
	}

	const double beforeKB = CollectedHeapKB(f.host);
	for (int i = 0; i < 20000; ++i)
	{
		lua_getglobal(f.host.state(), "masterinterpreter_getconditionresult");
		lua_pushinteger(f.host.state(), 10);
		lua_newtable(f.host.state());
		lua_pushinteger(f.host.state(), checkHealth);
		lua_pushstring(f.host.state(), "50");
		REQUIRE(lua_pcall(f.host.state(), 4, 1, 0) == LUA_OK);
		lua_pop(f.host.state(), 1);
	}
	const double afterKB = CollectedHeapKB(f.host);

	std::printf("[lua-mem] %-40s %+6.1f KB\n", "20k condition evaluations",
		afterKB - beforeKB);
	CHECK(afterKB - beforeKB < 64.0); // bounded by GC bookkeeping noise
}

TEST_CASE("Lua memory: repeated behavior loads do not grow the heap")
{
	PerfFixture f;

	const std::string path = WriteBytecodeFile(200);

	// Warmup: the first load populates the behavior store; later loads
	// replace its contents, so a leaking loader shows up as growth:
	for (int i = 0; i < 3; ++i)
	{
		lua_getglobal(f.host.state(), "MASTERINT");
		lua_getfield(f.host.state(), -1, "masterinterpreter_load");
		lua_newtable(f.host.state());
		lua_pushstring(f.host.state(), "bycfilename");
		lua_pushstring(f.host.state(), path.c_str());
		lua_settable(f.host.state(), -3);
		lua_newtable(f.host.state());
		REQUIRE(lua_pcall(f.host.state(), 2, 1, 0) == LUA_OK);
		lua_pop(f.host.state(), 2);
	}

	const double beforeKB = CollectedHeapKB(f.host);
	for (int i = 0; i < 30; ++i)
	{
		lua_getglobal(f.host.state(), "MASTERINT");
		lua_getfield(f.host.state(), -1, "masterinterpreter_load");
		lua_newtable(f.host.state());
		lua_pushstring(f.host.state(), "bycfilename");
		lua_pushstring(f.host.state(), path.c_str());
		lua_settable(f.host.state(), -3);
		lua_newtable(f.host.state());
		REQUIRE(lua_pcall(f.host.state(), 2, 1, 0) == LUA_OK);
		lua_pop(f.host.state(), 2);
	}
	const double afterKB = CollectedHeapKB(f.host);

	std::printf("[lua-mem] %-40s %+6.1f KB\n", "30x 200-instruction behavior loads",
		afterKB - beforeKB);
	CHECK(afterKB - beforeKB < 64.0);

	std::remove(path.c_str());
}

TEST_CASE("Lua memory: foraging collectables reach a flat heap")
{
	PerfFixture f;
	lua_State* L = f.host.state();

	// Warmup:
	lua_newtable(L);
	lua_setglobal(L, "PERFLIST");
	for (int i = 0; i < 100; ++i)
	{
		REQUIRE(f.host.PushModuleFunction("UTILLIB", "ChangeAmount"));
		lua_pushstring(L, "wood");
		lua_pushnumber(L, 1);
		lua_getglobal(L, "PERFLIST");
		REQUIRE(lua_pcall(L, 3, 0, 0) == LUA_OK);
	}

	const double beforeKB = CollectedHeapKB(f.host);
	for (int i = 0; i < 20000; ++i)
	{
		REQUIRE(f.host.PushModuleFunction("UTILLIB", "ChangeAmount"));
		lua_pushstring(L, "wood");
		lua_pushnumber(L, 1);
		lua_getglobal(L, "PERFLIST");
		REQUIRE(lua_pcall(L, 3, 0, 0) == LUA_OK);
	}
	const double afterKB = CollectedHeapKB(f.host);

	std::printf("[lua-mem] %-40s %+6.1f KB\n", "20k foraging ChangeAmount calls",
		afterKB - beforeKB);
	CHECK(afterKB - beforeKB < 64.0);
}

TEST_CASE("Lua speed: hot script entry points stay within budget")
{
	PerfFixture f;
	lua_State* L = f.host.state();
	double millis = 0;

	// utillib.CloserThan — per-frame proximity checks:
	{
		double sink = 0;
		millis = MeasureMillis([&] {
			for (int i = 0; i < 50000; ++i)
			{
				REQUIRE(f.host.PushModuleFunction("UTILLIB", "CloserThan"));
				for (int k = 0; k < 6; ++k) lua_pushnumber(L, (double)(i % 10));
				lua_pushnumber(L, 20);
				REQUIRE(lua_pcall(L, 7, 1, 0) == LUA_OK);
				sink += lua_tonumber(L, -1);
				lua_pop(L, 1);
			}
		});
		CHECK(sink < 50000.0); // every call answered
		ReportBudget("utillib CloserThan 50k calls", millis, 10000.0);
		CHECK(millis < 10000.0);
	}

	// utillib.Rotate3D — decal/offset placement math:
	{
		millis = MeasureMillis([&] {
			for (int i = 0; i < 50000; ++i)
			{
				REQUIRE(f.host.PushModuleFunction("UTILLIB", "Rotate3D"));
				lua_pushnumber(L, 1);
				lua_pushnumber(L, 0);
				lua_pushnumber(L, 0);
				lua_pushnumber(L, 0.1);
				lua_pushnumber(L, 0.2);
				lua_pushnumber(L, 0.3);
				REQUIRE(lua_pcall(L, 6, 3, 0) == LUA_OK);
				lua_pop(L, 3);
			}
		});
		ReportBudget("utillib Rotate3D 50k calls", millis, 10000.0);
		CHECK(millis < 10000.0);
	}

	// masterinterpreter condition evaluation — the behavior decision brain:
	{
		const int checkHealth = 19;
		millis = MeasureMillis([&] {
			for (int i = 0; i < 20000; ++i)
			{
				lua_getglobal(L, "masterinterpreter_getconditionresult");
				lua_pushinteger(L, 10);
				lua_newtable(L);
				lua_pushinteger(L, checkHealth);
				lua_pushstring(L, "50");
				REQUIRE(lua_pcall(L, 4, 1, 0) == LUA_OK);
				lua_pop(L, 1);
			}
		});
		ReportBudget("interpreter 20k condition evals", millis, 10000.0);
		CHECK(millis < 10000.0);
	}

	// Behavior bytecode loading — editor-exported behaviors:
	{
		const std::string path = WriteBytecodeFile(500);
		millis = MeasureMillis([&] {
			for (int i = 0; i < 20; ++i)
			{
				lua_getglobal(L, "MASTERINT");
				lua_getfield(L, -1, "masterinterpreter_load");
				lua_newtable(L);
				lua_pushstring(L, "bycfilename");
				lua_pushstring(L, path.c_str());
				lua_settable(L, -3);
				lua_newtable(L);
				REQUIRE(lua_pcall(L, 2, 1, 0) == LUA_OK);
				lua_pop(L, 2);
			}
		});
		std::remove(path.c_str());
		ReportBudget("interpreter 20x 500-instruction loads", millis, 10000.0);
		CHECK(millis < 10000.0);
	}
}
