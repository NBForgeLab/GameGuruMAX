//----------------------------------------------------
//--- GAMEGURU - M-MapFile
//----------------------------------------------------

// Includes 
#include "stdafx.h"
#include "gameguru.h"

#include "GGTerrain\GGTerrainFile.h"
#include "GGTerrain\GGTerrain.h"
#include "GGTerrain\GGTrees.h"
#include "GGTerrain\GGGrass.h"
using namespace GGTerrain;
using namespace GGTrees;
using namespace GGGrass;


#include "GGThread.h"
using namespace GGThread;

class ExtractZipThread : public GGThread
{
protected:
	static char* pZipFileName;
	static const char* pExtractPath;
	static const char* const* pFileNames;
	static const unsigned long* pFileSizes;
	static volatile unsigned char* pFileDone;
	static volatile uint32_t iNextFile;
	static uint32_t iNumFiles;
	static threadLock lock;
	
	static ExtractZipThread* pThreads;
	static uint32_t iNumThreads;

	uint32_t iFileBlockID;

public:

	static void SetWork( char* zipFile, const char* extractPath, const char* const* files, const unsigned long* sizes, uint32_t numFiles )
	{
		pZipFileName = zipFile;
		pExtractPath = extractPath;
		iNextFile = 0;
		iNumFiles = numFiles;
		pFileNames = files;
		pFileSizes = sizes;

		if ( pFileDone ) delete [] pFileDone;
		pFileDone = new unsigned char[ numFiles ];
		for( uint32_t i = 0; i < numFiles; i++ ) pFileDone[ i ] = 0;
	}

	static bool AnyRunning()
	{
		for( uint32_t i = 0; i < iNumThreads; i++ ) 
		{
			if ( pThreads[i].IsRunning() ) return true;
		}
		return false;
	}

	static void WaitForAll()
	{
		for( uint32_t i = 0; i < iNumThreads; i++ ) pThreads[i].Join();
	}

	static void StartThreads()
	{
		for( uint32_t i = 0; i < iNumThreads; i++ ) pThreads[i].Start();
	}

	static void SetThreads( uint32_t numThreads )
	{
		if ( numThreads > 250 ) numThreads = 250; // limited by file block IDs to 256 but play it safe
		if ( numThreads == iNumThreads ) return;
		if ( pThreads ) delete [] pThreads;
		
		pThreads = new ExtractZipThread[ numThreads ];
		iNumThreads = numThreads;
		for( uint32_t i = 0; i < iNumThreads; i++ ) pThreads[ i ].iFileBlockID = i+2; // start at index 2
	}

	static bool IsSetup() { return pThreads != 0; }

	static uint32_t GetProgress()
	{
		// no need for lock here since it is an atomic read
		return (iNumFiles > 0) ? (iNextFile * 100) / iNumFiles : 100;
	}

	uint32_t Run( ) 
	{
		OpenFileBlock ( pZipFileName, iFileBlockID, "mypassword" );

		while( 1 )
		{
			if ( bTerminate ) break;
			
			int localIndex = -1;
			while( !lock.Acquire() );

			unsigned long maxSize = 0;
			for( uint32_t i = 0; i < iNumFiles; i++ )
			{
				if ( pFileDone[i] == 0 && pFileSizes[i] >= maxSize )
				{
					maxSize = pFileSizes[ i ];
					localIndex = i;
				}
			}

			if ( localIndex >= 0 ) pFileDone[localIndex] = 1;

			lock.Release();

			if ( localIndex < 0 ) break;

			const char* filename = pFileNames[ localIndex ];
			ExtractFileFromBlock( iFileBlockID, filename, pExtractPath );
		}

		CloseFileBlock( iFileBlockID );

		return 0;
	}

	ExtractZipThread() : GGThread() 
	{
		
	}

	~ExtractZipThread() 
	{
		if ( pThreads ) delete [] pThreads;
	}
};

char* ExtractZipThread::pZipFileName = 0;
const char* ExtractZipThread::pExtractPath = 0;
const char* const* ExtractZipThread::pFileNames = 0;
const unsigned long* ExtractZipThread::pFileSizes = 0;
volatile unsigned char* ExtractZipThread::pFileDone = 0;
volatile uint32_t ExtractZipThread::iNextFile = 0;
uint32_t ExtractZipThread::iNumFiles = 0;
threadLock ExtractZipThread::lock;
ExtractZipThread* ExtractZipThread::pThreads = 0;
uint32_t ExtractZipThread::iNumThreads = 0;

#include "..\Imgui\imgui.h"
#include "..\Imgui\imgui_impl_win32.h"
#include "..\Imgui\imgui_gg_dx11.h"
std::vector<cstr> g_sDefaultAssetFiles;

extern StoryboardStruct Storyboard;
extern int g_Storyboard_First_Level_Node;
extern int g_Storyboard_Current_Level;
extern char g_Storyboard_First_fpm[256];
extern char g_Storyboard_Current_fpm[256];
extern char g_Storyboard_Current_lua[256];
extern char g_Storyboard_Current_Loading_Page[256];

void AddWPETextures(const char* filename);
// 
//  MAP FILE FORMAT
// 

// MapFile Globals
int g_mapfile_iStage = 0;
int g_mapfile_iNumberOfLevels = 0;
int g_mapfile_iNumberOfEntitiesAcrossAllLevels = 0;
float g_mapfile_fProgress = 0.0f;
float g_mapfile_fProgressSpan = 0.0f;
std::vector<cstr> g_mapfile_fppFoldersToRemoveList;
std::vector<cstr> g_mapfile_fppFilesToRemoveList;
cstr g_mapfile_mapbankpath;
cstr g_mapfile_levelpathfolder;
bool g_bAllowBackwardCompatibleConversion = false;
bool g_bMakingAStandaloneUsingFileCollectionArray = false;
cstr g_mapfile_told_s;
cstr g_mapfile_tlevelfile_s;
int g_mapfile_tlevelstoprocess = 0;
int g_mapfile_tignorelevelbankfiles = 0;
int g_mapfile_iEntityElementIndex = 1;

bool restore_old_map = false;


