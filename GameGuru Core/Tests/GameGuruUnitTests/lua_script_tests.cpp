// Behavior tests for the product's Lua gameplay library
// (Scripts\scriptbank\utillib.lua) running against the vendored Lua 5.2
// runtime with stubbed engine entry points.
#include "doctest.h"

#include "LuaHost.h"

#include <cmath>

namespace
{
	struct LuaFixture
	{
		ggtest::LuaHost host;

		LuaFixture()
		{
			INFO("Lua error: ", host.GetLastError());
			const std::string scriptsRoot = ggtest::FindScriptsRoot();
			REQUIRE(scriptsRoot.size() > 0);
			REQUIRE(host.LoadScriptAs("UTILLIB", scriptsRoot + "/scriptbank/utillib.lua"));
		}
	};

	// Calls UTILLIB.<name> with numeric arguments and leaves the results on
	// the stack on success. Three results are always requested; Lua pads
	// with nil when the function returns fewer.
	bool CallDoubles(ggtest::LuaHost& host, const char* name, std::initializer_list<double> args)
	{
		if (!host.PushModuleFunction("UTILLIB", name)) return false;
		for (double a : args) lua_pushnumber(host.state(), a);
		return lua_pcall(host.state(), (int)args.size(), 3, 0) == LUA_OK;
	}

	void SetPlayerGlobals(ggtest::LuaHost& host, double x, double y, double z, double angX, double angY)
	{
		lua_State* L = host.state();
		lua_pushnumber(L, x);  lua_setglobal(L, "g_PlayerPosX");
		lua_pushnumber(L, y);  lua_setglobal(L, "g_PlayerPosY");
		lua_pushnumber(L, z);  lua_setglobal(L, "g_PlayerPosZ");
		lua_pushnumber(L, angX); lua_setglobal(L, "g_PlayerAngX");
		lua_pushnumber(L, angY); lua_setglobal(L, "g_PlayerAngY");
		lua_pushnumber(L, 0);  lua_setglobal(L, "g_PlayerAngZ");
	}

	// Adds entities into the existing g_Entity table. utillib captures
	// g_Entity by reference at load time, so the table must be mutated in
	// place rather than replaced.
	void SetEntityTable(ggtest::LuaHost& host, const int ids[3], const double xyz[3][3], const int health[3])
	{
		lua_State* L = host.state();
		lua_getglobal(L, "g_Entity");
		for (int i = 0; i < 3; ++i)
		{
			if (ids[i] == 0) continue;
			lua_pushinteger(L, ids[i]);
			lua_newtable(L);
			lua_pushnumber(L, xyz[i][0]); lua_setfield(L, -2, "x");
			lua_pushnumber(L, xyz[i][1]); lua_setfield(L, -2, "y");
			lua_pushnumber(L, xyz[i][2]); lua_setfield(L, -2, "z");
			lua_pushinteger(L, health[i]); lua_setfield(L, -2, "health");
			lua_settable(L, -3);
		}
		lua_pop(L, 1);
	}

	// Sets a string-keyed collectables list table as a global.
	void SetListTable(ggtest::LuaHost& host, const char* globalName)
	{
		lua_newtable(host.state());
		lua_setglobal(host.state(), globalName);
	}
}

TEST_CASE("utillib.Rotate3D keeps points unchanged without rotation")
{
	LuaFixture f;

	REQUIRE(CallDoubles(f.host, "Rotate3D", { 1, 2, 3, 0, 0, 0 }));
	CHECK(lua_tonumber(f.host.state(), -3) == doctest::Approx(1));
	CHECK(lua_tonumber(f.host.state(), -2) == doctest::Approx(2));
	CHECK(lua_tonumber(f.host.state(), -1) == doctest::Approx(3));
}

TEST_CASE("utillib.Rotate3D rotates a point around the Y axis")
{
	LuaFixture f;

	// (0,0,1) rotated by +90 degrees around Y lands on (1,0,0):
	const double ninetyDegrees = 3.14159265358979 / 2.0;
	REQUIRE(CallDoubles(f.host, "Rotate3D", { 0, 0, 1, 0, ninetyDegrees, 0 }));
	CHECK(lua_tonumber(f.host.state(), -3) == doctest::Approx(1).epsilon(0.0001));
	CHECK(lua_tonumber(f.host.state(), -2) == doctest::Approx(0).epsilon(0.0001));
	CHECK(lua_tonumber(f.host.state(), -1) == doctest::Approx(0).epsilon(0.0001));
}

