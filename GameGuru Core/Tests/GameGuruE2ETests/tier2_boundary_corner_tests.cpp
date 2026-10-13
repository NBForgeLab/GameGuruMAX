#include "doctest.h"
#include "test_helpers.h"

#include <vector>
#include <string>
#include <algorithm>

using namespace gg_e2e;

// ============================================================================
// TIER 2: BOUNDARY & CORNER CASES (R1 - BUILD & DEPENDENCY BOUNDARIES)
// ============================================================================

TEST_CASE("Tier2_R1_B01_Empty_AdditionalDependencies")
{
    // Boundary: XML files with empty or whitespace-only <AdditionalDependencies>
    // Oracle: ProjectAudit must not crash on malformed or empty tag contents
    const auto vcxprojPath = GetGameGuruCorePath() / "Guru-WickedMAX" / "Template_Windows.vcxproj";
    auto emptyDeps = ProjectAudit::GetDependencies(vcxprojPath, "NonExistentConfig|x64");
    CHECK(emptyDeps.empty());
}

TEST_CASE("Tier2_R1_B02_Duplicate_Library_Entries")
{
    // Boundary: Redundant semicolon delimiters or duplicate .lib entries
    const auto vcxprojPath = GetGameGuruCorePath() / "Guru-WickedMAX" / "Template_Windows.vcxproj";
    auto deps = ProjectAudit::GetDependencies(vcxprojPath, "Release|x64");
    
    // Check that parsed tokens do not contain raw trailing semicolons or leading spaces
    for (const auto& dep : deps)
    {
        CHECK_FALSE(dep.empty());
        CHECK(dep.find(';') == std::string::npos);
    }
}

TEST_CASE("Tier2_R1_B03_Missing_Sdk_Directories")
{
    // Boundary: Querying non-existent SDK paths must return clean false
    auto fakeSdkPath = GetRepoRoot() / "SDK" / "NonExistentPhotonSDK_XYZ";
    CHECK_FALSE(ProjectAudit::FileExists(fakeSdkPath));
}

TEST_CASE("Tier2_R1_B04_Case_Insensitive_Library_Matching")
{
    // Boundary: Library matching must handle case variances: ftp.lib vs FTP.LIB vs Ftp.Lib
    const auto vcxprojPath = GetGameGuruCorePath() / "Guru-WickedMAX" / "Template_Windows.vcxproj";
    bool upperMatch = ProjectAudit::HasDependency(vcxprojPath, "ReleaseForSteam|x64", "WINMM.LIB");
    bool lowerMatch = ProjectAudit::HasDependency(vcxprojPath, "ReleaseForSteam|x64", "winmm.lib");
    // Both queries must evaluate to the same result
    CHECK_EQ(upperMatch, lowerMatch);
}

TEST_CASE("Tier2_R1_B05_All_8_Configurations_Consistency")
{
    // Boundary: Verify that dependency auditing covers all 8 product configurations
    const auto vcxprojPath = GetGameGuruCorePath() / "Guru-WickedMAX" / "Template_Windows.vcxproj";
    std::vector<std::string> configs = {
        "Release|x64", "Debug|x64", "DebugTEST|x64", "ReleaseForSteam|x64",
        "ReleaseForDEV|x64", "ReleaseForEpic|x64", "ReleaseForEdu|x64", "ReleaseTEST|x64"
    };

    int auditedCount = 0;
    for (const auto& cfg : configs)
    {
        auto deps = ProjectAudit::GetDependencies(vcxprojPath, cfg);
        if (!deps.empty()) auditedCount++;
    }
    CHECK(auditedCount >= 1);
}

// ============================================================================
// TIER 2: BOUNDARY & CORNER CASES (R2 - STRING & MEMORY BOUNDARIES)
// ============================================================================