void mapfile_saveproject_fpm ( void )
{
	cstr pOldDir = GetDir();

	//  use default or special worklevel stored
	cstr tempprojfilename_s;
	if (  t.goverridefpmdestination_s != "" ) 
	{
		tempprojfilename_s = t.goverridefpmdestination_s;
	}
	else
	{
		tempprojfilename_s = g.projectfilename_s;
	}
	//PE: Default folder could be d:\max\ , and write folder could be f:\docwrite\
	//PE: So then moving zip, it takes from d:\... and move to f:\... , d:\\ dont exists so .fpm file is deleted.
	//PE: To solve always use full path to .fpm
	char destination[MAX_PATH];
	strcpy(destination, tempprojfilename_s.Get());
	GG_GetRealPath(destination, 1);
	tempprojfilename_s = destination;
	if (g.editorsavebak == 1) 
	{
		//PE: Make a backup before overwriting a fpm level.
		char backupname[1024];
		strcpy(backupname, tempprojfilename_s.Get());
		size_t bLen = strlen(backupname);
		if (bLen >= 3)
		{
			backupname[bLen - 1] = 'k';
			backupname[bLen - 2] = 'a';
			backupname[bLen - 3] = 'b';
			DeleteAFile(backupname);
			CopyAFile(tempprojfilename_s.Get(), backupname);
		}
	}

	//  log prompts
	timestampactivity(0, cstr(cstr("Saving FPM level file: ") + tempprojfilename_s).Get() );

	//PE: Save heightmap.
	uint32_t iHeightmapSize = 0, iHeightmapWidth = 0, iHeightmapHeight = 0;
	iHeightmapSize = GGTerrain::GGTerrain_GetHeightmapDataSize(iHeightmapWidth, iHeightmapHeight);
	t.visuals.iHeightmapWidth = t.gamevisuals.iHeightmapWidth = iHeightmapWidth;
	t.visuals.iHeightmapHeight = t.gamevisuals.iHeightmapHeight = iHeightmapHeight;

	// ensure when export visual, always start with HIGHEST mode to reflect settings chosen when editing level (i.e. grass distance)
	//PE: Respect custom settings, some levels need to disable some optimizing settings...
	if (t.gamevisuals.shaderlevels.entities != 2)
	{
		t.gamevisuals.shaderlevels.terrain = 1;
		t.gamevisuals.shaderlevels.entities = 1;
		t.gamevisuals.shaderlevels.vegetation = 1;
		t.gamevisuals.shaderlevels.lighting = 1;
	}
	//  Switch visuals to gamevisuals as this is what we want to save
	t.editorvisuals=t.visuals ; t.visuals=t.gamevisuals  ; visuals_save ( );

	//  Copy visuals.ini into levelfile folder
	int includevisualsfile = 0;
	char pRealVisFile[MAX_PATH];
	strcpy(pRealVisFile, g.fpscrootdir_s.Get());
	strcat(pRealVisFile, "\\visuals.ini");
	GG_GetRealPath(pRealVisFile, 1);
	if (FileExist(pRealVisFile) == 1)
	{
		cstr visfile_s = g.mysystem.levelBankTestMap_s + "visuals.ini";
		if (FileExist(visfile_s.Get()) == 1)  DeleteAFile(visfile_s.Get());
		CopyAFile(pRealVisFile, visfile_s.Get());
		includevisualsfile = 1;
	}

	//  And switch back for benefit to editor visuals
	t.visuals=t.editorvisuals; // messes up when click test game again, old: gosub _visuals_save

	//  Delete any old file
	if (  FileExist(tempprojfilename_s.Get()) == 1  )  DeleteAFile (  tempprojfilename_s.Get() );

	//  Copy CFG to testgame area for saving with other files
	const char* tttfile_s = "cfg.cfg";
	cstr cfgfile_s = g.mysystem.editorsGridedit_s + tttfile_s;
	cstr cfginlevelbank_s = g.mysystem.levelBankTestMap_s + tttfile_s;
	if (  FileExist( cfgfile_s.Get() ) == 1 )
	{
		if ( FileExist( cfginlevelbank_s.Get() ) == 1 ) DeleteAFile ( cfginlevelbank_s.Get() );
		//PE: We need full path from write folder in wicked.
		char destination[MAX_PATH];
		strcpy(destination, cfgfile_s.Get());
		GG_GetRealPath(destination, 1);
		cfgfile_s = destination;
		CopyAFile ( cfgfile_s.Get(), cfginlevelbank_s.Get() );
	}

	//PE: For some reason cfg.cfg is missing from the fpm ?
	if (FileExist(cfginlevelbank_s.Get()) == 0)
	{
		//PE: Create a new directly inside testmap.
		editor_savecfg(cfginlevelbank_s.Get());
		timestampactivity(0, cstr(cstr("Creating cfg.cfg file: ") + cfginlevelbank_s).Get());
	}

	extern std::vector<sRubberBandType> vEntityLockedList;
	cfginlevelbank_s = g.mysystem.levelBankTestMap_s + "locked.cfg";
	//PE: Save a copy of locked objects.
	if (FileExist(cfginlevelbank_s.Get()) == 1)  DeleteAFile(cfginlevelbank_s.Get());
	OpenToWrite(1, cfginlevelbank_s.Get());
	WriteLong(1, static_cast<int>(vEntityLockedList.size()));
	for (size_t i = 0; i < vEntityLockedList.size(); i++)
	{
		WriteLong(1, vEntityLockedList[i].e);
	}
	CloseFile(1);

	// Create a FPM (zipfile)
	CreateFileBlock (  1, tempprojfilename_s.Get() );
	SetFileBlockKey (  1, "mypassword" );
	SetDir ( g.mysystem.levelBankTestMap_s.Get() ); // "levelbank\\testmap\\" );
	AddFileToBlock (  1, "header.dat" );
	AddFileToBlock (  1, "playerconfig.dat" );
	AddFileToBlock(1, "locked.cfg");
	AddFileToBlock (  1, "cfg.cfg" );
	// entity and waypoint files
	AddFileToBlock (  1, "map.ele" );
	AddFileToBlock (  1, "map.ent" );
	AddFileToBlock (  1, "map.way" );
	// darkai obstacle data (container zero)
	AddFileToBlock (  1, "map.obs" );
	// terrain files
	// save all node folders (containing terrain geometry and virtual textures)
	ChecklistForFiles (  );
	std::vector<std::string> terrainNodeFolders;
	terrainNodeFolders.clear();
	const int tnodeCount = ChecklistQuantity();
	for ( int c = 1 ; c <= tnodeCount; ++c )
	{
		cstr tfile_s = ChecklistString(c);
		if ( tfile_s != "." && tfile_s != ".." && tfile_s != "ttsfiles" ) 
		{
			if (ChecklistValueA(c) == 1)
			{
				if (cstr(Lower(Left(tfile_s.Get(), 2))) == "tt")
				{
					// found terrain node folder
					terrainNodeFolders.push_back(tfile_s.Get());
				}
			}
		}
	}
	for (size_t tt = 0; tt < terrainNodeFolders.size(); tt++)
	{
		LPSTR pTerrainNodeFolder = (LPSTR)terrainNodeFolders[tt].c_str();
		SetDir(pTerrainNodeFolder);
		ChecklistForFiles();
		SetDir("..");
		const int nodeFileCount = ChecklistQuantity();
		for (int c = 1; c <= nodeFileCount; ++c)
		{
			cstr tfile_s = ChecklistString(c);
			if (tfile_s != "." && tfile_s != "..")
			{
				cstr pFullRelPathToTerrainFile = cstr(pTerrainNodeFolder) + "\\" + tfile_s.Get();
				AddFileToBlock ( 1, pFullRelPathToTerrainFile.Get() );
			}
		}
	}

	#define MAXGROUPSLISTS 100 // duplicated in GridEdit.cpp
	for (int gi = 0; gi < MAXGROUPSLISTS; gi++)
	{
		char pGroupImgFilename[MAX_PATH];
		sprintf(pGroupImgFilename, "groupimg%d.png", gi);
		if (FileExist(pGroupImgFilename) == 1)
		{
			AddFileToBlock (1, pGroupImgFilename);
		}
	}

	// new multi-grass system stores grass choices in testmap folder
	AddFileToBlock ( 1, "grass_coloronly.dds" );

	// new terrain system saves its data settings
	cstr TerrainDataFile_s = g.mysystem.levelBankTestMap_s + "ggterrain.dat";
	GGTerrainFile_SaveTerrainData(TerrainDataFile_s.Get(), static_cast<int>(g.gdefaultwaterheight));
	AddFileToBlock (1, "ggterrain.dat");
	uint32_t terrain_sculpt_size = GGTerrain::GGTerrain_GetSculptDataSize();
	if (terrain_sculpt_size > 0)
	{
		std::vector<char> sculptData(terrain_sculpt_size);
		cstr sculpt_data_name = cstr((int)terrain_sculpt_size) + cstr(".dat");
		TerrainDataFile_s = g.mysystem.levelBankTestMap_s + sculpt_data_name;
		GGTerrain::GGTerrain_GetSculptData((uint8_t*)sculptData.data());
		if (FileExist(TerrainDataFile_s.Get()) == 1) DeleteAFile(TerrainDataFile_s.Get());
		HANDLE hwritefile = GG_CreateFile(TerrainDataFile_s.Get(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hwritefile != INVALID_HANDLE_VALUE)
		{
			DWORD byteswritten;
			WriteFile(hwritefile, sculptData.data(), terrain_sculpt_size, &byteswritten, NULL);
			CloseHandle(hwritefile);
			AddFileToBlock(1, sculpt_data_name.Get());
		}
	}

	//PE: Save Paint Data.
	uint32_t terrain_paint_size = GGTerrain::GGTerrain_GetPaintDataSize();
	if (terrain_paint_size > 0)
	{
		std::vector<char> paintData(terrain_paint_size);
		cstr paint_data_name = cstr((int)terrain_paint_size) + cstr(".ptd");
		TerrainDataFile_s = g.mysystem.levelBankTestMap_s + paint_data_name;
		GGTerrain::GGTerrain_GetPaintData((uint8_t*)paintData.data());
		if (FileExist(TerrainDataFile_s.Get()) == 1) DeleteAFile(TerrainDataFile_s.Get());
		HANDLE hwritefile = GG_CreateFile(TerrainDataFile_s.Get(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hwritefile != INVALID_HANDLE_VALUE)
		{
			DWORD byteswritten;
			WriteFile(hwritefile, paintData.data(), terrain_paint_size, &byteswritten, NULL);
			CloseHandle(hwritefile);
			AddFileToBlock(1, paint_data_name.Get());
		}
	}

	//PE: Save Tree Data.
	uint32_t terrain_tree_size = GGTrees::GGTrees_GetDataSize() * 4;
	if (terrain_tree_size > 0)
	{
		std::vector<char> treeData(terrain_tree_size);
		cstr tree_data_name = cstr((int)terrain_tree_size) + cstr(".tre");
		TerrainDataFile_s = g.mysystem.levelBankTestMap_s + tree_data_name;
		GGTrees::GGTrees_GetData((float*)treeData.data());
		if (FileExist(TerrainDataFile_s.Get()) == 1) DeleteAFile(TerrainDataFile_s.Get());
		HANDLE hwritefile = GG_CreateFile(TerrainDataFile_s.Get(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hwritefile != INVALID_HANDLE_VALUE)
		{
			DWORD byteswritten;
			WriteFile(hwritefile, treeData.data(), terrain_tree_size, &byteswritten, NULL);
			CloseHandle(hwritefile);
			AddFileToBlock(1, tree_data_name.Get());
		}
	}

	//PE: Save grass Data.
	uint32_t terrain_grass_size = GGGrass::GGGrass_GetDataSize();
	if (terrain_grass_size > 0)
	{
		std::vector<char> grassData(terrain_grass_size);
		cstr grass_data_name = cstr((int)terrain_grass_size) + cstr(".gra");
		TerrainDataFile_s = g.mysystem.levelBankTestMap_s + grass_data_name;
		GGGrass::GGGrass_GetData((uint8_t*)grassData.data());
		if (FileExist(TerrainDataFile_s.Get()) == 1) DeleteAFile(TerrainDataFile_s.Get());
		HANDLE hwritefile = GG_CreateFile(TerrainDataFile_s.Get(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hwritefile != INVALID_HANDLE_VALUE)
		{
			DWORD byteswritten;
			WriteFile(hwritefile, grassData.data(), terrain_grass_size, &byteswritten, NULL);
			CloseHandle(hwritefile);
			AddFileToBlock(1, grass_data_name.Get());
		}
	}

	//PE: Save heightmap data if any.
	if (iHeightmapSize > 0 && iHeightmapWidth > 0 && iHeightmapHeight > 0)
	{
		std::vector<char> heightmapData(iHeightmapSize);
		cstr heightmap_data_name = "heightmapdata.raw";
		TerrainDataFile_s = g.mysystem.levelBankTestMap_s + heightmap_data_name;
		GGTerrain::GGTerrain_GetHeightmapData((uint16_t*)heightmapData.data());
		if (FileExist(TerrainDataFile_s.Get()) == 1) DeleteAFile(TerrainDataFile_s.Get());
		HANDLE hwritefile = GG_CreateFile(TerrainDataFile_s.Get(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hwritefile != INVALID_HANDLE_VALUE)
		{
			DWORD byteswritten;
			WriteFile(hwritefile, heightmapData.data(), iHeightmapSize, &byteswritten, NULL);
			CloseHandle(hwritefile);
			AddFileToBlock(1, heightmap_data_name.Get());
		}
	}

	// lightmap files
	if ( PathExist("lightmaps") == 1 ) 
	{
		AddFileToBlock (  1, "lightmaps\\objectlist.dat" );
		AddFileToBlock (  1, "lightmaps\\objectnummax.dat" );
		cstr nummaxfile_s = t.lightmapper.lmpath_s + "objectnummax.dat";
		int emaxinfolder = 4999;
		if (  FileExist(nummaxfile_s.Get()) == 1 ) 
		{
			OpenToRead (  1, nummaxfile_s.Get() );
			emaxinfolder = ReadLong ( 1 );
			CloseFile (  1 );
		}
		for ( int e = 0 ; e <= (emaxinfolder * 2) + 100; ++e )
		{
			cstr tname_s = t.lightmapper.lmpath_s + Str(e) + ".dds";
			if (  FileExist(tname_s.Get()) == 1 ) 
			{
				AddFileToBlock (  1, cstr(cstr("lightmaps\\") + Str(e) + ".dds").Get() );
			}
		}
		int furthestobjnumber = g.lightmappedobjectoffset;
		SetDir (  "lightmaps" );
		ChecklistForFiles (  );
		const int lmFileCount = ChecklistQuantity();
		for ( int c = 1 ; c <= lmFileCount; ++c )
		{
			cstr tfile_s = ChecklistString(c);
			if (  tfile_s != "." && tfile_s != ".." ) 
			{
				if (  cstr(Lower(Right(tfile_s.Get(),4))) == ".dbo" ) 
				{
					tfile_s = Right(tfile_s.Get(), Len(tfile_s.Get()) - Len("object"));
					tfile_s = Left(tfile_s.Get(), Len(tfile_s.Get()) - 4);
					int filevalue = static_cast<int>(ValF(tfile_s.Get()));
					if ( filevalue > furthestobjnumber ) furthestobjnumber = filevalue;
				}
			}
		}
		SetDir (  ".." );
		for ( int tobj = g.lightmappedobjectoffset; tobj <= furthestobjnumber; ++tobj )
		{
			cstr tname_s = cstr("lightmaps\\object") + Str(tobj) + ".dbo";
			if (  FileExist(tname_s.Get()) == 1  )  AddFileToBlock (  1, tname_s.Get() );
		}
	}

	// ttsfiles files
	if ( PathExist("ttsfiles") == 1 ) 
	{
		SetDir ( "ttsfiles" );
		ChecklistForFiles (  );
		std::vector<cstr> ttsfileslist;
		ttsfileslist.clear();
		const int ttsQty = ChecklistQuantity();
		for ( int c = 1 ; c <= ttsQty; ++c )
		{
			cstr tfile_s = ChecklistString(c);
			if ( tfile_s != "." && tfile_s != ".." ) 
			{
				if ( cstr(Lower(Right(tfile_s.Get(),4))) == ".txt" || cstr(Lower(Right(tfile_s.Get(),4))) == ".wav" || cstr(Lower(Right(tfile_s.Get(),4))) == ".lip") 
				{
					ttsfileslist.push_back(tfile_s);
				}
			}
		}
		SetDir ( ".." );
		for ( size_t c = 0; c < ttsfileslist.size(); ++c )
		{
			AddFileToBlock ( 1, cstr(cstr("ttsfiles\\") + ttsfileslist[c]).Get() );
		}
	}

	//  visual settings
	if (  includevisualsfile == 1  )  AddFileToBlock (  1, "visuals.ini" );

	//  conkit data
	if (  FileExist("conkit.dat")  )  AddFileToBlock (  1,"conkit.dat" );

	//  ebe files
	ChecklistForFiles (  );
	const int ebeQty = ChecklistQuantity();
	for ( int c = 1 ; c <= ebeQty; ++c )
	{
		cstr tfile_s = ChecklistString(c);
		if ( tfile_s != "." && tfile_s != ".." ) 
		{
			cstr strEnt = cstr(Lower(Right(tfile_s.Get(),4)));
			if ( strcmp ( strEnt.Get(), ".ebe" ) == NULL )
			{
				AddFileToBlock ( 1, tfile_s.Get() );
				cstr tNameOnly = Left(tfile_s.Get(), static_cast<int>(strlen(tfile_s.Get())) - 4);
				cstr tThisFile = tNameOnly + cstr(".fpe");
				if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
				// wicked saves DBOs
				tThisFile = tNameOnly + cstr(".dbo");
				if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
			}
		}
	}

	// cannot just discover EBE textures, so go through all entity parents for all EBEs and save their textures
	for ( int iEntID = 1; iEntID <= g.entidmaster; iEntID++ )
	{
		if (strlen(t.entitybank_s[iEntID].Get()) > 0)
		{
			if (t.entityprofile[iEntID].ebe.dwRLESize > 0)
			{	
				char pJustNameOfTex[MAX_PATH];
				strcpy(pJustNameOfTex, t.entityprofile[iEntID].texd_s.Get());
				pJustNameOfTex[strlen(pJustNameOfTex) - strlen("_color.dds")] = 0;
				cstr tThisFile = cstr(pJustNameOfTex) + "_color.dds";
				if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
				tThisFile = cstr(pJustNameOfTex) + "_normal.dds";
				if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
				tThisFile = cstr(pJustNameOfTex) + "_surface.dds";
				if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
			}
		}
	}

	cstr terrainMaterialFile = g.mysystem.levelBankTestMap_s + "custommaterials.dat";
	SaveTerrainTextureFolder(terrainMaterialFile.Get());
	AddFileToBlock(1, "custommaterials.dat");

	SetDir ( pOldDir.Get() );
	SaveFileBlock ( 1 );

	// New automated backup system for FPM files (stored in destination var)
	#define AUTOSAVEBACKUP
	#ifdef AUTOSAVEBACKUP
	timestampactivity(0, "Auto-save a copy of every save to the mapbank\\_automatedbackups folder");
	char automatedcopyfpm[MAX_PATH];
	char automatedcopyname[MAX_PATH];
	strcpy(automatedcopyfpm, "");
	int autoNameLen = static_cast<int>(strlen(automatedcopyname));
	for (int i = autoNameLen - 8; i > 0; i--)
	{
		if (strnicmp(automatedcopyname + i, "mapbank\\", 8)==NULL || strnicmp(automatedcopyname + i, "mapbank/", 8) == NULL)
		{
			strcpy(automatedcopyfpm, automatedcopyname+i+8);
			automatedcopyname[i+8] = 0;
			break;
		}
	}
	if (strlen(automatedcopyfpm) > 0)
	{
		// replace \\ and chop ext
		for (int i = 0; i < strlen(automatedcopyfpm); i++)
		{
			if (automatedcopyfpm[i] == '\\' || automatedcopyfpm[i] == '/') automatedcopyfpm[i] = '_';
		}
		LPSTR pExtDot = strrchr(automatedcopyfpm, '.');
		if(pExtDot) *pExtDot = 0;

		// create folder
		strcpy(automatedcopyname, automatedcopyname);
		strcat(automatedcopyname, "_automatedbackups\\");
		CreateDirectoryA(automatedcopyname, NULL);

		// version num tracker file
		int iVersionNumber = 1;
		char pTrackerFile[MAX_PATH];
		strcpy(pTrackerFile, automatedcopyname);
		strcat(pTrackerFile, automatedcopyfpm);
		strcat(pTrackerFile, ".dat");
		if (FileExist(pTrackerFile) == 1)
		{
			OpenToRead(1, pTrackerFile);
			iVersionNumber = atoi(ReadString(1));
			CloseFile(1);
		}

		// create auto backup filename
		char pCheckFile[MAX_PATH];
		strcpy(pCheckFile, automatedcopyname);
		strcat(pCheckFile, automatedcopyfpm);
		strcat(pCheckFile, "_1");
		char* pNum = strrchr(pCheckFile, '_');
		if (pNum)
		{
			sprintf(pNum, "_%d", iVersionNumber);
			strcat(pCheckFile, ".fpm");
		}

		// find next available version number
		while(FileExist(pCheckFile) == 1)
		{
			char *pDot = strrchr(pCheckFile, '.');
			if (pDot)
			{
				char *pNum = strrchr(pCheckFile, '_');
				if (pNum && pNum < pDot)
				{
					int iNum = atoi(pNum + 1);
					iNum++;
					*pNum = 0;
					sprintf(pNum, "_%d", iNum);
					strcat(pCheckFile, ".fpm");
					iVersionNumber = iNum;
				}
			}
		}
		DeleteAFile(pCheckFile);
		CopyAFile(destination, pCheckFile);

		// save version tracker file for next time
		if (FileExist(pTrackerFile) == 1) DeleteFileA(pTrackerFile);
		{
			OpenToWrite(1, pTrackerFile);
			char pVersionNum[256];
			sprintf(pVersionNum, "%d", iVersionNumber);
			WriteString(1, pVersionNum);
			CloseFile(1);
		}

		// and remove older files so storage does not become an issue
		if(iVersionNumber > 9)
		{
			for (int i = 1; i < iVersionNumber - 9; i++)
			{
				strcpy(pCheckFile, automatedcopyname);
				strcat(pCheckFile, automatedcopyfpm);
				strcat(pCheckFile, "_");
				strcat(pCheckFile, Str(i));
				strcat(pCheckFile, ".fpm");
				DeleteAFile(pCheckFile);
			}
		}
	}
	#endif

	// collect ALL entity profile files
	g.filecollectionmax = 0;
	Undim (t.filecollection_s);
	Dim (t.filecollection_s, 500);
	void addthisentityprofilesfilestocollection (entityeleproftype * pEleProf, int entid);
	for (int entid = 1; entid <= g.entidmaster; entid++)
	{
		if (strlen(t.entitybank_s[entid].Get()) > 0)
		{
			addthisentityprofilesfilestocollection (NULL, entid);
		}
	}

	// create an itinery LST file (so auto updater can find and download dependent files)
	cstr LSTFile_s = cstr(Left(g.projectfilename_s.Get(), Len(g.projectfilename_s.Get()) - 4)) + ".lst";
	if (FileExist(LSTFile_s.Get()) == 1) DeleteAFile (LSTFile_s.Get());
	std::vector <cstr> lstlist_s;
	Dim (lstlist_s, g.filecollectionmax);
	int iListIndex = 0;
	for (int i = 0; i <= g.filecollectionmax; i++)
	{
		LPSTR pThisFile = t.filecollection_s[i].Get();
		int iThisSize = static_cast<int>(strlen(pThisFile));
		if (iThisSize > 0)
		{
			// must have a file specified
			if (pThisFile[iThisSize - 1] == '\\' || pThisFile[iThisSize - 1] == '/')
			{
				// ignore folders
			}
			else
			{
				if (FileExist(pThisFile) == 1)
				{
					lstlist_s[iListIndex++] = pThisFile;
				}
			}
		}
	}
	lstlist_s.resize(iListIndex);
	SaveArray (LSTFile_s.Get(), lstlist_s);
	UnDim (lstlist_s);

	// save any changes to game collection list and ELE file
	extern preferences pref;
	save_rpg_system(pref.cLastUsedStoryboardProject, true);

	//  does crazy cool stuff
	t.tsteamsavefilename_s = tempprojfilename_s;

	//  log prompts
	timestampactivity(0,"Saving FPM level file complete");
}

void mapfile_emptyebesfromtestmapfolder(bool bIgnoreValidTextureFiles)
{
	ChecklistForFiles();
	const int count = ChecklistQuantity();
	for (int c = 1; c <= count; ++c)
	{
		const char* pFile = ChecklistString(c);
		if (!pFile || strcmp(pFile, ".") == 0 || strcmp(pFile, "..") == 0) continue;

		// only if not a CUSTOM content piece
		if (_strnicmp(pFile, "CUSTOM_", 7) != 0)
		{
			size_t fileLen = strlen(pFile);
			if (fileLen >= 4)
			{
				const char* pExt4 = pFile + fileLen - 4;
				if (_stricmp(pExt4, ".ebe") == 0 || _stricmp(pExt4, ".fpe") == 0)
				{
					DeleteAFile(const_cast<char*>(pFile));
					std::string tNameOnly(pFile, fileLen - 4);
					const char* exts[] = {
						".fpe", ".dbo", ".x", ".bmp",
						"_D.dds", "_N.dds", "_S.dds",
						"_D.jpg", "_N.jpg", "_S.jpg"
					};
					for (const char* ext : exts)
					{
						std::string target = tNameOnly + ext;
						if (FileExist(const_cast<char*>(target.c_str())) == 1)
							DeleteAFile(const_cast<char*>(target.c_str()));
					}
				}
			}
			if (!bIgnoreValidTextureFiles && fileLen >= 6)
			{
				const char* pExt6 = pFile + fileLen - 6;
				if (_stricmp(pExt6, "_d.dds") == 0 || _stricmp(pExt6, "_n.dds") == 0 || _stricmp(pExt6, "_s.dds") == 0 ||
					_stricmp(pExt6, "_d.jpg") == 0 || _stricmp(pExt6, "_n.jpg") == 0 || _stricmp(pExt6, "_s.jpg") == 0)
				{
					DeleteAFile(const_cast<char*>(pFile));
				}
			}
		}
	}
}

void mapfile_emptyterrainfilesfromtestmapfolder ( void )
{
	// also makes sense to empty terrain files too as we are clearing this folder for new activity
	// move to terrain node folder location
	cstr pOldDir = GetDir();
	char pRealWritableArea[MAX_PATH];
	strcpy(pRealWritableArea, pOldDir.Get());
	strcat(pRealWritableArea, "\\levelbank\\testmap\\");
	GG_GetRealPath(pRealWritableArea, 1);
	SetDir(pRealWritableArea);
	// all node folders (containing terrain geometry and virtual textures)
	ChecklistForFiles (  );
	std::vector<std::string> terrainNodeFolders;
	terrainNodeFolders.clear();
	const int count = ChecklistQuantity();
	for ( int c = 1 ; c <= count; ++c )
	{
		cstr tfile_s = ChecklistString(c);
		if ( tfile_s != "." && tfile_s != ".." && tfile_s != "ttsfiles" ) 
		{
			if (ChecklistValueA(c) == 1)
			{
				if (cstr(Lower(Left(tfile_s.Get(), 2))) == "tt")
				{
					// found terrain node folder
					terrainNodeFolders.push_back(tfile_s.Get());
				}
			}
		}
	}
	for (size_t tt = 0; tt < terrainNodeFolders.size(); tt++)
	{
		LPSTR pTerrainNodeFolder = (LPSTR)terrainNodeFolders[tt].c_str();
		SetDir(pTerrainNodeFolder);
		ChecklistForFiles();
		const int nodeFileCount = ChecklistQuantity();
		for (int c = 1; c <= nodeFileCount; ++c)
		{
			cstr tfile_s = ChecklistString(c);
			if (tfile_s != "." && tfile_s != "..")
			{
				DeleteAFile ( tfile_s.Get() );
			}
		}
		SetDir("..");
		RemoveDirectoryA(pTerrainNodeFolder);
	}
	SetDir(pOldDir.Get());
}

void mapfile_emptylightmapandttsfilesfolder_wicked( void )
{
	//PE: lightmaps
	cstr pOldDir = GetDir();
	char pRealWritableArea[MAX_PATH];
	strcpy(pRealWritableArea, pOldDir.Get());
	if( pestrcasestr(pOldDir.Get(), "\\testmap"))
		strcat(pRealWritableArea, "\\lightmaps");
	else
		strcat(pRealWritableArea, "\\levelbank\\testmap\\lightmaps");
	GG_GetRealPath(pRealWritableArea, 1);
	if (PathExist(pRealWritableArea))
	{
		SetDir(pRealWritableArea);
		ChecklistForFiles();
		const int lmCount = ChecklistQuantity();
		for (int c = 1; c <= lmCount; ++c)
		{
			cstr tfile_s = ChecklistString(c);
			if (tfile_s != "." && tfile_s != "..")
			{
				if (FileExist(tfile_s.Get()) == 1)  DeleteAFile(tfile_s.Get());
			}
		}
		SetDir(pOldDir.Get());
		RemoveDirectoryA(pRealWritableArea);
	}
	//PE: ttsfiles
	strcpy(pRealWritableArea, pOldDir.Get());
	if (pestrcasestr(pOldDir.Get(), "\\testmap"))
		strcat(pRealWritableArea, "\\ttsfiles");
	else
		strcat(pRealWritableArea, "\\levelbank\\testmap\\ttsfiles");
	GG_GetRealPath(pRealWritableArea, 1);
	if (PathExist(pRealWritableArea))
	{
		SetDir(pRealWritableArea);
		ChecklistForFiles();
		const int ttsCount = ChecklistQuantity();
		for (int c = 1; c <= ttsCount; ++c)
		{
			cstr tfile_s = ChecklistString(c);
			if (tfile_s != "." && tfile_s != "..")
			{
				if (FileExist(tfile_s.Get()) == 1)  DeleteAFile(tfile_s.Get());
			}
		}
		SetDir(pOldDir.Get());
		RemoveDirectoryA(pRealWritableArea);
	}
	//PE: Somehow it was not reverted, added here.
	SetDir(pOldDir.Get());
}

void mapfile_loadproject_fpm ( void )
{
	//PE: Deselect any objects, if we load a level with less entityties then t.widget.pickedEntityIndex it can crash.
	t.widget.pickedEntityIndex = 0;
	t.gridentity = 0;

	// can do extra steps when load in FPM
	bool bThisIsTheNewTerrainSystem = false;

	//  Ensure FPM exists
	t.trerfeshvisualsassets=0;
	timestampactivity(0, cstr(cstr("_mapfile_loadproject_fpm: ")+g.projectfilename_s+" "+GetDir()).Get() );
	if ( FileExist(g.projectfilename_s.Get()) == 1 ) 
	{
		//  Empty the lightmap folder
		timestampactivity(0,"LOADMAP: mapfile_emptylightmapandttsfilesfolder_wicked");
		mapfile_emptylightmapandttsfilesfolder_wicked();

		// empty any terrain node files
		mapfile_emptyterrainfilesfromtestmapfolder();

		//  Store and switch folders
		cstr oldDir_s = GetDir(); SetDir ( g.mysystem.levelBankTestMap_s.Get() ); // "levelbank\\testmap\\" );

		// Delete key testmap file (if any)
		if ( FileExist("header.dat") == 1 ) DeleteAFile ( "header.dat" );
		if ( FileExist("playerconfig.dat") == 1 ) DeleteAFile ( "playerconfig.dat" );
		if ( FileExist("watermask.dds") == 1 ) DeleteAFile ( "watermask.dds" );
		if ( FileExist("watermask.png") == 1 ) DeleteAFile ( "watermask.png" );
		if ( FileExist("vegmask.png") == 1) DeleteAFile( "vegmask.png"); //PE: If we switch from a new fpm to a old only with .dds, old need to be removed.
		if ( FileExist("visuals.ini") == 1 ) DeleteAFile ( "visuals.ini" );
		if ( FileExist("conkit.dat") == 1 ) DeleteAFile ( "conkit.dat" );
		if ( FileExist("map.obs") == 1 ) DeleteAFile ( "map.obs" );
		if ( FileExist("locked.cfg") == 1) DeleteAFile("locked.cfg");

		if (FileExist("cfg.cfg") == 1) DeleteAFile("cfg.cfg");

		// new terrain system loads its data settings in this file (see below)
		if (FileExist("ggterrain.dat") == 1) DeleteAFile("ggterrain.dat");
		uint32_t terrain_sculpt_size = GGTerrain::GGTerrain_GetSculptDataSize();
		cstr sculpt_data_name = cstr( (int) terrain_sculpt_size) + cstr(".dat");
		if (FileExist(sculpt_data_name.Get()) == 1) DeleteAFile(sculpt_data_name.Get());

		uint32_t terrain_paint_size = GGTerrain::GGTerrain_GetPaintDataSize();
		cstr paint_data_name = cstr((int)terrain_paint_size) + cstr(".ptd");
		if (FileExist(paint_data_name.Get()) == 1) DeleteAFile(paint_data_name.Get());

		uint32_t tree_data_size = GGTrees::GGTrees_GetDataSize() * 4;
		cstr tree_data_name = cstr((int)tree_data_size) + cstr(".tre");
		if (FileExist(tree_data_name.Get()) == 1) DeleteAFile(tree_data_name.Get());

		cstr old_tree_data_name = "4800000.tre"; //PE: Tree format 1.0. Make sure it deleted.
		if (FileExist(old_tree_data_name.Get()) == 1) DeleteAFile(old_tree_data_name.Get());

		uint32_t grass_data_size = GGGrass::GGGrass_GetDataSize();
		cstr grass_data_name = cstr((int)grass_data_size) + cstr(".gra");
		if (FileExist(grass_data_name.Get()) == 1) DeleteAFile(grass_data_name.Get());

		if (FileExist("heightmapdata.raw") == 1) DeleteAFile("heightmapdata.raw");

		//  Delete env map for PBR (if any)
		if ( FileExist("globalenvmap.dds") == 1 ) DeleteAFile ( "globalenvmap.dds" );

		if (FileExist("custommaterials.dat") == 1) DeleteAFile("custommaterials.dat");

		LPSTR pTerrainPreference = "TerrainPreference.tmp";
		if (FileExist(pTerrainPreference) == 1) DeleteFileA(pTerrainPreference);

		// empty any ebe files
		mapfile_emptyebesfromtestmapfolder(false);

		//  Restore folder to Files (for extraction)
		SetDir ( oldDir_s.Get() );

		//  Read FPM into testmap area
		timestampactivity(0,"LOADMAP: read FPM block");
		cstr tpath_s = g.mysystem.levelBankTestMap_s.Get(); //"levelbank\\testmap\\";

		if ( !ExtractZipThread::IsSetup() )
		{
			SYSTEM_INFO sysinfo;
			GetSystemInfo( &sysinfo );
			uint32_t numThreads = sysinfo.dwNumberOfProcessors;
			if ( numThreads > 3 ) numThreads--;
			ExtractZipThread::SetThreads( numThreads );
		}

		OpenFileBlock (  g.projectfilename_s.Get(),1,"mypassword" );
		uint32_t numFiles = GetFileBlockNumFiles( 1 );
		const char* const* pAllFiles = GetFileBlockAllFileNames( 1 );
		const unsigned long* pAllSizes = GetFileBlockAllFileSizes( 1 );
		ExtractZipThread::SetWork( g.projectfilename_s.Get(), tpath_s.Get(), pAllFiles, pAllSizes, numFiles );
		ExtractZipThread::StartThreads();
		ExtractZipThread::WaitForAll();
		CloseFileBlock( 1 ); // must remain open until all work completed for pAllFiles to remain valid

		SetDir ( g.mysystem.levelBankTestMapAbs_s.Get() );

		//  If file still not present, extraction failed
		if (  FileExist("header.dat") == 0 ) 
		{
			//  inform user the FPM could not be loaded (corrupt file)
			t.tloadsuccessfully=0;
		}

		// If file still not present, extraction failed
		SetDir ( g.mysystem.levelBankTestMapAbs_s.Get() );		
		if ( g.memskipwatermask == 0 && (FileExist("watermask.dds") == 0 || FileExist("watermask.png") == 0) )
		{
			//  Only Reloaded Formats have this texture file, so fail load if not there (Classic FPM)
			t.tloadsuccessfully=2;
		}

		// new terrain system loads its data settings
		//PE: We are already inside "levelbank\\testmap\\" ?
		//PE: In editor g.mysystem.levelBankTestMap_s have "c:" and works, in standalone we dont have that so:
		cstr TerrainDataFile_s = "ggterrain.dat";
		GGTerrainFile_LoadTerrainData(TerrainDataFile_s.Get(),false);
		extern bool bTreeGlobalInit;
		bTreeGlobalInit = false;

		//PE: Restore Sculpt Data.
		char* data;
		extern int g_iDisableTerrainSystem;
		if (g_iDisableTerrainSystem == 0)
		{
			terrain_sculpt_size = GGTerrain::GGTerrain_GetSculptDataSize();
			data = new char[terrain_sculpt_size];
			if (data)
			{
				cstr sculpt_data_name = cstr((int)terrain_sculpt_size) + cstr(".dat");
				if (FileExist(sculpt_data_name.Get()) == 1)
				{
					char FilenameString[_MAX_PATH];
					strcpy(FilenameString, sculpt_data_name.Get());
					if (DB_FileExist(FilenameString))
					{
						// Open file to be read
						HANDLE hreadfile = GG_CreateFile(FilenameString, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hreadfile != INVALID_HANDLE_VALUE)
						{
							// Read file into memory
							DWORD bytesread;
							ReadFile(hreadfile, data, terrain_sculpt_size, &bytesread, NULL);
							CloseHandle(hreadfile);
							GGTerrain::GGTerrain_SetSculptData(terrain_sculpt_size, (uint8_t*)data);
						}
					}
				}
				delete[] data;
			}

			void check_new_terrain_parameters(void);
			check_new_terrain_parameters();

			void reset_terrain_paint_date(void);
			reset_terrain_paint_date(); //PE: Reset old paint textures, after this. try loading from saved fpm.

			if (FileExist("custommaterials.dat") == 1)
			{
				LoadTerrainTextureFolder("custommaterials.dat");
			}

			//PE: Restore Paint Texture Data.
			terrain_paint_size = GGTerrain::GGTerrain_GetPaintDataSize();
			data = new char[terrain_paint_size];
			if (data)
			{
				cstr paint_data_name = cstr((int)terrain_paint_size) + cstr(".ptd");
				if (FileExist(paint_data_name.Get()) == 1)
				{
					char FilenameString[_MAX_PATH];
					strcpy(FilenameString, paint_data_name.Get());
					if (DB_FileExist(FilenameString))
					{
						// Open file to be read
						HANDLE hreadfile = GG_CreateFile(FilenameString, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hreadfile != INVALID_HANDLE_VALUE)
						{
							// Read file into memory
							DWORD bytesread;
							ReadFile(hreadfile, data, terrain_paint_size, &bytesread, NULL);
							CloseHandle(hreadfile);
							GGTerrain::GGTerrain_SetPaintData(terrain_paint_size, (uint8_t*)data);
							//PE: We already made a delay invalidate region so all fine...
						}
					}
				}
				delete[] data;
			}

			//PE: Default hide all tree's.
			GGTrees::GGTrees_HideAll();

			//PE: Restore Tree Data.
			tree_data_size = GGTrees::GGTrees_GetDataSize() * 4;
			data = new char[tree_data_size];
			if (data)
			{
				cstr tree_data_name = cstr((int)tree_data_size) + cstr(".tre");
				cstr old_tree_data_name = "4800000.tre"; //PE: Tree format 1.0
				bool bConvertOldFormat = false;
				if (FileExist(old_tree_data_name.Get()) == 1)
				{
					bConvertOldFormat = true;
				}
				if (FileExist(tree_data_name.Get()) == 1 || bConvertOldFormat)
				{
					char FilenameString[_MAX_PATH];
					strcpy(FilenameString, tree_data_name.Get());
					if (bConvertOldFormat)
					{
						strcpy(FilenameString, old_tree_data_name.Get());
						uint32_t* dataInt = (uint32_t*)data;
						dataInt[0] = 2; //PE: New version
						data += 4;
						tree_data_size -= 4;
					}
					if (DB_FileExist(FilenameString))
					{
						// Open file to be read
						HANDLE hreadfile = GG_CreateFile(FilenameString, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hreadfile != INVALID_HANDLE_VALUE)
						{
							// Read file into memory
							DWORD bytesread;
							ReadFile(hreadfile, data, tree_data_size, &bytesread, NULL);
							CloseHandle(hreadfile);
							if (!bConvertOldFormat)
							{
								//PE: Skip conversion from 2.0 to 3.0 as the 2.0 using 3.0 data have been out for some time now. and people would already have saved 3.0 trees with 2.0 version.
								//PE: Mainly so people will not have to correct the trees again. No way to see if 3.0 data has been saved as 2.0 now.
								uint32_t* dataInt = (uint32_t*)data;
								if (dataInt[0] == 2) dataInt[0] = 3;
							}
							if (bConvertOldFormat)
							{
								int size = GGTrees::GGTrees_GetDataSize();
								size--; //PE: Remove version.
								uint32_t* dataInt = (uint32_t*)data;
								int scale = 85; //PE: 85=1.0 hlsl: GetTreeScale( uint data ) { return ((data >> 16) & 0xFF) / 170.0 + 0.5; }

								for (int i = 0; i < size; i += 3)
								{
									//PE: Convert old tree data to new 2.0 format.
									uint32_t data = dataInt[2 + i];
									uint32_t type = (data & 0xF);
									uint32_t visible = (data & 0x100);
									uint32_t id = (data >> 10);
									uint32_t scaleindex = (data >> 4) & 0x7; //hlsl: uint index = (IN.data >> 4) & 0x7;
									float randScale = static_cast<float>((scaleindex / 7.0) * 0.25 + 0.75); //hlsl. old scale.
									int newscale = (int)((float)(57 + (scaleindex * 4.0)) * randScale);
									type &= 0x1F;
									uint32_t varIndex = id & 0x7;
									data = (type << 11) | (varIndex << 8);
									if (visible) data |= 0x1;
									else data &= ~0x1;
									dataInt[2 + i] = data;
									dataInt[2 + i] = (dataInt[2 + i] & 0xFF00FFFF) | (newscale << 16);
								}
								data -= 4;
							}
							GGTrees::GGTrees_SetData((float*)data);
						}
						else if (bConvertOldFormat)
						{
							data -= 4;
						}
					}
					else if (bConvertOldFormat)
					{
						data -= 4;
					}
				}
				delete[] data;
			}


			//PE: Restore grass Data.
			grass_data_size = GGGrass::GGGrass_GetDataSize();
			data = new char[grass_data_size];
			if (data)
			{
				cstr grass_data_name = cstr((int)grass_data_size) + cstr(".gra");
				if (FileExist(grass_data_name.Get()) == 1)
				{
					char FilenameString[_MAX_PATH];
					strcpy(FilenameString, grass_data_name.Get());
					if (DB_FileExist(FilenameString))
					{
						// Open file to be read
						HANDLE hreadfile = GG_CreateFile(FilenameString, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hreadfile != INVALID_HANDLE_VALUE)
						{
							// Read file into memory
							DWORD bytesread;
							ReadFile(hreadfile, data, grass_data_size, &bytesread, NULL);
							CloseHandle(hreadfile);
							GGGrass::GGGrass_SetData(grass_data_size, (uint8_t*)data);
						}
					}
				}
				delete[] data;
			}
			//PE: Make sure to trigger a delayed update, so everything is placed correctly, grass/trees need height.
			extern int iTriggerInvalidateAfterFrames;
			iTriggerInvalidateAfterFrames = 60; //PE: Height data need to be set first, then we can adjust grass/trees/virtual texture.

		}

		//  load in visuals from loaded file
		timestampactivity(0,"LOADMAP: load in visuals");
		if (  FileExist("visuals.ini") == 1 ) 
		{
			cstr storefpscrootdir_s = g.fpscrootdir_s;
			g.fpscrootdir_s = "" ; visuals_load ( );
			g.fpscrootdir_s = storefpscrootdir_s;
			t.trerfeshvisualsassets = 1;

			//  Ensure visuals settings are copied to gamevisuals (the true destination)
			t.gamevisuals=t.visuals;
			t.editorvisuals=t.visuals;

			//  And ensure editor visuals mimic required settings from loaded data
			visuals_editordefaults ( );
			t.visuals=t.editorvisuals;
			t.visuals.skyindex=t.gamevisuals.skyindex;
			t.visuals.sky_s=t.gamevisuals.sky_s;
			t.visuals.terrainindex=t.gamevisuals.terrainindex;
			t.visuals.terrain_s=t.gamevisuals.terrain_s;
			t.visuals.vegetationindex=t.gamevisuals.vegetationindex;
			t.visuals.vegetation_s=t.gamevisuals.vegetation_s;

			//  Re-acquire indices now the lists have changed
			//  takes visuals.sky$ visuals.terrain$ visuals.vegetation$
			visuals_updateskyterrainvegindex ( );
			t.gamevisuals.skyindex=t.visuals.skyindex;
			t.gamevisuals.terrainindex=t.visuals.terrainindex;
			t.gamevisuals.vegetationindex=t.visuals.vegetationindex;
		}


		uint32_t iHeightmapSize = 0, iHeightmapWidth = 0, iHeightmapHeight = 0;
		iHeightmapWidth = t.gamevisuals.iHeightmapWidth;
		iHeightmapHeight = t.gamevisuals.iHeightmapHeight;
		iHeightmapSize = iHeightmapWidth * iHeightmapHeight * sizeof(uint16_t);

		//PE: Load heightmap data if any.
		if (iHeightmapSize > 0 && iHeightmapWidth > 0 && iHeightmapHeight > 0)
		{
			data = new char[iHeightmapSize];
			if (data)
			{
				cstr heightmap_data_name = "heightmapdata.raw";
				if (FileExist(heightmap_data_name.Get()) == 1)
				{
					char FilenameString[_MAX_PATH];
					strcpy(FilenameString, heightmap_data_name.Get());
					if (DB_FileExist(FilenameString))
					{
						// Open file to be read
						HANDLE hreadfile = GG_CreateFile(FilenameString, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hreadfile != INVALID_HANDLE_VALUE)
						{
							// Read file into memory
							DWORD bytesread;
							ReadFile(hreadfile, data, iHeightmapSize, &bytesread, NULL);
							CloseHandle(hreadfile);
							GGTerrain::GGTerrain_SetHeightmapData((uint16_t*) data, iHeightmapWidth, iHeightmapHeight);
						}
					}
				}
				delete(data);
			}
		}

		// must trigger the terrain textures to update after visuals.ini load in terrain texture preferences
		if (strlen(t.visuals.sTerrainTextures[0].Get()) > 0)
		{
			extern std::vector<std::string> g_DeferTextureUpdate;
			g_DeferTextureUpdate.clear();
			g_DeferTextureUpdate.reserve(32);
			for (int i = 0; i < 32; i++)
			{
				std::string texture = t.visuals.sTerrainTextures[i].Get();
				g_DeferTextureUpdate.push_back(texture);
			}
			extern int g_iDeferTextureUpdateToNow;
			g_iDeferTextureUpdateToNow = 2;
		}


		//  if MAP OBS exists, we are not generating OBS data this time
		t.aisystem.generateobs = 1;
		cstr obsfile_s = "map.obs";
		if ( FileExist(obsfile_s.Get()) == 1 ) 
		{
			t.aisystem.generateobs = 0;
		}

		//  if CFG file present, copy to editor folder for later use (stores FPG for us)
		if ( t.game.runasmultiplayer == 0 ) 
		{
			//  single player/editor only - not needed when loading multiplayer map
			if ( t.tloadsuccessfully == 1 ) 
			{
				char tttfile_s[] = "cfg.cfg";
				if ( FileExist(tttfile_s) == 1 ) 
				{
					cstr cfgfile_s = g.mysystem.editorsGrideditAbs_s + tttfile_s;
					if ( FileExist( cfgfile_s.Get() ) == 1 ) DeleteAFile ( cfgfile_s.Get() );
					CopyAFile ( tttfile_s, cfgfile_s.Get() );
				}

				//PE: Load locked objects.
				extern std::vector<sRubberBandType> vEntityLockedList;
				vEntityLockedList.clear();
				if (FileExist("locked.cfg") == 1)
				{
					OpenToRead(1, "locked.cfg");
					int iAll = ReadLong(1);
					for (int i = 0; i < iAll; i++)
					{
						int e = ReadLong(1);
						sRubberBandType vEntityLockedItem;
						vEntityLockedItem.e = e;
						vEntityLockedList.push_back(vEntityLockedItem);
					}
					CloseFile(1);
				}
				// LB: The above list MAY carry around corrupt references, might be an idea
				// to scan and sanitise this agains the known entityelement and object list to ensure
				// bad refernces will not cause out of bounds errors. This will happen for older levels
				// that exploited a bug that caused objects to be changed to unlocked, but stayed in
				// the locked list and not removed, later being adopted by new objects added to level.
				// done later when editlock flags being set and know final size of entityelement array
				// search for gridedit_load_map with comment: "Restore locked state. from locked.cfg"
			}
		}

		// Retore and switch folders
		SetDir ( oldDir_s.Get() );

		//  if visuals file present, apply it
		timestampactivity(0,"LOADMAP: apply visuals");
		if (  t.trerfeshvisualsassets == 1 ) 
		{
			//  if loading from game (level load), ensure it's the game visuals we use
			if (  t.game.gameisexe == 1 || t.game.runasmultiplayer == 1  )  t.visuals = t.gamevisuals;
			//  and refresh assets based on restore
			t.visuals.refreshshaders=1;
			t.visuals.refreshterraintexture=1;
			t.visuals.refreshvegtexture=1;

			//PE: Remember sun angle.
			float oSx = t.visuals.SunAngleX;
			float oSy = t.visuals.SunAngleY;
			float oSz = t.visuals.SunAngleZ;

			//PE: Special wicked setup so retain visuals.
			t.visuals.refreshskysettings = 0;
			g.skyindex = t.visuals.skyindex; if (g.skyindex > g.skymax)  g.skyindex = g.skymax;
			t.visuals.sky_s = t.skybank_s[g.skyindex];
			t.terrainskyspecinitmode = 0;
			sky_skyspec_init( false );
			t.sky.currenthour_f = 8.0;
			t.sky.daynightprogress = 0;
			t.sky.changingsky = 1;

			// if change sky, regenerate env map
			timestampactivity(0, "LOADMAP: cubemap_generateglobalenvmap");
			cubemap_generateglobalenvmap();
			timestampactivity(0, "LOADMAP: visuals_loop");
			visuals_loop();

			//PE: In wicked we want to restore the sun angle from the map and not use skyspec.ini settings. (only when loading a old level).
			timestampactivity(0, "LOADMAP: Wicked_Update_Visuals");
			if (t.visuals.skyindex == 0 || t.visuals.bDisableSkybox)
			{
				//PE: Only if we re not using a simple skybox.
				t.visuals.SunAngleX = oSx;
				t.visuals.SunAngleY = oSy;
				t.visuals.SunAngleZ = oSz;
			}
			extern void Wicked_Update_Visuals(void *voidvisual);
			Wicked_Update_Visuals((void*) &t.visuals );
		}
	}
	else
	{
		t.tloadsuccessfully=0;
	}

	// fire up light probe update when get new level loaded
	extern bool g_bLightProbeScaleChanged;
	g_bLightProbeScaleChanged = true;

}

void mapfile_newmap ( void )
{
	//  Defaults
	t.layermax=20 ; t.maxx=500 ; t.maxy=500;
	t.olaylistmax=100;

	// when new map called, empty all terrain files 
	mapfile_emptyterrainfilesfromtestmapfolder();
}

void mapfile_loadmap ( void )
{
	// Load header data (need main mapdata for visdata)
	cstr filename_s = t.levelmapptah_s + "header.dat";
	int versionmajor = 0;
	int versionminor = 0;
	if ( FileExist(filename_s.Get()) == 1 ) 
	{
		// Header - version number
		OpenToRead ( 1, filename_s.Get() );
		versionmajor = ReadLong ( 1 );
		versionminor = ReadLong ( 1 );
		CloseFile ( 1 );
	}

	// 080917 - if old header, delete map.obj as it contains corrupt waypoint data
	if ( versionmajor < 1 )
	{
		cstr obstacleWaypointData_s = g.mysystem.levelBankTestMap_s + "map.obs";
		if ( FileExist ( obstacleWaypointData_s.Get() ) == 1 ) DeleteFileA ( obstacleWaypointData_s.Get() );
	}
}

void mapfile_savemap ( void )
{
	// Store old folder
	cstr oldDir_s = GetDir();

	// Enter folder
	SetDir ( g.mysystem.levelBankTestMap_s.Get() ); //"levelbank\\testmap\\" );

	// Clear old files out (TEMP)
	if ( FileExist("header.dat") == 1 ) DeleteAFile ( "header.dat" );

	// Create header file
	OpenToWrite ( 1, "header.dat" );

	// Version 0.0 = Reloaded
	// Version 1.0 = GameGuru DX11 (new obstacle data save fixes)
	const int versionmajor = 1; WriteLong ( 1, versionmajor );
	const int versionminor = 0; WriteLong ( 1, versionminor );

	// end of header
	CloseFile ( 1 );

	// Restore
	SetDir ( oldDir_s.Get() );
}

void mapfile_loadplayerconfig ( void )
{
	// Load player settings
	cstr filename_s = t.levelmapptah_s + "playerconfig.dat";
	if ( FileExist(filename_s.Get()) == 1 ) 
	{
		// Reloaded Header
		OpenToRead ( 1, filename_s.Get() );

		// version header
		cstr tmp_s = ReadString ( 1 );
		int tversion = ReadLong ( 1 );

		// player settings
		if ( tversion >= 201006 ) 
		{
			t.playercontrol.jumpmax_f = ReadFloat ( 1 );
			t.playercontrol.gravity_f = ReadFloat ( 1 );
			t.playercontrol.fallspeed_f = ReadFloat ( 1 );
			t.playercontrol.climbangle_f = ReadFloat ( 1 );
			t.playercontrol.footfallpace_f = ReadFloat ( 1 );
			t.playercontrol.wobblespeed_f = ReadFloat ( 1 );
			t.playercontrol.wobbleheight_f = ReadFloat ( 1 );
			t.playercontrol.accel_f = ReadFloat ( 1 );
		}

		// extra player settings
		if ( tversion >= 20100651 ) 
		{
			t.playercontrol.regenrate = static_cast<int>(ReadFloat ( 1 ));
			t.playercontrol.regenspeed = static_cast<int>(ReadFloat ( 1 ));
			t.playercontrol.regendelay = static_cast<int>(ReadFloat ( 1 ));
		}

		// extra player settings for third person (V1.01.002)
		if ( tversion >= 20100652 ) 
		{
			t.playercontrol.thirdperson.enabled = ReadLong ( 1 );
			t.playercontrol.thirdperson.startmarkere = ReadLong ( 1 );
			t.playercontrol.thirdperson.charactere = ReadLong ( 1 );
			t.playercontrol.thirdperson.cameradistance = ReadFloat ( 1 );
			t.playercontrol.thirdperson.cameraheight = ReadFloat ( 1 );
			t.playercontrol.thirdperson.cameraspeed = ReadFloat ( 1 );
		}
		if ( tversion >= 20100653 ) 
		{
			t.playercontrol.thirdperson.camerafocus = ReadFloat ( 1 );
			t.playercontrol.thirdperson.cameralocked = ReadLong ( 1 );
			t.playercontrol.thirdperson.camerashoulder = ReadLong ( 1 );
			t.playercontrol.thirdperson.camerafollow = ReadLong ( 1 );
			t.playercontrol.thirdperson.camerareticle = ReadLong ( 1 );
		}

		CloseFile ( 1 );
	}

	// No third person mode in MAX
	t.playercontrol.thirdperson.enabled = 0;
}

void mapfile_saveplayerconfig ( void )
{
	// Store old folder
	cstr oldDir_s = GetDir();

	// Enter folder
	SetDir ( g.mysystem.levelBankTestMap_s.Get() ); // "levelbank\\testmap\\" );

	// Version for player config has minor value (between betas)
	const int gtweakversion = 20100653;

	// WriteLong ( out )
	if ( FileExist("playerconfig.dat") == 1 ) DeleteAFile ( "playerconfig.dat" );
	OpenToWrite ( 1, "playerconfig.dat" );

	// version header
	WriteString ( 1, "version" );
	WriteLong ( 1, gtweakversion );

	// player settings
	WriteFloat ( 1, t.playercontrol.jumpmax_f );
	WriteFloat ( 1, t.playercontrol.gravity_f );
	WriteFloat ( 1, t.playercontrol.fallspeed_f );
	WriteFloat ( 1, t.playercontrol.climbangle_f );
	WriteFloat ( 1, t.playercontrol.footfallpace_f );
	WriteFloat ( 1, t.playercontrol.wobblespeed_f );
	WriteFloat ( 1, t.playercontrol.wobbleheight_f );
	WriteFloat ( 1, t.playercontrol.accel_f );

	// extra settings from V1.0065 (20100651)
	WriteFloat ( 1, static_cast<float>(t.playercontrol.regenrate) );
	WriteFloat ( 1, static_cast<float>(t.playercontrol.regenspeed) );
	WriteFloat ( 1, static_cast<float>(t.playercontrol.regendelay) );

	// extra settings from V1.01.030 (20100652)
	WriteLong ( 1, t.playercontrol.thirdperson.enabled );
	WriteLong ( 1, t.playercontrol.thirdperson.startmarkere );
	WriteLong ( 1, t.playercontrol.thirdperson.charactere );
	WriteFloat ( 1, t.playercontrol.thirdperson.cameradistance );
	WriteFloat ( 1, t.playercontrol.thirdperson.cameraheight );
	WriteFloat ( 1, t.playercontrol.thirdperson.cameraspeed );

	// 20100653 additions
	WriteFloat ( 1, t.playercontrol.thirdperson.camerafocus );
	WriteLong ( 1, t.playercontrol.thirdperson.cameralocked );
	WriteLong ( 1, t.playercontrol.thirdperson.camerashoulder );
	WriteLong ( 1, t.playercontrol.thirdperson.camerafollow );
	WriteLong ( 1, t.playercontrol.thirdperson.camerareticle );

	CloseFile ( 1 );

	// Restore
	SetDir ( oldDir_s.Get() );
}

void addthisentityprofilesfilestocollection (entityeleproftype* pEleProf, int entid)
{
	if ( entid < 0 || entid >= static_cast<int>(t.entityprofile.size()) ) return;
	
	// Ensure we also collect any textures for Building Editor entities - they are not included with the export
	entityprofiletype& entProfile = t.entityprofile[entid];
	if (strstr(entProfile.model_s.Get(), "smartchild"))
	{
		WickedMaterial& material = entProfile.WEMaterial;
		for (int i = 0; i < MAXMESHMATERIALS; i++)
		{
			if (material.baseColorMapName[i].Len() > 0)
			{
				addtocollection(material.baseColorMapName[i].Get());
				addtocollection(material.normalMapName[i].Get());
				addtocollection(material.emissiveMapName[i].Get());
				addtocollection(material.surfaceMapName[i].Get());
				addtocollection(material.displacementMapName[i].Get());
			}
		}
	}

	// entity profile file
	cstr tentityname1_s = cstr("entitybank\\") + t.entitybank_s[entid];
	cstr tentityname2_s = cstr(Left(tentityname1_s.Get(), Len(tentityname1_s.Get()) - 4)) + ".bin";
	cstr tentityname_s;
	if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + tentityname2_s).Get()) == 1)
	{
		tentityname_s = tentityname2_s;
	}
	else
	{
		tentityname_s = tentityname1_s;
	}
	addtocollection(tentityname_s.Get());

	//PE: NEWLOD
	std::string lodname = tentityname_s.Get();
	replaceAll(lodname, ".fpe", "_lod.dbo");
	replaceAll(lodname, ".bin", "_lod.dbo");
	addtocollection((char*)lodname.c_str());

	// and the BULLET file if exist (caused slowdown of standalone level loads!)
	std::string bulletname = tentityname_s.Get();
	replaceAll(bulletname, ".fpe", ".bullet");
	addtocollection((char*)bulletname.c_str());

	// entity files in folder
	cstr tentityfolder_s = tentityname_s;
	for (int n = Len(tentityname_s.Get()); n >= 1; n += -1)
	{
		if (cstr(Mid(tentityname_s.Get(), n)) == "\\" || cstr(Mid(tentityname_s.Get(), n)) == "/")
		{
			tentityfolder_s = Left(tentityfolder_s.Get(), n);
			break;
		}
	}

	//  model files (main model, final appended model and all other append
	int iModelAppendFileCount = t.entityprofile[entid].appendanimmax;
	if (Len (t.entityappendanim[entid][0].filename.Get()) > 0) iModelAppendFileCount = 0;
	cstr tfile1_s;
	for (int iModels = -1; iModels <= iModelAppendFileCount; iModels++)
	{
		LPSTR pModelFile = "";
		if (iModels == -1)
		{
			pModelFile = t.entityprofile[entid].model_s.Get();
		}
		else
		{
			pModelFile = t.entityappendanim[entid][iModels].filename.Get();
		}
		int tlocaltofpe = 1;
		for (int n = 1; n <= Len(pModelFile); n++)
		{
			if (cstr(Mid(pModelFile, n)) == "\\" || cstr(Mid(pModelFile, n)) == "/")
			{
				tlocaltofpe = 0; break;
			}
		}
		if (tlocaltofpe == 1)
		{
			tfile1_s = tentityfolder_s + pModelFile;
		}
		else
		{
			tfile1_s = pModelFile;
		}

		cstr tfile2_s = cstr(Left(tfile1_s.Get(), Len(tfile1_s.Get()) - 2)) + ".dbo";
		cstr tfile_s;
		if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + tfile2_s).Get()) == 1)
		{
			tfile_s = tfile2_s;
		}
		else
		{
			tfile_s = tfile1_s;
		}
		cstr tmodelfile_s = tfile_s;
		addtocollection(tmodelfile_s.Get());

		//PE: CCP have missing textures the body part, i seen entrys like 'baseColorMap0    = tempfinalalbedo0.dds' , so always copy over main texture.
		if (t.entityprofile[entid].ischaracter)
		{
			cstr ccpname = tmodelfile_s;
			std::string sParseName = tmodelfile_s.Get();
			std::string sParseNext = sParseName;
			replaceAll(sParseName, ".dbo", "0.dds"); //CCP missing main texture (body).
			addtocollection((char *)sParseName.c_str());
			sParseName = sParseNext;
			replaceAll(sParseName, ".dbo", "1.dds"); //PE: Legs with custom skin also need to be added.
			addtocollection((char *)sParseName.c_str());
			sParseName = sParseNext;
			replaceAll(sParseName, ".dbo", "2.dds"); //PE: Feets with custom skin also need to be added.
			addtocollection((char *)sParseName.c_str());
		}

		// if entity did not specify texture it is multi-texture, so interogate model file
		findalltexturesinmodelfile(tmodelfile_s.Get(), tentityfolder_s.Get(), t.entityprofile[entid].texpath_s.Get());
	}

	// Export entity FPE BMP file if flagged
	if (g.gexportassets == 1)
	{
		cstr tfile3_s = cstr(Left(tentityname_s.Get(), Len(tentityname_s.Get()) - 4)) + ".bmp";
		if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + tfile3_s).Get()) == 1)
		{
			addtocollection(tfile3_s.Get());
		}
	}

	// entity characterpose file (if any)
	cstr tfile3_s = cstr(Left(tfile1_s.Get(), Len(tfile1_s.Get()) - 2)) + ".dat";
	if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + tfile3_s).Get()) == 1)
	{
		addtocollection(tfile3_s.Get());
	}

	// bullet physics hull decomp file (if any)
	tfile3_s = cstr(Left(tentityname_s.Get(), Len(tentityname_s.Get()) - 4)) + ".bullet";
	if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + tfile3_s).Get()) == 1)
	{
		addtocollection(tfile3_s.Get());
	}

	// texture files
	int iStartingType = (pEleProf != nullptr) ? 0 : 1;
	for (int iBothTypes = iStartingType; iBothTypes < 2; iBothTypes++)
	{
		// can be from ELEPROF of entityelement (older maps point to old texture names) or parent ELEPROF original
		cstr pTextureFile = "", pAltTextureFile = "";
		if (iBothTypes == 0)
		{ 
			if (pEleProf)
			{
				pTextureFile = pEleProf->texd_s; pAltTextureFile = pEleProf->texaltd_s;
			}
		}
		if (iBothTypes == 1) 
		{ 
			pTextureFile = t.entityprofile[entid].texd_s; pAltTextureFile = t.entityprofile[entid].texaltd_s; 
		}
		int tlocaltofpe = 1;
		for (int n = 1; n <= Len(pTextureFile.Get()); n++)
		{
			if (cstr(Mid(pTextureFile.Get(), n)) == "\\" || cstr(Mid(pTextureFile.Get(), n)) == "/")
			{
				tlocaltofpe = 0; break;
			}
		}
		cstr tfile_s;
		if (tlocaltofpe == 1)
		{
			tfile_s = tentityfolder_s + pTextureFile;
		}
		else
		{
			tfile_s = pTextureFile;
		}
		addtocollection(tfile_s.Get());

		// always allow a DDS texture of same name to be copied over (for test game compatibility)
		for (int iTwoExtensions = 0; iTwoExtensions <= 1; iTwoExtensions++)
		{
			cstr tfileext_s;
			if (iTwoExtensions == 0) tfileext_s = Right (tfile_s.Get(), 3);
			if (iTwoExtensions == 1) tfileext_s = "dds";
			if (cstr(Left(Lower(Right(tfile_s.Get(), 6)), 2)) == "_d")
			{
				cstr tex = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - 6)) + "_n." + tfileext_s; addtocollection(tex.Get());
				tex = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - 6)) + "_s." + tfileext_s; addtocollection(tex.Get());
				tex = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - 6)) + "_i." + tfileext_s; addtocollection(tex.Get());
				tex = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - 6)) + "_o." + tfileext_s; addtocollection(tex.Get());
				tex = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - 6)) + "_cube." + tfileext_s; addtocollection(tex.Get());
			}
			int iNewPBRTextureMode = 0;
			if (cstr(Left(Lower(Right(tfile_s.Get(), 10)), 6)) == "_color") iNewPBRTextureMode = 6 + 4;
			if (cstr(Left(Lower(Right(tfile_s.Get(), 11)), 7)) == "_albedo") iNewPBRTextureMode = 7 + 4;
			if (iNewPBRTextureMode > 0)
			{
				cstr pToAdd;
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_color." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_albedo." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_normal." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_specular." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_metalness." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_gloss." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_mask." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_ao." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_height." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_detail." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_surface." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_emissive." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_illumination." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_illum." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_i." + tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(tfile_s.Get(), Len(tfile_s.Get()) - iNewPBRTextureMode)) + "_cube." + tfileext_s; addtocollection(pToAdd.Get());
			}
		}
		if (tlocaltofpe == 1)
		{
			tfile_s = tentityfolder_s + pAltTextureFile;
		}
		else
		{
			tfile_s = pAltTextureFile;
		}
		addtocollection(tfile_s.Get());
	}

	// also include textures specified by textureref entries
	// and assemblyccp from new character creator plus
	cstr tFPEFilePath = g.fpscrootdir_s + "\\Files\\";
	tFPEFilePath += tentityname1_s;
	FILE* tFPEFile = GG_fopen (tFPEFilePath.Get(), "r");
	if (tFPEFile)
	{
		char tTempLine[2048];
		while (!feof(tFPEFile))
		{
			fgets (tTempLine, 2047, tFPEFile);
			//PE: We need to add all custom mesh textures.
			if (strstr(tTempLine, "baseColorMap"))
			{
				char* pToFilename = strstr(tTempLine, "=");
				if (pToFilename)
				{
					while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					//PE: CCP is special as we dont have to add entitybank...
					if (pestrcasestr(pToFilename, "charactercreatorplus\\parts\\"))
					{
						addtocollection(pToFilename);
					}
					else
					{
						cstr tTextureFile = cstr(tentityfolder_s + cstr(pToFilename));
						addtocollection(tTextureFile.Get());
					}
				}
			}
			if (strstr(tTempLine, "normalMap"))
			{
				char* pToFilename = strstr(tTempLine, "=");
				if (pToFilename)
				{
					while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					//PE: CCP is special as we dont have to add entitybank...
					if (pestrcasestr(pToFilename, "charactercreatorplus\\parts\\"))
					{
						addtocollection(pToFilename);
					}
					else
					{
						cstr tTextureFile = cstr(tentityfolder_s + cstr(pToFilename));
						addtocollection(tTextureFile.Get());
					}
				}
			}
			if (strstr(tTempLine, "surfaceMap"))
			{
				char* pToFilename = strstr(tTempLine, "=");
				if (pToFilename)
				{
					while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					//PE: CCP is special as we dont have to add entitybank...
					if (pestrcasestr(pToFilename, "charactercreatorplus\\parts\\"))
					{
						addtocollection(pToFilename);
					}
					else
					{
						cstr tTextureFile = cstr(tentityfolder_s + cstr(pToFilename));
						addtocollection(tTextureFile.Get());
					}
				}
			}
			if (strstr (tTempLine, "textureref"))
			{
				char* pToFilename = strstr (tTempLine, "=");
				if (pToFilename)
				{
					while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					cstr tTextureFile = cstr(tentityfolder_s + cstr(pToFilename));
					addtocollection (tTextureFile.Get());
				}
			}
			if (strstr (tTempLine, "ccpassembly"))
			{
				// LB: Copied this so I can work out textures from this field, but may be able to adapt 
				// this "CreateVectorListFromCPPAssembly" call to here as well.
				char* pAssemblyString = strstr (tTempLine, "=");
				if (pAssemblyString)
				{
					// get past equals and any spaces
					while (*pAssemblyString == '=' || *pAssemblyString == 32) pAssemblyString++;

					// now we have the assembly string; adult male hair 01,adult male head 01,adult male body 03,adult male legs 04e,adult male feet 04
					// delimited by a comma, and indicates which parts we used (to specify the textures to copy over)
					char pCustomPathToFolder[MAX_PATH];
					cstr assemblyString_s = FirstToken(pAssemblyString, ",");
					while (assemblyString_s.Len() > 0)
					{
						// work out texture files from this reference, i.e adultmalehair01
						char pAssemblyReference[1024];
						strcpy(pAssemblyReference, assemblyString_s.Get());
						if (pAssemblyReference[strlen(pAssemblyReference) - 1] == '\n') pAssemblyReference[strlen(pAssemblyReference) - 1] = 0;
						strlwr(pAssemblyReference);
						int iBaseCount = static_cast<int>(g_CharacterType.size());// 4; //(3) PE: Added zombie female
						for (int iBaseIndex = 0; iBaseIndex < iBaseCount; iBaseIndex++)
						{
							LPSTR pBaseName = "";
							if (iBaseIndex == 0) pBaseName = "adult male";
							if (iBaseIndex == 1) pBaseName = "adult female";
							if (iBaseIndex == 2) pBaseName = "zombie male";
							if (iBaseIndex == 3) pBaseName = "zombie female";
							if (iBaseIndex > 3) pBaseName = g_CharacterType[iBaseIndex].pPartsFolder;
							if (strstr(pAssemblyReference, pBaseName) != NULL)
							{
								// found category
								cstr pPartFolder = "";
								if (iBaseIndex == 0) pPartFolder = "charactercreatorplus\\parts\\adult male\\";
								if (iBaseIndex == 1) pPartFolder = "charactercreatorplus\\parts\\adult female\\";
								if (iBaseIndex == 2) pPartFolder = "charactercreatorplus\\parts\\zombie male\\";
								if (iBaseIndex == 3) pPartFolder = "charactercreatorplus\\parts\\zombie female\\";
								if (iBaseIndex > 3)
								{
									sprintf(pCustomPathToFolder, "charactercreatorplus\\parts\\%s\\", g_CharacterType[iBaseIndex].pPartsFolder);
									pPartFolder = pCustomPathToFolder;
								}

								// add final texture files
								cstr pTmpFile = pPartFolder + pAssemblyReference;
								char pRemoveTag[MAX_PATH];
								strcpy(pRemoveTag, pTmpFile.Get());
								const int tagLen = static_cast<int>(strlen(pRemoveTag));
								for (int nnn = 0; nnn < tagLen; nnn++)
								{
									if (pRemoveTag[nnn] == '[')
									{
										if (nnn > 0 && pRemoveTag[nnn - 1] == ' ') nnn--;
										pRemoveTag[nnn] = 0;
										break;
									}
								}

								// need to strip out the tag [xxx] part to find texture proper
								pTmpFile = pRemoveTag;
								addtocollection (cstr(pTmpFile + "_ao.dds").Get());
								addtocollection (cstr(pTmpFile + "_color.dds").Get());
								addtocollection (cstr(pTmpFile + "_gloss.dds").Get());
								addtocollection (cstr(pTmpFile + "_mask.dds").Get());
								addtocollection (cstr(pTmpFile + "_metalness.dds").Get());
								addtocollection(cstr(pTmpFile + "_surface.dds").Get());
								addtocollection (cstr(pTmpFile + "_normal.dds").Get());
							}
						}
						assemblyString_s = NextToken(",");
					}
				}
			}
		}
		fclose (tFPEFile);
	}
}

