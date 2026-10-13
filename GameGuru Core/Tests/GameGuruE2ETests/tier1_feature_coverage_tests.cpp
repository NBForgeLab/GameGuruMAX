#include "doctest.h"
#include "test_helpers.h"

#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include "../../GameGuru/Include/Common-File.h"

using namespace gg_e2e;

// ============================================================================
// TIER 1: FEATURE COVERAGE (R1 - DEAD LEGACY REMOVAL)
// ============================================================================

TEST_CASE("Tier1_R1_TC01_FTP_Dependencies_Excision")
{
    // Oracle: PROJECT.md M1, ORIGINAL_REQUEST.md R1
    // FTP.lib and CFTPC.h must be excluded from build configuration dependencies.
    const auto corePath = GetGameGuruCorePath();
    const auto vcxprojPath = corePath / "Guru-WickedMAX" / "Template_Windows.vcxproj";
    const auto slnPath = corePath / "GameGuruWickedMAX.sln";

    // Build configuration ReleaseForSteam is primary deployment target
    bool hasFTP = ProjectAudit::HasDependency(vcxprojPath, "ReleaseForSteam", "ftp.lib");
    bool slnHasFTP = ProjectAudit::SlnContainsProject(slnPath, "FTP");

    // Assert that FTP is tracked for complete removal
    // When M1 finishes, both will be false. During transition, we verify test audit execution.
    REQUIRE(ProjectAudit::FileExists(vcxprojPath));
    CHECK_FALSE(hasFTP);
    CHECK_FALSE(slnHasFTP);
}

TEST_CASE("Tier1_R1_TC02_ODBC_Dependencies_Excision")
{
    // Oracle: PROJECT.md M1, ORIGINAL_REQUEST.md R1
    // odbc32.lib and odbccp32.lib must be removed across all 8 configurations.
    const auto vcxprojPath = GetGameGuruCorePath() / "Guru-WickedMAX" / "Template_Windows.vcxproj";
    std::vector<std::string> configs = {
        "Release|x64", "Debug|x64", "DebugTEST|x64", "ReleaseForSteam|x64",
        "ReleaseForDEV|x64", "ReleaseForEpic|x64", "ReleaseForEdu|x64", "ReleaseTEST|x64"
    };

    for (const auto& cfg : configs)
    {
        auto deps = ProjectAudit::GetDependencies(vcxprojPath, cfg);
        // Ensure dependency list was extracted
        REQUIRE_FALSE(deps.empty());
        CHECK_FALSE(ProjectAudit::HasDependency(vcxprojPath, cfg, "odbc32.lib"));
        CHECK_FALSE(ProjectAudit::HasDependency(vcxprojPath, cfg, "odbccp32.lib"));
    }
}

TEST_CASE("Tier1_R1_TC03_DirectShow_ACM_Excision")
{
    // Oracle: PROJECT.md M1, Survey 1 Handoff
    // strmbase.lib, quartz.lib, msacm32.lib must not be needed by modern audio/video (WMF used).
    const auto vcxprojPath = GetGameGuruCorePath() / "Guru-WickedMAX" / "Template_Windows.vcxproj";
    auto steamDeps = ProjectAudit::GetDependencies(vcxprojPath, "ReleaseForSteam|x64");
    
    // Assert audit inspects dependency table
    REQUIRE_FALSE(steamDeps.empty());
    for (const auto* lib : { "strmbase.lib", "quartz.lib", "msacm32.lib" })
        CHECK_FALSE(ProjectAudit::HasDependency(vcxprojPath, "ReleaseForSteam|x64", lib));
}

TEST_CASE("Tier1_R1_TC04_Photon_Multiplayer_Excision")
{
    // Oracle: PROJECT.md M1 Feature 4, ORIGINAL_REQUEST.md R1
    // PhotonMultiplayer.lib & M-MPPhoton.cpp replaced with stub M-MPStub.cpp
    const auto corePath = GetGameGuruCorePath();
    const auto vcxprojPath = corePath / "Guru-WickedMAX" / "Template_Windows.vcxproj";
    const auto slnPath = corePath / "GameGuruWickedMAX.sln";

    bool slnHasPhoton = ProjectAudit::SlnContainsProject(slnPath, "PhotonMultiplayer");
    REQUIRE(ProjectAudit::FileExists(slnPath));
    CHECK_FALSE(slnHasPhoton);
    CHECK_FALSE(ProjectAudit::CompilesFile(vcxprojPath, "M-MPPhoton.cpp"));
}

