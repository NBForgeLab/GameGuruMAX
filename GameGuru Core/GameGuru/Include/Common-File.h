//----------------------------------------------------
//--- GAMEGURU - Common-File
//----------------------------------------------------

#pragma once

#include <string>
#include <cstring>
#include <cctype>

// Pure string/path/version utility functions
inline std::string ExtractFileName ( const std::string& path )
{
	if ( path.empty() ) return "";
	size_t lastSlash = path.find_last_of("\\/");
	if ( lastSlash == std::string::npos ) return path;
	return path.substr(lastSlash + 1);
}

inline std::string ExtractPath ( const std::string& path )
{
	if ( path.empty() ) return "";
	size_t lastSlash = path.find_last_of("\\/");
	if ( lastSlash == std::string::npos ) return "";
	return path.substr(0, lastSlash + 1);
}

inline int versionnumberislarger ( int tNum1, int tNum2 )
{
	std::string s1 = std::to_string(tNum1);
	std::string s2 = std::to_string(tNum2);
	return (s1 > s2) ? 1 : 0;
}

inline int getdatevaluefromdatestring ( const char* tdayofyear_s )
{
	if ( !tdayofyear_s || tdayofyear_s[0] == '\0' )
		return 0;

	size_t full_len = strlen(tdayofyear_s);
	if ( full_len < 20 )
		return 0;

	// Trim trailing whitespace and newlines
	while ( full_len > 0 && (tdayofyear_s[full_len - 1] == ' ' || 
	                         tdayofyear_s[full_len - 1] == '\r' || 
	                         tdayofyear_s[full_len - 1] == '\n' || 
	                         tdayofyear_s[full_len - 1] == '\t') )
	{
		--full_len;
	}
	if ( full_len < 20 )
		return 0;

	// Year is the last 4 characters
	int year = 0;
	for ( size_t i = full_len - 4; i < full_len; ++i )
	{
		if ( tdayofyear_s[i] < '0' || tdayofyear_s[i] > '9' ) return 0;
		year = year * 10 + (tdayofyear_s[i] - '0');
	}

	size_t prefix_len = full_len - 5; // e.g. "Wed Jun 30 21:49:08"

	// Skip day of week (up to first space)
	size_t idx = 0;
	while ( idx < prefix_len && tdayofyear_s[idx] != ' ' ) ++idx;
	while ( idx < prefix_len && tdayofyear_s[idx] == ' ' ) ++idx;
	if ( idx >= prefix_len ) return 0;

	// Month: 3 letters
	if ( idx + 3 > prefix_len ) return 0;
	char m0 = (char)tolower((unsigned char)tdayofyear_s[idx]);
	char m1 = (char)tolower((unsigned char)tdayofyear_s[idx + 1]);
	char m2 = (char)tolower((unsigned char)tdayofyear_s[idx + 2]);
	idx += 3;

	int month = 0;
	if      ( m0 == 'j' && m1 == 'a' && m2 == 'n' ) month = 1;
	else if ( m0 == 'f' && m1 == 'e' && m2 == 'b' ) month = 2;
	else if ( m0 == 'm' && m1 == 'a' && m2 == 'r' ) month = 3;
	else if ( m0 == 'a' && m1 == 'p' && m2 == 'r' ) month = 4;
	else if ( m0 == 'm' && m1 == 'a' && m2 == 'y' ) month = 5;
	else if ( m0 == 'j' && m1 == 'u' && m2 == 'n' ) month = 6;
	else if ( m0 == 'j' && m1 == 'u' && m2 == 'l' ) month = 7;
	else if ( m0 == 'a' && m1 == 'u' && m2 == 'g' ) month = 8;
	else if ( m0 == 's' && m1 == 'e' && m2 == 'p' ) month = 9;
	else if ( m0 == 'o' && m1 == 'c' && m2 == 't' ) month = 10;
	else if ( m0 == 'n' && m1 == 'o' && m2 == 'v' ) month = 11;
	else if ( m0 == 'd' && m1 == 'e' && m2 == 'c' ) month = 12;
	else return 0;

	// Skip spaces to date
	while ( idx < prefix_len && tdayofyear_s[idx] == ' ' ) ++idx;
	if ( idx >= prefix_len ) return 0;

	// Day of month
	int day = 0;
	while ( idx < prefix_len && tdayofyear_s[idx] >= '0' && tdayofyear_s[idx] <= '9' )
	{
		day = day * 10 + (tdayofyear_s[idx] - '0');
		++idx;
	}

	// Skip spaces to time
	while ( idx < prefix_len && tdayofyear_s[idx] == ' ' ) ++idx;
	if ( idx >= prefix_len ) return 0;

	// Time: HH:MM[:SS] -> extract HH and MM
	int hour = 0;
	while ( idx < prefix_len && tdayofyear_s[idx] >= '0' && tdayofyear_s[idx] <= '9' )
	{
		hour = hour * 10 + (tdayofyear_s[idx] - '0');
		++idx;
	}
	if ( idx < prefix_len && tdayofyear_s[idx] == ':' ) ++idx;
	int minute = 0;
	while ( idx < prefix_len && tdayofyear_s[idx] >= '0' && tdayofyear_s[idx] <= '9' )
	{
		minute = minute * 10 + (tdayofyear_s[idx] - '0');
		++idx;
	}

	int iAddTime = hour * 100 + minute;
	int filedatevalue = ((year - 2004) * 12 * 32 * 2500) + (month * 32 * 2500) + (day * 2500) + iAddTime;
	return filedatevalue;
}

// Engine file system operations (implemented in Common-File.cpp)
void file_createmydocsfolder ( void );
int isbinaryfileolderthantxtfile ( const char* binaryfilefull_s, const char* textfilefull_s );
int isbinaryfileolderthantxtfileforseg ( const char* binaryfilefull_s, const char* textfilefull_s );
int isbinaryfileolderthanglobalversion ( const char* binaryfilefull_s );
void deleteOutOfDateDBO ( const char* tfile_s );
