//
// cStr code - Modern C++ Implementation
//

// Includes 
#include "stdafx.h"
#include "cStr.h"
#include "windows.h"
#include <map>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <ctype.h>

// External Includes
#include "globstruct.h"
#include "..\..\Dark Basic Public Shared\Dark Basic Pro SDK\DarkSDK\Core\resource.h"
#include "..\..\Dark Basic Public Shared\Dark Basic Pro SDK\Shared\Core\SteamCheckForWorkshop.h"

extern "C" FILE* GG_fopen( const char* filename, const char* mode );

#include "cStrCore.inl"

int ArrayCount2 ( const std::vector<std::vector<cstr>>& array )
{
	if ( array.empty() || array[0].empty() ) return -1;
	return ( (int) array.size() * (int) array[0].size() ) - 1;
}

cstr ArrayAt ( const std::vector<std::vector<cstr>>& array, int l )
{
	if ( array.empty() ) return cstr("");
	int a, b;
	b = (int)(floor((float)l / (float)array.size()));
	a = l - (b * (int)array.size());
	if ( a >= 0 && a < (int)array.size() && b >= 0 && b < (int)array[a].size() )
		return array[a][b];
	return cstr("");
}

void SaveArray ( const char* filename , const std::vector<cstr>& array )
{
	if ( !filename ) return;
	DeleteFileA ( filename );

	FILE* file = GG_fopen ( filename , "w" );
	if ( file )
	{
		for ( size_t c = 0 ; c < array.size() ; c++ )
		{
			fputs ( array[c].c_str() , file );
			fputs ( "\n" , file );
		}
		fclose ( file );
	}
}

void SaveArray ( const char* filename , const std::vector<std::vector<cstr>>& array )
{
	if ( !filename || array.empty() ) return;
	size_t size = array[0].size();

	DeleteFileA ( filename );

	FILE* file = GG_fopen ( filename , "w" );
	if ( file )
	{		
		for ( size_t b = 0 ; b < size ; b++ )
		{
			for ( size_t a = 0 ; a < array.size() ; a++ )
			{
				fputs ( array[a][b].c_str() , file );
				fputs ( "\n" , file );
			}
		}
		fclose ( file );
	}
}

void LoadArray ( const char* filename , std::vector<cstr>& array )
{
	if ( !filename ) return;
	char VirtualFilename[_MAX_PATH];
	strncpy_s(VirtualFilename, filename, _TRUNCATE);

	CheckForWorkshopFile ( VirtualFilename );
	g_pGlob->Decrypt( VirtualFilename );

	FILE* file = GG_fopen ( VirtualFilename , "r" );
	if ( file )
	{
		std::vector<std::string> lines;
		char t[4096];
		while ( fgets( t, sizeof(t) - 1, file ) )
		{
			size_t len = strlen(t);
			while ( len > 0 && (t[len - 1] == '\r' || t[len - 1] == '\n') )
			{
				t[--len] = '\0';
			}
			lines.emplace_back(t);
		}
		fclose ( file );

		if ( !lines.empty() && lines.back().empty() )
			lines.pop_back();

		if ( lines.size() > array.size() )
			array.resize( lines.size() );

		for ( size_t c = 0 ; c < lines.size() ; c++ )
		{
			array[c] = lines[c];
		}
	}

	g_pGlob->Encrypt( VirtualFilename );
}

void LoadArray ( const char* filename , std::vector<std::vector<cstr>>& array )
{
	if ( !filename || array.empty() ) return;
	char VirtualFilename[_MAX_PATH];
	strncpy_s(VirtualFilename, filename, _TRUNCATE);

	CheckForWorkshopFile ( VirtualFilename );
	g_pGlob->Decrypt( VirtualFilename );

	FILE* file = GG_fopen ( VirtualFilename , "r" );
	if ( file )
	{
		size_t size = array[0].size();
		char t[4096];
		for ( size_t b = 0 ; b < size ; b++ )
		{
			for ( size_t a = 0 ; a < array.size() ; a++ )
			{
				if ( fgets( t, sizeof(t) - 1, file ) )
				{
					size_t len = strlen(t);
					while ( len > 0 && (t[len - 1] == '\r' || t[len - 1] == '\n') )
					{
						t[--len] = '\0';
					}
					array[a][b] = t;
				}
			}
		}
		fclose ( file );
	}

	g_pGlob->Encrypt( VirtualFilename );
}

// MATHEMATICAL COMMANDS
extern double gDegToRad;
extern double gRadToDeg;
float atan2deg( float fA , float fB ) { return (float)(atan2(fA, fB)*gRadToDeg); }

// Other string functions
void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
}

static std::string s_soundexTarget;
static std::string removeChars(std::string s, int pos, bool f(char c))
{
	if (pos >= (int)s.size()) return s;
	std::string::iterator end = std::remove_if(s.begin() + pos, s.end(), f);
	return s.substr(0, end - s.begin());
}
static bool isIn(char c) { return s_soundexTarget.find(toupper(c)) != std::string::npos; }
static bool notAlpha(char c) { return !isalpha((unsigned char)c); }
static bool notAlphaPlusSpace(char c) { return c != ' ' && !isalpha((unsigned char)c); }

static std::string soundex(std::string word)
{
	std::map<char, char> cmap = { {'B','1'}, {'F','1'}, {'P','1'}, {'V','1'},
							{'C','2'}, {'G','2'}, {'J','2'}, {'K','2'}, {'Q','2'}, {'S','2'}, {'X','2'}, {'Z','2'},
							{'D','3'}, {'T','3'},
							{'L','4'},
							{'M','5'}, {'N','5'},
							{'R','6'} };

	// Remove non-alphabetic characters
	std::string result = removeChars(word, 0, notAlpha);

	// Put in upper case
	for (char &c : result) c = (char)toupper((unsigned char)c);

	if (result.empty()) return "0000";

	// Store first character
	char firstLetter = result[0];

	// Remove all occurrences of H and W except first letter
	s_soundexTarget = "HW";   result = removeChars(result, 1, isIn);

	// Map consonants to digits
	for (char &c : result) { if (cmap.count(c)) c = cmap[c]; }

	// Replace all adjacent same digits with one digit
	for (size_t i = 1; i < result.size(); i++) if (result[i] == result[i - 1]) result[i - 1] = '*';
	s_soundexTarget = "*";   result = removeChars(result, 1, isIn);

	// Remove all occurrences of AEIOUY except first letter
	s_soundexTarget = "AEIOUY";   result = removeChars(result, 1, isIn);

	// Replace first letter
	result[0] = firstLetter;

	// Get correct length and return
	result += "000";
	return result.substr(0, 4);
}

std::string soundexall(std::string all)
{
	std::string result = "";
	std::string search = removeChars(all, 0, notAlphaPlusSpace);
	std::istringstream iss(search);
	std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{} };
	for (size_t i = 0; i < tokens.size(); i++)
	{
		result += soundex(tokens[i]);
		if (i < tokens.size() - 1)
			result += " ";
	}
	return result;
}
