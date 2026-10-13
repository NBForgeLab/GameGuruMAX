//----------------------------------------------------
//--- GAMEGURU - Common-File
//----------------------------------------------------

#include "stdafx.h"
#include "gameguru.h"
#include "Common-File.h"
#include <string>
#include <algorithm>
#include <cctype>

// 
//  Common File Functions
// 

void file_createmydocsfolder ( void )
{
	//  create a GameGuru folder inside MyDocs
	char* pOldDir = GetDir();
	std::string told_s = pOldDir ? pOldDir : "";
	SetDir ( g.mydocumentsdir_s.Get() );
	if ( PathExist(g.myfpscfiles_s.Get()) == 0 ) MakeDirectory ( g.myfpscfiles_s.Get() );
	if ( !told_s.empty() ) SetDir ( const_cast<char*>(told_s.c_str()) );
	if ( PathExist(g.myownrootdir_s.Get()) == 0 ) 
	{
		char szwork[2048];
		snprintf ( szwork, sizeof(szwork), "Cannot create :%s", g.myownrootdir_s.Get() );
		ExitPrompt ( szwork, const_cast<char*>("Path Error") );
		ExitProcess ( 0 );
	}
}

int isbinaryfileolderthantxtfile ( const char* binaryfilefull_s, const char* textfilefull_s )
{
	if ( !binaryfilefull_s || !*binaryfilefull_s || !textfilefull_s || !*textfilefull_s ) return 0;
	if ( FileExist(const_cast<char*>(binaryfilefull_s)) != 1 ) return 0;

	std::string binaryfile_s = ExtractFileName(binaryfilefull_s);
	std::string textfile_s = ExtractFileName(textfilefull_s);
	std::string thispath_s = ExtractPath(binaryfilefull_s);

	char* pOldDir = nullptr;
	std::string stdir_s;
	if ( !thispath_s.empty() )
	{
		if ( PathExist(const_cast<char*>(thispath_s.c_str())) != 1 )
		{
			return 0;
		}
		pOldDir = GetDir();
		stdir_s = pOldDir ? pOldDir : "";
		SetDir(const_cast<char*>(thispath_s.c_str()));
	}

	int filedatevalueB = 0;
	int filedatevalueT = 0;
	int result = 0;

	FindFirst();
	while ( GetFileType() != -1 )
	{
		if ( GetFileType() == 0 )
		{
			char* pCurrentName = GetFileName();
			if ( pCurrentName )
			{
				if ( _stricmp(pCurrentName, binaryfile_s.c_str()) == 0 )
				{
					char* pFileDate = GetFileDate();
					filedatevalueB = getdatevaluefromdatestring(pFileDate);

					char* pCreationDate = GetFileCreation();
					int filedatevalueC = getdatevaluefromdatestring(pCreationDate);
					if ( filedatevalueC > filedatevalueB ) filedatevalueB = filedatevalueC;
				}
				else if ( _stricmp(pCurrentName, textfile_s.c_str()) == 0 )
				{
					char* pFileDate = GetFileDate();
					filedatevalueT = getdatevaluefromdatestring(pFileDate);

					char* pCreationDate = GetFileCreation();
					int filedatevalueC = getdatevaluefromdatestring(pCreationDate);
					if ( filedatevalueC > filedatevalueT ) filedatevalueT = filedatevalueC;
				}
			}
			if ( filedatevalueB != 0 && filedatevalueT != 0 ) break;
		}
		FindNext();
	}

	if ( filedatevalueB != 0 && filedatevalueT != 0 )
	{
		if ( filedatevalueB < filedatevalueT )
		{
			result = 1;
		}
	}

	if ( !stdir_s.empty() )
	{
		SetDir(const_cast<char*>(stdir_s.c_str()));
	}

	return result;
}