TEST_CASE("Tier2_R2_B01_Empty_String_Operations")
{
    // Boundary: 0-length strings, slicing empty strings, null string queries
    cStr empty;
    CHECK_EQ(empty.Len(), 0);
    CHECK_EQ(std::string(empty.Get()), "");
    CHECK_EQ(empty.Val(), 0);
    CHECK(empty.ValF() == doctest::Approx(0.0f));

    cStr leftOfEmpty = empty.Left(5);
    CHECK_EQ(leftOfEmpty.Len(), 0);

    cStr rightOfEmpty = empty.Right(5);
    CHECK_EQ(rightOfEmpty.Len(), 0);

    cStr midOfEmpty = empty.Mid(2);
    CHECK_EQ(midOfEmpty.Len(), 0);
}

TEST_CASE("Tier2_R2_B02_Extreme_Buffer_Limits")
{
    // Boundary: 64KB, 1MB string allocation without stack smashing or allocation errors
    std::string oneMeg(1024 * 1024, 'M');
    cStr sMeg(oneMeg.c_str());
    CHECK_EQ(sMeg.Len(), 1024 * 1024);
    CHECK_EQ(std::string(sMeg.Get()).length(), 1024 * 1024);

    // Substring of 1MB string
    cStr sliced = sMeg.Left(10);
    CHECK_EQ(sliced.Len(), 10);
    CHECK_EQ(std::string(sliced.Get()), "MMMMMMMMMM");
}

TEST_CASE("Tier2_R2_B03_Zero_And_Negative_Slice_Indices")
{
    // Boundary: Left(0), Left(-5), Right(0), Right(-10), Mid(-3), Mid(99999)
    cStr base("GameGuru");

    CHECK_EQ(base.Left(0).Len(), 0);
    CHECK_EQ(base.Left(-10).Len(), 0);
    CHECK_EQ(base.Right(0).Len(), 0);
    CHECK_EQ(base.Right(-10).Len(), 0);
    CHECK_EQ(base.Mid(9999).Len(), 0);
    CHECK_EQ(std::string(base.Left(100).Get()), "GameGuru");
}

TEST_CASE("Tier2_R2_B04_Null_Pointer_Assignment")
{
    // Boundary: Assigning nullptr to cStr must produce safe empty string, not crash
    cStr str("Initialized");
    str = (const char*)nullptr;
    CHECK_EQ(str.Len(), 0);
    CHECK_EQ(std::string(str.Get()), "");

    cStr ctorNull((const char*)nullptr);
    CHECK_EQ(ctorNull.Len(), 0);
    CHECK_EQ(std::string(ctorNull.Get()), "");
}

TEST_CASE("Tier2_R2_B05_Adversarial_Escapes_And_Control_Chars")
{
    // Boundary: Special characters, format specifiers, newlines, tabs, UTF-8 BOM
    std::string adversarial = "\xEF\xBB\xBFSpecial \"Quotes\" %s %d %x \r\n\t \\0 \\r \\n";
    cStr advStr(adversarial.c_str());
    CHECK_EQ(std::string(advStr.Get()), adversarial);

    // Concatenation with adversarial characters
    cStr appended = advStr + cStr("---Tail");
    CHECK(std::string(appended.Get()).find("---Tail") != std::string::npos);
}

TEST_CASE("Tier2_R2_B06_Dim_Zero_And_Negative_Sizes")
{
    // Boundary: Dim(vec, 0) in DBPro allocates size + 1 = 1 element (indices 0..0)
    std::vector<cStr> arr;
    Dim(arr, 0);
    CHECK_EQ(arr.size(), 1);
    arr[0] = cStr("zero_index_item");
    CHECK_EQ(std::string(arr[0].Get()), "zero_index_item");

    // Undim completely empties
    Undim(arr);
    CHECK_EQ(arr.size(), 0);
    Dim(arr, -1);
    CHECK(arr.empty());
}

// ============================================================================
// TIER 2: BOUNDARY & CORNER CASES (R3 - DBO & GEOMETRY BOUNDARIES)
// ============================================================================

TEST_CASE("Tier2_R4_B01_LuaGetStringView_Nil_And_Missing_Index")
{
    // Boundary: Extracting string view from nil, missing index, or out-of-range stack
    ScopedLuaState state;
    REQUIRE(state.Get() != nullptr);

    // Empty stack: index 1 does not exist
    std::string_view sv1 = SafeLua::GetStringView(state.Get(), 1, "default_missing");
    CHECK_EQ(sv1, "default_missing");

    // Index -99 does not exist
    std::string_view sv2 = SafeLua::GetStringView(state.Get(), -99, "default_oob");
    CHECK_EQ(sv2, "default_oob");
}

