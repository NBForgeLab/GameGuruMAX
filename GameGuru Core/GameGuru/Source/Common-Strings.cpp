//----------------------------------------------------
//--- GAMEGURU - Common-Strings
//----------------------------------------------------

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#if defined(__has_include)
  #if __has_include("stdafx.h")
    #include "stdafx.h"
  #endif
  #if __has_include("gameguru.h") && !defined(GAMEGURU_NO_T)
    #include "gameguru.h"
    #define HAS_GAMEGURU_T 1
  #endif
#else
  #include "stdafx.h"
  #include "gameguru.h"
  #define HAS_GAMEGURU_T 1
#endif

#if __has_include("Common-Strings.h")
  #include "Common-Strings.h"
#elif __has_include("../Include/Common-Strings.h")
  #include "../Include/Common-Strings.h"
#else
  #include "Common-Strings.h"
#endif

#include <string>
#include <string_view>
#include <vector>
#include <cstring>

// 
//  Common Code - String and FileList Functions
// 

namespace
{
	constexpr size_t TLS_STRING_POOL_MAX = 32;
	static thread_local std::string s_commonStringsTlsPool[TLS_STRING_POOL_MAX];
	static thread_local size_t s_commonStringsTlsIndex = 0;

	char* ReturnTlsPooledString(std::string_view sv)
	{
		size_t slot = s_commonStringsTlsIndex;
		s_commonStringsTlsIndex = (s_commonStringsTlsIndex + 1) % TLS_STRING_POOL_MAX;
		if (sv.empty())
		{
			s_commonStringsTlsPool[slot].clear();
		}
		else
		{
			s_commonStringsTlsPool[slot].assign(sv.data(), sv.size());
		}
		return s_commonStringsTlsPool[slot].data();
	}
}

std::string removeedgespaces_str ( std::string_view text_s )
{
	if ( text_s.empty() ) return std::string();
	size_t st = 0;
	const size_t len = text_s.size();
	while ( st < len && static_cast<unsigned char>(text_s[st]) <= 32 ) ++st;
	if ( st >= len ) return std::string();
	size_t en = len - 1;
	while ( en > st && static_cast<unsigned char>(text_s[en]) <= 32 ) --en;
	return std::string(text_s.substr(st, en - st + 1));
}

char* removeedgespaces ( const char* text_s )
{
	if ( !text_s ) return ReturnTlsPooledString(std::string_view());
	return ReturnTlsPooledString(removeedgespaces_str(text_s));
}

std::string minstring_str ( std::string_view text_s, int minsize )
{
	if ( minsize <= 0 ) return std::string(text_s);
	std::string res(text_s);
	if ( static_cast<int>(res.size()) < minsize )
	{
		res.append(static_cast<size_t>(minsize - static_cast<int>(res.size())), ' ');
	}
	return res;
}

char* minstring ( const char* text_s, int minsize )
{
	if ( !text_s ) text_s = "";
	return ReturnTlsPooledString(minstring_str(text_s, minsize));
}

//FUNCTIONS TO CUT-UP STRINGS

std::string getfirstdir_str ( std::string_view file_s )
{
	if ( file_s.empty() ) return std::string();
	size_t pos = file_s.find_first_of("/\\");
	if ( pos != std::string_view::npos )
	{
		return std::string(file_s.substr(0, pos + 1));
	}
	return std::string();
}

char* getfirstdir ( const char* file_s )
{
	if ( !file_s ) return ReturnTlsPooledString(std::string_view());
	return ReturnTlsPooledString(getfirstdir_str(file_s));
}

std::string getseconddir_str ( std::string_view file_s )
{
	if ( file_s.empty() ) return std::string();
	size_t first = file_s.find_first_of("/\\");
	if ( first != std::string_view::npos && first + 1 < file_s.size() )
	{
		size_t second = file_s.find_first_of("/\\", first + 1);
		if ( second != std::string_view::npos )
		{
			return std::string(file_s.substr(0, second + 1));
		}
	}
	return std::string();
}

char* getseconddir ( const char* file_s )
{
	if ( !file_s ) return ReturnTlsPooledString(std::string_view());
	return ReturnTlsPooledString(getseconddir_str(file_s));
}

