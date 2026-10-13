//----------------------------------------------------
//--- GAMEGURU - Common-Strings
//----------------------------------------------------

#pragma once

#include "cstr.h"
#include <string>
#include <string_view>
#include <vector>

// Modern reentrant string and path utilities (return std::string, accept std::string_view)
std::string removeedgespaces_str ( std::string_view text_s );
std::string minstring_str ( std::string_view text_s, int minsize );
std::string getfirstdir_str ( std::string_view file_s );
std::string getseconddir_str ( std::string_view file_s );
std::string getpath_str ( std::string_view file_s );
std::string getfile_str ( std::string_view file_s );

// Modern reentrant directory scanner
void buildfilelist ( std::string_view folder_s, std::string_view rel_s, std::vector<std::string>& outFiles );
void buildfilelist ( std::string_view folder_s, std::string_view rel_s, std::vector<cstr>& outFiles );
void buildfilelist ( std::string_view folder_s, std::vector<std::string>& outFiles );
void buildfilelist ( std::string_view folder_s, std::vector<cstr>& outFiles );

// Legacy reentrant, thread-safe string and path API (uses per-thread rotation pool, zero scratchpads)
char* removeedgespaces ( const char* text_s );
char* minstring ( const char* text_s, int minsize );
char* getfirstdir ( const char* file_s );
char* getseconddir ( const char* file_s );
char* getpath ( const char* file_s );
char* getfile ( const char* file_s );

// Legacy directory scanner populating global filelist_s when available
void buildfilelist ( const char* folder_s, const char* rel_s );