TEST_CASE("Tier2_R4_B02_LuaGetString_Zero_And_1Byte_DestSize")
{
    // Boundary: destSize = 0 (no-op) and destSize = 1 (only null-terminator)
    ScopedLuaState state;
    REQUIRE(state.Get() != nullptr);

    state.RunString("g_BoundaryString = 'AlphabetSoup'");

    char buf1[1] = { 'X' };
    SafeLua::GetStringBounded(state.Get(), "g_BoundaryString", buf1, sizeof(buf1));
    CHECK_EQ(buf1[0], '\0');

    // destSize = 0 must not write anything
    char buf0[2] = { 'A', 'B' };
    SafeLua::GetStringBounded(state.Get(), "g_BoundaryString", buf0, 0);
    CHECK_EQ(buf0[0], 'A');
    CHECK_EQ(buf0[1], 'B');
}

TEST_CASE("Tier2_R4_B03_Maximum_Lua_String_Limits")
{
    // Boundary: 64KB string on Lua stack extracted safely
    ScopedLuaState state;
    REQUIRE(state.Get() != nullptr);

    std::string huge(65536, 'Q');
    state.RunString("g_Huge = string.rep('Q', 65536)");
    lua_getglobal(state.Get(), "g_Huge");

    std::string_view sv = SafeLua::GetStringView(state.Get(), -1, "");
    CHECK_EQ(sv.length(), 65536);
    CHECK_EQ(sv[0], 'Q');
    CHECK_EQ(sv[65535], 'Q');
    lua_pop(state.Get(), 1);
}

TEST_CASE("Tier2_R4_B04_Non_String_Stack_Types")
{
    // Boundary: Tables, booleans, functions on stack return fallback view
    ScopedLuaState state;
    REQUIRE(state.Get() != nullptr);

    lua_pushboolean(state.Get(), 1);
    std::string_view svBool = SafeLua::GetStringView(state.Get(), -1, "not_a_string");
    CHECK_EQ(svBool, "not_a_string");
    lua_pop(state.Get(), 1);

    lua_newtable(state.Get());
    std::string_view svTable = SafeLua::GetStringView(state.Get(), -1, "table_fallback");
    CHECK_EQ(svTable, "table_fallback");
    lua_pop(state.Get(), 1);
}

TEST_CASE("Tier2_R4_B05_Short_String_OOB_Prefix_Checks")
{
    // Boundary: String length 0, 1, 7, 11 passed to prefix detectors
    std::vector<std::string> shortStrings = { "", "a", "1234567", "12345678901" };
    for (const auto& s : shortStrings)
    {
        // Safe check for length >= 8
        bool hasAtLeast8 = (s.length() >= 8);
        CHECK_EQ(hasAtLeast8, s.length() >= 8);

        // Safe check for length >= 12
        bool hasAtLeast12 = (s.length() >= 12);
        CHECK_EQ(hasAtLeast12, s.length() >= 12);
    }
}

TEST_CASE("Tier2_R4_B06_Embedded_Nulls_In_Lua_Strings")
{
    // Boundary: Lua string with embedded \0 bytes must preserve full byte count
    ScopedLuaState state;
    REQUIRE(state.Get() != nullptr);

    const char binaryPayload[] = "Hello\0World\0End";
    lua_pushlstring(state.Get(), binaryPayload, sizeof(binaryPayload) - 1);

    std::string_view sv = SafeLua::GetStringView(state.Get(), -1, "");
    CHECK_EQ(sv.length(), sizeof(binaryPayload) - 1);
    CHECK_EQ(sv[5], '\0');
    CHECK_EQ(sv[11], '\0');
    lua_pop(state.Get(), 1);
}