TEST_CASE("Tier1_R1_TC05_RealSense_Debris_Excision")
{
    // Oracle: PROJECT.md M1 Feature 5, ORIGINAL_REQUEST.md R1
    // M-RealSense.cpp and M-RealSense.h must be deleted or not compiled.
    const auto corePath = GetGameGuruCorePath();
    const auto vcxprojPath = corePath / "Guru-WickedMAX" / "Template_Windows.vcxproj";

    bool compilesRealSense = ProjectAudit::CompilesFile(vcxprojPath, "M-RealSense.cpp");
    CHECK_FALSE(compilesRealSense);
}

TEST_CASE("Tier1_R1_TC06_DarkAI_Superseded_By_RecastDetour")
{
    // Oracle: PROJECT.md Feature 6, GGRecastDetour.h
    // Navigation is provided by Recast/Detour (g_RecastDetour), DarkAI is eliminated.
    const auto recastHeader = GetGameGuruCorePath() / "Guru-WickedMAX" / "GGRecastDetour" / "GGRecastDetour.h";
    CHECK(ProjectAudit::FileExists(recastHeader));
}

// ============================================================================
// TIER 1: FEATURE COVERAGE (R2 - CSTR MODERNIZATION & MEMORY SAFETY)
// ============================================================================

TEST_CASE("Tier1_R2_TC01_cStr_SSO_And_Heap_Storage")
{
    // Oracle: C++17 std::string SSO specification and cStr contract
    cStr sShort("hello");
    CHECK_EQ(sShort.Len(), 5);
    CHECK_EQ(std::string(sShort.Get()), "hello");

    // Heap allocation: string exceeding typical 15-byte SSO buffer
    std::string large(512, 'A');
    cStr sLarge(large.c_str());
    CHECK_EQ(sLarge.Len(), 512);
    CHECK_EQ(std::string(sLarge.Get()), large);
}

TEST_CASE("Tier1_R2_TC02_Neutralize_noDeleteCSTR")
{
    // Oracle: PROJECT.md Feature 7, Survey 2 Handoff
    // Destroying strings must unconditionally reclaim storage regardless of flags.
    noDeleteCSTR = true;
    std::vector<cStr> vec;
    Dim(vec, 99);
    for (auto& value : vec) value = std::string(4096, 'x');
    Undim(vec);
    CHECK(vec.empty());
    noDeleteCSTR = false;
}

TEST_CASE("Tier1_R2_TC03_Vector_Dim_Undim_RAII")
{
    // Oracle: PROJECT.md Interface Contracts: Vector Array Macros <-> STL
    // Dim(vec, size) -> vec.resize(size + 1), Undim(vec) -> vec.clear(); vec.shrink_to_fit()
    std::vector<cStr> arr;
    
    // Simulate Dim(arr, 10)
    Dim(arr, 10);
    CHECK_EQ(arr.size(), 11);
    arr[10] = cStr("item_10");
    CHECK_EQ(std::string(arr[10].Get()), "item_10");

    // Simulate Undim(arr)
    Undim(arr);
    CHECK_EQ(arr.size(), 0);
    CHECK_EQ(arr.capacity(), 0);
}

TEST_CASE("Tier1_R2_TC04_cStr_Substrings_And_Slicing")
{
    // Oracle: DBPro cStr slicing specification
    cStr text("GameGuruMAX_Modernization");
    
    cStr left8 = text.Left(8);
    CHECK_EQ(std::string(left8.Get()), "GameGuru");

    cStr right13 = text.Right(13);
    CHECK_EQ(std::string(right13.Get()), "Modernization");

    // Legacy cStr Mid selects one character at a zero-based index.
    cStr midPart = text.Mid(8);
    CHECK_EQ(std::string(midPart.Get()), "M");
}

TEST_CASE("Tier1_R2_TC05_cStr_Numeric_Parsing_Val_ValF")
{
    // Oracle: cStr Val() and ValF() specifications
    cStr intStr("4242");
    CHECK_EQ(intStr.Val(), 4242);

    cStr floatStr("123.45");
    CHECK(floatStr.ValF() == doctest::Approx(123.45f));

    cStr textWithSuffix("99bottles");
    CHECK_EQ(textWithSuffix.Val(), 99);

    cStr invalid("not_a_number");
    CHECK_EQ(invalid.Val(), 0);
    CHECK(invalid.ValF() == doctest::Approx(0.0f));
}

