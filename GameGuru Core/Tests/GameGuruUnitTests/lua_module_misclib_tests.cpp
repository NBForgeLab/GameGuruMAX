// Behavior tests for the product's Module_Misclib gameplay script
// (Scripts\scriptbank\module_misclib.lua). The script is loaded through the
// real require() machinery (package.path points at the product's script
// banks), and the engine entry points it calls are recorded stubs so tests
// can assert exactly how the script drives the engine.
#include "doctest.h"

#include "LuaHost.h"

#include <map>
#include <string>

namespace
{
	// Visual state the script manipulates per entity through engine calls.
	struct VisualState
	{
		int r = 0, g = 0, b = 0;
		double strength = 0;
		int outline = 0;
	};
	std::map<int, VisualState> g_visual;
	int g_iconPasteCalls = 0;
	int g_textCalls = 0;
	std::string g_lastText;

	void ResetVisualState()
	{
		g_visual.clear();
		g_iconPasteCalls = 0;
		g_textCalls = 0;
		g_lastText.clear();
	}

	VisualState& Visual(int entity) { return g_visual[entity]; }

	int Stub_GetEntityEmissiveColor(lua_State* L)
	{
		const VisualState& v = Visual((int)luaL_checkinteger(L, 1));
		lua_pushinteger(L, v.r);
		lua_pushinteger(L, v.g);
		lua_pushinteger(L, v.b);
		return 3;
	}
	int Stub_GetEntityEmissiveStrength(lua_State* L)
	{
		lua_pushnumber(L, Visual((int)luaL_checkinteger(L, 1)).strength);
		return 1;
	}
	int Stub_SetEntityEmissiveStrength(lua_State* L)
	{
		Visual((int)luaL_checkinteger(L, 1)).strength = luaL_checknumber(L, 2);
		return 0;
	}
	int Stub_SetEntityEmissiveColor(lua_State* L)
	{
		VisualState& v = Visual((int)luaL_checkinteger(L, 1));
		v.r = (int)luaL_checkinteger(L, 2);
		v.g = (int)luaL_checkinteger(L, 3);
		v.b = (int)luaL_checkinteger(L, 4);
		return 0;
	}
	int Stub_SetEntityOutline(lua_State* L)
	{
		Visual((int)luaL_checkinteger(L, 1)).outline = (int)luaL_checkinteger(L, 2);
		return 0;
	}
	int Stub_PasteSpritePosition(lua_State* L)
	{
		++g_iconPasteCalls;
		return 0;
	}
	int Stub_TextCenterOnXColor(lua_State* L)
	{
		++g_textCalls;
		const char* text = lua_tostring(L, 4);
		g_lastText = text != nullptr ? text : "";
		return 0;
	}
}

namespace ggtest
{
	// Registers the engine entry points module_misclib.pinpoint() uses.
	void RegisterMisclibStubs(lua_State* L)
	{
		struct Entry { const char* name; lua_CFunction fn; };
		const Entry entries[] = {
			{ "GetEntityEmissiveColor", Stub_GetEntityEmissiveColor },
			{ "GetEntityEmissiveStrength", Stub_GetEntityEmissiveStrength },
			{ "SetEntityEmissiveStrength", Stub_SetEntityEmissiveStrength },
			{ "SetEntityEmissiveColor", Stub_SetEntityEmissiveColor },
			{ "SetEntityOutline", Stub_SetEntityOutline },
			{ "PasteSpritePosition", Stub_PasteSpritePosition },
			{ "TextCenterOnXColor", Stub_TextCenterOnXColor },
		};
		for (const Entry& e : entries)
		{
			lua_pushcfunction(L, e.fn);
			lua_setglobal(L, e.name);
		}
	}
}

namespace
{
	struct MisclibFixture
	{
		ggtest::LuaHost host;

		MisclibFixture()
		{
			ResetVisualState();
			ggtest::RegisterMisclibStubs(host.state());
			const std::string scriptsRoot = ggtest::FindScriptsRoot();
			REQUIRE(scriptsRoot.size() > 0);
			// utillib is pulled in by the module itself through require(),
			// exactly like in the game:
			REQUIRE(host.LoadScriptAs("MISCLIB", scriptsRoot + "/scriptbank/module_misclib.lua"));
		}

