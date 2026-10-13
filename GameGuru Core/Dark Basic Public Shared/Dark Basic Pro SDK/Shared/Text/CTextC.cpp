//
// Text Code (Module Core) - Modernized
//

// Includes
#include "stdio.h"
#include <string>
#include <vector>
#include "ctextc.h"
#include ".\..\..\Dark Basic Public Shared\Dark Basic Pro SDK\Shared\Error\CError.h"
#include "globstruct.h"

// Defines
#define RETURNSTRONGPOOLMAX 40

// Globals
char*			m_szTokenString = NULL;
unsigned int	m_dwTokenStringSize = 0;
unsigned int	m_dwTEXTWorkStringSize = 0;
char*			m_pTEXTWorkString = NULL;
char			g_szReturnStringPool[RETURNSTRONGPOOLMAX][1024];
int				g_iReturnStringPoolCount = 0;

int currentringbuffer = 0;
char ReturnString[20][1024];

// Module Utility Functions

void InitCTextC( void )
{
	if (!m_szTokenString)
	{
		m_dwTokenStringSize = 1024;
		m_szTokenString = new char[1024];
		memset(m_szTokenString, 0, 1024);
	}
	if (!m_pTEXTWorkString)
	{
		m_dwTEXTWorkStringSize = 2048;
		m_pTEXTWorkString = new char[2048];
		memset(m_pTEXTWorkString, 0, 2048);
	}
}

char* GetReturnStringFromTEXTWorkString(char* WorkString = m_pTEXTWorkString)
{
	if (WorkString)
	{
		static thread_local std::string s_returnPool[RETURNSTRONGPOOLMAX];
		static thread_local size_t s_returnPoolIndex = 0;

		size_t slot = s_returnPoolIndex;
		s_returnPoolIndex = (s_returnPoolIndex + 1) % RETURNSTRONGPOOLMAX;

		s_returnPool[slot] = WorkString;

		// Maintain legacy pool boundedly (max 1023 bytes) for binary/symbol inspection
		size_t legacySlot = (size_t)(g_iReturnStringPoolCount >= 0 && g_iReturnStringPoolCount < RETURNSTRONGPOOLMAX ? g_iReturnStringPoolCount : 0);
		if (++g_iReturnStringPoolCount >= RETURNSTRONGPOOLMAX) g_iReturnStringPoolCount = 0;
		size_t copyLen = strlen(WorkString);
		if (copyLen >= 1024) copyLen = 1023;
		memcpy(g_szReturnStringPool[legacySlot], WorkString, copyLen);
		g_szReturnStringPool[legacySlot][copyLen] = '\0';

		return s_returnPool[slot].data();
	}
	return const_cast<char*>("");
}

void ValidateWorkStringBySize ( unsigned int dwSize )
{
	if (m_pTEXTWorkString)
	{
		if (m_dwTEXTWorkStringSize < dwSize)
		{
			delete[] m_pTEXTWorkString;
			m_pTEXTWorkString = NULL;
		}
	}

	// create new work string of good size
	if ( m_pTEXTWorkString==NULL )
	{
		m_dwTEXTWorkStringSize = dwSize + 512;
		m_pTEXTWorkString = new char[m_dwTEXTWorkStringSize];
		memset ( m_pTEXTWorkString, 0, m_dwTEXTWorkStringSize );
	}
}

void ValidateWorkString(char* pString)
{
	if ( pString ) ValidateWorkStringBySize ( (unsigned int)strlen(pString) + 1 );
}

// Module Functions

int	Asc ( char* dwSrcStr )
{
	if(dwSrcStr)
		return (int)*dwSrcStr;
	else
		return 0;
}

char* Bin ( char* pDestStr, int iValue )
{
	ValidateWorkStringBySize(64);
	char* text=m_pTEXTWorkString;
	int t=0;
	text[t++]='%';
	for(int bit=31; bit>=0; bit--)
	{
		unsigned int mask = 1 << bit;
		if(iValue & mask)
			text[t++]='1';
		else
			text[t++]='0';
	}
	text[t]=0;

	if(pDestStr && g_pGlob && g_pGlob->CreateDeleteString) g_pGlob->CreateDeleteString((char**)&pDestStr, 0);
	char* pReturnString=GetReturnStringFromTEXTWorkString();
	return pReturnString;
}

char* Chr ( int iValue )
{
	ValidateWorkStringBySize(4);
	m_pTEXTWorkString[0]=(char)iValue;
	m_pTEXTWorkString[1]=0;

	char* pReturnString=GetReturnStringFromTEXTWorkString();
	return pReturnString;
}

char* Hex ( char* pDestStr, int iValue )
{
	ValidateWorkStringBySize(64);
	wsprintf(m_pTEXTWorkString, "%X", iValue);

	if(pDestStr && g_pGlob && g_pGlob->CreateDeleteString) g_pGlob->CreateDeleteString((char**)&pDestStr, 0);
	char* pReturnString=GetReturnStringFromTEXTWorkString();
	return pReturnString;
}