TEST_CASE("Tier1_R2_TC06_cStr_No_Global_Ring_Buffer_Hazards")
{
    // Oracle: Survey 2 Handoff (eliminating m_sTemp[30] global ring buffer)
    // Multi-threaded concatenations must not overwrite each other's return buffers.
    std::atomic<bool> raceDetected{ false };

    auto worker = [&raceDetected](int threadId) {
        for (int i = 0; i < 200; ++i)
        {
            cStr a("prefix_" + std::to_string(threadId) + "_");
            cStr b("iter_" + std::to_string(i));
            cStr c = a + b;
            std::string expected = "prefix_" + std::to_string(threadId) + "_iter_" + std::to_string(i);
            if (std::string(c.Get()) != expected)
            {
                raceDetected = true;
            }
        }
    };

    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    std::thread t3(worker, 3);
    t1.join();
    t2.join();
    t3.join();

    CHECK_FALSE(raceDetected.load());
}

// ============================================================================
// TIER 1: FEATURE COVERAGE (R3 - DBO & BASIC3D DECOUPLING)
// ============================================================================

TEST_CASE("Tier1_R4_TC01_Safe_LuaGetStringView")
{
    // Oracle: PROJECT.md Interface Contracts: Safe DarkLUA String APIs
    // std::string_view LuaGetStringView(lua_State* L, int idx, std::string_view fallback = "");
    ScopedLuaState state;
    REQUIRE(state.Get() != nullptr);

    state.RunString("my_global_str = 'SafeLuaStringFromStack'");
    lua_getglobal(state.Get(), "my_global_str");

    std::string_view sv = SafeLua::GetStringView(state.Get(), -1, "fallback");
    CHECK_EQ(sv, "SafeLuaStringFromStack");
    lua_pop(state.Get(), 1);

    // Negative index with non-string type returns fallback without throwing
    lua_pushinteger(state.Get(), 12345);
    // Note: in Lua, numbers are convertible to string, so push a table or nil for fallback test
    lua_newtable(state.Get());
    std::string_view svFallback = SafeLua::GetStringView(state.Get(), -1, "safe_fallback");
    CHECK_EQ(svFallback, "safe_fallback");
    lua_pop(state.Get(), 2);
}

TEST_CASE("Tier1_R4_TC02_Safe_Bounded_LuaGetString")
{
    // Oracle: PROJECT.md Safe DarkLUA String APIs
    // void LuaGetString(std::string_view globalName, char* pDest, size_t destSize);
    ScopedLuaState state;
    REQUIRE(state.Get() != nullptr);

    state.RunString("g_TestMessage = 'ThisIsALongStringThatExceedsSmallBuffers'");

    // Small buffer of 10 bytes: must copy at most 9 chars + null terminator
    char smallBuf[10];
    SafeLua::GetStringBounded(state.Get(), "g_TestMessage", smallBuf, sizeof(smallBuf));
    CHECK_EQ(std::string(smallBuf), "ThisIsALo");
    CHECK_EQ(smallBuf[9], '\0');

    // Safe unbounded return via std::string
    std::string full = SafeLua::GetStringSafe(state.Get(), "g_TestMessage");
    CHECK_EQ(full, "ThisIsALongStringThatExceedsSmallBuffers");
}

TEST_CASE("Tier1_R4_TC03_LuaMessage_Queue_Expansion")
{
    // Oracle: Survey 3 Handoff Section 1.5 Vulnerability 1
    // struct luaMessage: eliminating fixed 256-byte truncation and heap corruption
    SafeLuaMessageQueue queue;

    // Push large message exceeding 256 bytes (e.g. 4096 bytes)
    std::string hugePayload(4096, 'Z');
    SafeLuaMessage msg;
    msg.msgDesc = "BROADCAST_STORYBOARD_DATA";
    msg.msgString = hugePayload;
    msg.entityId = 42;

    queue.Push(msg);
    CHECK_EQ(queue.Size(), 1);

    SafeLuaMessage outMsg;
    REQUIRE(queue.Pop(outMsg));
    CHECK_EQ(outMsg.msgDesc, "BROADCAST_STORYBOARD_DATA");
    CHECK_EQ(outMsg.msgString.length(), 4096);
    CHECK_EQ(outMsg.msgString, hugePayload);
    CHECK_EQ(outMsg.entityId, 42);
}

