// Smoke and behavior tests for the product's large entity scripts
// (Scripts\scriptbank\objects\sentry.lua, objects\aircraft.lua,
// people\npc_control.lua).
//
// These scripts call dozens of engine functions per frame. The auto-stub
// layer (Ext::InstallAutoStubs) turns every *undefined*
// global function into a recording stub returning 0, so whole init/main
// loops run headless while the tests assert on:
//   - concrete state the scripts write (entity health boosts, muzzle-flash
//     discovery through GetEntityName),
//   - recorded engine calls (Hide, SetEntityHealth, ...),
//   - simply running N frames without a Lua error (the smoke contract for
//     the physics-heavy vehicle scripts).
// Functions whose return values the scripts USE in arithmetic or string
// operations are overridden with typed stubs before the scripts load —
// auto-stubs only fill the gaps.
#include "doctest.h"

#include "LuaHost.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
	namespace Ext = ggtest::LuaHostExtensions;

	struct EntityScriptFixture
	{
		ggtest::LuaHost host;

		EntityScriptFixture()
		{
			// Typed engine returns the scripts rely on (registered before
			// the scripts load, so they shadow the auto-stub layer):
			// GetEntityName returns the muzzle-flash name for entity 15 and
			// a non-matching name otherwise:
			lua_pushcclosure(host.state(), [](lua_State* L) -> int {
				const int id = (int)luaL_checkinteger(L, 1);
				lua_pushstring(L, id == 15 ? "sentry muzzleflash" : "unrelated");
				return 1;
			}, 0);
			lua_setglobal(host.state(), "GetEntityName");
			// GetObjectPosAng returns x,y,z + pitch,yaw,roll:
			lua_pushcclosure(host.state(), [](lua_State* L) -> int {
				for (double v : { 1.0, 2.0, 3.0, 0.0, 90.0, 0.0 }) lua_pushnumber(L, v);
				return 6;
			}, 0);
			lua_setglobal(host.state(), "GetObjectPosAng");

			// Engine-provided numbers the scripts iterate over:
			lua_pushinteger(host.state(), 20);
			lua_setglobal(host.state(), "g_EntityElementMax");

			// Multi-return collision queries (auto-stubs return one value):
			lua_pushcclosure(host.state(), [](lua_State* L) -> int {
				for (double v : { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 }) lua_pushnumber(L, v);
				return 6;
			}, 0);
			lua_setglobal(host.state(), "GetObjectColBox");
			lua_pushcclosure(host.state(), [](lua_State* L) -> int {
				for (double v : { 1.0, 1.0, 1.0 }) lua_pushnumber(L, v);
				return 3;
			}, 0);
			lua_setglobal(host.state(), "GetObjectScales");

			// Entities: 10 = the sentry/vehicle, 15 = the muzzle-flash object:
			auto placeEntity = [&](int id, int health, double angley) {
				lua_getglobal(host.state(), "g_Entity");
				lua_pushinteger(host.state(), id);
				lua_newtable(host.state());
				lua_pushnumber(host.state(), 0);        lua_setfield(host.state(), -2, "x");
				lua_pushnumber(host.state(), 0);        lua_setfield(host.state(), -2, "y");
				lua_pushnumber(host.state(), 0);        lua_setfield(host.state(), -2, "z");
				lua_pushnumber(host.state(), angley);   lua_setfield(host.state(), -2, "angley");
				lua_pushinteger(host.state(), health);  lua_setfield(host.state(), -2, "health");
				lua_pushinteger(host.state(), 100);     lua_setfield(host.state(), -2, "obj");
				lua_pushinteger(host.state(), 0);       lua_setfield(host.state(), -2, "activated");
				lua_pushnumber(host.state(), 0);        lua_setfield(host.state(), -2, "avoid");
				lua_settable(host.state(), -3);
				lua_pop(host.state(), 1);
			};
			placeEntity(10, 40, 45);
			placeEntity(15, 100, 0);
			// Entity names (scripts run string.lower on them):
			lua_getglobal(host.state(), "g_Entity");
			lua_pushinteger(host.state(), 10);
			lua_gettable(host.state(), -2);
			lua_pushstring(host.state(), "TestNPC");
			lua_setfield(host.state(), -2, "name");
			lua_pop(host.state(), 2);

			// Engine DATA globals the scripts read every frame (a function
			// from the auto-stub layer would break their arithmetic):
			lua_pushnumber(host.state(), 0);
			lua_setglobal(host.state(), "g_Time");
			lua_pushnumber(host.state(), 0);
			lua_setglobal(host.state(), "g_TimeElapsed");
			lua_newtable(host.state());
			lua_setglobal(host.state(), "lastpressed");

			// Common player globals the scripts read numerically:
			for (const char* g : { "g_PlayerPosX", "g_PlayerPosY", "g_PlayerPosZ",
				"g_PlayerAngX", "g_PlayerAngY", "g_PlayerAngZ" })
			{
				lua_pushnumber(host.state(), 0);
				lua_setglobal(host.state(), g);
			}

			Ext::InstallAutoStubs(host.state());

			if (luaL_dostring(host.state(),
				"local mt = getmetatable(_G); "
				"if mt == nil then return 'NOMETATABLE' end; "
				"return type(mt.__index)") == LUA_OK)
			{
				std::cout << "DIAG metatable state: "
					<< lua_tostring(host.state(), -1) << std::endl;
				lua_pop(host.state(), 1);
			}
		}

		bool Load(const char* globalName, const char* relativePath)
		{
			const std::string scriptsRoot = ggtest::FindScriptsRoot();
			REQUIRE(scriptsRoot.size() > 0);
			return host.LoadScriptAs(globalName, scriptsRoot + "/" + relativePath);
		}

		// Calls a global script function and reports whether it survived.
		bool CallGlobal(const char* name, int nargs)
		{
			lua_State* L = host.state();
			lua_getglobal(L, name);
			if (!lua_isfunction(L, -1))
			{
				lua_pop(L, 1);
				return false;
			}
			// Arguments were already pushed by the caller before CallGlobal;
			// handled below via CallGlobalN for clarity.
			(void)nargs;
			return true;
		}

		bool ProtectedCall(int nargs)
		{
			const int rc = lua_pcall(host.state(), nargs, 0, 0);
			if (rc != LUA_OK)
			{
				std::cout << "LUAERR[" << name_context << "]: "
					<< (lua_isstring(host.state(), -1) ? lua_tostring(host.state(), -1) : "?")
					<< std::endl;
				lua_pop(host.state(), 1);
				return false;
			}
			return true;
		}

		std::string name_context = "script";
	};
}