int isbinaryfileolderthantxtfileforseg ( const char* binaryfilefull_s, const char* textfilefull_s )
{
	if ( !binaryfilefull_s || !*binaryfilefull_s || !textfilefull_s || !*textfilefull_s ) return 0;
	if ( FileExist(const_cast<char*>(binaryfilefull_s)) != 1 ) return 0;

	std::string binaryfile_s = ExtractFileName(binaryfilefull_s);
	std::string textfile_s = ExtractFileName(textfilefull_s);
	std::string thispath_s = ExtractPath(binaryfilefull_s);

	char* pOldDir = nullptr;
	std::string stdir_s;
	if ( !thispath_s.empty() )
	{
		if ( PathExist(const_cast<char*>(thispath_s.c_str())) != 1 )
		{
			return 0;
		}
		pOldDir = GetDir();
		stdir_s = pOldDir ? pOldDir : "";
		SetDir(const_cast<char*>(thispath_s.c_str()));
	}

	int filedatevalueB = 0;
	int filedatevalueT = 0;
	int result = 0;

	FindFirst();
	while ( GetFileType() != -1 )
	{
		if ( GetFileType() == 0 )
		{
			char* pCurrentName = GetFileName();
			if ( pCurrentName )
			{
				if ( _stricmp(pCurrentName, binaryfile_s.c_str()) == 0 )
				{
					char* pFileDate = GetFileDate();
					filedatevalueB = getdatevaluefromdatestring(pFileDate);
				}
				else if ( _stricmp(pCurrentName, textfile_s.c_str()) == 0 )
				{
					char* pFileDate = GetFileDate();
					filedatevalueT = getdatevaluefromdatestring(pFileDate);
				}
			}
			if ( filedatevalueB != 0 && filedatevalueT != 0 ) break;
		}
		FindNext();
	}

	if ( filedatevalueB != 0 && filedatevalueT != 0 )
	{
		if ( filedatevalueB < filedatevalueT )
		{
			result = 1;
		}
	}

	if ( !stdir_s.empty() )
	{
		SetDir(const_cast<char*>(stdir_s.c_str()));
	}

	if ( result == 0 )
	{
		// V110 - 160508 - added extra code to hold new version number in segment BIN file
		OpenToRead(3, const_cast<char*>(binaryfilefull_s));
		int tval = ReadLong(3);
		if ( tval == -1 )
		{
			int tversion = ReadLong(3);
			// if BIN created with older version, trigger delete BIN file
			result = versionnumberislarger(g.gversion, tversion);
		}
		else
		{
			// if an old BIN, also delete it
			result = 1;
		}
		CloseFile(3);
	}

	return result;
}

int isbinaryfileolderthanglobalversion ( const char* binaryfilefull_s )
{
	if ( !binaryfilefull_s || !*binaryfilefull_s ) return 0;
	if ( FileExist(const_cast<char*>(binaryfilefull_s)) != 1 ) return 0;

	int result = 0;
	OpenToRead(3, const_cast<char*>(binaryfilefull_s));
	char* pVal = ReadString(3);
	std::string tval_s = pVal ? pVal : "";
	if ( _stricmp(tval_s.c_str(), "version") == 0 )
	{
		int tversion = ReadLong(3);
		result = versionnumberislarger(g.gentityprofileversion, tversion);
		if ( result == 1 )
		{
			char szwork[2048];
			snprintf(szwork, sizeof(szwork), "refreshing BIN file for:%s (from %i to %i)", binaryfilefull_s, tversion, g.gentityprofileversion);
			timestampactivity(0, szwork);
		}
	}
	else
	{
		// if an old BIN, also delete it
		result = 1;
	}
	CloseFile(3);

	return result;
}

void deleteOutOfDateDBO ( const char* tfile_s )
{
	if ( !tfile_s || !*tfile_s ) return;

	std::string file_str = tfile_s;
	size_t len = file_str.length();
	std::string tfilex_s;
	std::string tfiledbo_s;

	// Case-insensitive check of extension
	if ( len >= 2 && _stricmp(file_str.c_str() + len - 2, ".x") == 0 )
	{
		tfilex_s = file_str;
		tfiledbo_s = file_str.substr(0, len - 2) + ".dbo";
	}
	else if ( len >= 4 && _stricmp(file_str.c_str() + len - 4, ".dbo") == 0 )
	{
		tfiledbo_s = file_str;
		tfilex_s = file_str.substr(0, len - 4) + ".x";
	}
	else
	{
		return;
	}

	if ( FileExist(const_cast<char*>(tfilex_s.c_str())) == 1 && 
	     FileExist(const_cast<char*>(tfiledbo_s.c_str())) == 1 )
	{
		if ( isbinaryfileolderthantxtfile(tfiledbo_s.c_str(), tfilex_s.c_str()) == 1 )
		{
			DeleteAFile(const_cast<char*>(tfiledbo_s.c_str()));
		}
	}
}