TEST_CASE("utillib.Rotate3D returns a full turn to its starting point")
{
	LuaFixture f;

	const double twoPi = 3.14159265358979 * 2.0;
	REQUIRE(CallDoubles(f.host, "Rotate3D", { 2, 0, 0, twoPi, twoPi, twoPi }));
	CHECK(lua_tonumber(f.host.state(), -3) == doctest::Approx(2).epsilon(0.0001));
	CHECK(lua_tonumber(f.host.state(), -2) == doctest::Approx(0).epsilon(0.0001));
	CHECK(lua_tonumber(f.host.state(), -1) == doctest::Approx(0).epsilon(0.0001));
}

TEST_CASE("utillib.CloserThan compares squared distance against the threshold")
{
	LuaFixture f;

	lua_State* L = f.host.state();
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "CloserThan"));
	for (int i = 0; i < 6; ++i) lua_pushnumber(L, 0);
	lua_pushnumber(L, 10);
	REQUIRE(lua_pcall(L, 7, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 1); // identical point is closer than 10

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "CloserThan"));
	lua_pushnumber(L, 0); lua_pushnumber(L, 0); lua_pushnumber(L, 0);
	lua_pushnumber(L, 9); lua_pushnumber(L, 0); lua_pushnumber(L, 0);
	lua_pushnumber(L, 10);
	REQUIRE(lua_pcall(L, 7, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 1);

	// Exactly on the threshold is NOT closer (strict comparison):
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "CloserThan"));
	lua_pushnumber(L, 0); lua_pushnumber(L, 0); lua_pushnumber(L, 0);
	lua_pushnumber(L, 10); lua_pushnumber(L, 0); lua_pushnumber(L, 0);
	lua_pushnumber(L, 10);
	REQUIRE(lua_pcall(L, 7, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 0);
}

TEST_CASE("utillib collectables: amounts accumulate and are queried")
{
	LuaFixture f;
	lua_State* L = f.host.state();
	SetListTable(f.host, "TESTLIST");

	// ChangeAmount(name, by, list) with an explicit list:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "ChangeAmount"));
	lua_pushstring(L, "wood");
	lua_pushnumber(L, 5);
	lua_getglobal(L, "TESTLIST");
	REQUIRE(lua_pcall(L, 3, 0, 0) == LUA_OK);

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "ChangeAmount"));
	lua_pushstring(L, "wood");
	lua_pushnumber(L, 2);
	lua_getglobal(L, "TESTLIST");
	REQUIRE(lua_pcall(L, 3, 0, 0) == LUA_OK);

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "HaveAmount"));
	lua_pushstring(L, "wood");
	lua_getglobal(L, "TESTLIST");
	REQUIRE(lua_pcall(L, 2, 1, 0) == LUA_OK);
	CHECK(lua_tonumber(L, -1) == doctest::Approx(7));

	// Unknown items report zero:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "HaveAmount"));
	lua_pushstring(L, "stone");
	lua_getglobal(L, "TESTLIST");
	REQUIRE(lua_pcall(L, 2, 1, 0) == LUA_OK);
	CHECK(lua_tonumber(L, -1) == doctest::Approx(0));

	// SetAmount replaces instead of accumulating:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "SetAmount"));
	lua_pushstring(L, "wood");
	lua_pushnumber(L, 3);
	lua_getglobal(L, "TESTLIST");
	REQUIRE(lua_pcall(L, 3, 0, 0) == LUA_OK);

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "HaveAmount"));
	lua_pushstring(L, "wood");
	lua_getglobal(L, "TESTLIST");
	REQUIRE(lua_pcall(L, 2, 1, 0) == LUA_OK);
	CHECK(lua_tonumber(L, -1) == doctest::Approx(3));
}