// sentry_init creates the per-entity settings table; sentry_main then
// aliases g_sentry[e] = g_Entity[e] every frame and reads the level
// settings from the entity table, so this helper seeds those settings
// there (values match what the shipped editor writes).
void CallSentrySetup(EntityScriptFixture& f)
{
	struct Setting { const char* key; double value; };
	const Setting numbers[] = {
		{ "scan_range", 1000 }, { "scan_radius", 136 }, { "scan_speed", 3 },
		{ "alarm", 2 }, { "alarm_range", 500 }, { "sentry_damage", 60 },
		{ "muzzle_flash", 0 }, { "muzzle_y", 46 }, { "muzzle_z", 47 },
		{ "allegiance", 0 }, { "visibility", 1 },
	};
	lua_getglobal(f.host.state(), "g_Entity");
	lua_pushinteger(f.host.state(), 10);
	lua_gettable(f.host.state(), -2);
	for (const Setting& s : numbers)
	{
		lua_pushstring(f.host.state(), s.key);
		lua_pushnumber(f.host.state(), s.value);
		lua_settable(f.host.state(), -3);
	}
	lua_pushstring(f.host.state(), "muzzle_flash_name");
	lua_pushstring(f.host.state(), "sentry muzzleflash");
	lua_settable(f.host.state(), -3);
	lua_pop(f.host.state(), 2);
}

TEST_CASE("sentry_init boosts weak entity health and registers character mode")
{
	EntityScriptFixture f;
	f.name_context = "sentry_init";
	REQUIRE(f.Load("SENTRY", "scriptbank/objects/sentry.lua"));

	lua_getglobal(f.host.state(), "sentry_init");
	lua_pushinteger(f.host.state(), 10);
	REQUIRE(f.ProtectedCall(1));

	// Entity 10 arrived with health 40; the script tops it up by 100:
	CHECK(Ext::AutoCallCount(f.host.state(), "SetEntityHealth") == 1);
	CHECK(Ext::AutoCallCount(f.host.state(), "SetCharacterMode") == 1);
}

TEST_CASE("sentry_main discovers the muzzle flash object and hides it")
{
	EntityScriptFixture f;
	f.name_context = "sentry_main";
	REQUIRE(f.Load("SENTRY", "scriptbank/objects/sentry.lua"));

	lua_getglobal(f.host.state(), "sentry_init");
	lua_pushinteger(f.host.state(), 10);
	REQUIRE(f.ProtectedCall(1));
	CallSentrySetup(f);

	// First main frame runs the init state, scans entities 1..20 for the
	// muzzle flash by name and finds object 15:
	lua_getglobal(f.host.state(), "sentry_main");
	lua_pushinteger(f.host.state(), 10);
	REQUIRE(f.ProtectedCall(1));

	CHECK(Ext::AutoCallCount(f.host.state(), "Hide") >= 1);
}

TEST_CASE("sentry_main survives a full sweep of frames")
{
	EntityScriptFixture f;
	f.name_context = "sentry_main_sweep";
	REQUIRE(f.Load("SENTRY", "scriptbank/objects/sentry.lua"));

	lua_getglobal(f.host.state(), "sentry_init");
	lua_pushinteger(f.host.state(), 10);
	REQUIRE(f.ProtectedCall(1));
	CallSentrySetup(f);

	for (int frame = 0; frame < 120; ++frame)
	{
		lua_getglobal(f.host.state(), "sentry_main");
		lua_pushinteger(f.host.state(), 10);
		REQUIRE(f.ProtectedCall(1));
	}
}