		// Places entity 10 holding object 55 at the given position.
		void PlaceSelectionEntity(double x, double y, double z)
		{
			lua_State* L = host.state();
			lua_getglobal(L, "g_Entity");
			lua_pushinteger(L, 10);
			lua_newtable(L);
			lua_pushinteger(L, 55);      lua_setfield(L, -2, "obj");
			lua_pushnumber(L, x);        lua_setfield(L, -2, "x");
			lua_pushnumber(L, y);        lua_setfield(L, -2, "y");
			lua_pushnumber(L, z);        lua_setfield(L, -2, "z");
			lua_pushinteger(L, 100);     lua_setfield(L, -2, "health");
			lua_settable(L, -3);
			lua_pop(L, 1);

			// Player at the origin looking east, standing:
			lua_pushnumber(L, 0);   lua_setglobal(L, "g_PlayerPosX");
			lua_pushnumber(L, 0);   lua_setglobal(L, "g_PlayerPosY");
			lua_pushnumber(L, 0);   lua_setglobal(L, "g_PlayerPosZ");
			lua_pushnumber(L, 0);   lua_setglobal(L, "g_PlayerAngX");
			lua_pushnumber(L, 90);  lua_setglobal(L, "g_PlayerAngY");
			lua_pushnumber(L, 0);   lua_setglobal(L, "g_PlayerAngZ");
			ggtest::SetDuckingState(0);
		}

		void Pinpoint(int entity, double range, int highlight, int icon)
		{
			REQUIRE(host.PushModuleFunction("MISCLIB", "pinpoint"));
			lua_State* L = host.state();
			lua_pushinteger(L, entity);
			lua_pushnumber(L, range);
			lua_pushinteger(L, highlight);
			lua_pushinteger(L, icon);
			REQUIRE(lua_pcall(L, 4, 0, 0) == LUA_OK);
		}
	};
}

TEST_CASE("module_misclib.pinpoint outlines the entity the player is looking at")
{
	MisclibFixture f;
	f.PlaceSelectionEntity(1000, 31, 0);
	ggtest::SetNextRayCastResult(55); // the ray hits entity 10's object

	f.Pinpoint(10, 2000, 2, 0);
	CHECK(Visual(10).outline == 1);

	// When the ray no longer hits, the outline is removed again:
	ggtest::SetNextRayCastResult(0);
	f.Pinpoint(10, 2000, 2, 0);
	CHECK(Visual(10).outline == 0);
}

TEST_CASE("module_misclib.pinpoint highlights through emissive color and restores it")
{
	MisclibFixture f;
	f.PlaceSelectionEntity(1000, 31, 0);
	ggtest::SetNextRayCastResult(55);

	f.Pinpoint(10, 2000, 1, 0);
	// A black entity gets the green highlight tint and a boosted strength:
	CHECK(Visual(10).r == 0);
	CHECK(Visual(10).g == 80);
	CHECK(Visual(10).b == 0);
	CHECK(Visual(10).strength == doctest::Approx(500));

	// Looking away restores black and zeroes the strength:
	ggtest::SetNextRayCastResult(0);
	f.Pinpoint(10, 2000, 1, 0);
	CHECK(Visual(10).r == 0);
	CHECK(Visual(10).g == 0);
	CHECK(Visual(10).b == 0);
	CHECK(Visual(10).strength == doctest::Approx(0));
}

TEST_CASE("module_misclib.pinpoint pastes the icon for the icon highlight mode")
{
	MisclibFixture f;
	f.PlaceSelectionEntity(1000, 31, 0);
	ggtest::SetNextRayCastResult(55);

	f.Pinpoint(10, 2000, 3, 7);
	CHECK(g_iconPasteCalls == 1);

	// The icon mode draws neither the cross pointer nor the outline:
	CHECK(g_textCalls == 0);
	CHECK(Visual(10).outline == 0);
}

TEST_CASE("module_misclib.pinpoint draws the near-miss dot pointer")
{
	MisclibFixture f;
	// Entity sits in front of the player but the ray misses it:
	f.PlaceSelectionEntity(500, 31, 0);
	ggtest::SetNextRayCastResult(0);

	f.Pinpoint(10, 2000, 0, 0);

	CHECK(g_textCalls == 1);
	CHECK(g_lastText == ".");
}