// we set this during level loading, then action it near the end
bool g_bUsingSomeKindOfTerrain = false;

void mapfile_collectfoldersandfiles (cstr levelpathfolder)
{
	cstr pOldDir = GetDir();

	// Collect ALL files in string array list
	Undim (t.filecollection_s);
	g.filecollectionmax = 0;
	Dim (t.filecollection_s, 500);

	//  Stage 1 - specify all common files
	addtocollection(cstr(cstr("languagebank\\") + g.language_s + "\\textfiles\\guru-wordcount.ini").Get());
	addtocollection(cstr(cstr("languagebank\\") + g.language_s + "\\textfiles\\guru-words.txt").Get());
	addtocollection(cstr(cstr("languagebank\\") + g.language_s + "\\inittext.ssp").Get());
	addtocollection("audiobank\\misc\\silence.wav");
	addtocollection("audiobank\\misc\\explode.wav");
	addtocollection("audiobank\\misc\\ammo.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_01.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_02.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_03.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_04.wav");
	addtocollection("audiobank\\misc\\melee.wav");
	addtocollection("audiobank\\misc\\melee1.wav");
	addtocollection("audiobank\\misc\\melee2.wav");
	addtocollection("audiobank\\misc\\melee3.wav");
	addtocollection("audiobank\\misc\\melee4.wav");
	addtocollection("audiobank\\misc\\melee5.wav");
	addtocollection("audiobank\\misc\\melee6.wav");
	addtocollection("editors\\gfx\\14.png");
	addtocollection("editors\\gfx\\14-white.png");
	addtocollection("editors\\gfx\\14-red.png");
	addtocollection("editors\\gfx\\14-green.png");
	addtocollection("editors\\gfx\\dummy.png");
	addtocollection("editors\\gfx\\notexture.dds");
	addtocollection("editors\\keymap\\default.ini");
	addtocollection("editors\\keymap\\weaponslots.dat");
	addtocollection("editors\\templates\\ScreenEditor\\project.dat");
	addtocollection("scriptbank\\gameloop.lua");
	addtocollection("scriptbank\\gameplayercontrol.lua");
	addtocollection("scriptbank\\gameplayerhealth.lua");
	addtocollection("scriptbank\\global.lua");

	addfoldertocollection(cstr(cstr("languagebank\\") + g.language_s + "\\artwork\\watermark").Get());
	addfoldertocollection("scriptbank\\people\\ai");
	addtocollection("scriptbank\\people\\patrol.byc");
	addtocollection("scriptbank\\people\\patrol.lua");
	addfoldertocollection("scriptbank\\ai");
	addfoldertocollection("scriptbank\\images");
	addfoldertocollection("audiobank\\materials");
	addfoldertocollection("audiobank\\user");

	addtocollection("scriptbank\\perlin_noise.lua");
	addtocollection("scriptbank\\hud0.lua");
	addtocollection("scriptbank\\utillib.lua"); //PE: hud0 use  utillib.lua
	addtocollection("scriptbank\\huds\\cursorcontrol.lua");
	addtocollection("scriptbank\\gameplayerhealth.lua");
	addtocollection("scriptbank\\gameplayerspeed.lua");
	addtocollection("scriptbank\\huds\\cursorcontrol.lua");

	// not all cineguru scripts/associated files cover over, ensure they do
	addfoldertocollection("scriptbank\\Cine Guru MAX");
	addfoldertocollection("scriptbank\\user\\actors");

	//PE: Missing foot step material sounds
	addfoldertocollection("audiobank\\materials\\dirt");
	addfoldertocollection("audiobank\\materials\\grass");
	addfoldertocollection("audiobank\\materials\\grass\\extras");
	addfoldertocollection("audiobank\\materials\\gravel");
	addfoldertocollection("audiobank\\materials\\metal");
	addfoldertocollection("audiobank\\materials\\puddle");
	addfoldertocollection("audiobank\\materials\\sand");
	addfoldertocollection("audiobank\\materials\\snow");
	addfoldertocollection("audiobank\\materials\\tarmac");
	addfoldertocollection("audiobank\\materials\\underwater");
	addfoldertocollection("audiobank\\materials\\wood");

	addfoldertocollection("audiobank\\music\\theescape");
	addfoldertocollection("audiobank\\voices\\player");
	addfoldertocollection("audiobank\\voices\\characters");
	addfoldertocollection("audiobank\\character\\soldier\\onAggro");
	addfoldertocollection("audiobank\\character\\soldier\\onAlert");
	addfoldertocollection("audiobank\\character\\soldier\\onDeath");
	addfoldertocollection("audiobank\\character\\soldier\\onHurt");
	addfoldertocollection("audiobank\\character\\soldier\\onHurtPlayer");
	addfoldertocollection("audiobank\\character\\soldier\\onIdle");
	addfoldertocollection("audiobank\\character\\soldier\\onInteract");
	addfoldertocollection("databank");
	addallinfoldertocollection("titlesbank", "titlesbank"); // need the ENTIRE contents - now includes the root files not just the folders!
	addfoldertocollection("effectbank\\reloaded");
	addfoldertocollection("effectbank\\reloaded\\media");
	addfoldertocollection("effectbank\\reloaded\\media\\materials");
	addfoldertocollection("effectbank\\explosion");
	addfoldertocollection("effectbank\\particles");
	addfoldertocollection("effectbank\\particles\\weather");
	addfoldertocollection("lensflares");
	addfoldertocollection("fontbank");
	addfoldertocollection("languagebank\\neutral\\gamecore\\huds\\ammohealth");
	addfoldertocollection("languagebank\\neutral\\gamecore\\huds\\panels");

	addfoldertocollection("gamecore\\decals\\blood"); //PE: New particle effects.
	addfoldertocollection("gamecore\\decals\\explosion"); //PE: New particle effects.
	//PE: New added effects.
	addfoldertocollection("gamecore\\decals\\explosion huge");
	addfoldertocollection("gamecore\\decals\\explosion large");
	addfoldertocollection("gamecore\\decals\\explosion medium");
	addfoldertocollection("gamecore\\decals\\explosion small");
	addfoldertocollection("gamecore\\decals\\explosion_blood");
	addfoldertocollection("gamecore\\decals\\splat");
	addfoldertocollection("gamecore\\decals\\bloodsplat");
	addfoldertocollection("gamecore\\decals\\impact");
	addfoldertocollection("gamecore\\decals\\gunsmoke");
	addfoldertocollection("gamecore\\decals\\smoke1");
	addfoldertocollection("gamecore\\decals\\muzzleflash4");
	addfoldertocollection("gamecore\\decals\\splash_droplets");
	addfoldertocollection("gamecore\\decals\\splash_foam");
	addfoldertocollection("gamecore\\decals\\splash_large");
	addfoldertocollection("gamecore\\decals\\splash_misty");
	addfoldertocollection("gamecore\\decals\\splash_ripple");
	addfoldertocollection("gamecore\\decals\\splash_small");
	addfoldertocollection("gamecore\\decals\\splinters");
	addfoldertocollection("gamecore\\decals\\sparks");
	addfoldertocollection("gamecore\\decals\\dust");


	addfoldertocollection("gamecore\\vrcontroller");
	addfoldertocollection("gamecore\\vrcontroller\\oculus");
	addfoldertocollection("gamecore\\projectiletypes");
	addfoldertocollection("gamecore\\projectiletypes\\common\\explode");
	addfoldertocollection("gamecore\\projectiletypes\\enhanced\\m67");
	addfoldertocollection("gamecore\\bulletholes");
	addfoldertocollection("editors\\lut");

	// folders related to terrain system
	g_bUsingSomeKindOfTerrain = false;

	//PE: Still need old effects. arx
	addfoldertocollection("particlesbank");

	addtocollection("effectbank\\common\\noise64.png");
	addtocollection("effectbank\\common\\dist2.png");
	addfoldertocollection("effectbank\\common"); //Just in case we get more.

	addtocollection("pinetree_high_color_1024.dds");
	addtocollection("pinetree.dds");
	addtocollection("noise.dds");

	addtocollection("skybank\\clear\\"); //for fallback.

	// add any material decals that are active
	for (int m = 0; m <= g.gmaterialmax; m++)
	{
		if (t.material[m].usedinlevel == 1)
		{
			cstr decalFolder_s = cstr("gamecore\\decals\\") + t.material[m].decal_s;
			addfoldertocollection(decalFolder_s.Get());
		}
	}

	addfoldertocollection("gamecore\\muzzleflash");
	addfoldertocollection("gamecore\\projectiletypes");

	// we will much improve this with the new project system!!
	addfoldertocollection("gamecore\\hands\\Animations");
	addallinfoldertocollection("gamecore\\guns\\interactive", "gamecore\\guns\\interactive");
	//PE: We now have lua script that use this directly, so add by default for now.
	addfoldertocollection("gamecore\\guns\\enhanced\\Gloves_Unarmed");
	//PE: Need to read hudcustom.txt from all used guns folders, for now.
	addfoldertocollection("gamecore\\hands\\Male Light");
	addfoldertocollection("gamecore\\hands\\Male Dark");
	addfoldertocollection("gamecore\\hands\\Female Light");
	addfoldertocollection("gamecore\\hands\\Female Dark");
	addfoldertocollection("gamecore\\hands\\Combat Gloves Light");
	addfoldertocollection("gamecore\\hands\\Combat Gloves Dark");
	addfoldertocollection("gamecore\\hands\\Low Poly");
	addfoldertocollection("gamecore\\hands\\Fantasy Gauntlets");
	addfoldertocollection("gamecore\\hands\\Fantasy Leather Gloves");

	//  Stage 1B - Style dependent files
	titles_getstyle ();
	addtocollection("titlesbank\\style.txt");
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1280x720").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1280x800").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1366x768").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1440x900").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1600x900").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1680x1050").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1920x1080").Get());
	addfoldertocollection(cstr(cstr("titlesbank\\") + t.ttheme_s + "\\1920x1200").Get());

	// HUD elements
	addfoldertocollection("imagebank\\HUD");
	addfoldertocollection("imagebank\\HUD Library\\MAX");

	// include original FPM
	addtocollection(t.tmasterlevelfile_s.Get());

	//PE: Add .lst version , this is to be used for thread loading of level textures and objects.
	std::string lstfile = t.tmasterlevelfile_s.Get();
	replaceAll(lstfile, ".fpm", ".lst");
	addtocollection((char*)lstfile.c_str());

	// Pre-Stage 2 - clear a list which will collect all folders/files to REMOVE from the final standalone file transfer
	// list, courtesy of the special FPP file which controls the final files to be used for standalone creation
	// (eventually to be controlled from a nice UI)
	std::vector<cstr> fppFoldersToRemoveList;
	std::vector<cstr> fppFilesToRemoveList;
	fppFoldersToRemoveList.clear();
	fppFilesToRemoveList.clear();

	// Stage 2 - collect all files (from all levels)
	t.levelmax = 0;
	Dim (t.levellist_s, 100);
	for (int i = 0; i < 100; i++) t.levellist_s[i] = "";
	addtocollection(t.visuals.sAmbientMusicTrack.Get());
	addtocollection(t.visuals.sCombatMusicTrack.Get());
	t.levelindex = 1;

	//PE: Need adding images from g_collectionQuestList
	for (int n = 0; n < g_collectionQuestList.size(); n++)
	{
		if (g_collectionQuestList[n].collectionFields.size() > 2)
		{
			LPSTR pImageFile = g_collectionQuestList[n].collectionFields[2].Get();
			if (strlen(pImageFile) > 0)
			{
				if (stricmp(pImageFile, "default") != NULL && stricmp(pImageFile, "image") != NULL)
				{
					addtocollection(pImageFile);
				}
			}

		}
	}

	// Add images from collection list (can be stored in thumbbank)
	for (int n = 0; n < g_collectionList.size(); n++)
	{
		if (g_collectionList[n].collectionFields.size() > 2)
		{
			LPSTR pImageFile = g_collectionList[n].collectionFields[2].Get();
			if (strlen(pImageFile) > 0)
			{
				if (stricmp(pImageFile, "default") != NULL && stricmp(pImageFile, "image") != NULL)
				{
					addtocollection(pImageFile);
				}
			}
		}
	}

	//Add all storyboard files to scan list.
	if (g.bUseStoryBoardSetup)
	{
		addfoldertocollection("editors\\templates\\fonts");
		addtocollection("editors\\uiv3\\Roboto-Medium.ttf");

		// go through and add all FPMs to export
		char pIncludeMapFile[MAX_PATH];
		FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
		g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
		strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);
		int foundlevel = 1; // FindNextLevel(g_Storyboard_Current_Level, g_Storyboard_Current_fpm); thix would ignore the first level, instead assume first level is good level
		while (foundlevel == 1)
		{
			bool bAlreadyAdded = false;
			for (int n = 0; n < t.levelmax; n++)
			{
				if (stricmp(g_Storyboard_Current_fpm, t.levellist_s[n].Get()) == NULL)
				{
					bAlreadyAdded = true;
					break;
				}
			}
			if (bAlreadyAdded == false)
			{
				++t.levelmax;
				t.levellist_s[t.levelmax] = g_Storyboard_Current_fpm;
				addtocollection(g_Storyboard_Current_fpm);
				if (strlen(g_Storyboard_Current_fpm) > 5)
				{
					strcpy(pIncludeMapFile, g_Storyboard_Current_fpm);
					pIncludeMapFile[strlen(pIncludeMapFile) - 4] = 0;
					strcat(pIncludeMapFile, ".png");
					addtocollection(pIncludeMapFile);
				}
				foundlevel = FindNextLevel(g_Storyboard_Current_Level, g_Storyboard_Current_fpm);
			}
			else
			{
				// level recursed back on itself, end this loop!
				foundlevel = 0;
			}
		}

		// Now find any levels that are on the Storyboard, but have not been marked for collection (not connected to any screens - loaded from Winzone)
		for (int i = 0; i < STORYBOARD_MAXNODES; i++)
		{
			if (Storyboard.Nodes[i].used && Storyboard.Nodes[i].type == STORYBOARD_TYPE_LEVEL)
			{
				// get level name
				cStr levelName = Storyboard.Nodes[i].level_name;
				if (strlen(levelName.Get()) > 0)
				{
					// only valid if have a level name here
					// Check if this Storyboard level has already been marked for collection of its files
					bool bAlreadyCollected = false;
					for (int j = 1; j <= t.levelmax; j++)
					{
						if (strcmp(levelName.Get(), t.levellist_s[j].Get()) == 0)
						{
							bAlreadyCollected = true;
							break;
						}
					}
					if (!bAlreadyCollected)
					{
						// This level has not yet been marked for collection
						++t.levelmax;
						t.levellist_s[t.levelmax] = levelName;
						addtocollection(levelName.Get());
						if (levelName.Len() > 5)
						{
							strcpy(pIncludeMapFile, levelName.Get());
							pIncludeMapFile[strlen(pIncludeMapFile) - 4] = 0;
							strcat(pIncludeMapFile, ".png");
							addtocollection(pIncludeMapFile);
						}
					}
				}
			}
		}

		//Restore to first level.
		FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
		g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
		strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);

		//Use project name as exename
		if (strlen(Storyboard.gamename) > 0)
		{
			//Add project.
			char project[MAX_PATH];
			strcpy(project, "projectbank\\");
			strcat(project, Storyboard.gamename);
			addfoldertocollection(project);

			// add loading splash in case of a needed fallback
			addtocollection("editors\\uiv3\\loadingsplash.jpg");

			// add all media used by storyboard.
			for (int nodeid = 0; nodeid < STORYBOARD_MAXNODES; nodeid++)
			{
				if (Storyboard.Nodes[nodeid].used)
				{
					// include splashscreen if specified
					if (Storyboard.Nodes[nodeid].type == STORYBOARD_TYPE_SPLASH)
					{
						if (strlen(Storyboard.Nodes[nodeid].thumb) > 0)
						{
							addtocollection(Storyboard.Nodes[nodeid].thumb);
						}
					}

					if (strlen(Storyboard.Nodes[nodeid].screen_music) > 0)
					{
						addtocollection(Storyboard.Nodes[nodeid].screen_music);
					}
					if (strlen(Storyboard.Nodes[nodeid].screen_backdrop) > 0)
					{
						addtocollection(Storyboard.Nodes[nodeid].screen_backdrop);
					}

					for (int i = 0; i < STORYBOARD_MAXWIDGETS; i++)
					{
						if (Storyboard.Nodes[nodeid].widget_used[i] == 1)
						{
							if (strlen(Storyboard.Nodes[nodeid].widget_normal_thumb[i]) > 0)
							{
								addtocollection(Storyboard.Nodes[nodeid].widget_normal_thumb[i]);
							}
							if (strlen(Storyboard.Nodes[nodeid].widget_highlight_thumb[i]) > 0)
							{
								addtocollection(Storyboard.Nodes[nodeid].widget_highlight_thumb[i]);
							}
							if (strlen(Storyboard.Nodes[nodeid].widget_selected_thumb[i]) > 0)
							{
								addtocollection(Storyboard.Nodes[nodeid].widget_selected_thumb[i]);
							}
							//PE: We are going to reuse for widget_click_sound for default value for "Text Globals"
							if (Storyboard.Nodes[nodeid].widget_type[i] == STORYBOARD_WIDGET_BUTTON
								|| Storyboard.Nodes[nodeid].widget_type[i] == STORYBOARD_WIDGET_RADIOTYPE
								|| Storyboard.Nodes[nodeid].widget_type[i] == STORYBOARD_WIDGET_TICKBOX)
							{

								if (strlen(Storyboard.Nodes[nodeid].widget_click_sound[i]) > 0)
								{
									addtocollection(Storyboard.Nodes[nodeid].widget_click_sound[i]);
								}
							}
						}
					}
				}
			}
		}
	}

	// if remote project, add ALL files in remote project folder to the files to be copied over
	if ( strlen(Storyboard.customprojectfolder) > 0)
	{
		// add all files in the remote project folder
		char pAbsRemoteProjectFolderToScan[MAX_PATH];
		strcpy(pAbsRemoteProjectFolderToScan, Storyboard.customprojectfolder);
		strcat(pAbsRemoteProjectFolderToScan, Storyboard.gamename);
		strcat(pAbsRemoteProjectFolderToScan, "\\Files\\");
		addallinfoldertocollection(pAbsRemoteProjectFolderToScan, "");

		// and them remove any that are not needed in the standalone
		removeanymatchingfromcollection("mapbank\\_automatedbackups");
		removeanymatchingfromcollection("ebebank\\default");
		removeanymatchingfromcollection("levelbank\\testmap");
	}
}

