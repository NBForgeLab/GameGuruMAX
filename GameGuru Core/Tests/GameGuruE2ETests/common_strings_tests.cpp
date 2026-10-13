#include "doctest.h"
#include "../../GameGuru/Include/Common-Strings.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <filesystem>
#include <fstream>

TEST_CASE("CommonStrings_removeedgespaces_basic_and_boundaries")
{
	// Null and empty
	CHECK(removeedgespaces(nullptr) == std::string(""));
	CHECK(removeedgespaces("") == std::string(""));
	CHECK(removeedgespaces_str("").empty());

	// All whitespace
	CHECK(removeedgespaces("   ") == std::string(""));
	CHECK(removeedgespaces("\t\r\n \t ") == std::string(""));
	CHECK(removeedgespaces_str("   ").empty());
	CHECK(removeedgespaces_str("\r\n\t").empty());

	// Leading whitespace
	CHECK(removeedgespaces("   test") == std::string("test"));
	CHECK(removeedgespaces_str(" \t test") == "test");

	// Trailing whitespace
	CHECK(removeedgespaces("test   ") == std::string("test"));
	CHECK(removeedgespaces_str("test \r\n ") == "test");

	// Both leading and trailing
	CHECK(removeedgespaces("  hello world  ") == std::string("hello world"));
	CHECK(removeedgespaces_str("\t  hello world  \r\n") == "hello world");

	// Single character
	CHECK(removeedgespaces("a") == std::string("a"));
	CHECK(removeedgespaces(" a ") == std::string("a"));
	CHECK(removeedgespaces(" ") == std::string(""));

	// No trimming needed
	CHECK(removeedgespaces("no_spaces") == std::string("no_spaces"));
	CHECK(removeedgespaces_str("no_spaces") == "no_spaces");
}

TEST_CASE("CommonStrings_minstring_padding")
{
	// Null handling
	CHECK(minstring(nullptr, 4) == std::string("    "));
	CHECK(minstring_str("", 4) == "    ");

	// Padding needed
	CHECK(minstring("abc", 6) == std::string("abc   "));
	CHECK(minstring_str("abc", 6) == "abc   ");

	// Exact length
	CHECK(minstring("abc", 3) == std::string("abc"));
	CHECK(minstring_str("abc", 3) == "abc");

	// Longer than minsize
	CHECK(minstring("abcdef", 3) == std::string("abcdef"));
	CHECK(minstring_str("abcdef", 3) == "abcdef");

	// Zero or negative minsize
	CHECK(minstring("hello", 0) == std::string("hello"));
	CHECK(minstring("hello", -5) == std::string("hello"));
	CHECK(minstring_str("hello", -1) == "hello");
}

TEST_CASE("CommonStrings_getfirstdir_extraction")
{
	// Null and empty
	CHECK(getfirstdir(nullptr) == std::string(""));
	CHECK(getfirstdir("") == std::string(""));
	CHECK(getfirstdir_str("").empty());

	// Backslash
	CHECK(getfirstdir("folder\\sub\\file.txt") == std::string("folder\\"));
	CHECK(getfirstdir_str("folder\\sub\\file.txt") == "folder\\");

	// Forward slash
	CHECK(getfirstdir("folder/sub/file.txt") == std::string("folder/"));
	CHECK(getfirstdir_str("folder/sub/file.txt") == "folder/");

	// Leading slash
	CHECK(getfirstdir("\\root\\file.txt") == std::string("\\"));
	CHECK(getfirstdir("/root/file.txt") == std::string("/"));

	// No slash
	CHECK(getfirstdir("file.txt") == std::string(""));
	CHECK(getfirstdir_str("file.txt").empty());
}

TEST_CASE("CommonStrings_getseconddir_extraction")
{
	// Null and empty
	CHECK(getseconddir(nullptr) == std::string(""));
	CHECK(getseconddir("") == std::string(""));
	CHECK(getseconddir_str("").empty());

	// Two or more dirs
	CHECK(getseconddir("a\\b\\c.txt") == std::string("a\\b\\"));
	CHECK(getseconddir_str("a/b/c.txt") == "a/b/");
	CHECK(getseconddir("a\\b\\c\\d.txt") == std::string("a\\b\\"));

	// Mixed slashes
	CHECK(getseconddir("a/b\\c/d.txt") == std::string("a/b\\"));

	// Only one slash -> returns empty
	CHECK(getseconddir("a\\c.txt") == std::string(""));
	CHECK(getseconddir_str("a/c.txt").empty());

	// No slash -> returns empty
	CHECK(getseconddir("c.txt") == std::string(""));
	CHECK(getseconddir_str("c.txt").empty());
}