TEST_CASE("utillib collectables: HaveEnough compares against the required amount")
{
	LuaFixture f;
	lua_State* L = f.host.state();
	SetListTable(f.host, "TESTLIST");

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "ChangeAmount"));
	lua_pushstring(L, "wood");
	lua_pushnumber(L, 5);
	lua_getglobal(L, "TESTLIST");
	REQUIRE(lua_pcall(L, 3, 0, 0) == LUA_OK);

	// Asking for zero is always satisfied:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "HaveEnough"));
	lua_pushstring(L, "stone");
	lua_pushnumber(L, 0);
	lua_getglobal(L, "TESTLIST");
	REQUIRE(lua_pcall(L, 3, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 1);

	// Enough:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "HaveEnough"));
	lua_pushstring(L, "wood");
	lua_pushnumber(L, 4);
	lua_getglobal(L, "TESTLIST");
	REQUIRE(lua_pcall(L, 3, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 1);

	// Not enough:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "HaveEnough"));
	lua_pushstring(L, "wood");
	lua_pushnumber(L, 6);
	lua_getglobal(L, "TESTLIST");
	REQUIRE(lua_pcall(L, 3, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 0);

	// Missing item with a positive requirement:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "HaveEnough"));
	lua_pushstring(L, "stone");
	lua_pushnumber(L, 1);
	lua_getglobal(L, "TESTLIST");
	REQUIRE(lua_pcall(L, 3, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 0);
}

TEST_CASE("utillib collectables: HaveEnough must not crash before any list is set")
{
	// Regression test: HaveEnough indexed the nil module list and raised a
	// Lua error when called before SetList, unlike HaveAmount which guards.
	LuaFixture f;
	lua_State* L = f.host.state();

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "HaveEnough"));
	lua_pushstring(L, "wood");
	lua_pushnumber(L, 5);
	lua_pushnil(L);
	CHECK_MESSAGE(lua_pcall(L, 3, 1, 0) == LUA_OK,
		"HaveEnough crashed: ", lua_tostring(L, -1));
	CHECK(lua_toboolean(L, -1) == 0);
}

TEST_CASE("utillib.PlayerCloserThanPos uses the stubbed player position")
{
	LuaFixture f;
	SetPlayerGlobals(f.host, 100, 0, 100, 0, 0);

	lua_State* L = f.host.state();
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "PlayerCloserThanPos"));
	lua_pushnumber(L, 100); lua_pushnumber(L, 0); lua_pushnumber(L, 110);
	lua_pushnumber(L, 15);
	REQUIRE(lua_pcall(L, 4, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 1);

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "PlayerCloserThanPos"));
	lua_pushnumber(L, 100); lua_pushnumber(L, 0); lua_pushnumber(L, 130);
	lua_pushnumber(L, 15);
	REQUIRE(lua_pcall(L, 4, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 0);
}

TEST_CASE("utillib.RandomPos returns a point on the circle around the center")
{
	LuaFixture f;
	lua_State* L = f.host.state();

	lua_pushnumber(L, 42); // deterministic seed for the run
	lua_setglobal(L, "SEED");
	// Seed the RNG through the sandbox's math table:
	lua_getglobal(L, "math");
	lua_getfield(L, -1, "randomseed");
	lua_pushnumber(L, 42);
	REQUIRE(lua_pcall(L, 1, 0, 0) == LUA_OK);
	lua_pop(L, 1);

	for (int i = 0; i < 25; ++i)
	{
		REQUIRE(f.host.PushModuleFunction("UTILLIB", "RandomPos"));
		lua_pushnumber(L, 30); // dist
		lua_pushnumber(L, 0);  // x
		lua_pushnumber(L, 0);  // z
		REQUIRE(lua_pcall(L, 3, 2, 0) == LUA_OK);

		const double rx = lua_tonumber(L, -2);
		const double rz = lua_tonumber(L, -1);
		const double radius = std::sqrt(rx * rx + rz * rz);
		CHECK(radius == doctest::Approx(30).epsilon(0.0001));
		lua_pop(L, 2);
	}
}

TEST_CASE("utillib.ClosestEntToPos picks the nearest entity inside the range")
{
	LuaFixture f;
	lua_State* L = f.host.state();

	const int ids[3] = { 10, 20, 30 };
	const double xyz[3][3] = { { 5, 0, 5 }, { 100, 0, 100 }, { 12, 0, 9 } };
	const int health[3] = { 100, 100, 100 };
	SetEntityTable(f.host, ids, xyz, health);

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "ClosestEntToPos"));
	lua_pushnumber(L, 0); lua_pushnumber(L, 0);
	lua_pushnumber(L, 50); // search range
	REQUIRE(lua_pcall(L, 3, 1, 0) == LUA_OK);
	CHECK(lua_tointeger(L, -1) == 10);

	// Out of range for everything:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "ClosestEntToPos"));
	lua_pushnumber(L, 200); lua_pushnumber(L, 200);
	lua_pushnumber(L, 10);
	REQUIRE(lua_pcall(L, 3, 1, 0) == LUA_OK);
	CHECK(lua_isnil(L, -1));
}