std::string getpath_str ( std::string_view file_s )
{
	if ( file_s.empty() ) return std::string();
	size_t pos = file_s.find_last_of("/\\");
	if ( pos != std::string_view::npos )
	{
		return std::string(file_s.substr(0, pos + 1));
	}
	return std::string();
}

char* getpath ( const char* file_s )
{
	if ( !file_s ) return ReturnTlsPooledString(std::string_view());
	return ReturnTlsPooledString(getpath_str(file_s));
}

std::string getfile_str ( std::string_view file_s )
{
	if ( file_s.empty() ) return std::string();
	size_t pos = file_s.find_last_of("/\\");
	if ( pos != std::string_view::npos )
	{
		return std::string(file_s.substr(pos + 1));
	}
	return std::string(file_s);
}

char* getfile ( const char* file_s )
{
	if ( !file_s ) return ReturnTlsPooledString(std::string_view());
	return ReturnTlsPooledString(getfile_str(file_s));
}

//File List Building

namespace
{
	constexpr int MAX_DIR_RECURSION_DEPTH = 32;

	template<typename Container>
	void BuildFileListRecursive(std::string_view searchFolder, std::string_view relPrefix, Container& outFiles, int depth = 0)
	{
		if (searchFolder.empty() || depth >= MAX_DIR_RECURSION_DEPTH) return;

		std::string searchPattern(searchFolder);
		if (!searchPattern.empty() && searchPattern.back() != '\\' && searchPattern.back() != '/')
		{
			searchPattern += "\\*.*";
		}
		else
		{
			searchPattern += "*.*";
		}

		WIN32_FIND_DATAA findData;
		HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);
		if (hFind == INVALID_HANDLE_VALUE) return;

		do
		{
			const char* name = findData.cFileName;
			if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
			{
				continue;
			}

			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// Skip directory junctions / symlinks to prevent cyclic infinite recursion
				if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
				{
					continue;
				}

				std::string subFolder(searchFolder);
				if (!subFolder.empty() && subFolder.back() != '\\' && subFolder.back() != '/')
				{
					subFolder += "\\";
				}
				subFolder += name;

				std::string subRel(relPrefix);
				subRel += name;
				subRel += "\\";

				BuildFileListRecursive(subFolder, subRel, outFiles, depth + 1);
			}
			else
			{
				std::string filePath(relPrefix);
				filePath += name;
				outFiles.emplace_back(std::move(filePath));
			}
		} while (FindNextFileA(hFind, &findData) != 0);

		FindClose(hFind);
	}
}

void buildfilelist ( std::string_view folder_s, std::string_view rel_s, std::vector<std::string>& outFiles )
{
	BuildFileListRecursive(folder_s, rel_s, outFiles);
}

void buildfilelist ( std::string_view folder_s, std::string_view rel_s, std::vector<cstr>& outFiles )
{
	BuildFileListRecursive(folder_s, rel_s, outFiles);
}

void buildfilelist ( std::string_view folder_s, std::vector<std::string>& outFiles )
{
	BuildFileListRecursive(folder_s, "", outFiles);
}

void buildfilelist ( std::string_view folder_s, std::vector<cstr>& outFiles )
{
	BuildFileListRecursive(folder_s, "", outFiles);
}

void buildfilelist ( const char* folder_s, const char* rel_s )
{
#if defined(HAS_GAMEGURU_T)
	std::vector<std::string> localFiles;
	buildfilelist(folder_s ? std::string_view(folder_s) : std::string_view(),
	              rel_s ? std::string_view(rel_s) : std::string_view(),
	              localFiles);
	if (!localFiles.empty())
	{
		int existing = ArrayCount(t.filelist_s);
		int startIndex = (existing > 0) ? existing : ((!t.filelist_s.empty() && !t.filelist_s[0].empty()) ? 1 : 0);
		int newCount = startIndex + static_cast<int>(localFiles.size());
		Redim(t.filelist_s, newCount);
		for (size_t i = 0; i < localFiles.size(); ++i)
		{
			t.filelist_s[startIndex + static_cast<int>(i)] = localFiles[i];
		}
		t.filelist_s[newCount] = "";
	}
#else
	(void)folder_s;
	(void)rel_s;
#endif
}
