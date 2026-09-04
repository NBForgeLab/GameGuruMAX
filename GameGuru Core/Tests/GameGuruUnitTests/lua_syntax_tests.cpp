// Syntax validation for every product Lua script: each file under
// Scripts\scriptbank (and Scripts\titlesbank) must at least compile. This is
// a cheap regression net that catches corrupted or half-saved scripts long
// before they fail on a customer's machine.
//
// Loading here does NOT execute the scripts (luaL_loadfile only compiles).
#include "doctest.h"

#include "LuaHost.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
	// Reads a script and strips a UTF-8 byte order mark if present. Scripts
	// authored on Windows commonly carry one, and the game's Lua loader
	// tolerates it, so the test loader does too.
	bool ReadScriptBytes(const std::string& path, std::string& out)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open())
		{
			return false;
		}
		out.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		if (out.size() >= 3 &&
			(unsigned char)out[0] == 0xEF && (unsigned char)out[1] == 0xBB && (unsigned char)out[2] == 0xBF)
		{
			out.erase(0, 3);
		}
		return true;
	}

	void CollectLuaFiles(const std::filesystem::path& directory, std::vector<std::string>& out)
	{
		namespace fs = std::filesystem;
		std::error_code ec;
		fs::recursive_directory_iterator it(directory, fs::directory_options::skip_permission_denied, ec);
		if (ec) return;
		for (const fs::directory_entry& entry : it)
		{
			if (entry.is_regular_file() && entry.path().extension() == ".lua")
			{
				out.push_back(entry.path().string());
			}
		}
	}
}

TEST_CASE("every product Lua script compiles")
{
	ggtest::LuaHost host;
	REQUIRE(host.state() != nullptr);

	const std::string scriptsRoot = ggtest::FindScriptsRoot();
	REQUIRE(scriptsRoot.size() > 0);

	namespace fs = std::filesystem;
	std::vector<std::string> scripts;
	for (fs::directory_iterator it{fs::path(scriptsRoot)}, end; it != end; ++it)
	{
		if (it->is_directory())
		{
			CollectLuaFiles(it->path(), scripts);
		}
	}

	// The product ships a few hundred gameplay scripts; a discovery failure
	// must not masquerade as an empty, passing check.
	CHECK(scripts.size() > 300);

	int checked = 0;
	int failures = 0;
	for (const std::string& path : scripts)
	{
		std::string bytes;
		if (!ReadScriptBytes(path, bytes))
		{
			FAIL("Cannot read script: ", path);
			continue;
		}
		const char* chunkName = path.c_str();
		if (luaL_loadbuffer(host.state(), bytes.data(), bytes.size(), chunkName) != LUA_OK)
		{
			const char* message = lua_tostring(host.state(), -1);
			MESSAGE("SYNTAX ERROR in ", path, ": ", message != nullptr ? message : "?");
			lua_pop(host.state(), 1);
			++failures;
		}
		++checked;
	}
	CHECK(checked == (int)scripts.size());
	CHECK_MESSAGE(failures == 0, failures << " of " << checked << " scripts failed to compile");
}