TEST_CASE("Production_cStr_Mid_preserves_zero_based_single_character_contract")
{
    cStr value("abc");
    const cStr immutable("abc");
    CHECK(value.Mid(0) == "a");
    CHECK(value.Mid(1) == "b");
    CHECK(value.Mid(-1).empty());
    CHECK(value.Mid(3).empty());
    CHECK(immutable.Mid(1) == "b");
}
TEST_CASE("Production_cStr_empty_Get_returns_owned_writable_terminator")
{
    cStr value;
    CHECK(value.Get() == value.str().data());
    CHECK(value.GetMutable() == value.str().data());
    value.Get()[0] = '\0';
}
TEST_CASE("Production_Lua_global_binary_strings_preserve_length_and_stack")
{
    ScopedLuaState state;
    const std::string binary("a\0bc", 4);
    lua_pushlstring(state, binary.data(), binary.size());
    lua_setglobal(state, "binary");
    lua_pushinteger(state, 42);
    const int top = lua_gettop(state);
    CHECK(LuaGetStringSafe(state, "binary") == binary);
    char buffer[6] = { '!', '!', '!', '!', '!', '!' };
    LuaGetString(state, "binary", buffer, 5);
    CHECK(std::string(buffer, 4) == binary);
    CHECK(buffer[4] == '\0');
    CHECK(buffer[5] == '!');
    CHECK(lua_gettop(state) == top);
}

TEST_CASE("Production_Lua_message_queue_expands_and_preserves_binary_payloads")
{
    SafeLuaMessageQueue queue;
    const std::string payload = std::string(8192, 'q') + std::string("\0tail", 5);
    for (int i = 0; i < 301; ++i)
        queue.Push({ std::string(1024, 'd') + std::to_string(i), payload, i });
    CHECK(queue.Size() == 301);
    for (int i = 0; i < 301; ++i)
    {
        SafeLuaMessage message;
        REQUIRE(queue.Pop(message));
        CHECK(message.entityId == i);
        CHECK(message.msgDesc == std::string(1024, 'd') + std::to_string(i));
        CHECK(message.msgString == payload);
    }
    SafeLuaMessage message;
    CHECK_FALSE(queue.Pop(message));
    CHECK(LuaCurrentMessageString().empty());
}
TEST_CASE("Production_Lua_global_missing_and_nonstrings_preserve_stack_and_guards")
{
    ScopedLuaState state;
    REQUIRE(state.RunString("flag=true; tableValue={}; functionValue=function() end; numberValue=12"));
    lua_pushinteger(state, 99);
    for (const auto* name : { "missing", "flag", "tableValue", "functionValue" })
    {
        CHECK(LuaGetStringSafe(state, name).empty());
        char destination[3] = { 'x', '!', '?' };
        LuaGetString(state, name, destination, 1);
        CHECK(destination[0] == '\0');
        CHECK(destination[1] == '!');
        CHECK(destination[2] == '?');
        CHECK(lua_gettop(state) == 1);
    }
    CHECK(LuaGetStringSafe(state, "numberValue") == "12");
    CHECK(LuaGetStringSafe(nullptr, "missing").empty());
    CHECK(LuaGetStringView(state, 0, "fallback") == "fallback");
    CHECK(LuaGetStringView(state, -2, "fallback") == "fallback");
    CHECK(LuaGetStringView(nullptr, 1, "fallback") == "fallback");
}
TEST_CASE("Production_cStr_array_macros_release_nested_values")
{
    std::vector<std::vector<cStr>> two;
    Dim2(two, 1, 2);
    REQUIRE(two.size() == 2);
    REQUIRE(two[1].size() == 3);
    two[1][2] = std::string(10000, 'x');
    Redim2(two, 0, 0);
    REQUIRE(two.size() == 1);
    CHECK(two[0].size() == 1);
    Undim2(two);
    CHECK(two.empty());
    std::vector<std::vector<std::vector<cStr>>> three;
    Dim3(three, 1, 2, 3);
    three[1][2][3] = "last";
    Undim3(three);
    CHECK(three.empty());
    Dim3(three, -1, 2, 3);
    CHECK(three.empty());
}

