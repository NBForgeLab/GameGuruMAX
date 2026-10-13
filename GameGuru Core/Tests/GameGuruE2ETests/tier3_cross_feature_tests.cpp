#include "doctest.h"
#include "test_helpers.h"

#include <vector>
#include <string>
#include <sstream>

using namespace gg_e2e;

// ============================================================================
// TIER 3: CROSS-FEATURE COMBINATIONS (PAIRWISE INTERACTIONS)
// ============================================================================

TEST_CASE("Tier3_CrossFeature_02_Production_cStr_Asset_Path_Concatenation")
{
    // Exercise production cStr path concatenation and extension extraction.
    cStr baseDir("Files\\entitybank\\scifi\\");
    cStr modelFile("drone.dbo");
    cStr fullPath = baseDir + modelFile;

    CHECK_EQ(std::string(fullPath.Get()), "Files\\entitybank\\scifi\\drone.dbo");

    // Slicing modern string to extract extension
    cStr ext = fullPath.Right(4);
    CHECK_EQ(std::string(ext.Lower().Get()), ".dbo");
}

TEST_CASE("Tier3_CrossFeature_03_Modern_String_Lua_Message_Pipeline")
{
    // Interaction: R2 (Modern strings) + R4 (Lua message queue)
    // Lua emits a structured message formatted with dynamic variables,
    // which is received and stored in SafeLuaMessageQueue using std::string.
    ScopedLuaState state;
    REQUIRE(state.Get() != nullptr);

    SafeLuaMessageQueue queue;

    state.RunString("player_score = 9950; player_name = 'Commander'");
    state.RunString("formatted_msg = string.format('STATUS:%s:SCORE:%d', player_name, player_score)");

    lua_getglobal(state.Get(), "formatted_msg");
    std::string_view msgView = SafeLua::GetStringView(state.Get(), -1, "");
    
    SafeLuaMessage msg;
    msg.msgDesc = "SCORE_SYNC";
    msg.msgString = std::string(msgView);
    msg.entityId = 1;
    queue.Push(msg);
    lua_pop(state.Get(), 1);

    SafeLuaMessage popped;
    REQUIRE(queue.Pop(popped));
    CHECK_EQ(popped.msgDesc, "SCORE_SYNC");
    CHECK_EQ(popped.msgString, "STATUS:Commander:SCORE:9950");
}

TEST_CASE("Tier3_CrossFeature_06_String_Array_Dim_Lua_Global_Sync")
{
    // Interaction: R2 (Dim array) + R4 (Lua table synchronization)
    // Resize using production Dim and populate cStr values from Lua.
    ScopedLuaState state;
    REQUIRE(state.Get() != nullptr);

    state.RunString("entity_names = { 'Guard', 'Vendor', 'Boss', 'Civilian' }");

    std::vector<cStr> arr;
    Dim(arr, 4);

    for (int i = 1; i <= 4; ++i)
    {
        state.RunString("cur_name = entity_names[" + std::to_string(i) + "]");
        std::string name = SafeLua::GetStringSafe(state.Get(), "cur_name");
        arr[i] = cStr(name);
    }

    CHECK_EQ(std::string(arr[1].Get()), "Guard");
    CHECK_EQ(std::string(arr[3].Get()), "Boss");

    // Undim
    Undim(arr);
    CHECK_EQ(arr.size(), 0);
}