char* Left ( char* szText, int iValue )
{
	ValidateWorkString ( (char*)szText );
	char* text=m_pTEXTWorkString;
	if(szText)
		strcpy(text, (char*)szText);
	else
		strcpy(text, "");

	if(iValue>0 && iValue<=(int)strlen(text))
		text[iValue]=0;
	else
		if(iValue<=0)
			strcpy(text, "");

	char* pReturnString=GetReturnStringFromTEXTWorkString();
	return pReturnString;
}

int Len ( char* dwSrcStr )
{
	if(dwSrcStr)
		return (int)strlen(dwSrcStr);
	else
		return 0;
}

char* Lower ( char* szText )
{
	ValidateWorkString ( (char*)szText );
	if(szText)
		strcpy(m_pTEXTWorkString, (char*)szText);
	else
		strcpy(m_pTEXTWorkString, "");

	_strlwr(m_pTEXTWorkString);

	char* pReturnString=GetReturnStringFromTEXTWorkString();
	return pReturnString;
}

char* Mid ( char* szText, int iValue )
{
	ValidateWorkString ( szText );
	char character;
	char* text = m_pTEXTWorkString;
	if(szText)
		strcpy_s(text, m_dwTEXTWorkStringSize, szText);
	else
		strcpy_s(text, 2, "");

	unsigned int index = iValue;
	if(index>0 && index<=(unsigned int)strlen(text))
	{
		character=text[index-1];	
		text[0]=character;
		text[1]=0;
	}
	else
		text[0]=0;

	char* pReturnString=GetReturnStringFromTEXTWorkString();
	return pReturnString;
}

char* Right ( char* szText, int iValue )
{
	ValidateWorkString ( (char*)szText );
	char* text=m_pTEXTWorkString;
	if(szText)
		strcpy(text, (char*)szText);
	else
		strcpy(text, "");

	int w = 0;
	int length = (int)strlen(text);
	int rightmost = length-iValue;
	if(rightmost>=0 && rightmost<=length)
	{
		for(int n=rightmost; n<length; n++)
			text[w++]=text[n];
		text[w]=0;
	}
	else
		if(rightmost>length)
			strcpy(text, "");
		
	char* pReturnString=GetReturnStringFromTEXTWorkString();
	return pReturnString;
}

char* Str ( float fValue )
{
	ValidateWorkStringBySize(64);
	sprintf(m_pTEXTWorkString, "%.12g", fValue);

	char* pReturnString=GetReturnStringFromTEXTWorkString();
	return pReturnString;
}

char* StrEx ( float fValue, int iDecPlaces )
{
	ValidateWorkStringBySize(64 + (iDecPlaces > 0 ? iDecPlaces : 0));
	char format[32];
	sprintf(format, "%%.%df", iDecPlaces );
	sprintf(m_pTEXTWorkString, format, fValue);

	char* pReturnString=GetReturnStringFromTEXTWorkString();
	return pReturnString;
}

char* Str ( int iValue )
{
	ValidateWorkStringBySize(64);
	if ( m_pTEXTWorkString ) sprintf(m_pTEXTWorkString, "%d", iValue);

	char* pReturnString=GetReturnStringFromTEXTWorkString();
	return pReturnString;
}

char* Upper ( char* szText )
{
	ValidateWorkString ( (char*)szText );
	if(szText)
		strcpy(m_pTEXTWorkString, (char*)szText);
	else
		strcpy(m_pTEXTWorkString, "");

	_strupr(m_pTEXTWorkString);

	char* pReturnString=GetReturnStringFromTEXTWorkString();
	return pReturnString;
}

float ValF ( char* dwSrcStr )
{
	float fValue = 0.0f;
	if(dwSrcStr) fValue = (float)atof(dwSrcStr);
	return fValue;
}

LONGLONG ValR ( unsigned int dwSrcStr )
{
	LONGLONG lValue = 0;
	if(dwSrcStr) lValue = _atoi64((char*)dwSrcStr);
	return lValue;
}

char* StrDouble ( char* pDestStr, double dValue )
{
	ValidateWorkStringBySize(64);
	sprintf(m_pTEXTWorkString, "%.16g", dValue);

	if(pDestStr && g_pGlob && g_pGlob->CreateDeleteString) g_pGlob->CreateDeleteString((char**)&pDestStr, 0);
	char* pReturnString=GetReturnStringFromTEXTWorkString();
	return pReturnString;
}

char* StrDoubleInt ( char* pDestStr, long long lValue )
{
	ValidateWorkStringBySize(64);
	sprintf(m_pTEXTWorkString, "%I64d", lValue);

	if(pDestStr && g_pGlob && g_pGlob->CreateDeleteString) g_pGlob->CreateDeleteString((char**)&pDestStr, 0);
	char* pReturnString=GetReturnStringFromTEXTWorkString();
	return pReturnString;
}

char* SetupString ( char* szInput )
{
	if (!szInput) return const_cast<char*>("");

	static thread_local std::string s_setupPool[20];
	static thread_local size_t s_setupIndex = 0;

	s_setupIndex = (s_setupIndex + 1) % 20;
	s_setupPool[s_setupIndex] = szInput;

	currentringbuffer = (currentringbuffer + 1) % 20;
	size_t len = strlen(szInput);
	if (len >= 1024) len = 1023;
	memcpy(&ReturnString[currentringbuffer][0], szInput, len);
	ReturnString[currentringbuffer][len] = '\0';

	return s_setupPool[s_setupIndex].data();
}