TEST_CASE("utillib.SortPairs iterates keys in sorted order")
{
	LuaFixture f;
	lua_State* L = f.host.state();

	lua_newtable(L);
	lua_pushstring(L, "v"); lua_pushnumber(L, 3); lua_settable(L, -3);
	lua_pushstring(L, "b"); lua_pushnumber(L, 1); lua_settable(L, -3);
	lua_pushstring(L, "m"); lua_pushnumber(L, 2); lua_settable(L, -3);
	lua_setglobal(L, "SORTME");

	// Collect U.SortPairs(SORTME) keys via a small driver chunk:
	const char* driver =
		"local result = {} "
		"for k, v in UTILLIB.SortPairs(SORTME) do result[#result + 1] = k end "
		"return table.concat(result, ',')";
	REQUIRE(luaL_dostring(L, driver) == LUA_OK);
	CHECK(std::string(lua_tostring(L, -1)) == "b,m,v");
}

TEST_CASE("utillib.ClosestEntities orders by distance, honors the limit and health")
{
	LuaFixture f;
	lua_State* L = f.host.state();

	SetPlayerGlobals(f.host, 0, 0, 0, 0, 0);
	const int ids[3] = { 10, 20, 30 };
	// Entity 20 is closest, 30 middle, 10 farthest; entity 10 also far
	// outside a small search radius.
	const double xyz[3][3] = { { 40, 0, 0 }, { 5, 0, 0 }, { 15, 0, 0 } };
	const int health[3] = { 100, 0, 100 }; // entity 20 is dead
	SetEntityTable(f.host, ids, xyz, health);

	// ClosestEntities(dist, num, x, z) defaults to player position:
	const char* driver =
		"local result = {} "
		"for _, e in ipairs(UTILLIB.ClosestEntities(100)) do result[#result + 1] = e end "
		"return table.concat(result, ',')";
	REQUIRE(luaL_dostring(L, driver) == LUA_OK);
	CHECK(std::string(lua_tostring(L, -1)) == "30,10");

	// A tight radius excludes the far entity:
	const char* driverTight =
		"local result = {} "
		"for _, e in ipairs(UTILLIB.ClosestEntities(20)) do result[#result + 1] = e end "
		"return table.concat(result, ',')";
	REQUIRE(luaL_dostring(L, driverTight) == LUA_OK);
	CHECK(std::string(lua_tostring(L, -1)) == "30");
}

TEST_CASE("utillib.ObjectPlayerLookingAt casts a ray from the player's eye")
{
	LuaFixture f;
	lua_State* L = f.host.state();

	SetPlayerGlobals(f.host, 10, 0, 20, 0, 0);
	ggtest::SetDuckingState(0);
	ggtest::SetNextRayCastResult(42);
	ggtest::ResetRayCastCount();

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "ObjectPlayerLookingAt"));
	REQUIRE(lua_pcall(L, 0, 1, 0) == LUA_OK);
	CHECK(lua_tointeger(L, -1) == 42);

	const ggtest::RayCastRecord ray = ggtest::LastRayCast();
	CHECK(ray.callCount == 1);
	CHECK(ray.startX == doctest::Approx(10));
	CHECK(ray.startY == doctest::Approx(31)); // standing eye offset +31
	CHECK(ray.startZ == doctest::Approx(20));

	// A custom range and ignore id are forwarded to the raycast call:
	ggtest::ResetRayCastCount();
	ggtest::SetNextRayCastResult(3);
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "ObjectPlayerLookingAt"));
	lua_pushnumber(L, 500); // dist
	lua_pushinteger(L, 7);  // ignore
	REQUIRE(lua_pcall(L, 2, 1, 0) == LUA_OK);
	CHECK(lua_tointeger(L, -1) == 3);
	{
		const ggtest::RayCastRecord ray = ggtest::LastRayCast();
		CHECK(ray.callCount == 1);
		CHECK(ray.ignore == 7);
		// The ray spans 500 units straight ahead (zero rotation):
		const double dx = ray.endX - ray.startX;
		const double dy = ray.endY - ray.startY;
		const double dz = ray.endZ - ray.startZ;
		CHECK(std::sqrt(dx * dx + dy * dy + dz * dz) == doctest::Approx(500).epsilon(0.0001));
	}

	// Ducked players raycast from a lower eye point:
	ggtest::SetDuckingState(1);
	ggtest::SetNextRayCastResult(7);
	ggtest::ResetRayCastCount();
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "ObjectPlayerLookingAt"));
	REQUIRE(lua_pcall(L, 0, 1, 0) == LUA_OK);
	CHECK(lua_tointeger(L, -1) == 7);
	CHECK(ggtest::LastRayCast().startY == doctest::Approx(10)); // crouched offset +10
}

