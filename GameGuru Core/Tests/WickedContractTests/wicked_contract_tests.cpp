// Cross-repository integration contract tests.
//
// GameGuru MAX talks to the Wicked Engine through two contracts:
//   1. The engine headers, included from the sibling WickedRepo exactly as
//      stdafx.h does (#include <WickedEngine.h>).
//   2. wickedcalls.cpp, the translation layer between GameGuru operations
//      and Wicked Engine objects.
// This suite compiles the REAL wickedcalls.cpp (function-level linking with
// /Gy keeps only the code paths under test) and pins the behavior of the
// pure seams on that boundary: image path resolution relative to the
// product's Files\ root and the image list bookkeeping, plus the engine
// header API that GameGuru code calls inline.
//
// The file deliberately stays at the product's compilation level (no C++17
// std::filesystem) so it mirrors the shipped build environment exactly.
#include "doctest.h"

#include <WickedEngine.h>

#include "wickedcalls.h"


#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

// wickedcalls.cpp defines these at file scope; the tests drive them directly:
extern std::vector<sImageList> g_imageList;
extern std::string g_rootFolder;
extern uint32_t SetMasterObject;

// The engine's global entity manager lives in wiECS.cpp (engine library);
// the header-inline ComponentManager calls into it, so the contract test
// binary provides its own instance.
wiECS::ECSManager wiECS::ecs;

namespace
{
	const char* TEMP_LAYOUT = "wicked_contract_tests\\Max";

	void MakeDir(const std::string& path)
	{
		const bool created = CreateDirectoryA(path.c_str(), nullptr) != 0;
		const bool alreadyExisted = GetLastError() == ERROR_ALREADY_EXISTS;
		const bool usable = created || alreadyExisted;
		REQUIRE(usable);
	}

	// Builds a realistic <temp>\Max\Files\scriptbank layout on disk and
	// returns the root path, like the shipped editor uses.
	std::string MakeRootLayout()
	{
		char temp[MAX_PATH] = {};
		GetTempPathA(MAX_PATH, temp);
		std::string root = std::string(temp) + TEMP_LAYOUT;
		// CreateDirectoryA does not create intermediate levels:
		MakeDir(std::string(temp) + "wicked_contract_tests");
		MakeDir(root);
		MakeDir(root + "\\Files");
		MakeDir(root + "\\Files\\scriptbank");
		MakeDir(root + "\\Files\\textures");
		return root;
	}

	// Temporarily re-points the process working directory (the path
	// contract under test is defined against GetCurrentDirectoryA).
	class ScopedWorkingDirectory
	{
	public:
		ScopedWorkingDirectory()
		{
			char buffer[MAX_PATH] = {};
			GetCurrentDirectoryA(MAX_PATH, buffer);
			m_old = buffer;
		}
		~ScopedWorkingDirectory()
		{
			SetCurrentDirectoryA(m_old.c_str());
		}
		void Set(const std::string& directory)
		{
			REQUIRE(SetCurrentDirectoryA(directory.c_str()));
		}

	private:
		std::string m_old;
	};
}

TEST_CASE("The sibling WickedRepo engine headers compile and behave on this side")
{
	// stdafx.h in Guru-WickedMAX does exactly this include through the
	// sibling-repository include path; if the layout or the header contract
	// drifts, this test stops compiling in the GameGuru gate.

	// Header-inline math API used across the boundary:
	CHECK(wiMath::Lerp(2.0f, 10.0f, 0.25f) == doctest::Approx(4.0f));

	// Header-inline color packing:
	const wiColor white = wiColor(0xFFFFFFFF);
	CHECK(white.rgba == 0xFFFFFFFF);

	// Header-inline ECS component storage:
	wiECS::ComponentManager<int> manager(64);
	const wiECS::Entity entity = 42;
	manager.Create(entity) = 7;
	REQUIRE(manager.Contains(entity));
	CHECK(*manager.GetComponent(entity) == 7);
	manager.Remove(entity);
	CHECK_FALSE(manager.Contains(entity));
}

TEST_CASE("WickedCall_GetRelativeAfterRoot resolves paths relative to the Files root")
{
	std::string root = MakeRootLayout();
	ScopedWorkingDirectory cwd;
	cwd.Set(root + "\\Files\\scriptbank");
	g_rootFolder = root;

	char resolved[MAX_PATH] = {};
	WickedCall_GetRelativeAfterRoot("bricks.png", resolved);
	CHECK(std::string(resolved) == "scriptbank\\bricks.png");

	// Subdirectories keep their relative structure:
	WickedCall_GetRelativeAfterRoot("shared\\wood.png", resolved);
	CHECK(std::string(resolved) == "scriptbank\\shared\\wood.png");
}

TEST_CASE("WickedCall_GetRelativeAfterRoot skips the prefix outside the root")
{
	std::string root = MakeRootLayout();
	ScopedWorkingDirectory cwd;
	// Above the Files root the caller gets the bare filename back:
	cwd.Set(root);
	g_rootFolder = root;

	char resolved[MAX_PATH] = {};
	WickedCall_GetRelativeAfterRoot("bricks.png", resolved);
	CHECK(std::string(resolved) == "bricks.png");
}

TEST_CASE("WickedCall_FindImageIndexInList matches loaded images by resolved path")
{
	std::string root = MakeRootLayout();
	ScopedWorkingDirectory cwd;
	cwd.Set(root + "\\Files\\textures");
	g_rootFolder = root;
	g_imageList.clear();

	// Empty name never matches:
	CHECK(WickedCall_FindImageIndexInList("", nullptr) == -1);

	// Register an image exactly as the loader does, with the resolved key:
	SetMasterObject = 55000; // in the master-object range, so it is recorded
	WickedCall_AddImageToList(std::shared_ptr<wiResource>(), IMAGERES_LEVEL, "textures\\model_diffuse.png", 128);

	char resolved[MAX_PATH] = {};
	const int index = WickedCall_FindImageIndexInList("model_diffuse.png", resolved);
	REQUIRE(index == 0);
	CHECK(std::string(resolved) == "textures\\model_diffuse.png");

	// The master-object bookkeeping from the 50000..70000 range sticks:
	REQUIRE(g_imageList.size() == 1);
	CHECK(g_imageList[0].MasterObject == 55000);
	CHECK(g_imageList[0].iMemUsedKB == 128);

	// Unknown images report no index:
	CHECK(WickedCall_FindImageIndexInList("missing.png", nullptr) == -1);

	// Outside the master-object range the association is not recorded:
	SetMasterObject = 100;
	WickedCall_AddImageToList(std::shared_ptr<wiResource>(), IMAGERES_LEVEL, "textures\\other.png", 1);
	const int second = WickedCall_FindImageIndexInList("other.png", nullptr);
	REQUIRE(second == 1);
	CHECK(g_imageList[1].MasterObject == 0);

	g_imageList.clear();
}

TEST_CASE("WickedCall_InitImageManagement resets the list and stores the root")
{
	std::string root = MakeRootLayout();
	g_imageList.clear();
	SetMasterObject = 100;
	WickedCall_AddImageToList(std::shared_ptr<wiResource>(), IMAGERES_LEVEL, "stale.png", 1);
	REQUIRE(g_imageList.size() == 1);

	WickedCall_InitImageManagement(const_cast<LPSTR>(root.c_str()));

	// The list is wiped and the root folder is stored for later lookups:
	CHECK(g_imageList.empty());
	CHECK(g_rootFolder == root);
}