void mapfile_savestandalone_start ( void )
{
	// first grab current folder for later restoring
	g_mapfile_told_s = GetDir();

	// In Classic, I would run through the load process and collect files as they
	// where loaded in. In Reloaded, the currently loaded level data is scanned
	// to arrive at the required files for the Standalone EXE
	t.interactive.savestandaloneused=1;

	// this flag ensures the loadassets splash does not appear when making standalone
	t.levelsforstandalone = 1;

	// 040316 - v1.13b1 - find the nested folder structure of the level (could be in map bank\Easter\level1.fpm)
	g_mapfile_told_s = GetDir();
	if ( g.projectfilename_s.Get()[1] != ':' )
	{
		// relative project path
		g_mapfile_mapbankpath = cstr("mapbank\\");
	}
	else
	{
		// absolute project path
		g_mapfile_mapbankpath = g.mysystem.mapbankAbs_s;
	}
	int projLen = static_cast<int>(strlen(g.projectfilename_s.Get()));
	int bankLen = static_cast<int>(strlen(g_mapfile_mapbankpath.Get()));
	g_mapfile_levelpathfolder = (projLen >= bankLen) ? Right(g.projectfilename_s.Get(), projLen - bankLen) : "";

	bool bGotNestedPath = false;
	for ( int n = Len(g_mapfile_levelpathfolder.Get()) ; n >= 1 ; n+= -1 )
	{
		if ( cstr(Mid(g_mapfile_levelpathfolder.Get(),n)) == "\\" || cstr(Mid(g_mapfile_levelpathfolder.Get(),n)) == "/" ) 
		{
			g_mapfile_levelpathfolder = Left ( g_mapfile_levelpathfolder.Get(), n );
			bGotNestedPath = true;
			break;
		}
	}
	if ( bGotNestedPath==false )
	{
		// 240316 - V1.131v1 - if NO nested folder, string must be empty!
		g_mapfile_levelpathfolder = "";
	}

	//  Name without EXE
	t.exename_s=g.projectfilename_s;
	if (  cstr(Lower(Right(t.exename_s.Get(),4))) == ".fpm" ) 
	{
		t.exename_s=Left(t.exename_s.Get(),Len(t.exename_s.Get())-4);
	}
	for ( int n = Len(t.exename_s.Get()) ; n >= 1 ; n-- )
	{
		if (  cstr(Mid(t.exename_s.Get(),n)) == "\\" || cstr(Mid(t.exename_s.Get(),n)) == "/" ) 
		{
			t.exename_s=Right(t.exename_s.Get(),Len(t.exename_s.Get())-n);
			break;
		}
	}
	//PE: issue https://github.com/TheGameCreators/GameGuruRepo/issues/444
	if (  Len(t.exename_s.Get())<1  )  t.exename_s = "mylevel";

	//  the level to start off standalone export
	t.tmasterlevelfile_s=cstr("mapbank\\")+g_mapfile_levelpathfolder+t.exename_s+".fpm";

	if (g.bUseStoryBoardSetup)
	{
		//Use project name as exename
		if (strlen(Storyboard.gamename) > 0)
		{
			t.exename_s = Storyboard.gamename;
		}
	}

	timestampactivity(0,cstr(cstr("Saving standalone from ")+t.tmasterlevelfile_s).Get() );

	//  Create MYDOCS folder if not exist
	if ( PathExist(g.myownrootdir_s.Get()) == 0 ) file_createmydocsfolder ( );

	// Create absolute My Games folder (if not exist)
	if ( PathExist ( g.exedir_s.Get() ) == 0 )
	{
		g.exedir_s="?";
		SetDir ( g.myownrootdir_s.Get() );
		cstr mygamesfolder_s = "My Games";
		if ( PathExist(mygamesfolder_s.Get()) == 0 ) MakeDirectory ( mygamesfolder_s.Get() );
		if ( PathExist(mygamesfolder_s.Get()) == 1 ) 
		{
			SetDir ( mygamesfolder_s.Get() );
			g.exedir_s = GetDir();
		}
	}
	SetDir ( g_mapfile_told_s.Get() );

	// Path to EXE (for dealing with relative EXE paths later)
	if ( g.exedir_s.Get()[1] == ':' )
	{
		t.exepath_s = g.exedir_s;
	}
	else
	{
		t.exepath_s = g.exedir_s;
	}
	if ( cstr(Right(t.exepath_s.Get(),1)) != "\\"  ) t.exepath_s = t.exepath_s+"\\";

	// ensure filecollection array unmolesterd during level loads (some remote project code would have tried to reuse this array)
	g_bMakingAStandaloneUsingFileCollectionArray = true;

	// Collect all files and folders and store in t.filecollection_s
	mapfile_collectfoldersandfiles ( "" );

	// Pre-Stage 2 - clear a list which will collect all folders/files to REMOVE from the final standalone file transfer
	// list, courtesy of the special FPP file which controls the final files to be used for standalone creation
	g_mapfile_fppFoldersToRemoveList.clear();
	g_mapfile_fppFilesToRemoveList.clear();

	// process in stages
	g_mapfile_iStage = 1;
	g_mapfile_fProgress = 0.0f;
}