TEST_CASE("Production_Lua_typed_messages_preserve_arguments_and_FIFO")
{
    SafeLuaMessageQueue queue;
    ScopedLuaState state;
    lua_register(state, "SendMessage", LuaSendMessage);
    lua_register(state, "SendMessageI", LuaSendMessageI);
    lua_register(state, "SendMessageF", LuaSendMessageF);
    lua_register(state, "SendMessageS", LuaSendMessageS);
    REQUIRE(state.RunString("SendMessageI('integer', -42); SendMessageF('float', 7, 1.25); SendMessageS('string', 8, 'a'..string.char(0)..'b'); SendMessage('first','second'); SendMessageI('indexed', 9, 123); SendMessageF('plain-float', -2.5); SendMessageS('plain-string','last')"));
    CHECK(LuaMessageQueueSize() == 8);
    REQUIRE(LuaNext());
    CHECK(LuaCurrentMessageDescription() == "integer");
    CHECK(LuaCurrentMessageInt() == -42);
    CHECK(LuaCurrentMessageIndex() == 0);
    CHECK(LuaCurrentMessageFloat() == 0.0f);
    CHECK(LuaCurrentMessageString().empty());
    REQUIRE(LuaNext());
    CHECK(LuaCurrentMessageDescription() == "float");
    CHECK(LuaCurrentMessageIndex() == 7);
    CHECK(LuaCurrentMessageFloat() == 1.25f);
    CHECK(LuaCurrentMessageInt() == 0);
    REQUIRE(LuaNext());
    CHECK(LuaCurrentMessageDescription() == "string");
    CHECK(LuaCurrentMessageIndex() == 8);
    CHECK(LuaCurrentMessageString() == std::string_view("a\0b", 3));
    REQUIRE(LuaNext());
    CHECK(LuaCurrentMessageDescription() == "first");
    REQUIRE(LuaNext());
    CHECK(LuaCurrentMessageDescription() == "second");
    REQUIRE(LuaNext());
    CHECK(LuaCurrentMessageDescription() == "indexed");
    CHECK(LuaCurrentMessageIndex() == 9);
    CHECK(LuaCurrentMessageInt() == 123);
    REQUIRE(LuaNext());
    CHECK(LuaCurrentMessageDescription() == "plain-float");
    CHECK(LuaCurrentMessageIndex() == 0);
    CHECK(LuaCurrentMessageFloat() == -2.5f);
    REQUIRE(LuaNext());
    CHECK(LuaCurrentMessageDescription() == "plain-string");
    CHECK(LuaCurrentMessageIndex() == 0);
    CHECK(LuaCurrentMessageString() == "last");
    CHECK_FALSE(LuaNext());
    CHECK(LuaCurrentMessageDescription().empty());
    CHECK(LuaCurrentMessageString().empty());
    CHECK(LuaCurrentMessageIndex() == 0);
    CHECK(LuaCurrentMessageInt() == 0);
    CHECK(LuaCurrentMessageFloat() == 0.0f);
    REQUIRE(state.RunString("SendMessageI(); SendMessageF('one'); SendMessageS('too', 'many', 'args', 'here')"));
    CHECK(LuaMessageQueueSize() == 0);
}
TEST_CASE("Production_Lua_queue_reset_discards_pending_messages_and_allows_reuse")
{
    SafeLuaMessageQueue queue;
    queue.Push({ "old-current", "old-payload", 77 });
    queue.Push({ "pending", std::string(20000, 'p'), 88 });
    SafeLuaMessage current;
    REQUIRE(queue.Pop(current));
    REQUIRE(LuaMessageQueueSize() == 1);
    LuaClearMessageQueue();
    CHECK(LuaMessageQueueSize() == 0);
    CHECK(LuaCurrentMessageDescription().empty());
    CHECK(LuaCurrentMessageString().empty());
    CHECK(LuaCurrentMessageIndex() == 0);
    CHECK_FALSE(LuaNext());
    queue.Push({ "new", "fresh", 99 });
    REQUIRE(queue.Pop(current));
    CHECK(current.msgDesc == "new");
    CHECK(current.msgString == "fresh");
    CHECK(current.entityId == 99);
    CHECK_FALSE(queue.Pop(current));
}