TEST_CASE("utillib.PlayerLookingAt resolves entities through g_Entity")
{
	LuaFixture f;
	lua_State* L = f.host.state();

	SetPlayerGlobals(f.host, 0, 0, 0, 0, 0);
	ggtest::SetDuckingState(0);

	// Ray hits object 55, entity 10 holds object 55:
	ggtest::SetNextRayCastResult(55);
	const int ids[3] = { 10, 0, 0 };
	const double xyz[3][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
	const int health[3] = { 100, 0, 0 };
	SetEntityTable(f.host, ids, xyz, health);
	// Entity 10 is represented by object 55 in the engine:
	lua_getglobal(L, "g_Entity");
	lua_pushinteger(L, 10);
	lua_gettable(L, -2);
	lua_pushinteger(L, 55);
	lua_setfield(L, -2, "obj");
	lua_pop(L, 2);

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "PlayerLookingAt"));
	lua_pushinteger(L, 10);
	REQUIRE(lua_pcall(L, 1, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 1);

	// A ray that lands on a different object:
	ggtest::SetNextRayCastResult(99);
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "PlayerLookingAt"));
	lua_pushinteger(L, 10);
	REQUIRE(lua_pcall(L, 1, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 0);

	// Unknown entity:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "PlayerLookingAt"));
	lua_pushinteger(L, 999);
	REQUIRE(lua_pcall(L, 1, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 0);
}

TEST_CASE("utillib module list: SetList routes list-less calls to the stored list")
{
	LuaFixture f;
	lua_State* L = f.host.state();
	SetListTable(f.host, "MODULELIST");

	// Store the list in the module's internal state:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "SetList"));
	lua_getglobal(L, "MODULELIST");
	REQUIRE(lua_pcall(L, 1, 0, 0) == LUA_OK);

	// List-less calls now operate on the stored list:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "ChangeAmount"));
	lua_pushstring(L, "herb");
	lua_pushnumber(L, 4);
	lua_pushnil(L);
	REQUIRE(lua_pcall(L, 3, 0, 0) == LUA_OK);

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "HaveAmount"));
	lua_pushstring(L, "herb");
	lua_pushnil(L);
	REQUIRE(lua_pcall(L, 2, 1, 0) == LUA_OK);
	CHECK(lua_tonumber(L, -1) == doctest::Approx(4));
}

TEST_CASE("utillib.PlayerCloserThan resolves the entity through g_Entity")
{
	LuaFixture f;
	lua_State* L = f.host.state();
	SetPlayerGlobals(f.host, 0, 0, 0, 0, 0);

	const int ids[3] = { 10, 0, 0 };
	const double xyz[3][3] = { { 5, 0, 5 }, { 0, 0, 0 }, { 0, 0, 0 } };
	const int health[3] = { 100, 0, 0 };
	SetEntityTable(f.host, ids, xyz, health);

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "PlayerCloserThan"));
	lua_pushinteger(L, 10);
	lua_pushnumber(L, 20); // distance: sqrt(50) fits
	REQUIRE(lua_pcall(L, 2, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 1);

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "PlayerCloserThan"));
	lua_pushinteger(L, 10);
	lua_pushnumber(L, 7); // distance: sqrt(50) does not fit
	REQUIRE(lua_pcall(L, 2, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 0);

	// Unknown entities are never closer:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "PlayerCloserThan"));
	lua_pushinteger(L, 999);
	lua_pushnumber(L, 20000);
	REQUIRE(lua_pcall(L, 2, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 0);
}

TEST_CASE("utillib.ClosestEntToPlayer uses the stubbed player position")
{
	LuaFixture f;
	lua_State* L = f.host.state();
	SetPlayerGlobals(f.host, 0, 0, 0, 0, 0);

	const int ids[3] = { 10, 20, 0 };
	const double xyz[3][3] = { { 30, 0, 0 }, { 10, 0, 0 }, { 0, 0, 0 } };
	const int health[3] = { 100, 100, 0 };
	SetEntityTable(f.host, ids, xyz, health);

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "ClosestEntToPlayer"));
	REQUIRE(lua_pcall(L, 0, 1, 0) == LUA_OK);
	CHECK(lua_tointeger(L, -1) == 20);

	// A tight range still finds the near entity and skips the far one:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "ClosestEntToPlayer"));
	lua_pushnumber(L, 15);
	REQUIRE(lua_pcall(L, 1, 1, 0) == LUA_OK);
	CHECK(lua_tointeger(L, -1) == 20);
}