void mapfile_savestandalone_stage2a ( void )
{
	// Stage 2 - have all level from previous start step
	bool bWeUnloadedTheFirstLevel = false;
	t.levelindex = 1;
	if (g.bUseStoryBoardSetup)
	{
		// restore to first level.
		FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
		g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
		strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);
	}
	cstr tlevelfile_s = "";
	int tlevelstoprocess = 1;
	g_mapfile_iNumberOfEntitiesAcrossAllLevels = 0;
	while ( tlevelstoprocess == 1 ) 
	{
		if ( Len(tlevelfile_s.Get())>1 ) 
		{
			g.projectfilename_s = tlevelfile_s;
			mapfile_loadproject_fpm ( );
			game_loadinentitiesdatainlevel ( );
			bWeUnloadedTheFirstLevel = true;
			if (t.gamevisuals.bEnableEmptyLevelMode == false)
			{
				// if even one level does NOT use completely-empty mode, we have basic terrain requirements
				g_bUsingSomeKindOfTerrain = true;
			}
		}
		g_mapfile_iNumberOfEntitiesAcrossAllLevels += g.entityelementlist;
		cstr pLevelObjectsCountAllLevels = cstr("g_mapfile_iNumberOfEntitiesAcrossAllLevels==") + cstr(g_mapfile_iNumberOfEntitiesAcrossAllLevels);
		timestampactivity(0, pLevelObjectsCountAllLevels.Get());
		for ( int e = 1; e <= g.entityelementlist; ++e )
		{
			int entid = t.entityelement[e].bankindex;
			if ( entid > 0 ) 
			{
				// suspect old entID references, so produce a log if so
				if (entid >= (int)t.entityprofile.size())
				{
					// this is bad
					cstr pOldEntIDReference = cstr("ENTITY REF ERROR:") + cstr(e) + cstr(" uses ParentID of ") + cstr(entid) + cstr(" >= ") + cstr((int)t.entityprofile.size());
					timestampactivity(0, pOldEntIDReference.Get());
					continue;
				}

				// zone marker can reference other levels to jump to
				if ( t.entityprofile[entid].ismarker == 3 ) 
				{
					tlevelfile_s = t.entityelement[e].eleprof.ifused_s;
					if ( Len(tlevelfile_s.Get()) > 1 ) 
					{
						tlevelfile_s = cstr("mapbank\\") + g_mapfile_levelpathfolder + tlevelfile_s + ".fpm";
						timestampactivity(0, tlevelfile_s.Get());
						if ( FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + tlevelfile_s).Get()) == 1 )
						{
							++t.levelmax;
							t.levellist_s[t.levelmax] = tlevelfile_s;
							cstr pLogLevelAdded = cstr("added to list ") + cstr(t.levelmax);
							timestampactivity(0, pLogLevelAdded.Get());
						}
						else
							tlevelfile_s = "";
					}
				}
			}
		}
		if ( t.levelindex < t.levelmax ) 
		{
			tlevelfile_s = "";
			tlevelstoprocess = 0;
			while ( t.levelindex < t.levelmax && strcmp ( tlevelfile_s.Get(), "" ) == 0 ) 
			{
				++t.levelindex;
				cstr ttrylevelfile_s = t.levellist_s[t.levelindex];
				for ( int n = 1; n <= t.levelindex - 1; ++n )
				{
					if ( ttrylevelfile_s == t.levellist_s[n] ) 
					{
						ttrylevelfile_s = "";
						break;
					}
				}
				if ( ttrylevelfile_s != "" ) 
				{
					tlevelfile_s = ttrylevelfile_s;
					tlevelstoprocess = 1;
				}
			}
		}
		else
		{
			tlevelstoprocess = 0;
		}
	}
	g_mapfile_iNumberOfLevels = 1 + t.levelmax;
	cstr pLogNumberOfLevels = cstr("number of levels==") + cstr(g_mapfile_iNumberOfLevels);
	timestampactivity(0, pLogNumberOfLevels.Get());

	// Stage 2 - collect all files (from all levels)
	t.levelindex = 0;
	g_mapfile_tlevelfile_s = "";
	g_mapfile_tlevelstoprocess = 1;

	if (g.projectfilename_s != t.tmasterlevelfile_s)
		restore_old_map = true;
	g.projectfilename_s = t.tmasterlevelfile_s;
	if ( bWeUnloadedTheFirstLevel == true )
		g_mapfile_tlevelfile_s = t.tmasterlevelfile_s;
}

int mapfile_savestandalone_stage2b ( void )
{
	int iMoveAlong = 0;
	cstr pProgressMarkerLog = cstr("tlevelstoprocess:") + g_mapfile_tlevelstoprocess;
	timestampactivity(0, pProgressMarkerLog.Get());
	if ( g_mapfile_tlevelstoprocess == 1 )
	{
		// load in level FPM
		cstr pProgressLevelLog = cstr("tlevelfile:") + g_mapfile_tlevelfile_s;
		timestampactivity(0, pProgressLevelLog.Get());
		if ( Len(g_mapfile_tlevelfile_s.Get())>1 )
		{
			g.projectfilename_s = g_mapfile_tlevelfile_s;
			mapfile_loadproject_fpm ( );
			game_loadinentitiesdatainlevel ( );
			if (t.gamevisuals.bEnableEmptyLevelMode == false)
			{
				// if even one level does NOT use completely-empty mode, we have basic terrain requirements
				g_bUsingSomeKindOfTerrain = true;
			}
		}

		// 061018 - check if an FPP file exists for this level file
		cstr pFPPFile = cstr(Left(g.projectfilename_s.Get(), static_cast<int>(strlen(g.projectfilename_s.Get())) - 4)) + ".fpp";
		cstr pLogFPP = cstr("pFPPFile:") + pFPPFile;
		timestampactivity(0, pLogFPP.Get());
		if ( FileExist( pFPPFile.Get() ) == 1 )
		{
			// used to specify additional files required for standalone executable
			// handy as a workaround until reported issue resolved
			int iFPPStandaloneExtraFilesMode = 0;
			std::vector<cstr> data_s(1000);
			LoadArray ( pFPPFile.Get(), data_s );
			for ( size_t l = 0 ; l < data_s.size(); l++ )
			{
				cstr line_s = data_s[l];
				LPSTR pLine = line_s.Get();
				if ( Len(pLine) > 0 )
				{
					cstr pLofFPPLine = cstr("FPP:") + pLine;
					timestampactivity(0, pLofFPPLine.Get());
					if ( strnicmp ( pLine, "[standalone add files]", 22 ) == NULL )
					{
						// denotes our standalone extra files
						iFPPStandaloneExtraFilesMode = 1;
					}
					else
					{
						if ( strnicmp ( pLine, "[standalone delete files]", 25 ) == NULL )
						{
							// denotes our standalone remove files
							iFPPStandaloneExtraFilesMode = 2;
						}
						else
						{
							// this prevents newer FPP files from getting confused with this original simple method
							if ( iFPPStandaloneExtraFilesMode == 1 )
							{
								// add
								if ( pLine[strlen(pLine)-1] == '\\' )
								{
									// include whole folder
									addfoldertocollection(pLine);
								}
								else
								{
									// include specific file
									addtocollection(pLine);
								}
							}
							if ( iFPPStandaloneExtraFilesMode == 2 )
							{
								// remove
								if ( pLine[strlen(pLine)-1] == '\\' )
								{
									// remove whole folder
									g_mapfile_fppFoldersToRemoveList.push_back(cstr(pLine));
								}
								else
								{
									// remove specific file
									g_mapfile_fppFilesToRemoveList.push_back(cstr(pLine));
								}
							}
						}
					}
				}
			}
		}	

		//  chosen sky, terrain and veg
		cstr pSkyLog = cstr("adding skybank files:") + t.skybank_s[g.skyindex];
		timestampactivity(0, pSkyLog.Get());
		addfoldertocollection(cstr(cstr("skybank\\")+t.skybank_s[g.skyindex]).Get() );

		// pre-add the skins folder - can optimize later to find only skins we used (118MB)
		timestampactivity(0, "adding charactercreatorplus skin files");
		addfoldertocollection("charactercreatorplus\\skins");

		// start for loop
		g_mapfile_iEntityElementIndex = 1;
		cstr pProgressPercLog = cstr("g_mapfile_fProgressSpan:") + g_mapfile_fProgressSpan;
		timestampactivity(0, pProgressPercLog.Get());
		g_mapfile_fProgressSpan = (g_mapfile_iNumberOfEntitiesAcrossAllLevels > 0) ? static_cast<float>(g_mapfile_iNumberOfEntitiesAcrossAllLevels) : 1.0f;
	}
	else
	{
		iMoveAlong = 1;
	}
	return iMoveAlong;
}

void mapfile_addallentityrelatedfiles ( int entid, entityeleproftype* pEleProf )
{
	//PE: Add any custom explosions.
	if (pEleProf->explodable_decalname.Len() > 0)
	{
		char effectname[MAX_PATH];
		strcpy(effectname, "gamecore\\decals\\");
		strcat(effectname, pEleProf->explodable_decalname.Get());
		addfoldertocollection(effectname);
	}

	if (pEleProf->newparticle.emittername.Len() > 0)
	{
		char effectname[MAX_PATH];
		if (pEleProf->newparticle.emittername != "particlesbank/default")
		{
			AddWPETextures(pEleProf->newparticle.emittername.Get());
			//PE: Looks like some do not have .arx extension.
			strcpy(effectname, pEleProf->newparticle.emittername.Get());
			strcat(effectname, ".arx");
			AddWPETextures(effectname);
		}
	}

	// Check for custom images loaded in lua script
	if (pEleProf->aimain_s != "")
	{
		// Check for files required by the script via DLua
		extern void InitParseLuaScript(entityeleproftype * tmpeleprof);
		extern void ParseLuaScript(entityeleproftype * tmpeleprof, char* script);

		// PropertiesVariables are not filled automatically for t.entityelement[t.e], so create a temp variable and parse the script to fill the values
		// PropertiesVariables may eventually be saved with entityelement data, so we could remove this step in future
		entityeleproftype tempeleprof = *pEleProf;
		InitParseLuaScript(&tempeleprof);
		cstr script_name = "";
		script_name = "scriptbank\\";
		script_name += tempeleprof.aimain_s;
		ParseLuaScript(&tempeleprof, script_name.Get());

		// We now have the properties variables
		for (int i = 0; i < MAXPROPERTIESVARIABLES; i++)
		{
			// Check the lua variable is a string.
			//PE: We can now have variabletype == 7 that contain media.
			if (tempeleprof.PropertiesVariable.VariableType[i] == 2 || tempeleprof.PropertiesVariable.VariableType[i] == 7)
			{
				// CHeck if the string specifies a folder within the "imagebank\"
				int variableLength = static_cast<int>(strlen(tempeleprof.PropertiesVariable.VariableValue[i]));
				if (variableLength > 9 && (strnicmp(tempeleprof.PropertiesVariable.VariableValue[i], "imagebank", 9) == NULL) )
				{
					// and to avoid adding the whole imagebank, only add if a known 'grouped folder' used by specific behaviors such as imagepanel.lua
					LPSTR pRelativePathOfImageBankFolder = tempeleprof.PropertiesVariable.VariableValue[i];
					char pFinalFolder[MAX_PATH];
					for (int n = 1; n < 10; n++)
					{
						sprintf(pFinalFolder, "%s\\set%d\\", pRelativePathOfImageBankFolder, n);
						addfoldertocollection(pFinalFolder);
					}
					continue;
				}
				
				// Check if the string contains a file.
				if (variableLength > 4 && ( tempeleprof.PropertiesVariable.VariableValue[i][variableLength - 4] == '.' || tempeleprof.PropertiesVariable.VariableValue[i][variableLength - 3] == '.') )
				{
					// can specify a textfile, but needs to be specified as relative
					char cRel[MAX_PATH];
					LPSTR pStringOrFile = tempeleprof.PropertiesVariable.VariableValue[i];
					if (pStringOrFile[1] == ':')
					{
						// replace absolute paths with relative ones
						char pRelativePathAndFile[MAX_PATH];
						strcpy(pRelativePathAndFile, pStringOrFile);
						GG_GetRealPath(pRelativePathAndFile, 0);
						extern char szWriteDir[MAX_PATH];
						char pRemoveAbsPart[MAX_PATH];
						strcpy(pRemoveAbsPart, szWriteDir);
						strcat(pRemoveAbsPart, "Files\\");
						if (strnicmp(pRelativePathAndFile, pRemoveAbsPart, strlen(pRemoveAbsPart)) == NULL)
						{
							strcpy(pRelativePathAndFile, pStringOrFile + strlen(pRemoveAbsPart));
						}
						addtocollection(pRelativePathAndFile);
						strcpy(cRel, pRelativePathAndFile);
						if (pRelativePathAndFile[1] == ':')
						{
							//PE: If this is a projectfolder above dont work so.
							extern char szBeforeChangeWriteDir[MAX_PATH];
							strcpy(pRemoveAbsPart, szBeforeChangeWriteDir);
							strcat(pRemoveAbsPart, "Files\\");
							if (strnicmp(pRelativePathAndFile, pRemoveAbsPart, strlen(pRemoveAbsPart)) == NULL)
							{
								strcpy(pRelativePathAndFile, pStringOrFile + strlen(pRemoveAbsPart));
							}
							addtocollection(pRelativePathAndFile);
							strcpy(cRel, pRelativePathAndFile);
						}
					}
					else
					{
						addtocollection(pStringOrFile);
						strcpy(cRel, pStringOrFile);
					}

					//PE: Add WPE Textures.
					char cPE[MAX_PATH];
					strcpy(cPE, cRel);
					char* find = (char*)pestrcasestr(cPE, ".pe");
					if (!find) find = (char*)pestrcasestr(cPE, ".arx");;
					if (find)
					{
						AddWPETextures(cPE);
					}

					//PE: if .dds or.png also add - _normal and _emissive and _surface (behavior: Change Texture).
					if (pestrcasestr(tempeleprof.PropertiesVariable.VariableValue[i], ".dds") || pestrcasestr(tempeleprof.PropertiesVariable.VariableValue[i], ".png"))
					{
						if (pestrcasestr(tempeleprof.PropertiesVariable.VariableValue[i], "_color"))
						{
							std::string sParseName = tempeleprof.PropertiesVariable.VariableValue[i];
							replaceAll(sParseName, "_color.", "_normal.");
							addtocollection((char*)sParseName.c_str());
							replaceAll(sParseName, "_normal.", "_surface.");
							addtocollection((char*)sParseName.c_str());
							replaceAll(sParseName, "_surface.", "_emissive.");
							addtocollection((char*)sParseName.c_str());
							replaceAll(sParseName, "_emissive.", "_illumination.");
							addtocollection((char*)sParseName.c_str());
						}
					}
				}
			}
		}

		// Copy any files specified directly in Lua script
		cstr tLuaScript = g.fpscrootdir_s + "\\Files\\scriptbank\\";
		tLuaScript += pEleProf->aimain_s;
		FILE* tLuaScriptFile = GG_fopen (tLuaScript.Get(), "r");
		if (tLuaScriptFile)
		{
			char tTempLine[2048];
			while (!feof(tLuaScriptFile))
			{
				fgets (tTempLine, 2047, tLuaScriptFile);
				if (strstr (tTempLine, "LoadImages"))
				{
					char* pImageFolder = strstr (tTempLine, "\"");
					if (pImageFolder)
					{
						pImageFolder++;
						char* pImageFolderEnd = strstr (pImageFolder, "\"");
						if (pImageFolderEnd)
						{
							*pImageFolderEnd = '\0';
							cstr tFolderToAdd = cstr(cstr("scriptbank\\images\\") + cstr(pImageFolder));
							addfoldertocollection (tFolderToAdd.Get());
						}
					}
				}

				// Handle new load image and sound commands, they can be in nested folders
				if (strstr (tTempLine, "LoadImage ")
				|| strstr (tTempLine, "LoadImage(")
				|| strstr (tTempLine, "LoadGlobalSound ")
				|| strstr (tTempLine, "LoadGlobalSound(")
				|| strstr(tTempLine, "WParticleEffectLoad(")
				|| strstr(tTempLine, "WParticleEffectLoad "))
				{
					char* pImageFolder = strstr (tTempLine, "\"");
					if (pImageFolder)
					{
						pImageFolder++;
						char* pImageFolderEnd = strstr (pImageFolder, "\"");
						if (pImageFolderEnd)
						{
							*pImageFolderEnd = '\0';
							cstr pFile = cstr(pImageFolder);
							addtocollection (pFile.Get());

							//PE: Resolve wpe effects textures.
							char cPE[MAX_PATH];
							strcpy(cPE, pFile.Get());
							char* find = (char*)pestrcasestr(cPE, ".pe");
							if (!find) find = (char*)pestrcasestr(cPE, ".arx");;
							if(find)
							{
								AddWPETextures(cPE);
							}
						}
					}
				}
				if (strstr(tTempLine, "SetSkyTo(")) 
				{
					char* pSkyFolder = strstr(tTempLine, "\"");
					if (pSkyFolder)
					{
						pSkyFolder++;
						char* pSkyFolderEnd = strstr(pSkyFolder, "\"");
						if (pSkyFolderEnd)
						{
							*pSkyFolderEnd = '\0';
							cstr tFolderToAdd = cstr(cstr("skybank\\") + cstr(pSkyFolder));
							addfoldertocollection(tFolderToAdd.Get());
						}
					}
				}
			}
			fclose (tLuaScriptFile);
		}
	}

	// gives "entid" and adds ALL entity profile related files to the collection (not pEleProf specific)
	addthisentityprofilesfilestocollection(pEleProf, entid);

	// shader file
	cstr effectFile = pEleProf->effect_s;
	addtocollection(effectFile.Get());
	//Try to take the .blob.
	if (cstr(Lower(Right(effectFile.Get(), 3))) == ".fx") 
	{
		cstr blobFile = cstr(Left(effectFile.Get(), Len(effectFile.Get()) - 3)) + ".blob";
		if (FileExist(blobFile.Get()) == 1)
		{
			addtocollection(blobFile.Get());
		}
	}

	// script files
	cstr script_name = cstr("scriptbank\\") + pEleProf->aimain_s;
	addtocollection(script_name.Get());
	// Copy .byc too
	std::string sLuaFile = script_name.Get();
	replaceAll(sLuaFile, ".lua", ".byc");
	addtocollection((char*)sLuaFile.c_str());

	// for the script associated, scan it and include any references to other scripts
	scanscriptfileandaddtocollection(script_name.Get());

	// sound files
	if (t.entityprofile[entid].ismarker == 1 && pEleProf->soundset_s.Len() > 0) 
	{
		//PE: Make sure voiceset from player start marker is added.
		addfoldertocollection(cstr(cstr("audiobank\\voices\\") + pEleProf->soundset_s).Get());
	}
	addtocollection(pEleProf->soundset_s.Get());
	addtocollection(pEleProf->soundset1_s.Get());
	addtocollection(pEleProf->soundset2_s.Get());
	addtocollection(pEleProf->soundset3_s.Get());
	addtocollection(pEleProf->soundset4a_s.Get());
	addtocollection(pEleProf->soundset5_s.Get());
	addtocollection(pEleProf->soundset6_s.Get());
	addtocollection(pEleProf->overrideanimset_s.Get());

	// lipsync files associated with soundset references
	auto addLipIfValid = [](const cstr& soundset) {
		if (soundset.Len() > 4) {
			std::string lip(soundset.Get(), soundset.Len() - 4);
			lip += ".lip";
			addtocollection(lip.c_str());
		}
	};
	addLipIfValid(pEleProf->soundset_s);
	addLipIfValid(pEleProf->soundset1_s);
	addLipIfValid(pEleProf->soundset2_s);
	addLipIfValid(pEleProf->soundset3_s);
	addLipIfValid(pEleProf->soundset4a_s);
	addLipIfValid(pEleProf->soundset5_s);
	addLipIfValid(pEleProf->soundset6_s);

	// collectable guns
	cstr pGunPresent = "";
	if (Len(t.entityprofile[entid].isweapon_s.Get()) > 1) pGunPresent = t.entityprofile[entid].isweapon_s;
	if (t.entityprofile[entid].isammo == 0)
	{
		// 270618 - only accept HASWEAPON if NOT ammo, so executables are not bloated with ammo that specifies another weapon type
		if (Len(pEleProf->hasweapon_s.Get()) > 1) pGunPresent = pEleProf->hasweapon_s;
	}
	if (Len(pGunPresent.Get()) > 1)
	{
		addfoldertocollection(cstr(cstr("gamecore\\guns\\") + pGunPresent).Get());
		t.findgun_s = Lower(pGunPresent.Get());
		gun_findweaponindexbyname ();
		if (t.foundgunid > 0)
		{
			// ammo and brass
			for (int x = 0; x <= 1; x++)
			{
				// ammo files
				int tpoolindex = g.firemodes[t.foundgunid][x].settings.poolindex;
				if (tpoolindex > 0)
				{
					cstr poolFile = cstr("gamecore\\ammo\\") + t.ammopool[tpoolindex].name_s;
					if (PathExist (poolFile.Get())) addfoldertocollection(poolFile.Get());
				}

				// brass files
				int iBrassIndex = g.firemodes[t.foundgunid][x].settings.brass;
				if (iBrassIndex > 0)
				{
					cstr brassFile = cstr(cstr("gamecore\\brass\\brass") + Str(iBrassIndex));
					if (PathExist (brassFile.Get()))
						addfoldertocollection(brassFile.Get());
				}

				// and any projectile files associated with it
				cstr pProjectilePresent = t.gun[t.foundgunid].projectile_s;
				if (Len(pProjectilePresent.Get()) > 1)
				{
					cstr projFile = cstr("gamecore\\projectiletypes\\") + pProjectilePresent;
					addfoldertocollection(projFile.Get());
				}
			}

			// and any projectile files associated with it
			cstr pProjectilePresent = t.gun[t.foundgunid].projectile_s;
			if (Len(pProjectilePresent.Get()) > 1)
			{
				cstr projFile = cstr("gamecore\\projectiletypes\\") + pProjectilePresent;
				addfoldertocollection(projFile.Get());
			}
		}
	}

	// player start marler
	if (t.entityprofile[entid].ismarker == 1)
	{
		// can specify custom arms for weapons, need the hands
		if (pEleProf->texaltd_s.Len() > 0)
		{
			cstr handsFile = cstr("gamecore\\hands\\") + pEleProf->texaltd_s;
			addfoldertocollection(handsFile.Get());
		}
	}

	// zone marker can reference other levels to jump to
	if (t.entityprofile[entid].ismarker == 3)
	{
		cstr tlevelfile_s = pEleProf->ifused_s;
		if (Len(tlevelfile_s.Get()) > 1)
		{
			tlevelfile_s = cstr("mapbank\\") + g_mapfile_levelpathfolder + tlevelfile_s + ".fpm";
			if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + tlevelfile_s).Get()) == 1)
			{
				addtocollection(tlevelfile_s.Get());
			}
		}
	}
}