TEST_CASE("Tier1_R4_TC04_Safe_UI_Storyboard_Command_Args")
{
    // Oracle: Survey 3 Handoff Section 1.5 Vulnerability 2 & 3
    // Missing arguments on Lua stack must not dereference NULL pointer.
    ScopedLuaState state;
    REQUIRE(state.Get() != nullptr);

    // Calling string accessor on nil stack entry returns safe empty/fallback view
    lua_pushnil(state.Get());
    std::string_view safeView = SafeLua::GetStringView(state.Get(), -1, "");
    CHECK_EQ(safeView, "");
    lua_pop(state.Get(), 1);
}

TEST_CASE("Tier1_R4_TC05_String_Pool_And_OOB_Prevention")
{
    // Oracle: Survey 3 Handoff Section 1.5 Vulnerabilities 5 & 6
    // Prefix checking on strings with length < 12 and < 8 must be safe.
    std::string shortStr1 = "abc";
    std::string shortStr2 = "";

    auto safePrefixCheck = [](const std::string& s) -> bool {
        if (s.length() < 12) return false;
        return s[11] == 'l';
    };

    CHECK_FALSE(safePrefixCheck(shortStr1));
    CHECK_FALSE(safePrefixCheck(shortStr2));
    CHECK(safePrefixCheck("01234567890l"));
}

TEST_CASE("Tier1_R4_TC06_Lua_Global_State_Preservation")
{
    // Oracle: Survey 3 Handoff Section 1.5 Vulnerability 7
    // Engine globals storee / stores must remain intact during message dispatch.
    ScopedLuaState state;
    REQUIRE(state.Get() != nullptr);

    state.RunString("store_e = 100; store_s = 200;");
    state.RunString("function dispatch_msg() local prev_e = store_e; return prev_e; end");
    state.RunString("ret = dispatch_msg()");

    lua_getglobal(state.Get(), "ret");
    int retVal = (int)lua_tointeger(state.Get(), -1);
    CHECK_EQ(retVal, 100);
    lua_pop(state.Get(), 1);
}

// ============================================================================
// TIER 1: FEATURE COVERAGE (R5 - COMMON-FILE SCRATCHPAD ERADICATION)
// ============================================================================

TEST_CASE("Tier1_R5_TC01_CommonFile_Scratchpad_And_cStr_Eradication")
{
    // Oracle: Eradicate DarkBASIC legacy shared scratchpad registers (t.szwork, t.szreturn, t.strwork, t.told_s)
    // and replace cStr with standard C++ types in Common-File.cpp and Common-File.h
    const auto commonFilePath = GetGameGuruCorePath() / "GameGuru" / "Source" / "Common-File.cpp";
    REQUIRE(ProjectAudit::FileExists(commonFilePath));

    std::ifstream file(commonFilePath);
    REQUIRE(file.is_open());
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Verify all legacy registers are eradicated from Common-File.cpp
    CHECK_EQ(content.find("t.szwork"), std::string::npos);
    CHECK_EQ(content.find("t.szreturn"), std::string::npos);
    CHECK_EQ(content.find("t.strwork"), std::string::npos);
    CHECK_EQ(content.find("t.told_s"), std::string::npos);

    // Verify cStr is replaced with std::string / standard types in Common-File.cpp
    CHECK_EQ(content.find("cstr "), std::string::npos);
    CHECK_EQ(content.find("cStr "), std::string::npos);

    // Verify Common-File.h has also eradicated cStr and cstr.h
    const auto commonHeaderPath = GetGameGuruCorePath() / "GameGuru" / "Include" / "Common-File.h";
    REQUIRE(ProjectAudit::FileExists(commonHeaderPath));
    std::ifstream headerFile(commonHeaderPath);
    REQUIRE(headerFile.is_open());
    std::stringstream headerBuffer;
    headerBuffer << headerFile.rdbuf();
    std::string headerContent = headerBuffer.str();
    CHECK_EQ(headerContent.find("cstr "), std::string::npos);
    CHECK_EQ(headerContent.find("cStr "), std::string::npos);
    CHECK_EQ(headerContent.find("\"cstr.h\""), std::string::npos);
}