TEST_CASE("aircraft script initializes and runs frames headless")
{
	EntityScriptFixture f;
	f.name_context = "aircraft";
	REQUIRE(f.Load("AIRCRAFT", "scriptbank/objects/aircraft.lua"));

	// aircraft_properties(e, vehicle_type, use_range, prompt_text, use_text1,
	// use_text2, lz_text, display_x, display_y, pilot_x, pilot_y, pilot_z,
	// max_speed, velocity, hover_amount, hover_oscillation, fx_animation,
	// fx_modulation, show_readouts, particle_no, can_bailout, weapon_name,
	// weapon_ammo, use_weapon, crosshair_imagefile):
	lua_getglobal(f.host.state(), "aircraft_init");
	lua_pushinteger(f.host.state(), 10);
	REQUIRE(f.ProtectedCall(1));

	lua_getglobal(f.host.state(), "aircraft_properties");
	lua_pushinteger(f.host.state(), 10);
	for (double v : { 1, 500, 0, 0, 0, 0, 50, 50, 0, 2, 0, 120, 60, 20, 5, 1, 50, 1, 0, 1 })
	{
		lua_pushnumber(f.host.state(), v);
	}
	// weapon_name is a numeric weapon index in the shipped format:
	lua_pushinteger(f.host.state(), 1);
	lua_pushnumber(f.host.state(), 0);
	lua_pushnumber(f.host.state(), 0);
	lua_pushstring(f.host.state(), "");
	REQUIRE(f.ProtectedCall(25));

	for (int frame = 0; frame < 30; ++frame)
	{
		lua_getglobal(f.host.state(), "aircraft_main");
		lua_pushinteger(f.host.state(), 10);
		REQUIRE(f.ProtectedCall(1));
	}
}

TEST_CASE("npc_control script initializes and runs frames headless")
{
	EntityScriptFixture f;
	f.name_context = "npc_control";
	REQUIRE(f.Load("NPCCONTROL", "scriptbank/people/npc_control.lua"));

	// npc_control_properties(e, ...) takes a long tail of settings; pass a
	// plausible all-neutral configuration:
	lua_getglobal(f.host.state(), "npc_control_init_name");
	lua_pushinteger(f.host.state(), 10);
	lua_pushstring(f.host.state(), "TestNPC");
	REQUIRE(f.ProtectedCall(2));

	lua_getglobal(f.host.state(), "npc_control_properties");
	lua_pushinteger(f.host.state(), 10);
	// Settings in shipped order: strings for text fields, numbers for the
	// numeric ones (sense_range, times, speeds, ranges, hit frames, ...).
	struct NpcSetting { const char* value; bool isNumber; };
	const NpcSetting settings[] = {
		{ "", false },        // sense_text
		{ "500", true },      // sense_range
		{ "1", true },        // sense_mode
		{ "0", true },        // npc_can_flee
		{ "2000", true },     // idle_time
		{ "100", true },      // attack_range
		{ "10", true },       // attack_damage
		{ "0", true },        // random_damage
		{ "1", true },        // npc_can_roam
		{ "400", true },      // roam_range
		{ "100", true },      // npc_anim_speed
		{ "1", true },        // npc_move_speed
		{ "2", true },        // npc_run_speed
		{ "4", true },        // npc_turn_speed
		{ "0", true },        // npc_can_shoot
		{ "idle", false },    // idle1_animation
		{ "idle", false },    // idle2_animation
		{ "walk", false },    // walk_animation
		{ "run", false },     // run_animation
		{ "threat", false },  // threat_animation
		{ "attack", false },  // attack1_animation
		{ "10", true },       // attack1_hitframe
		{ "attack", false },  // attack2_animation
		{ "10", true },       // attack2_hitframe
		{ "attack", false },  // attack3_animation
		{ "10", true },       // attack3_hitframe
		{ "shoot", false },   // shoot_animation
		{ "hurt", false },    // hurt_animation
		{ "death", false },   // death1_animation
		{ "death", false },   // death2_animation
		{ "flag", false },    // lastflag_animation
		{ "1000", true },     // lastflag_time
		{ "0", true },        // lastflag_loop
		{ "0", true },        // force_move
		{ "1", true },        // npc_tilting
		{ "1", true },        // on_death
		{ "0", true },        // ifused
		{ "0", true },        // diagnostics
	};
	for (const NpcSetting& setting : settings)
	{
		if (setting.isNumber)
		{
			lua_pushnumber(f.host.state(), atof(setting.value));
		}
		else
		{
			lua_pushstring(f.host.state(), setting.value);
		}
	}
	REQUIRE(f.ProtectedCall(39));

	for (int frame = 0; frame < 30; ++frame)
	{
		lua_getglobal(f.host.state(), "npc_control_main");
		lua_pushinteger(f.host.state(), 10);
		REQUIRE(f.ProtectedCall(1));
	}
}