void mapfile_copyallfilecollectiontopreferredprojectfolder(void)
{
	// before the copy, some files should never move across (core scripts, etc)
	removefromcollection ("scriptbank\\global.lua");
	removefromcollection ("scriptbank\\gameloop.lua");
	removefromcollection ("scriptbank\\masterinterpreter.lua");
	removefromcollection ("scriptbank\\physlib.lua");
	removefromcollection ("scriptbank\\quatlib.lua");
	removefromcollection ("scriptbank\\utillib.lua");
	removefromcollection ("scriptbank\\vectlib.lua");
	removefromcollection ("scriptbank\\hud0.lua");
	removefromcollection ("scriptbank\\module_activationcontrol.lua");
	removefromcollection ("scriptbank\\module_misclib.lua");
	removefromcollection ("scriptbank\\navmeshlib.lua");
	removefromcollection ("scriptbank\\perlin_noise.lua");
	removefromcollection ("scriptbank\\ai\\module_cameraoverride.lua");
	removefromcollection ("scriptbank\\no_behavior_selected.lua");
	
	// do the copy
	char pPreferredProjectEntityFolder[MAX_PATH];
	strcpy(pPreferredProjectEntityFolder, Storyboard.customprojectfolder);
	strcat(pPreferredProjectEntityFolder, Storyboard.gamename);
	for (int files = 1; files <= g.filecollectionmax; files++)
	{
		char pFileToCopy[MAX_PATH];
		strcpy(pFileToCopy, t.filecollection_s[files].Get());
		GG_GetRealPath(pFileToCopy, 0);
		if (strnicmp (pFileToCopy, pPreferredProjectEntityFolder, strlen(pPreferredProjectEntityFolder)) != NULL)
		{
			char pFileToCopyTo[MAX_PATH];
			strcpy(pFileToCopyTo, t.filecollection_s[files].Get());
			GG_GetRealPath(pFileToCopyTo, 1);
			CopyFileA(pFileToCopy, pFileToCopyTo, FALSE);
		}
	}
}

void mapfile_ensurethisfolderexistsinremoteproject(LPSTR pFolderToCopy)
{
	//PE: Bug fix , in standalone skybox got copied to docwrite folder with _e_.
	if (t.game.gameisexe == 1) return;
	extern bool g_bMakingAStandaloneUsingFileCollectionArray;
	if (g_bMakingAStandaloneUsingFileCollectionArray == false)
	{
		// clear filecollection, specify new folder, copy all to preferred project folder
		g.filecollectionmax = 0;
		Undim (t.filecollection_s);
		Dim (t.filecollection_s, 500);
		addfoldertocollection (pFolderToCopy);
		extern void mapfile_copyallfilecollectiontopreferredprojectfolder (void);
		mapfile_copyallfilecollectiontopreferredprojectfolder();
	}
}

int mapfile_savestandalone_stage2c ( void )
{
	// choose all entities and associated files
	int iMoveAlong = 0;
	if ( g_mapfile_iEntityElementIndex <= g.entityelementlist )
	{
		int entid = t.entityelement[g_mapfile_iEntityElementIndex].bankindex;
		if ( entid > 0 ) 
		{
			// most eleprof related files
			mapfile_addallentityrelatedfiles(entid, &t.entityelement[g_mapfile_iEntityElementIndex].eleprof);

			// and purey entity element related files
			if (!t.entityelement[g_mapfile_iEntityElementIndex].eleprof.bUseFPESettings)
			{
				// Also add any custom material textures
				sObject* pObject = GetObjectData(t.entityelement[g_mapfile_iEntityElementIndex].obj);
				if (pObject)
				{
					for (int i = 0; i < pObject->iFrameCount; i++)
					{
						sFrame* pFrame = pObject->ppFrameList[i];
						if (pFrame)
						{
							sMesh* pMesh = pFrame->pMesh;
							if (pMesh)
							{
								wiScene::MaterialComponent* pMaterialComponent = wiScene::GetScene().materials.GetComponent((wiECS::Entity)pMesh->wickedmaterialindex);
								if (pMaterialComponent)
								{
									if (pMaterialComponent->textures[0].name.length() > 0)
									{
										addtocollection((char*)pMaterialComponent->textures[0].name.c_str());
										addtocollection((char*)pMaterialComponent->textures[1].name.c_str());
										addtocollection((char*)pMaterialComponent->textures[2].name.c_str());
										addtocollection((char*)pMaterialComponent->textures[3].name.c_str());
									}
								}
							}
						}
					}
				}
				//PE: And add any custom wematerial texture settings.
				if (t.entityelement[g_mapfile_iEntityElementIndex].eleprof.WEMaterial.MaterialActive)
				{
					//PE: Need relative path here.
					cstr entpath = cstr("entitybank\\") + t.entitybank_s[entid];
					for (int n = Len(entpath.Get()); n >= 1; n--)
					{
						if (cstr(Mid(entpath.Get(), n)) == "\\" || cstr(Mid(entpath.Get(), n)) == "/")
						{
							entpath = Left(entpath.Get(), n);
							break;
						}
					}

					cstr texture;
					for (int loop = 0; loop < MAXMESHMATERIALS; loop++)
					{
						for (int l = 0; l < 4; l++)
						{
							if (l == 0) texture = t.entityelement[g_mapfile_iEntityElementIndex].eleprof.WEMaterial.baseColorMapName[loop];
							if (l == 1) texture = t.entityelement[g_mapfile_iEntityElementIndex].eleprof.WEMaterial.normalMapName[loop];
							if (l == 2) texture = t.entityelement[g_mapfile_iEntityElementIndex].eleprof.WEMaterial.surfaceMapName[loop];
							if (l == 3) texture = t.entityelement[g_mapfile_iEntityElementIndex].eleprof.WEMaterial.emissiveMapName[loop];

							if (texture.Len() > 0)
							{
								if (!pestrcasestr(texture.Get(), "\\") &&
									!pestrcasestr(texture.Get(), "/"))
								{
									cstr finalname = entpath + texture;
									addtocollection((char*)finalname.Get());
								}
								else
									addtocollection((char*)texture.Get());
							}
						}
					}
				}
			}
		}
	}
	else
	{
		if (t.visuals.customTexturesFolder.Len() > 0)
		{
			// Collect all .dds textures in this folder and add them to the standalone file collection
			char customTexturePath[MAX_PATH];
			strcpy(customTexturePath, GG_GetWritePath());
			strcat(customTexturePath, t.visuals.customTexturesFolder.Get());
			std::vector<std::string> filesToCollect;
			CollectFilesWithExtension(".dds", customTexturePath, &filesToCollect);
			for (auto& file : filesToCollect)
			{
				const char* filename = strstr(file.c_str(), "Files\\");
				if (filename)
				{
					const char* finalName = filename + 6;
					addtocollection((char*)finalName);
				}
			}
			// TODO: If we ever release DLC with terrain textures stored in root dir, we would need to do an additional scan here.
		}
		iMoveAlong = 1;
	}
	g_mapfile_iEntityElementIndex++;
	return iMoveAlong;
}

void mapfile_savestandalone_stage2d ( void )
{
	// decide if another level needs loading/processing
	if ( t.levelindex < t.levelmax ) 
	{
		g_mapfile_tlevelfile_s = "";
		g_mapfile_tlevelstoprocess = 0;
		while ( t.levelindex < t.levelmax && strcmp ( g_mapfile_tlevelfile_s.Get(), "" ) == 0 ) 
		{
			++t.levelindex;
			cstr ttrylevelfile_s = t.levellist_s[t.levelindex];
			for ( int n = 1; n <= t.levelindex-1; n++ )
			{
				if ( ttrylevelfile_s == t.levellist_s[n] ) 
				{
					ttrylevelfile_s = "";
					break;
				}
			}
			if ( ttrylevelfile_s != "" ) 
			{
				g_mapfile_tlevelfile_s = ttrylevelfile_s;
				g_mapfile_tlevelstoprocess = 1;
			}
		}
	}
	else
	{
		g_mapfile_tlevelstoprocess = 0;
	}
}

void mapfile_savestandalone_stage2e ( void )
{
	//  if multi-level, do NOT include the levelbank\testmap temp files
	g_mapfile_tignorelevelbankfiles = 0;
	if (  g.projectfilename_s != t.tmasterlevelfile_s ) 
	{
		timestampactivity(0,"Ignoring levelbank testmap folder for multilevel standalone");
		g_mapfile_tignorelevelbankfiles = 1;
	}
	else
	{
		addtocollection("levelbank\\testmap\\header.dat");
	}
}

void mapfile_savestandalone_stage3 ( void )
{
	// if any of the levels used terrain, add the terrain/tree/grass folders
	if (g_bUsingSomeKindOfTerrain == true)
	{
		addfoldertocollection("treebank"); // all but completely empty level (basic flat terrain)
		addfoldertocollection("treebank\\billboards");
		addfoldertocollection("treebank\\textures");
		if (strlen(Storyboard.customprojectfolder) > 0)
		{
			// if remote project being used, it will copy its OWN terraintexture folder over
			// so no need to include the default in case all custom textures used
		}
		else
		{
			//PE: Storyboard - Need standalone / lua menu's working.
			addfoldertocollection("terraintextures");
			for (int i = 0; i < 42; i++)
			{
				char addfolder[MAX_PATH];
				sprintf(addfolder, "terraintextures\\mat%d", i);
				addfoldertocollection(addfolder);
			}
		}
		addfoldertocollection("grassbank");
	}

	//  Create game folder
	SetDir (  t.exepath_s.Get() );
	MakeDirectory (  t.exename_s.Get() );
	SetDir (  t.exename_s.Get() );
	MakeDirectory (  "Files" );
	SetDir (  "Files" );

	//  Ensure gamesaves files are removed (if any)
	if (  PathExist("gamesaves") == 1 ) 
	{
		SetDir (  "gamesaves" );
		ChecklistForFiles (  );
		for ( int c = 1 ; c <= ChecklistQuantity(); c++ )
		{
			cstr tfile_s = ChecklistString(c);
			if (  Len(tfile_s.Get()) > 2 ) 
			{
				if (  FileExist(tfile_s.Get()) == 1  )  DeleteAFile (  tfile_s.Get() );
			}
		}
		SetDir (  ".." );
	}

	//PE: Ensure old navmesh cache is deleted.
	if (PathExist("navbank") == 1)
	{
		SetDir("navbank");
		ChecklistForFiles();
		for (int c = 1; c <= ChecklistQuantity(); c++)
		{
			cstr tfile_s = ChecklistString(c);
			if (Len(tfile_s.Get()) > 2)
			{
				if (FileExist(tfile_s.Get()) == 1)
				{
					DeleteAFile(tfile_s.Get());
				}
			}
		}
		SetDir("..");
	}

	//  Ensure file path exists (by creating folders)
	createallfoldersincollection();

	// If not copying levelbank files, must still create the folder
	if ( g_mapfile_tignorelevelbankfiles == 1 ) 
	{
		cstr olddir_s = GetDir();
		SetDir (  cstr(t.exepath_s+t.exename_s+"\\Files").Get() );
		if (  PathExist("levelbank") == 0  )  MakeDirectory (  "levelbank" );
		SetDir (  "levelbank" );
		if (  PathExist("testmap") == 0  )  MakeDirectory (  "testmap" );
		SetDir (  "testmap" );
		if (  PathExist("lightmaps") == 0  )  MakeDirectory (  "lightmaps" );
		if (  PathExist("ttsfiles") == 0  )  MakeDirectory (  "ttsfiles" );
		SetDir (  olddir_s.Get() );
	}

	// If existing standalone there, ensure lightmaps are removed (as they will be unintentionally encrypted)
	cstr destpath_s = t.exepath_s+t.exename_s+"\\Files\\levelbank\\testmap\\lightmaps";
	if (  PathExist(destpath_s.Get()) == 1 ) 
	{
		cstr olddir_s = GetDir();
		SetDir (  destpath_s.Get() );
		ChecklistForFiles (  );
		for ( int c = 1 ; c <= ChecklistQuantity(); c++ )
		{
			cstr tfile_s = ChecklistString(c);
			if (  tfile_s != "." && tfile_s != ".." ) 
			{
				if (  FileExist(tfile_s.Get()) == 1  )  DeleteAFile (  tfile_s.Get() );
			}
		}
		SetDir (  olddir_s.Get() );
	}

	// If existing standalone there, ensure ttsfiles are removed
	destpath_s = t.exepath_s+t.exename_s+"\\Files\\levelbank\\testmap\\ttsfiles";
	if ( PathExist(destpath_s.Get()) == 1 ) 
	{
		cstr olddir_s = GetDir();
		SetDir ( destpath_s.Get() );
		ChecklistForFiles ( );
		for ( int c = 1 ; c <= ChecklistQuantity(); c++ )
		{
			cstr tfile_s = ChecklistString(c);
			if ( tfile_s != "." && tfile_s != ".." ) 
			{
				if ( FileExist(tfile_s.Get()) == 1  ) DeleteAFile ( tfile_s.Get() );
			}
		}
		SetDir (  olddir_s.Get() );
	}

	// 010917 - go through and remove any X files that have DBO counterparts
	SetDir ( cstr(g.fpscrootdir_s+"\\Files\\").Get() );
	for ( int fileindex = 1 ; fileindex <= g.filecollectionmax; fileindex++ )
	{
		cstr src_s = t.filecollection_s[fileindex];
		if ( FileExist(src_s.Get()) == 1 ) 
		{
			char pSrcFile[1024];
			strcpy ( pSrcFile, src_s.Get() );
			if ( strnicmp ( pSrcFile + strlen(pSrcFile) - 4, ".dbo", 4 ) == NULL )
			{
				cstr dboequiv = cstr(Left(pSrcFile, static_cast<int>(strlen(pSrcFile)) - 4)) + ".x";
				if ( FileExist(dboequiv.Get()) == 1 ) 
				{
					// Found DBO, and an X file sitting alongside it, remove the X from consideration
					removefromcollection ( dboequiv.Get() );
				}
			}
		}
	}

	// also remove folders/files marked by FPP file
	if ( g_mapfile_fppFoldersToRemoveList.size() > 0 || g_mapfile_fppFilesToRemoveList.size() > 0 )
	{
		for ( int n = 0; n < g_mapfile_fppFoldersToRemoveList.size(); n++ )
		{
			cstr pRemoveFolder = g_mapfile_fppFoldersToRemoveList[n];
			removeanymatchingfromcollection ( pRemoveFolder.Get() );
		}
		for ( int n = 0; n < g_mapfile_fppFilesToRemoveList.size(); n++ )
		{
			cstr pRemoveFile = g_mapfile_fppFilesToRemoveList[n];
			removeanymatchingfromcollection ( pRemoveFile.Get() );
		}
	}
}

void removeEmptyFolders(const std::string& directoryPath)
{
	std::string searchPath = directoryPath + "\\*";

	WIN32_FIND_DATAA findFileData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		return;
	}
	std::vector<std::string> subdirectories;

	do
	{
		if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
		{
			continue;
		}
		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			std::string subDirPath = directoryPath + "\\" + findFileData.cFileName;
			subdirectories.push_back(subDirPath);
			removeEmptyFolders(subDirPath);
		}
	} while (FindNextFileA(hFind, &findFileData) != 0);

	FindClose(hFind);

	for (const auto& subDir : subdirectories)
	{
		removeEmptyFolders(subDir);
	}

	//PE: Try removing current folder , if its empty it will work.
	RemoveDirectoryA(directoryPath.c_str());
}

