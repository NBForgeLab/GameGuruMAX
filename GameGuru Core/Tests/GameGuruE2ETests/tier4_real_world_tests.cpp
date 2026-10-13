#include "doctest.h"
#include "test_helpers.h"

#include <vector>
#include <string>
#include <cmath>

using namespace gg_e2e;

// ============================================================================
// TIER 4: REAL-WORLD APPLICATION SCENARIOS
// ============================================================================

TEST_CASE("Tier4_Lua_Interpreter_Callback_And_Production_String_Reads")
{
    // Real-World Scenario: Complete headless lifecycle of an entity script
    // (_properties -> _init -> _main x 5 frames -> _exit)
    ScopedLuaState state;
    REQUIRE(state.Get() != nullptr);

    // Define a representative entity behavior script in Lua
    const std::string scriptCode = R"(
        sentry = {}
        sentry_state = "IDLE"
        sentry_frame_count = 0
        sentry_health = 100

        function sentry_properties(e, health, range)
            if health ~= nil then sentry_health = health end
        end

        function sentry_init(e)
            sentry_state = "ACTIVE"
            sentry_frame_count = 0
        end

        function sentry_main(e)
            sentry_frame_count = sentry_frame_count + 1
            if sentry_frame_count > 3 then
                sentry_state = "PATROL"
            end
        end

        function sentry_exit(e)
            sentry_state = "SHUTDOWN"
        end
    )";

    std::string err;
    REQUIRE_MESSAGE(state.RunString(scriptCode, &err), err);

    // 1. Properties phase
    state.RunString("sentry_properties(1, 200, 50)");
    // 2. Init phase
    state.RunString("sentry_init(1)");
    
    lua_getglobal(state.Get(), "sentry_state");
    std::string_view stateInit = SafeLua::GetStringView(state.Get(), -1, "");
    CHECK_EQ(stateInit, "ACTIVE");
    lua_pop(state.Get(), 1);

    // 3. Main loop (5 frames)
    for (int frame = 0; frame < 5; ++frame)
    {
        state.RunString("sentry_main(1)");
    }

    lua_getglobal(state.Get(), "sentry_state");
    std::string_view statePatrol = SafeLua::GetStringView(state.Get(), -1, "");
    CHECK_EQ(statePatrol, "PATROL");
    lua_pop(state.Get(), 1);

    // 4. Exit phase
    state.RunString("sentry_exit(1)");
    lua_getglobal(state.Get(), "sentry_state");
    std::string_view stateExit = SafeLua::GetStringView(state.Get(), -1, "");
    CHECK_EQ(stateExit, "SHUTDOWN");
    lua_pop(state.Get(), 1);
}

TEST_CASE("Tier4_Production_Lua_Queue_FIFO_With_Script_Results")
{
    // Real-World Scenario: Storyboard UI message flow
    // Level starts -> Message queue emits HUD message -> Trigger tripped -> Victory
    SafeLuaMessageQueue msgQueue;
    ScopedLuaState state;
    REQUIRE(state.Get() != nullptr);

    // 1. Level starts
    SafeLuaMessage startMsg{ "UI_SHOW_HUD", "MISSION_OBJECTIVE: ESCAPE_SECTOR", 1 };
    msgQueue.Push(startMsg);

    // 2. Lua script updates objective
    state.RunString("objective_complete = false");
    state.RunString("function on_trigger_enter() objective_complete = true; return 'MISSION_SUCCESS'; end");
    state.RunString("res = on_trigger_enter()");

    std::string resStr = SafeLua::GetStringSafe(state.Get(), "res");
    CHECK_EQ(resStr, "MISSION_SUCCESS");

    // 3. Push victory message
    SafeLuaMessage winMsg{ "UI_SHOW_WIN", "LEVEL_CLEAR: PROCEED_NEXT_LEVEL", 1 };
    msgQueue.Push(winMsg);

    CHECK_EQ(msgQueue.Size(), 2);

    SafeLuaMessage m1, m2;
    REQUIRE(msgQueue.Pop(m1));
    REQUIRE(msgQueue.Pop(m2));
    CHECK_EQ(m1.msgDesc, "UI_SHOW_HUD");
    CHECK_EQ(m2.msgDesc, "UI_SHOW_WIN");
    CHECK_EQ(msgQueue.Size(), 0);
}