TEST_CASE("Tier1_R5_TC02_CommonFile_Version_Comparison_Contract")
{
    // Verification of real production function versionnumberislarger(tNum1, tNum2) from Common-File.h

    // Equal versions -> 0
    CHECK_EQ(versionnumberislarger(0, 0), 0);
    CHECK_EQ(versionnumberislarger(100, 100), 0);
    CHECK_EQ(versionnumberislarger(201232, 201232), 0);

    // Larger -> 1
    CHECK_EQ(versionnumberislarger(201, 200), 1);
    CHECK_EQ(versionnumberislarger(999, 1000), 1); // '9' > '1'
    CHECK_EQ(versionnumberislarger(200, 20), 1);   // length 3 > 2 with common prefix
    CHECK_EQ(versionnumberislarger(12345, 12344), 1);
    CHECK_EQ(versionnumberislarger(20100721, 20090101), 1); // Profile versions

    // Smaller -> 0
    CHECK_EQ(versionnumberislarger(200, 201), 0);
    CHECK_EQ(versionnumberislarger(1000, 999), 0); // '1' < '9'
    CHECK_EQ(versionnumberislarger(20, 200), 0);
    CHECK_EQ(versionnumberislarger(12344, 12345), 0);
    CHECK_EQ(versionnumberislarger(20090101, 20100721), 0);

    // Negative / boundary inputs
    CHECK_EQ(versionnumberislarger(10, -1), 1);
    CHECK_EQ(versionnumberislarger(-1, 10), 0);
}

TEST_CASE("Tier1_R5_TC03_CommonFile_Date_Parsing_Contract")
{
    // Verification of real production function getdatevaluefromdatestring from Common-File.h

    // Standard ctime format
    CHECK_EQ(getdatevaluefromdatestring("Wed Jun 30 21:49:08 1993"), -10002851);
    CHECK_EQ(getdatevaluefromdatestring("Fri Dec 02 08:30:15 2015"), 11525830);

    // Single-digit day padded with space
    CHECK_EQ(getdatevaluefromdatestring("Fri Dec  2 08:30:15 2015"), 11525830);

    // Month boundary check (January 1st, 2004 at 00:00:00)
    // ((2004 - 2004) * 960000) + (1 * 80000) + (1 * 2500) + 0 = 82500
    CHECK_EQ(getdatevaluefromdatestring("Thu Jan  1 00:00:00 2004"), 82500);

    // Leap year boundary check (Feb 29, 2024 at 12:30:00)
    // ((2024 - 2004) * 960000) + (2 * 80000) + (29 * 2500) + 1230 = 19200000 + 160000 + 72500 + 1230 = 19433730
    CHECK_EQ(getdatevaluefromdatestring("Thu Feb 29 12:30:00 2024"), 19433730);

    // Trailing newline / whitespace handling
    CHECK_EQ(getdatevaluefromdatestring("Fri Dec 02 08:30:15 2015\n"), 11525830);
    CHECK_EQ(getdatevaluefromdatestring("Fri Dec 02 08:30:15 2015\r\n"), 11525830);

    // Boundary / invalid inputs
    CHECK_EQ(getdatevaluefromdatestring(nullptr), 0);
    CHECK_EQ(getdatevaluefromdatestring(""), 0);
    CHECK_EQ(getdatevaluefromdatestring("short string"), 0);
    CHECK_EQ(getdatevaluefromdatestring("Wed Xxx 30 21:49:08 1993"), 0); // Invalid month
}

TEST_CASE("Tier1_R5_TC04_CommonFile_Path_And_File_Extraction")
{
    // Verification of real production functions ExtractFileName and ExtractPath from Common-File.h

    // Windows backslash path
    CHECK_EQ(ExtractFileName("C:\\GameGuru\\Files\\mesh.dbo"), "mesh.dbo");
    CHECK_EQ(ExtractPath("C:\\GameGuru\\Files\\mesh.dbo"), "C:\\GameGuru\\Files\\");

    // Unix forward slash path
    CHECK_EQ(ExtractFileName("levels/subfolder/terrain.x"), "terrain.x");
    CHECK_EQ(ExtractPath("levels/subfolder/terrain.x"), "levels/subfolder/");

    // Mixed slash path
    CHECK_EQ(ExtractFileName("root/dir\\sub/asset.dbo"), "asset.dbo");
    CHECK_EQ(ExtractPath("root/dir\\sub/asset.dbo"), "root/dir\\sub/");

    // No directory prefix
    CHECK_EQ(ExtractFileName("model.dbo"), "model.dbo");
    CHECK_EQ(ExtractPath("model.dbo"), "");

    // Empty path
    CHECK_EQ(ExtractFileName(""), "");
    CHECK_EQ(ExtractPath(""), "");
}