void mapfile_savestandalone_stage4 ( void )
{
	// restore dir before proceeding
	SetDir(g_mapfile_told_s.Get());

	//  CopyAFile (  collection to exe folder )
	int filesmax = g.filecollectionmax;
	for ( int fileindex = 1 ; fileindex <= filesmax; fileindex++ )
	{
		cstr src_s = t.filecollection_s[fileindex];
		char pRealSrc[MAX_PATH];
		strcpy(pRealSrc, src_s.Get());
		GG_GetRealPath(pRealSrc, 0);
		if (FileExist(pRealSrc) == 1)
		{
			cstr dest_s = t.exepath_s + t.exename_s + "\\Files\\" + src_s;
			if (FileExist(dest_s.Get()) == 1) DeleteAFile(dest_s.Get());
			CopyAFile(pRealSrc, dest_s.Get());
		}
	}

	//PE: Make sure needed folders exists.
	cstr particlesFolder = t.exepath_s + t.exename_s + "\\Files\\particlesbank";
	if (PathExist(particlesFolder.Get()) == 0) MakeDirectory(particlesFolder.Get());
	cstr particlesUserFolder = t.exepath_s + t.exename_s + "\\Files\\particlesbank\\user";
	if (PathExist(particlesUserFolder.Get()) == 0) MakeDirectory(particlesUserFolder.Get());


	//PE: Make sure we dont have a empty folder inside scriptbank.
	//PE: We do have needed empty folder so cant just run it on everything like 'levelbank' ...
	cstr scriptbankFolder = t.exepath_s + t.exename_s + "\\Files\\scriptbank";
	if (PathExist(scriptbankFolder.Get()))
	{
		//PE: Check if we have any empty folders here.
		removeEmptyFolders(scriptbankFolder.Get());
	}

	// switch to original root to copy exe files and dependencies
	SetDir ( g.originalrootdir_s.Get() );

	//  Copy game engine and rename it
	cstr exeDest_s = t.exepath_s + t.exename_s + "\\" + t.exename_s + ".exe";
	if ( FileExist(exeDest_s.Get()) == 1 ) DeleteAFile (  exeDest_s.Get() );
	if ( FileExist ( "GameGuruMAX.exe") == 1)
		CopyAFile ( "GameGuruMAX.exe", exeDest_s.Get());
	else
		CopyAFile ( "Guru-MapEditor.exe", exeDest_s.Get() );

	// and change the icon using project icons if exists
	char projectico[MAX_PATH];
	char projectfinal_ico[MAX_PATH];
	strcpy(projectico, "projectbank\\");
	strcat(projectico, Storyboard.gamename);
	strcpy(projectfinal_ico, "Files\\");
	strcat(projectfinal_ico, projectico);
	strcat(projectfinal_ico, "\\project256.ico");
	GG_GetRealPath(projectfinal_ico, 1);
	if (FileExist(projectfinal_ico)==1)
	{
		void InjectIconToExe(char* icon, char* exe, int intresourcenumber);
		InjectIconToExe(projectfinal_ico, exeDest_s.Get(), 1);
	}

	// AssImp DLL tied to executable, may also need it for non-DBO model loading?!
	char pCritDLLFilename[MAX_PATH];
	strcpy(pCritDLLFilename, "assimp.dll");
	cstr dllDest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if ( FileExist(dllDest_s.Get()) == 1 ) DeleteAFile ( dllDest_s.Get() );
	CopyAFile ( pCritDLLFilename, dllDest_s.Get() );

	// Steam DLL now required for authentication step (not needed for standalone game running)
	strcpy(pCritDLLFilename, "steam_api64.dll");
	dllDest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(dllDest_s.Get()) == 1) DeleteAFile (dllDest_s.Get());
	CopyAFile (pCritDLLFilename, dllDest_s.Get());

	// EPIC DLL required for authentication step (not needed for standalone game running)
	strcpy(pCritDLLFilename, "EOSSDK-Win64-Shipping.dll");
	dllDest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(dllDest_s.Get()) == 1) DeleteAFile (dllDest_s.Get());
	CopyAFile (pCritDLLFilename, dllDest_s.Get());	

	// Users report that people who don't have Max installed cannot play standalones
	strcpy(pCritDLLFilename, "dxil.dll");
	dllDest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(dllDest_s.Get()) == 1) DeleteAFile(dllDest_s.Get());
	CopyAFile(pCritDLLFilename, dllDest_s.Get());
	strcpy(pCritDLLFilename, "dxcompiler.dll");
	dllDest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(dllDest_s.Get()) == 1) DeleteAFile(dllDest_s.Get());
	CopyAFile(pCritDLLFilename, dllDest_s.Get());
	strcpy(pCritDLLFilename, "d3dcompiler_47.dll");
	dllDest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(dllDest_s.Get()) == 1) DeleteAFile(dllDest_s.Get());
	CopyAFile(pCritDLLFilename, dllDest_s.Get());

	// if exist, copy the OptickCore.dll so we can performance tune!
	strcpy(pCritDLLFilename, "OptickCore.dll");
	dllDest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(pCritDLLFilename) == 1) CopyAFile(pCritDLLFilename, dllDest_s.Get());

	// if exist, copy the WinPixEventRuntime.dll so we can performance tune!
	strcpy(pCritDLLFilename, "WinPixEventRuntime.dll");
	dllDest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(pCritDLLFilename) == 1) CopyAFile(pCritDLLFilename, dllDest_s.Get());

	// copy the version.ini file over (helps with crash triage system)
	strcpy(pCritDLLFilename, "version.ini");
	dllDest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(pCritDLLFilename) == 1) CopyAFile(pCritDLLFilename, dllDest_s.Get());

	// for wicked, create fonts and shaders folder
	cstr destExeRoot_s = t.exepath_s + t.exename_s;
	//SetDir ( destExeRoot_s.Get() );
	//if (PathExist("fonts") == 0) MakeDirectory("fonts");
	SetDir ( destExeRoot_s.Get() );
	if (PathExist("shaders") == 0) MakeDirectory("shaders");

	// for wicked, copy shaders folder
	SetDir ( g.originalrootdir_s.Get() );
	SetDir("shaders");
	ChecklistForFiles();
	for ( int c = 1; c <= ChecklistQuantity(); c++ )
	{
		LPSTR pShaderFile = ChecklistString(c);
		if (stricmp(pShaderFile, ".") != NULL && stricmp(pShaderFile, "..") != NULL)
		{
			cstr destShaderFile = t.exepath_s + t.exename_s + "\\shaders\\" + pShaderFile;
			if (FileExist(destShaderFile.Get()) == 1) DeleteAFile(destShaderFile.Get());
			CopyAFile(pShaderFile, destShaderFile.Get());
		}
	}

	// restore to original folder 
	SetDir ( g.originalrootdir_s.Get() );

	// Copy steam files (see above)
	 // No Steam in Photon build

	// copy visuals settings file
	t.visuals=t.gamevisuals ; visuals_save ( );
	
	// if visuals exists, switch to root folder and save to executable folder
	if ( FileExist("visuals.ini") == 1 ) 
	{
		SetDir (  g.fpscrootdir_s.Get() ); // odd this, but already set dir to root further up!
		char pSrcVisFile[MAX_PATH];
		strcpy(pSrcVisFile, "visuals.ini");
		cstr destVisFile = t.exepath_s + t.exename_s + "\\visuals.ini";

		char pRealVisFile[MAX_PATH];
		strcpy(pRealVisFile, destVisFile.Get());
		GG_GetRealPath(pRealVisFile, 1);
		destVisFile = pRealVisFile;
		char pRealSrcVisFile[MAX_PATH];
		strcpy(pRealSrcVisFile, pSrcVisFile);
		GG_GetRealPath(pRealSrcVisFile, 1);
		strcpy(pSrcVisFile, pRealSrcVisFile);

		if ( FileExist(destVisFile.Get()) == 1 ) DeleteAFile ( destVisFile.Get() );
		CopyAFile ( pSrcVisFile, destVisFile.Get() );
	}
	t.visuals=t.editorvisuals  ; visuals_save ( );

	//  Create a setup.ini file here reflecting game
	std::vector<cstr> setuparr_s;
	setuparr_s.reserve(200);
	cstr setupfile_s = t.exepath_s + t.exename_s + "\\setup.ini";
	setuparr_s.push_back("[GAMERUN]");
	setuparr_s.push_back(cstr("vsync=") + Str(g.gvsync));
	setuparr_s.push_back(cstr("superflatterrain=") + Str(t.terrain.superflat));
	setuparr_s.push_back(cstr("smoothcamerakeys=") + Str(g.globals.smoothcamerakeys));
	setuparr_s.push_back(cstr("occlusionmode=") + Str(g.globals.occlusionmode));
	setuparr_s.push_back(cstr("occlusionsize=") + Str(g.globals.occlusionsize));
	if ( g.vrqcontrolmode != 0 )
	{
		setuparr_s.push_back("hidelowfpswarning=1");
	}
	else
	{
		setuparr_s.push_back(cstr("hidelowfpswarning=") + Str(g.globals.hidelowfpswarning));
	}
	setuparr_s.push_back(cstr("hardwareinfomode=") + Str(g.ghardwareinfomode));
	setuparr_s.push_back(cstr("fullscreen=") + Str(g.gfullscreen));
	setuparr_s.push_back(cstr("aspectratio=") + Str(g.gaspectratio));
	setuparr_s.push_back(cstr("dividetexturesize=") + Str(g.gdividetexturesize));
	setuparr_s.push_back("producelogfiles=0");
	setuparr_s.push_back(cstr("adapterordinal=") + Str(g.gadapterordinal));
	setuparr_s.push_back(cstr("adapterd3d11only=") + Str(g.gadapterd3d11only));
	setuparr_s.push_back("hidedistantshadows=0");
	setuparr_s.push_back(cstr("invmouse=") + Str(g.gminvert));
	setuparr_s.push_back(cstr("disablerightmousehold=") + Str(g.gdisablerightmousehold));
	setuparr_s.push_back("profileinstandalone=0");
	setuparr_s.push_back(cstr("allowfragmentation=") + Str(t.game.allowfragmentation));
	setuparr_s.push_back(cstr("pbroverride=") + Str(g.gpbroverride));

	setuparr_s.push_back(cstr("memskipibr=") + Str(g.memskipibr));
	setuparr_s.push_back(cstr("underwatermode=") + Str(g.underwatermode));
	setuparr_s.push_back(cstr("usegrassbelowwater=") + Str(g.usegrassbelowwater));

	setuparr_s.push_back(cstr("memskipwatermask=") + Str(g.memskipwatermask));
	setuparr_s.push_back(cstr("lowestnearcamera=") + Str(g.lowestnearcamera));
	setuparr_s.push_back(cstr("standalonefreememorybetweenlevels=") + Str(g.standalonefreememorybetweenlevels));
	setuparr_s.push_back(cstr("videoprecacheframes=") + Str(g.videoprecacheframes));
	setuparr_s.push_back(cstr("aidisabletreeobstacles=") + Str(g.aidisabletreeobstacles));
	setuparr_s.push_back(cstr("aidisableobstacles=") + Str(g.aidisableobstacles));
	setuparr_s.push_back(cstr("skipunusedtextures=") + Str(g.skipunusedtextures));
	setuparr_s.push_back(cstr("videodelayedload=") + Str(g.videodelayedload));

	setuparr_s.push_back(cstr("maxtotalmeshlights=") + Str(g.maxtotalmeshlights));
	setuparr_s.push_back(cstr("maxpixelmeshlights=") + Str(g.maxpixelmeshlights));
	setuparr_s.push_back(cstr("terrainoldlight=") + Str(g.terrainoldlight));
	setuparr_s.push_back(cstr("terrainusevertexlights=") + Str(g.terrainusevertexlights));
	setuparr_s.push_back(cstr("maxterrainlights=") + Str(g.maxterrainlights));
	setuparr_s.push_back(cstr("terrainlightfadedistance=") + Str(g.terrainlightfadedistance));
	setuparr_s.push_back(cstr("showstaticlightinrealtime=") + Str(g.showstaticlightinrealtime));
	setuparr_s.push_back(cstr("drawcalloptimizer=") + Str(g.globals.drawcalloptimizer));
	setuparr_s.push_back(cstr("forcenowaterreflection=") + Str(g.globals.forcenowaterreflection));

	if ( t.DisableDynamicRes == false )
	{
		setuparr_s.push_back("disabledynamicres=0");
	}
	else
	{
		setuparr_s.push_back("disabledynamicres=1");
	}

	setuparr_s.push_back(cstr("curvedistancescaler=") + Str(g.globals.CurveDistanceScaler));

	setuparr_s.push_back(cstr("realshadowdistance=") + Str(g.globals.realshadowdistancehigh));
	setuparr_s.push_back(cstr("realshadowresolution=") + Str(g.globals.realshadowresolution));
	setuparr_s.push_back(cstr("realshadowcascadecount=") + Str(g.globals.realshadowcascadecount));
	setuparr_s.push_back(cstr("flashlightshadows=") + Str(g.globals.flashlightshadows));
	
	setuparr_s.push_back(cstr("realshadowcascade0=") + Str(g.globals.realshadowcascade[0]));
	if (g.globals.realshadowsize[0] > 0) {
		setuparr_s.push_back(cstr("realshadowsize0=") + Str(g.globals.realshadowsize[0]));
	}
	setuparr_s.push_back(cstr("realshadowcascade1=") + Str(g.globals.realshadowcascade[1]));
	if (g.globals.realshadowsize[1] > 0) {
		setuparr_s.push_back(cstr("realshadowsize1=") + Str(g.globals.realshadowsize[1]));
	}
	setuparr_s.push_back(cstr("realshadowcascade2=") + Str(g.globals.realshadowcascade[2]));
	if (g.globals.realshadowsize[2] > 0) {
		setuparr_s.push_back(cstr("realshadowsize2=") + Str(g.globals.realshadowsize[2]));
	}
	setuparr_s.push_back(cstr("realshadowcascade3=") + Str(g.globals.realshadowcascade[3]));
	if (g.globals.realshadowsize[3] > 0) {
		setuparr_s.push_back(cstr("realshadowsize3=") + Str(g.globals.realshadowsize[3]));
	}
	setuparr_s.push_back(cstr("realshadowcascade4=") + Str(g.globals.realshadowcascade[4]));
	if (g.globals.realshadowsize[4] > 0) {
		setuparr_s.push_back(cstr("realshadowsize4=") + Str(g.globals.realshadowsize[4]));
	}
	setuparr_s.push_back(cstr("realshadowcascade5=") + Str(g.globals.realshadowcascade[5]));
	if (g.globals.realshadowsize[5] > 0) {
		setuparr_s.push_back(cstr("realshadowsize5=") + Str(g.globals.realshadowsize[5]));
	}
	setuparr_s.push_back(cstr("realshadowcascade6=") + Str(g.globals.realshadowcascade[6]));
	if (g.globals.realshadowsize[6] > 0) {
		setuparr_s.push_back(cstr("realshadowsize6=") + Str(g.globals.realshadowsize[6]));
	}
	setuparr_s.push_back(cstr("realshadowcascade7=") + Str(g.globals.realshadowcascade[7]));
	if (g.globals.realshadowsize[7] > 0) {
		setuparr_s.push_back(cstr("realshadowsize7=") + Str(g.globals.realshadowsize[7]));
	}

	setuparr_s.push_back(cstr("enableplrspeedmods=") + Str(g.globals.enableplrspeedmods));
	setuparr_s.push_back(cstr("disableweaponjams=") + Str(g.globals.disableweaponjams));
	//PE: Add new setup.ini functions.
	setuparr_s.push_back("ConvertToDDS=1");
	setuparr_s.push_back("ConvertToDDSMaxsize=2048");
	setuparr_s.push_back("uselodobjects=1");

	setuparr_s.push_back("");

	setuparr_s.push_back("[GAMEMENUOPTIONS]");
	setuparr_s.push_back(cstr("graphicslowterrain=") + g.graphicslowterrain_s);
	setuparr_s.push_back(cstr("graphicslowentity=") + g.graphicslowentity_s);
	setuparr_s.push_back(cstr("graphicslowgrass=") + g.graphicslowgrass_s);
	setuparr_s.push_back(cstr("graphicsmediumterrain=") + g.graphicsmediumterrain_s);
	setuparr_s.push_back(cstr("graphicsmediumentity=") + g.graphicsmediumentity_s);
	setuparr_s.push_back(cstr("graphicsmediumgrass=") + g.graphicsmediumgrass_s);
	setuparr_s.push_back(cstr("graphicshighterrain=") + g.graphicshighterrain_s);
	setuparr_s.push_back(cstr("graphicshighentity=") + g.graphicshighentity_s);
	setuparr_s.push_back(cstr("graphicshighgrass=") + g.graphicshighgrass_s);

	setuparr_s.push_back("");
	setuparr_s.push_back("[CUSTOMIZATIONS]");
	setuparr_s.push_back(cstr("switchtoalt=") + Str(g.ggunaltswapkey1));
	setuparr_s.push_back(cstr("melee key=") + Str(g.ggunmeleekey));
	setuparr_s.push_back(cstr("zoomholdbreath=") + Str(g.gzoomholdbreath));
	setuparr_s.push_back(cstr("keyUP=") + Str(t.listkey[1]));
	setuparr_s.push_back(cstr("keyDOWN=") + Str(t.listkey[2]));
	setuparr_s.push_back(cstr("keyLEFT=") + Str(t.listkey[3]));
	setuparr_s.push_back(cstr("keyRIGHT=") + Str(t.listkey[4]));
	setuparr_s.push_back(cstr("keyJUMP=") + Str(t.listkey[5]));
	setuparr_s.push_back(cstr("keyCROUCH=") + Str(t.listkey[6]));
	setuparr_s.push_back(cstr("keyENTER=") + Str(t.listkey[7]));
	setuparr_s.push_back(cstr("keyRELOAD=") + Str(t.listkey[8]));
	setuparr_s.push_back(cstr("keyPEEKLEFT=") + Str(t.listkey[9]));
	setuparr_s.push_back(cstr("keyPEEKRIGHT=") + Str(t.listkey[10]));
	setuparr_s.push_back(cstr("keyRUN=") + Str(t.listkey[11]));

	// vr extras
	if ( g.vrqcontrolmode != 0 || g.gxbox != 0 )
	{
		// CONTROLLER
		setuparr_s.push_back("");
		setuparr_s.push_back("[CONTROLLER]");
		if ( g.vrqcontrolmode != 0 )
		{
			if ( g.vrqorggcontrolmode == 2 )
			{
				// No controller by default in EDU mode
				setuparr_s.push_back("xbox=0");
				setuparr_s.push_back("xboxcontrollertype=2");
				setuparr_s.push_back("xboxinvert=0");
				setuparr_s.push_back("xboxmag=100");
			}
			else
			{
				//PE: Could not get standalone working , until i see xbox=1 , should it not be based on original setup.ini g.gxbox ?
				setuparr_s.push_back(cstr("xbox=") + Str(g.gxbox));
				setuparr_s.push_back("xboxcontrollertype=2");
				setuparr_s.push_back("xboxinvert=0");
				setuparr_s.push_back("xboxmag=100");
			}
		}
		else
		{
			setuparr_s.push_back(cstr("xbox=") + Str(g.gxbox));
			setuparr_s.push_back(cstr("xboxcontrollertype=") + Str(g.gxboxcontrollertype));
			setuparr_s.push_back(cstr("xboxinvert=") + Str(g.gxboxinvert));
			setuparr_s.push_back(cstr("xboxmag=") + Str(g.gxboxmag));
		}
	}

	if (  FileExist(setupfile_s.Get()) == 1  )  DeleteAFile (  setupfile_s.Get() );
	SaveArray (  setupfile_s.Get(), setuparr_s );

	// separate VR setup file
	extern bool g_bStandaloneVRMode;
	if (g_bStandaloneVRMode == true)
	{
		std::vector<cstr> setupvr_s;
		setupvr_s.reserve(10);
		cstr setupvrfile_s = t.exepath_s + t.exename_s + "\\setupvr.ini";
		setupvr_s.push_back("[VR]");
		setupvr_s.push_back("vrmode=3");
		setupvr_s.push_back("vrmodemag=100");
		setupvr_s.push_back(cstr("vroffsetangx=") + Str(g.gvroffsetangx));
		setupvr_s.push_back(cstr("vrwmroffsetangx=") + Str(g.gvrwmroffsetangx));
		if (FileExist(setupvrfile_s.Get()) == 1)  DeleteAFile (setupvrfile_s.Get());
		SaveArray (setupvrfile_s.Get(), setupvr_s);
	}

	//  Also save out the localisation ptr file
	std::vector<cstr> userdetails_s(2);
	cstr userdetailsfile_s = t.exepath_s + t.exename_s + "\\userdetails.ini";
	userdetails_s[0] = "[LOCALIZATION]";
	userdetails_s[1] = cstr("language=") + g.language_s;
	SaveArray ( userdetailsfile_s.Get(), userdetails_s );

	//  Itinery of all files in standalone
	cstr itineraryfile_s = t.exepath_s + t.exename_s + "\\contents.txt";
	SaveArray ( itineraryfile_s.Get(), t.filecollection_s );

	//  cleanup file array
	UnDim (  t.filecollection_s );
}

int mapfile_savestandalone_continue ( void )
{
	int iSuccess = 0;
	switch ( g_mapfile_iStage )
	{
		case 1 :	g_mapfile_fProgress+=0.1f; 
					if ( g_mapfile_fProgress >= 5.0f )
						g_mapfile_iStage = 20;
					break;

		case 20 :	mapfile_savestandalone_stage2a();
					g_mapfile_fProgress = 6.0f; 
					g_mapfile_iStage = 21;
					break;

		case 21 :	if ( mapfile_savestandalone_stage2b() == 0 )
					{
						g_mapfile_iStage = 22;
					}
					else
					{
						g_mapfile_iStage = 29;
					}
					break;

		case 22: 
		{
			if (mapfile_savestandalone_stage2c() == 0)
			{
				g_mapfile_fProgress += (80.0f / g_mapfile_fProgressSpan);
				if (g_mapfile_fProgress > 84) g_mapfile_fProgress = 84;
			}
			else
			{
				g_mapfile_iStage = 23;
			}
			break;
		}

		case 23 :	mapfile_savestandalone_stage2d();
					g_mapfile_iStage = 21; 
					break;

		case 29 :	mapfile_savestandalone_stage2e(); 
					g_mapfile_iStage = 30; 
					break;

		case 30 :	mapfile_savestandalone_stage3(); 
					g_mapfile_fProgress = 90.0f; 
					g_mapfile_iStage = 40; 
					break;

		case 40 :	mapfile_savestandalone_stage4(); 
					g_mapfile_fProgress = 95.0f; 
					g_mapfile_iStage = 50; 
					break;

		case 50 :	g_mapfile_fProgress+=0.1f; 
					if ( g_mapfile_fProgress >= 100.0f )
						g_mapfile_iStage = 60;
					break;

		case 60 :	g_mapfile_fProgress = 100.0f; 
					g_mapfile_iStage = 99; 
					iSuccess = 1;
					break;
	}
	return iSuccess;
}

float mapfile_savestandalone_getprogress ( void )
{
	return g_mapfile_fProgress;
}

void mapfile_savestandalone_finish ( void )
{
	// completed exclusive use of filecollection array (some remote project code would have tried to reuse this array)
	g_bMakingAStandaloneUsingFileCollectionArray = false;

	// encrypt media
	cstr dest_s = t.exepath_s + t.exename_s;
	if ( g.gexportassets == 0 ) 
	{
		if ( PathExist( cstr(dest_s + "\\Files").Get() ) ) 
		{
			//  NOTE; Need to exclude lightmaps from encryptor  set encrypt ignore list "lightmaps"
			EncryptAllFiles ( cstr(dest_s + "\\Files").Get() );
		}
	}

	//  if not tignorelevelbankfiles, copy unencrypted files
	if (  g_mapfile_tignorelevelbankfiles == 0 ) 
	{
		//  now copy the files we do not want to encrypt
		g.filecollectionmax = 0;
		Dim (  t.filecollection_s,500  );

		//  lightmap DBOs
		SetDir (  cstr(g.fpscrootdir_s+"\\Files\\").Get() );
		int tfurthestobjnumber = g.lightmappedobjectoffset;
		for ( int tobj = g.lightmappedobjectoffset; tobj <= g.lightmappedobjectoffset+99999 ; tobj += 100 )
		{
			cstr tname_s = cstr("levelbank\\testmap\\lightmaps\\object") + Str(tobj) + ".dbo";
			if (  FileExist(tname_s.Get()) == 1  )  tfurthestobjnumber = tobj+100;
		}
		for ( int tobj = g.lightmappedobjectoffset; tobj <= tfurthestobjnumber; tobj++ )
		{
			cstr tname_s = cstr("levelbank\\testmap\\lightmaps\\object") + Str(tobj) + ".dbo";
			if (  FileExist(tname_s.Get()) == 1  )  addtocollection(tname_s.Get());
		}
		for ( int tobj = g.lightmappedobjectoffset; tobj <= g.lightmappedobjectoffsetfinish; tobj++ )
		{
			cstr tname_s = cstr("levelbank\\testmap\\lightmaps\\object") + Str(tobj) + ".dbo";
			if (  FileExist(tname_s.Get()) == 1  )  addtocollection(tname_s.Get());
		}
		int nmax = 500;
		for ( int n = 0 ; n <= 5000 ; n += 100 )
		{
			cstr tfile_s = cstr("levelbank\\testmap\\lightmaps\\") + Str(n) + ".dds";
			if (  FileExist(tfile_s.Get()) == 1  )  nmax = n + 100;
		}
		for ( int n = 0 ; n <= nmax; n++ )
		{
			cstr tfile_s = cstr("levelbank\\testmap\\lightmaps\\") + Str(n) + ".dds";
			if (  FileExist(tfile_s.Get()) == 1  )  addtocollection(tfile_s.Get());
		}
		addtocollection("levelbank\\testmap\\lightmaps\\objectlist.dat");
		addtocollection("levelbank\\testmap\\lightmaps\\objectnummax.dat");

		//  Copy the 'unencrypted files' collection to exe folder
		timestampactivity(0, cstr(cstr("filecollectionmax=") + Str(g.filecollectionmax)).Get() );
		SetDir ( cstr(t.exepath_s+t.exename_s+"\\Files\\levelbank\\testmap").Get() );
		if (  PathExist("lightmaps") == 0  )  MakeDirectory (  "lightmaps" );
		SetDir (  cstr(g.fpscrootdir_s+"\\Files\\").Get() );
		for ( int fileindex = 1 ; fileindex <= g.filecollectionmax; fileindex++ )
		{
			cstr src_s = t.filecollection_s[fileindex];
			if (  FileExist(src_s.Get()) == 1 ) 
			{
				cstr fileDest_s = t.exepath_s + t.exename_s + "\\Files\\" + src_s;
				if (  FileExist(fileDest_s.Get()) == 1  )  DeleteAFile (  fileDest_s.Get() );
				CopyAFile (  src_s.Get(), fileDest_s.Get() );
			}
		}
	}

	// restore directory, restore original level and close up
	mapfile_savestandalone_restoreandclose();
}

void mapfile_savestandalone_restoreandclose ( void )
{
	// Restore directory
	SetDir ( g_mapfile_told_s.Get() );

	// restore original level FPM files
	timestampactivity(0, cstr(cstr("check '")+g.projectfilename_s+"' vs '"+t.tmasterlevelfile_s+"'").Get() );
	if ( g.projectfilename_s != t.tmasterlevelfile_s || restore_old_map )
	{
		restore_old_map = false;
		if ( Len(t.tmasterlevelfile_s.Get()) > 1 )
		{
			g.projectfilename_s=t.tmasterlevelfile_s;
			// need to load EVERYTHING back in
			gridedit_load_map ( );
		}
	}

	// no longer making standalone
	t.levelsforstandalone = 0;

	// no longer exclusive use of file collection array
	g_bMakingAStandaloneUsingFileCollectionArray = false;
}