char* SetupStringOLD(char* szInput)
{
	char* pReturn = NULL;
	unsigned int dwSize = (unsigned int)strlen(szInput);
	if (g_pGlob && g_pGlob->CreateDeleteString) g_pGlob->CreateDeleteString((char**)&pReturn, dwSize + 1);
	if (!pReturn) RunTimeError(RUNTIMEERROR_NOTENOUGHMEMORY);
	memcpy(pReturn, szInput, dwSize);
	pReturn[dwSize] = 0;
	return pReturn;
}

void Reverse ( char* dwA )
{
	if (dwA) strrev ( dwA );
}

char* Spaces2(int iSpaces)
{
	if (iSpaces < 0) iSpaces = 0;
	if (iSpaces > 65536) iSpaces = 65536;

	static thread_local std::string s_spacesPool[20];
	static thread_local size_t s_spacesIndex = 0;

	s_spacesIndex = (s_spacesIndex + 1) % 20;
	s_spacesPool[s_spacesIndex].assign(iSpaces, ' ');

	currentringbuffer = (currentringbuffer + 1) % 20;
	size_t legSpaces = iSpaces < 1023 ? iSpaces : 1023;
	memset(&ReturnString[currentringbuffer][0], 32, legSpaces);
	ReturnString[currentringbuffer][legSpaces] = '\0';

	return s_spacesPool[s_spacesIndex].data();
}

char* Spaces ( int iSpaces )
{
	if (!g_pGlob || !g_pGlob->CreateDeleteString) return const_cast<char*>(" ");
	if ( iSpaces < 0 )
	{
		char* pReturnString=NULL;
		g_pGlob->CreateDeleteString((char**)&pReturnString, 2 );
		if (pReturnString)
		{
			pReturnString [ 0 ] = 0;
			pReturnString [ 1 ] = 0;
		}
		return pReturnString;	
	}

	char* pReturnString=NULL;
	g_pGlob->CreateDeleteString((char**)&pReturnString, iSpaces+1 );
	if (pReturnString)
	{
		memset((char*)pReturnString, 32, iSpaces);
		pReturnString[iSpaces]=0;
	}
	return pReturnString;
}

int FindFirstChar ( char* dwSource, char* dwChar )
{
	if (!dwSource || !dwChar) return 0;
	char* pInt    = dwChar;
	char  pIntA   = *pInt;
	char* pFirst  = strchr ( dwSource, ( int ) pIntA );
	if (!pFirst) return 0;
	int   iResult = (int)(pFirst - dwSource + 1);
	return iResult;
}

int FindLastChar ( char* dwSource, char* dwChar )
{
	if (!dwSource || !dwChar) return 0;
	char* pInt    = dwChar;
	char  pIntA   = *pInt;
	char* pFirst  = strrchr ( dwSource, ( int ) pIntA );
	if (!pFirst) return 0;
	int   iResult = (int)(pFirst - dwSource + 1);
	return iResult;
}

int FindSubString ( char* dwSource, char* dwString )
{
	if (!dwSource || !dwString) return 0;
	char* pFirst  = strstr ( dwSource, dwString );
	if (!pFirst) return 0;
	int   iResult = (int)(pFirst - dwSource + 1);
	return iResult;
}

char* FirstToken ( char* dwSource, char* dwDelim )
{
    char* szSource = dwSource;
    char* szDelim  = dwDelim;

    if (szDelim == 0 || szDelim[0] == 0)
    {
        szDelim = " ";
    }

    if ( szSource && szSource[0] )
    {
        unsigned int dwLength = (unsigned int)strlen( szSource ) + 1;

        if ( dwLength > m_dwTokenStringSize || !m_szTokenString )
        {
            m_dwTokenStringSize = dwLength + 256;
            delete[] m_szTokenString;
            m_szTokenString = new char[ m_dwTokenStringSize ];
			memset ( m_szTokenString, 0, m_dwTokenStringSize );
        }

        memcpy( m_szTokenString, szSource, dwLength );
    }
    else
    {
		if (!m_szTokenString)
		{
			m_dwTokenStringSize = 256;
			m_szTokenString = new char[m_dwTokenStringSize+1];
			memset(m_szTokenString, 0, m_dwTokenStringSize+1);
		}
        m_szTokenString[ 0 ] = 0;
		return SetupString("");
    }

    char* szToken = strtok ( m_szTokenString, szDelim );
    if ( szToken )
    {
	    return SetupString ( szToken );
    }
    return SetupString ( "" );
}

char* NextToken ( char* dwDelim )
{
    char* szDelim  = dwDelim;

    if (szDelim == 0 || szDelim[0] == 0)
    {
        szDelim = " ";
    }

	char* szToken = strtok ( NULL, szDelim );
    if ( szToken )
    {
	    return SetupString ( szToken );
    }
    return SetupString ( "" );
}