TEST_CASE("CommonStrings_getpath_and_getfile")
{
	// Null and empty
	CHECK(getpath(nullptr) == std::string(""));
	CHECK(getpath("") == std::string(""));
	CHECK(getpath_str("").empty());

	CHECK(getfile(nullptr) == std::string(""));
	CHECK(getfile("") == std::string(""));
	CHECK(getfile_str("").empty());

	// Standard path
	CHECK(getpath("gamecore\\weapon\\gun.txt") == std::string("gamecore\\weapon\\"));
	CHECK(getpath_str("gamecore\\weapon\\gun.txt") == "gamecore\\weapon\\");
	CHECK(getfile("gamecore\\weapon\\gun.txt") == std::string("gun.txt"));
	CHECK(getfile_str("gamecore\\weapon\\gun.txt") == "gun.txt");

	// Forward slashes
	CHECK(getpath("gamecore/weapon/gun.txt") == std::string("gamecore/weapon/"));
	CHECK(getpath_str("gamecore/weapon/gun.txt") == "gamecore/weapon/");
	CHECK(getfile("gamecore/weapon/gun.txt") == std::string("gun.txt"));
	CHECK(getfile_str("gamecore/weapon/gun.txt") == "gun.txt");

	// Mixed slashes
	CHECK(getpath("c:\\gamecore/weapon\\gun.txt") == std::string("c:\\gamecore/weapon\\"));
	CHECK(getfile("c:\\gamecore/weapon\\gun.txt") == std::string("gun.txt"));

	// File only (no path)
	CHECK(getpath("gun.txt") == std::string(""));
	CHECK(getpath_str("gun.txt").empty());
	CHECK(getfile("gun.txt") == std::string("gun.txt"));
	CHECK(getfile_str("gun.txt") == "gun.txt");

	// Directory only (trailing slash)
	CHECK(getpath("gamecore\\weapon\\") == std::string("gamecore\\weapon\\"));
	CHECK(getfile("gamecore\\weapon\\") == std::string(""));
	CHECK(getfile_str("gamecore\\weapon\\").empty());
}

TEST_CASE("CommonStrings_reentrancy_and_rotation_pool")
{
	// Test that multiple results in the same statement do not clobber each other
	char* p1 = getpath("dir1/file1.txt");
	char* p2 = getpath("dir2/file2.txt");
	char* p3 = getfile("dir3/file3.txt");
	char* p4 = removeedgespaces("  trimmed  ");

	CHECK(std::string(p1) == "dir1/");
	CHECK(std::string(p2) == "dir2/");
	CHECK(std::string(p3) == "file3.txt");
	CHECK(std::string(p4) == "trimmed");

	// Verify that all 32 pool slots can be used without premature collision
	std::vector<std::string> expected;
	std::vector<char*> pointers;
	expected.reserve(20);
	pointers.reserve(20);

	for (int i = 0; i < 20; ++i)
	{
		std::string input = "dir" + std::to_string(i) + "/file.txt";
		expected.push_back("dir" + std::to_string(i) + "/");
		pointers.push_back(getpath(input.c_str()));
	}

	for (size_t i = 0; i < pointers.size(); ++i)
	{
		CHECK(std::string(pointers[i]) == expected[i]);
	}
}

TEST_CASE("CommonStrings_multithreaded_thread_safety")
{
	constexpr int THREAD_COUNT = 4;
	constexpr int ITERATIONS = 500;
	std::atomic<bool> failed{false};

	std::vector<std::thread> threads;
	threads.reserve(THREAD_COUNT);

	for (int t = 0; t < THREAD_COUNT; ++t)
	{
		threads.emplace_back([t, &failed]() {
			for (int i = 0; i < ITERATIONS; ++i)
			{
				std::string pathInput = "thread" + std::to_string(t) + "/sub" + std::to_string(i) + "/asset.dbo";
				char* pDir = getpath(pathInput.c_str());
				char* pFile = getfile(pathInput.c_str());
				char* pTrim = removeedgespaces("   clean   ");

				std::string expectedDir = "thread" + std::to_string(t) + "/sub" + std::to_string(i) + "/";
				if (std::string(pDir) != expectedDir) failed = true;
				if (std::string(pFile) != "asset.dbo") failed = true;
				if (std::string(pTrim) != "clean") failed = true;
			}
		});
	}

	for (auto& th : threads)
	{
		th.join();
	}

	CHECK_FALSE(failed.load());
}