TEST_CASE("utillib.PlayerLookingNear checks bearing with wrap-around and vertical angle")
{
	LuaFixture f;
	lua_State* L = f.host.state();

	// Player stands at the origin looking east (bearing 90 degrees):
	SetPlayerGlobals(f.host, 0, 0, 0, 0, 90);
	ggtest::SetDuckingState(0);

	// Entity straight east at the eye height (0 + 31): inside a 90 degree fov.
	const int ids[3] = { 10, 0, 0 };
	const double xyz[3][3] = { { 100, 31, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
	const int health[3] = { 100, 0, 0 };
	SetEntityTable(f.host, ids, xyz, health);

	REQUIRE(f.host.PushModuleFunction("UTILLIB", "PlayerLookingNear"));
	lua_pushinteger(L, 10);
	lua_pushnumber(L, 500);
	lua_pushnumber(L, 90);
	REQUIRE(lua_pcall(L, 3, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 1);

	// Looking the opposite way (bearing 180) misses it:
	SetPlayerGlobals(f.host, 0, 0, 0, 0, 180);
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "PlayerLookingNear"));
	lua_pushinteger(L, 10);
	lua_pushnumber(L, 500);
	lua_pushnumber(L, 90);
	REQUIRE(lua_pcall(L, 3, 1, 0) == LUA_OK);
	CHECK(lua_toboolean(L, -1) == 0);
}

TEST_CASE("utillib.RandomOffsetPos centers the random point on the entity offset")
{
	LuaFixture f;
	lua_State* L = f.host.state();
	SetPlayerGlobals(f.host, 0, 0, 0, 0, 0);

	const int ids[3] = { 10, 0, 0 };
	const double xyz[3][3] = { { 100, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
	const int health[3] = { 100, 0, 0 };
	SetEntityTable(f.host, ids, xyz, health);

	// Documented behavior: the random point lies on a circle of the given
	// radius around the point halfway (offset=2) from (0,0) to the entity.
	for (int i = 0; i < 10; ++i)
	{
		REQUIRE(f.host.PushModuleFunction("UTILLIB", "RandomOffsetPos"));
		lua_pushinteger(L, 10);  // entity
		lua_pushnumber(L, 10);   // dist
		lua_pushnumber(L, 2);    // offset
		lua_pushnumber(L, 0);    // x
		lua_pushnumber(L, 0);    // z
		REQUIRE(lua_pcall(L, 5, 2, 0) == LUA_OK);
		const double rx = lua_tonumber(L, -2);
		const double rz = lua_tonumber(L, -1);
		lua_pop(L, 2);

		const double dx = rx - 50, dz = rz - 0;
		CHECK(std::sqrt(dx * dx + dz * dz) == doctest::Approx(10).epsilon(0.0001));
	}

	// A missing entity yields the (0, 0) sentinel pair:
	REQUIRE(f.host.PushModuleFunction("UTILLIB", "RandomOffsetPos"));
	lua_pushinteger(L, 999);
	lua_pushnumber(L, 10);
	lua_pushnumber(L, 2);
	lua_pushnumber(L, 0);
	lua_pushnumber(L, 0);
	REQUIRE(lua_pcall(L, 5, 2, 0) == LUA_OK);
	CHECK(lua_tonumber(L, -2) == 0);
	CHECK(lua_tonumber(L, -1) == 0);
}
