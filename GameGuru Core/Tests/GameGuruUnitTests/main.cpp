// Entry point for the GameGuru MAX product unit tests. doctest provides the
// framework and turns any failed test into a non-zero process exit code,
// which the local and deploy-pipeline test gates rely on.
//
// Set GG_LUA_COVERAGE=1 to record executed Lua lines across every test and
// dump them to the file named by GG_LUA_COVERAGE_OUT (or lua_coverage.txt)
// for compute_lua_coverage.py.
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#include "LuaHost.h"

#include <windows.h>

#include <string>

int main(int argc, char** argv)
{
	const int result = doctest::Context(argc, argv).run();

	char coverageFlag[2] = {};
	if (GetEnvironmentVariableA("GG_LUA_COVERAGE", coverageFlag, 2) > 0
		&& coverageFlag[0] == '1')
	{
		char outPath[MAX_PATH] = "lua_coverage.txt";
		GetEnvironmentVariableA("GG_LUA_COVERAGE_OUT", outPath, MAX_PATH);
		ggtest::LuaCoverage::Dump(outPath);
	}
	return result;
}
