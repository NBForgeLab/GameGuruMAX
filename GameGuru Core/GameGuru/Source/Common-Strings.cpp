//----------------------------------------------------
//--- GAMEGURU - Common-Strings
//----------------------------------------------------

#include "stdafx.h"
#include "gameguru.h"

// 
//  Common Code - String and FileList Functions
// 

char* removeedgespaces ( char* text_s )
{
	if ( !text_s || text_s[0] == '\0' ) 
	{
		t.szwork[0] = '\0';
		return t.szwork;
	}
	int len = (int)strlen(text_s);
	int st = 0;
	while ( st < len && (unsigned char)text_s[st] <= 32 ) st++;
	if ( st >= len ) 
	{
		t.szwork[0] = '\0';
		return t.szwork;
	}
	int en = len - 1;
	while ( en >= st && (unsigned char)text_s[en] <= 32 ) en--;
	int outLen = en - st + 1;
	if ( outLen >= (int)sizeof(t.szwork) ) outLen = (int)sizeof(t.szwork) - 1;
	memcpy( t.szwork, text_s + st, outLen );
	t.szwork[outLen] = '\0';
	return t.szwork;
}

char* minstring ( char* text_s, int minsize )
{
	if ( !text_s ) text_s = "";
	int len = (int)strlen(text_s);
	int sizegap = minsize - len;
	if ( sizegap < 0 ) sizegap = 0;
	int totalLen = len + sizegap;
	if ( totalLen >= (int)sizeof(t.szwork) ) totalLen = (int)sizeof(t.szwork) - 1;
	int copyLen = len < totalLen ? len : totalLen;
	memcpy( t.szwork, text_s, copyLen );
	int spaces = totalLen - copyLen;
	if ( spaces > 0 ) memset( t.szwork + copyLen, ' ', spaces );
	t.szwork[totalLen] = '\0';
	return t.szwork;
}

//FUNCTIONS TO CUT-UP STRINGS

char* getfirstdir ( char* file_s )
{
	if ( !file_s || file_s[0] == '\0' )
	{
		t.szwork[0] = '\0';
		return t.szwork;
	}
	int len = (int)strlen(file_s);
	for ( int c = 0 ; c < len ; c++ )
	{
		if ( file_s[c] == '\\' || file_s[c] == '/' )
		{
			int outLen = c + 1;
			if ( outLen >= (int)sizeof(t.szwork) ) outLen = (int)sizeof(t.szwork) - 1;
			memcpy( t.szwork, file_s, outLen );
			t.szwork[outLen] = '\0';
			return t.szwork;
		}
	}
	t.szwork[0] = '\0';
	return t.szwork;
}

char* getseconddir ( char* file_s )
{
	if ( !file_s || file_s[0] == '\0' )
	{
		t.szwork[0] = '\0';
		return t.szwork;
	}
	int count = 2;
	int len = (int)strlen(file_s);
	for ( int c = 0 ; c < len ; c++ )
	{
		if ( file_s[c] == '\\' || file_s[c] == '/' )
		{
			--count;
			if ( count == 0 )
			{
				int outLen = c + 1;
				if ( outLen >= (int)sizeof(t.szwork) ) outLen = (int)sizeof(t.szwork) - 1;
				memcpy( t.szwork, file_s, outLen );
				t.szwork[outLen] = '\0';
				return t.szwork;
			}
		}
	}
	t.szwork[0] = '\0';
	return t.szwork;
}

char* getpath ( char* file_s )
{
	if ( !file_s || file_s[0] == '\0' )
	{
		t.szwork[0] = '\0';
		return t.szwork;
	}
	int len = (int)strlen(file_s);
	for ( int c = len - 1 ; c >= 0 ; c-- )
	{
		if ( file_s[c] == '\\' || file_s[c] == '/' )
		{
			int outLen = c + 1;
			if ( outLen >= (int)sizeof(t.szwork) ) outLen = (int)sizeof(t.szwork) - 1;
			memcpy( t.szwork, file_s, outLen );
			t.szwork[outLen] = '\0';
			return t.szwork;
		}
	}
	t.szwork[0] = '\0';
	return t.szwork;
}

char* getfile ( char* file_s )
{
	if ( !file_s || file_s[0] == '\0' )
	{
		t.szwork[0] = '\0';
		return t.szwork;
	}
	int len = (int)strlen(file_s);
	int lastSlash = -1;
	for ( int c = len - 1 ; c >= 0 ; c-- )
	{
		if ( file_s[c] == '\\' || file_s[c] == '/' )
		{
			lastSlash = c;
			break;
		}
	}
	const char* pFile = (lastSlash >= 0) ? (file_s + lastSlash + 1) : file_s;
	int outLen = (int)strlen(pFile);
	if ( outLen >= (int)sizeof(t.szwork) ) outLen = (int)sizeof(t.szwork) - 1;
	memcpy( t.szwork, pFile, outLen );
	t.szwork[outLen] = '\0';
	return t.szwork;
}

//File List Building

void buildfilelist ( char* folder_s, char* rel_s )
{
	int tt = 0;
	cstr file_s = "";
	int fin = 0;
	if ( PathExist(folder_s) == 1 ) 
	{
		SetDir ( folder_s );
		FindFirst ( ); fin = 0;
		while ( GetFileType() > -1 ) 
		{
			file_s = GetFileName();
			if ( file_s == "." || file_s == ".." ) 
			{
				// ignore . and ..
			}
			else
			{
				if ( GetFileType() == 1 ) 
				{
					// folder
					buildfilelist( file_s.Get() , cstr(cstr(rel_s) + file_s + "\\").Get() );
					FindFirst ( );
					if ( fin > 0 ) 
					{
						for ( tt = 1 ; tt <= fin ; tt++ )
						{
							if ( GetFileType() > -1 ) 
							{
								FindNext ( );
							}
						}
					}
				}
				else
				{
					// file
					int size = ArrayCount( t.filelist_s ); if ( size < 1 ) size = 1; else size++;
					Redim ( t.filelist_s , size );					
					t.filelist_s[size-1] = cstr(rel_s) + file_s;
				}
			}
			FindNext ( );
			fin = fin + 1;
		}
		SetDir ( ".." );
	}
}