void scanscriptfileandaddtocollection ( const char* tfile_s , const char *pPath)
{
	if (!tfile_s || !*tfile_s) return;
	if (FileExist(const_cast<char*>(tfile_s)) != 1) return;

	FILE* fp = GG_fopen(tfile_s, "r");
	if (!fp) return;

	char lineBuf[2048];
	while (fgets(lineBuf, sizeof(lineBuf), fp))
	{
		size_t lineLen = strlen(lineBuf);
		while (lineLen > 0 && (lineBuf[lineLen - 1] == '\r' || lineBuf[lineLen - 1] == '\n'))
		{
			lineBuf[--lineLen] = '\0';
		}
		if (lineLen < 5) continue;

		char* pCur = lineBuf;
		while (*pCur == ' ' || *pCur == '\t') pCur++;
		if (pCur[0] == '-' && pCur[1] == '-') continue; // Line-level Lua comment

		// Check if line contains an inline comment "--"
		const char* pComment = strstr(pCur, "--");

		const char* pScan = pCur;
		while (*pScan)
		{
			// Find "require" or "include" case-insensitively
			const char* pReq = pestrcasestr(pScan, "require");
			const char* pInc = pestrcasestr(pScan, "include");

			const char* pFound = nullptr;
			size_t kwLen = 0;

			if (pReq && pInc)
			{
				if (pReq <= pInc) { pFound = pReq; kwLen = 7; }
				else { pFound = pInc; kwLen = 7; }
			}
			else if (pReq)
			{
				pFound = pReq;
				kwLen = 7;
			}
			else if (pInc)
			{
				pFound = pInc;
				kwLen = 7;
			}
			else
			{
				break;
			}

			// If keyword is inside an inline comment, stop scanning this line
			if (pComment && pFound >= pComment)
			{
				break;
			}

			// Check word boundary before keyword
			if (pFound > lineBuf)
			{
				char prevCh = *(pFound - 1);
				if (isalnum((unsigned char)prevCh) || prevCh == '_')
				{
					pScan = pFound + kwLen;
					continue;
				}
			}

			const char* pAfterKw = pFound + kwLen;
			// Skip whitespace
			while (*pAfterKw == ' ' || *pAfterKw == '\t') pAfterKw++;

			// Optional opening parenthesis '('
			if (*pAfterKw == '(')
			{
				pAfterKw++;
				while (*pAfterKw == ' ' || *pAfterKw == '\t') pAfterKw++;
			}

			// Must be opening quote '\"' or '\''
			if (*pAfterKw != '\"' && *pAfterKw != '\'')
			{
				pScan = pFound + kwLen;
				continue;
			}

			char quoteChar = *pAfterKw;
			pAfterKw++; // move past quote

			const char* pNameStart = pAfterKw;
			const char* pNameEnd = pNameStart;
			while (*pNameEnd && *pNameEnd != quoteChar)
			{
				pNameEnd++;
			}

			if (pNameEnd > pNameStart && *pNameEnd == quoteChar)
			{
				std::string script_name(pNameStart, pNameEnd - pNameStart);
				replaceAll(script_name, "\\\\", "\\");
				replaceAll(script_name, "/", "\\");

				// Strip redundant leading scriptbank\ if present
				if (strnicmp(script_name.c_str(), "scriptbank\\", 11) == 0)
				{
					script_name = script_name.substr(11);
				}

				if (!pestrcasestr(script_name.c_str(), ".lua"))
				{
					script_name += ".lua";
				}

				size_t luaPos = script_name.rfind(".lua");
				if (luaPos != std::string::npos)
				{
					script_name = script_name.substr(0, luaPos + 4);
				}

				std::string collectionScript;
				if (strnicmp(script_name.c_str(), "titlesbank\\", 11) == 0)
				{
					collectionScript = script_name;
				}
				else
				{
					collectionScript = std::string("scriptbank\\") + script_name;
				}

				if (addtocollection(collectionScript.c_str()))
				{
					if (pPath && *pPath)
					{
						std::string subScript = std::string(pPath) + collectionScript;
						scanscriptfileandaddtocollection(subScript.c_str(), pPath);
					}
					else
					{
						scanscriptfileandaddtocollection(collectionScript.c_str());
					}
				}

				pScan = pNameEnd + 1;
			}
			else
			{
				pScan = pFound + kwLen;
			}
		}
	}
	fclose(fp);
}

bool addtocollection ( const char* file_s )
{
	if (!file_s || !*file_s) return false;
	std::string lowerFile = file_s;
	for (char& ch : lowerFile) ch = (char)tolower((unsigned char)ch);

	// Ensure this entry is not already present
	for (int f = 1; f <= g.filecollectionmax; f++)
	{
		if (t.filecollection_s[f] == lowerFile.c_str())
			return false;
	}

	// Expand file collection array if nearly full
	++g.filecollectionmax;
	int tarrsize = ArrayCount(t.filecollection_s);
	if (g.filecollectionmax > tarrsize - 10)
	{
		Dim(t.filecollection_s, tarrsize + 50);
	}
	t.filecollection_s[g.filecollectionmax] = lowerFile.c_str();
	return true;
}

void removefromcollection ( const char* file_s )
{
	if (!file_s || !*file_s) return;
	std::string lowerFile = file_s;
	for (char& ch : lowerFile) ch = (char)tolower((unsigned char)ch);

	int tfound = 0;
	for (int f = 1; f <= g.filecollectionmax; f++)
	{
		if (t.filecollection_s[f] == lowerFile.c_str())
		{
			tfound = f;
			break;
		}
	}
	if (tfound > 0)
	{
		t.filecollection_s[tfound] = "";
	}
}

void removeanymatchingfromcollection ( const char* folderorfile_s )
{
	if (!folderorfile_s || !*folderorfile_s) return;
	std::string lowerPrefix = folderorfile_s;
	for (char& ch : lowerPrefix) ch = (char)tolower((unsigned char)ch);
	size_t prefixLen = lowerPrefix.length();

	for (int f = 1; f <= g.filecollectionmax; f++)
	{
		const char* pFile = t.filecollection_s[f].Get();
		if (pFile && _strnicmp(pFile, lowerPrefix.c_str(), prefixLen) == 0)
		{
			t.filecollection_s[f] = "";
		}
	}
}

bool g_bNormalOperations = true;

void addfoldertocollection ( const char* path_s )
{
	if (!path_s || !*path_s) return;
	std::string usePath = path_s;
	std::string told_s = GetDir() ? GetDir() : "";

	bool bAddAdditionalFilesFromDocWrite = false;
	if (pestrcasestr(path_s, "gamecore\\decals"))
		bAddAdditionalFilesFromDocWrite = true;
	if (pestrcasestr(path_s, "gamecore\\hands\\Animations"))
		bAddAdditionalFilesFromDocWrite = true;

	if (!PathExist(const_cast<char*>(usePath.c_str())))
	{
		if (g_bNormalOperations)
		{
			extern char szWriteDir[MAX_PATH];
			std::string testPath = std::string(szWriteDir) + "Files\\" + path_s;
			if (PathExist(const_cast<char*>(testPath.c_str())))
			{
				usePath = testPath;
			}
		}
	}
	if (PathExist(const_cast<char*>(usePath.c_str())))
	{
		SetDir(const_cast<char*>(usePath.c_str()));
		ChecklistForFiles();
		if (ChecklistQuantity() <= 2)
		{
			if (g_bNormalOperations)
			{
				extern char szWriteDir[MAX_PATH];
				std::string testPath = std::string(szWriteDir) + "Files\\" + path_s;
				SetDir(const_cast<char*>(told_s.c_str()));
				if (PathExist(const_cast<char*>(testPath.c_str())))
				{
					SetDir(const_cast<char*>(testPath.c_str()));
					ChecklistForFiles();
				}
			}
		}
		for (int c = 1; c <= ChecklistQuantity(); c++)
		{
			if (ChecklistValueA(c) == 0)
			{
				const char* pFile = ChecklistString(c);
				if (pFile && strcmp(pFile, ".") != 0 && strcmp(pFile, "..") != 0)
				{
					std::string relFile = std::string(path_s) + "\\" + pFile;
					addtocollection(relFile.c_str());
				}
			}
		}
		SetDir(const_cast<char*>(told_s.c_str()));
	}
	else
	{
		char pDebugPath[10240];
		snprintf(pDebugPath, sizeof(pDebugPath), "Tried adding path '%s' that does not exist here: %s", path_s, usePath.c_str());
		timestampactivity(0, pDebugPath);
	}

	if (bAddAdditionalFilesFromDocWrite && g_bNormalOperations)
	{
		extern char szWriteDir[MAX_PATH];
		std::string testPath = std::string(szWriteDir) + "Files\\" + path_s;
		if (PathExist(const_cast<char*>(testPath.c_str())))
		{
			usePath = testPath;
			SetDir(const_cast<char*>(usePath.c_str()));
			ChecklistForFiles();
			for (int c = 1; c <= ChecklistQuantity(); c++)
			{
				if (ChecklistValueA(c) == 0)
				{
					const char* pFile = ChecklistString(c);
					if (pFile && strcmp(pFile, ".") != 0 && strcmp(pFile, "..") != 0)
					{
						std::string relFile = std::string(path_s) + "\\" + pFile;
						addtocollection(relFile.c_str());
					}
				}
			}
			SetDir(const_cast<char*>(told_s.c_str()));
		}
	}
}

void addallinfoldertocollection ( const char* subThisFolder_s, const char* subFolder_s )
{
	std::string thisFolder = subThisFolder_s ? subThisFolder_s : "";
	std::string folder = subFolder_s ? subFolder_s : "";
	std::string olddir = "";
	if (!thisFolder.empty())
	{
		olddir = GetDir() ? GetDir() : "";
		SetDir(const_cast<char*>(thisFolder.c_str()));
	}

	ChecklistForFiles();
	int iFoldersCount = ChecklistQuantity();
	std::vector<std::string> folders(iFoldersCount + 1);
	for (int c = 1; c <= iFoldersCount; c++)
	{
		const char* pFileFolderName = ChecklistString(c);
		if (pFileFolderName && strcmp(pFileFolderName, ".") != 0 && strcmp(pFileFolderName, "..") != 0)
		{
			if (ChecklistValueA(c) == 1)
			{
				folders[c] = pFileFolderName;
			}
			else
			{
				std::string relativeFilePath_s = folder + "\\" + pFileFolderName;
				addtocollection(relativeFilePath_s.c_str());
			}
		}
	}

	for (int f = 1; f <= iFoldersCount; f++)
	{
		if (!folders[f].empty())
		{
			std::string relativeFolderPath_s = folder;
			if (!folder.empty()) relativeFolderPath_s += "\\";
			relativeFolderPath_s += folders[f];
			addallinfoldertocollection(folders[f].c_str(), relativeFolderPath_s.c_str());
		}
	}

	if (!olddir.empty())
	{
		SetDir(const_cast<char*>(olddir.c_str()));
	}
}

void createallfoldersincollection ( void )
{
	std::string pOldDir = GetDir() ? GetDir() : "";
	int filesmax = g.filecollectionmax;
	char szMsg[256];
	snprintf(szMsg, sizeof(szMsg), "Create full path structure (%d) for standalone executable", filesmax);
	timestampactivity(0, szMsg);

	for (int fileindex = 1; fileindex <= filesmax; fileindex++)
	{
		const char* pSrc = t.filecollection_s[fileindex].Get();
		if (!pSrc || !*pSrc) continue;

		std::string olddir_s = GetDir() ? GetDir() : "";
		std::string srcstring_s = pSrc;

		while (!srcstring_s.empty())
		{
			size_t found = srcstring_s.find_first_of("/\\");
			if (found == std::string::npos) break;

			std::string chunk_s = srcstring_s.substr(0, found);
			if (!chunk_s.empty())
			{
				if (PathExist(const_cast<char*>(chunk_s.c_str())) == 0)
					MakeDirectory(const_cast<char*>(chunk_s.c_str()));
				if (PathExist(const_cast<char*>(chunk_s.c_str())) == 0)
				{
					char szPathLog[1024];
					snprintf(szPathLog, sizeof(szPathLog), "Path:%s", pSrc);
					timestampactivity(0, szPathLog);
					char szErr[512];
					snprintf(szErr, sizeof(szErr), "Unable to create folder:'%s'", chunk_s.c_str());
					timestampactivity(0, szErr);
				}
				else
				{
					SetDir(const_cast<char*>(chunk_s.c_str()));
				}
			}
			srcstring_s = srcstring_s.substr(found + 1);
		}

		SetDir(const_cast<char*>(olddir_s.c_str()));
	}
	SetDir(const_cast<char*>(pOldDir.c_str()));
}

void findalltexturesinmodelfile ( const char* inputfile_s, const char* folder_s, const char* texpath_s )
{
	if (!inputfile_s || !*inputfile_s) return;
	std::string folderStr = folder_s ? folder_s : "";
	std::string texpathStr = texpath_s ? texpath_s : "";

	for (int iPass = 0; iPass < 2; iPass++)
	{
		std::string filestr = inputfile_s;
		if (iPass == 1)
		{
			size_t len = filestr.length();
			if (len >= 2 && _stricmp(filestr.c_str() + len - 2, ".x") == 0)
			{
				filestr = filestr.substr(0, len - 2) + ".dbo";
			}
		}

		if (FileExist(const_cast<char*>(filestr.c_str())) == 1)
		{
			int filesize = FileSize(const_cast<char*>(filestr.c_str()));
			int mbi = 255;
			OpenToRead(11, const_cast<char*>(filestr.c_str()));
			if (FileOpen(11) == 1)
			{
				MakeMemblockFromFile(mbi, 11);
				CloseFile(11);
				for (int b = 0; b <= filesize - 4; b++)
				{
					if (ReadMemblockByte(mbi, b + 0) == Asc("."))
					{
						int tfoundpiccy = 0;
						uint8_t b1 = ReadMemblockByte(mbi, b + 1);
						uint8_t b2 = ReadMemblockByte(mbi, b + 2);
						uint8_t b3 = ReadMemblockByte(mbi, b + 3);

						if ((b1 == 'T' || b1 == 't') && (b2 == 'G' || b2 == 'g') && (b3 == 'A' || b3 == 'a')) tfoundpiccy = 1;
						else if ((b1 == 'J' || b1 == 'j') && (b2 == 'P' || b2 == 'p') && (b3 == 'G' || b3 == 'g')) tfoundpiccy = 1;
						else if ((b1 == 'D' || b1 == 'd') && (b2 == 'D' || b2 == 'd') && (b3 == 'S' || b3 == 's')) tfoundpiccy = 1;
						else if ((b1 == 'B' || b1 == 'b') && (b2 == 'M' || b2 == 'm') && (b3 == 'P' || b3 == 'p')) tfoundpiccy = 1;
						else if ((b1 == 'P' || b1 == 'p') && (b2 == 'N' || b2 == 'n') && (b3 == 'G' || b3 == 'g')) tfoundpiccy = 1;
						else if ((b1 == 'P' || b1 == 'p') && (b2 == 'S' || b2 == 's') && (b3 == 'D' || b3 == 'd')) tfoundpiccy = 1;

						if (tfoundpiccy == 1)
						{
							int c = b;
							for (; c >= b - 255 && c >= 0; c--)
							{
								uint8_t byteVal = ReadMemblockByte(mbi, c);
								if (byteVal < ' ' || byteVal > 'z' || byteVal == '"')
								{
									break;
								}
							}
							std::string texfile_s;
							texfile_s.reserve((b + 3) - c);
							for (int d = c + 1; d <= b + 3; d++)
							{
								texfile_s += (char)tolower((unsigned char)ReadMemblockByte(mbi, d));
							}

							if (texfile_s.length() > 4)
							{
								size_t colonPos = texfile_s.find(':');
								if (colonPos != std::string::npos)
								{
									size_t lastSlash = texfile_s.find_last_of("/\\");
									if (lastSlash != std::string::npos)
									{
										texfile_s = texfile_s.substr(lastSlash + 1);
									}
								}
							}

							if (_strnicmp(texfile_s.c_str(), "effectbank\\", 11) == 0)
							{
								addtocollection(texfile_s.c_str());
							}
							else
							{
								bool bDetectedPBRTextureSetName = false;
								std::string texfilenoext_s = (texfile_s.length() >= 4) ? texfile_s.substr(0, texfile_s.length() - 4) : texfile_s;
								size_t noextLen = texfilenoext_s.length();

								auto checkSuffix = [&](const char* suffix, size_t suflen) -> bool {
									if (noextLen >= suflen && _strnicmp(texfilenoext_s.c_str() + noextLen - suflen, suffix, suflen) == 0)
									{
										texfilenoext_s = texfilenoext_s.substr(0, noextLen - suflen);
										bDetectedPBRTextureSetName = true;
										return true;
									}
									return false;
								};

								checkSuffix("_color", 6) || checkSuffix("_normal", 7) || checkSuffix("_metalness", 10) ||
								checkSuffix("_roughness", 10) || checkSuffix("_gloss", 6) || checkSuffix("_ao", 3) ||
								checkSuffix("_surface", 8);

								if (bDetectedPBRTextureSetName)
								{
									bool tex_found = false;
									size_t slashPos = texfilenoext_s.find_last_of("/\\");
									if (slashPos != std::string::npos)
									{
										std::string directfile = texfilenoext_s.substr(slashPos + 1);
										std::string tmp = folderStr + directfile + "_color.dds";
										if (FileExist(const_cast<char*>(tmp.c_str())))
										{
											addtocollection(tmp.c_str());
											addtocollection((folderStr + directfile + "_normal.dds").c_str());
											addtocollection((folderStr + directfile + "_metalness.dds").c_str());
											addtocollection((folderStr + directfile + "_gloss.dds").c_str());
											addtocollection((folderStr + directfile + "_ao.dds").c_str());
											addtocollection((folderStr + directfile + "_illumination.dds").c_str());
											addtocollection((folderStr + directfile + "_surface.dds").c_str());
											tex_found = true;
										}
										tmp = folderStr + directfile + "_color.png";
										if (FileExist(const_cast<char*>(tmp.c_str())))
										{
											addtocollection(tmp.c_str());
											addtocollection((folderStr + directfile + "_normal.png").c_str());
											addtocollection((folderStr + directfile + "_metalness.png").c_str());
											addtocollection((folderStr + directfile + "_gloss.png").c_str());
											addtocollection((folderStr + directfile + "_ao.png").c_str());
											addtocollection((folderStr + directfile + "_illumination.png").c_str());
											addtocollection((folderStr + directfile + "_surface.dds").c_str());
											tex_found = true;
										}
									}

									if (!tex_found)
									{
										std::string texfileColor_s = texfilenoext_s + "_color.dds";
										if (FileExist(const_cast<char*>((folderStr + texpathStr + texfileColor_s).c_str())) || FileExist(const_cast<char*>((folderStr + texfileColor_s).c_str())))
										{
											addtocollection((folderStr + texpathStr + texfileColor_s).c_str());
											addtocollection((folderStr + texfileColor_s).c_str());
											std::string texfileNormal_s = texfilenoext_s + "_normal.dds";
											addtocollection((folderStr + texpathStr + texfileNormal_s).c_str());
											addtocollection((folderStr + texfileNormal_s).c_str());
											std::string texfileMetalness_s = texfilenoext_s + "_metalness.dds";
											addtocollection((folderStr + texpathStr + texfileMetalness_s).c_str());
											addtocollection((folderStr + texfileMetalness_s).c_str());
											std::string texfileGloss_s = texfilenoext_s + "_gloss.dds";
											addtocollection((folderStr + texpathStr + texfileGloss_s).c_str());
											addtocollection((folderStr + texfileGloss_s).c_str());
											std::string texfileAO_s = texfilenoext_s + "_ao.dds";
											addtocollection((folderStr + texpathStr + texfileAO_s).c_str());
											addtocollection((folderStr + texfileAO_s).c_str());
											std::string texfileIllumination_s = texfilenoext_s + "_illumination.dds";
											addtocollection((folderStr + texpathStr + texfileIllumination_s).c_str());
											addtocollection((folderStr + texfileIllumination_s).c_str());
											std::string texfilesurface_s = texfilenoext_s + "_surface.dds";
											addtocollection((folderStr + texpathStr + texfilesurface_s).c_str());
											addtocollection((folderStr + texfilesurface_s).c_str());
										}
									}
								}

								if (FileExist(const_cast<char*>((folderStr + texpathStr + texfile_s).c_str())))
									addtocollection((folderStr + texpathStr + texfile_s).c_str());
								if (FileExist(const_cast<char*>((folderStr + texfile_s).c_str())))
									addtocollection((folderStr + texfile_s).c_str());

								size_t texLen = texfile_s.length();
								if (texLen >= 4 && _stricmp(texfile_s.c_str() + texLen - 4, ".dds") != 0)
								{
									if (FileExist(const_cast<char*>((folderStr + texfile_s + ".png").c_str())))
										addtocollection((folderStr + texfile_s + ".png").c_str());
									std::string ddsFile = texfile_s.substr(0, texLen - 4) + ".dds";
									if (FileExist(const_cast<char*>((folderStr + texpathStr + ddsFile).c_str())))
										addtocollection((folderStr + texpathStr + ddsFile).c_str());
									if (FileExist(const_cast<char*>((folderStr + ddsFile).c_str())))
										addtocollection((folderStr + ddsFile).c_str());
								}
							}
							b += 4;
						}
					}
				}
				DeleteMemblock(mbi);
			}
		}
	}
}

void CreateItineraryFile ( void )
{
	g_sDefaultAssetFiles.clear();
}

void scanallfolder ( const char* subThisFolder_s, const char* subFolder_s )
{
	std::string thisFolder = subThisFolder_s ? subThisFolder_s : "";
	std::string folder = subFolder_s ? subFolder_s : "";
	if (!thisFolder.empty()) SetDir(const_cast<char*>(thisFolder.c_str()));

	ChecklistForFiles();
	int iFoldersCount = ChecklistQuantity();
	std::vector<std::string> folders(iFoldersCount + 1);
	for (int c = 1; c <= iFoldersCount; c++)
	{
		const char* pFileFolderName = ChecklistString(c);
		if (pFileFolderName && strcmp(pFileFolderName, ".") != 0 && strcmp(pFileFolderName, "..") != 0)
		{
			if (ChecklistValueA(c) == 1)
			{
				folders[c] = pFileFolderName;
			}
			else
			{
				std::string relativeFilePath_s = folder + "\\" + pFileFolderName;
				std::string cleanStr;
				cleanStr.reserve(relativeFilePath_s.length());
				for (size_t n = 0; n < relativeFilePath_s.length(); n++)
				{
					if (relativeFilePath_s[n] == '\\' && n + 1 < relativeFilePath_s.length() && relativeFilePath_s[n + 1] == '\\')
						continue;
					cleanStr += relativeFilePath_s[n];
				}
				g_sDefaultAssetFiles.push_back(cleanStr.c_str());
			}
		}
	}

	for (int f = 1; f <= iFoldersCount; f++)
	{
		if (!folders[f].empty())
		{
			std::string relFolder = folders[f];
			if (!folder.empty()) relFolder = folder + "\\" + folders[f];
			scanallfolder(folders[f].c_str(), relFolder.c_str());
		}
	}

	if (!thisFolder.empty()) SetDir("..");
}

bool IsFileAStockAsset ( const char* pCheckThisFile )
{
	if (!pCheckThisFile || !*pCheckThisFile) return false;
	char pFileToCheck[2048];
	strncpy(pFileToCheck, pCheckThisFile, sizeof(pFileToCheck) - 1);
	pFileToCheck[sizeof(pFileToCheck) - 1] = '\0';

	for (size_t n = 0; n < g_sDefaultAssetFiles.size(); n++)
	{
		const char* pCompare = g_sDefaultAssetFiles[n].Get();
		if (pCompare && _stricmp(pFileToCheck, pCompare) == 0)
			return true;
	}

	bool bSecondaryCheck = false;
	size_t len = strlen(pCheckThisFile);
	if (len >= 4 && _stricmp(pCheckThisFile + len - 4, ".dbo") == 0)
	{
		if (len < sizeof(pFileToCheck) - 2)
		{
			strncpy(pFileToCheck, pCheckThisFile, sizeof(pFileToCheck) - 1);
			pFileToCheck[len - 4] = '\0';
			strcat(pFileToCheck, ".x");
			bSecondaryCheck = true;
		}
	}
	if (bSecondaryCheck)
	{
		for (size_t n = 0; n < g_sDefaultAssetFiles.size(); n++)
		{
			const char* pCompare = g_sDefaultAssetFiles[n].Get();
			if (pCompare && _stricmp(pFileToCheck, pCompare) == 0)
				return true;
		}
	}
	return false;
}

void AddWPETextures(const char* filename)
{
	if (!filename || !*filename) return;
	char cPE[MAX_PATH];
	strncpy(cPE, filename, sizeof(cPE) - 1);
	cPE[sizeof(cPE) - 1] = '\0';
	char* find = (char*)pestrcasestr(cPE, ".pe");
	if (find)
	{
		*find = '\0';
		char finalname[MAX_PATH];
		for (int idx = 0; idx <= 6; ++idx)
		{
			snprintf(finalname, sizeof(finalname), "%s%d_color.png", cPE, idx);
			addtocollection(finalname);
		}
	}
	else
	{
		find = (char*)pestrcasestr(cPE, ".arx");
		if (find)
		{
			*find = '\0';
			char finalname[MAX_PATH];
			const char* suffixes[] = { "_g.png", "_r.png", "_s1.png", "_sx.png" };
			for (const char* suf : suffixes)
			{
				snprintf(finalname, sizeof(finalname), "%s%s", cPE, suf);
				addtocollection(finalname);
			}
		}
	}
}