TEST_CASE("CommonStrings_buildfilelist_reentrant_scanner")
{
	// Create a temporary test directory hierarchy
	std::filesystem::path tempBase = std::filesystem::temp_directory_path() / "gg_test_buildfilelist";
	std::filesystem::remove_all(tempBase);
	std::filesystem::create_directories(tempBase / "sub1");
	std::filesystem::create_directories(tempBase / "sub2" / "nested");

	{
		std::ofstream(tempBase / "root_file.txt") << "content";
		std::ofstream(tempBase / "sub1" / "file1.txt") << "content";
		std::ofstream(tempBase / "sub2" / "file2.txt") << "content";
		std::ofstream(tempBase / "sub2" / "nested" / "file3.txt") << "content";
	}

	// Scan into std::vector<std::string>
	std::vector<std::string> files;
	buildfilelist(tempBase.string(), "", files);

	CHECK(files.size() == 4);

	bool hasRoot = false, hasSub1 = false, hasSub2 = false, hasNested = false;
	for (const auto& f : files)
	{
		if (f == "root_file.txt") hasRoot = true;
		if (f == "sub1\\file1.txt" || f == "sub1/file1.txt") hasSub1 = true;
		if (f == "sub2\\file2.txt" || f == "sub2/file2.txt") hasSub2 = true;
		if (f == "sub2\\nested\\file3.txt" || f == "sub2/nested/file3.txt") hasNested = true;
	}

	CHECK(hasRoot);
	CHECK(hasSub1);
	CHECK(hasSub2);
	// Scan using 2-argument overload (omits explicit empty relPrefix)
	std::vector<std::string> files2Arg;
	buildfilelist(tempBase.string(), files2Arg);
	CHECK(files2Arg.size() == 4);

	// Scan into std::vector<cstr> using 2-argument overload
	std::vector<cstr> cstrFiles2;
	buildfilelist(tempBase.string(), cstrFiles2);
	CHECK(cstrFiles2.size() == 4);

	// Append to existing vector
	buildfilelist(tempBase.string(), files2Arg);
	CHECK(files2Arg.size() == 8);

	// Clean up temp directory
	std::filesystem::remove_all(tempBase);
}

TEST_CASE("CommonStrings_complex_slashes_and_edge_paths")
{
	// All slashes
	CHECK(getpath("///") == std::string("///"));
	CHECK(getfile("///") == std::string(""));
	CHECK(getfirstdir("///") == std::string("/"));
	CHECK(getseconddir("///") == std::string("//"));

	CHECK(getpath("\\\\\\") == std::string("\\\\\\"));
	CHECK(getfile("\\\\\\") == std::string(""));
	CHECK(getfirstdir("\\\\\\") == std::string("\\"));
	CHECK(getseconddir("\\\\\\") == std::string("\\\\"));

	// Root path
	CHECK(getpath("/file.txt") == std::string("/"));
	CHECK(getfile("/file.txt") == std::string("file.txt"));
	CHECK(getfirstdir("/file.txt") == std::string("/"));
	CHECK(getseconddir("/file.txt") == std::string(""));

	// Trailing slash only
	CHECK(getpath("folder/") == std::string("folder/"));
	CHECK(getfile("folder/") == std::string(""));
	CHECK(getfirstdir("folder/") == std::string("folder/"));
	CHECK(getseconddir("folder/") == std::string(""));

	// Characters > 127 preserved in removeedgespaces
	unsigned char highAscii[] = { 0x20, 0x80, 0xC0, 0xFF, 0x20, 0x00 };
	std::string trimmedHigh = removeedgespaces((const char*)highAscii);
	CHECK(trimmedHigh.size() == 3);
	CHECK((unsigned char)trimmedHigh[0] == 0x80);
	CHECK((unsigned char)trimmedHigh[1] == 0xC0);
	CHECK((unsigned char)trimmedHigh[2] == 0xFF);

	// minstring with text longer than minsize
	CHECK(minstring_str("longer_text", 4) == "longer_text");
	CHECK(minstring("longer_text", 4) == std::string("longer_text"));
}

TEST_CASE("CommonStrings_buildfilelist_recursion_depth_limit")
{
	// Create deep directory hierarchy (35 levels, exceeds MAX_DIR_RECURSION_DEPTH = 32)
	// Use single-character directory names to stay well within MAX_PATH (260)
	std::filesystem::path deepBase = std::filesystem::temp_directory_path() / "gg_test_deep";
	std::filesystem::remove_all(deepBase);

	std::filesystem::path current = deepBase;
	for (int i = 0; i < 35; ++i)
	{
		current /= "d";
	}
	std::filesystem::create_directories(current);

	std::ofstream(deepBase / "d" / "top_file.txt") << "top";
	std::ofstream(current / "deep_file.txt") << "deep";

	std::vector<std::string> results;
	// Must not crash or stack overflow
	buildfilelist(deepBase.string(), "", results);

	// top_file.txt is scanned, deep_file.txt at level 35 is beyond depth cap
	bool foundTop = false;
	for (const auto& f : results)
	{
		if (f.find("top_file.txt") != std::string::npos) foundTop = true;
	}
	CHECK(foundTop);

	std::filesystem::remove_all(deepBase);
}

