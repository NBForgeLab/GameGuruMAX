#pragma once
#ifndef __C_STR__
#define __C_STR__

#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>
#if (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || (defined(__cplusplus) && __cplusplus >= 201703L)
#define CSTR_HAS_STRING_VIEW 1
#include <string_view>
#else
#define CSTR_HAS_STRING_VIEW 0
#endif
#include <algorithm>
#include <cmath>
#include <cctype>

#define cstr cStr

// Neutralized legacy flag maintained for binary link compatibility
extern bool noDeleteCSTR;

class cStr
{
public:
	std::string m_str;

	// Constructors
	cStr() noexcept : m_str() {}
	cStr(std::nullptr_t) noexcept : m_str() {}
	cStr(const cStr& other) : m_str(other.m_str) {}
	cStr(cStr&& other) noexcept : m_str(std::move(other.m_str)) {}
	cStr(const char* szString) : m_str(szString ? szString : "") {}
	cStr(char* szString) : m_str(szString ? szString : "") {}
	cStr(const std::string& str) : m_str(str) {}
	cStr(std::string&& str) noexcept : m_str(std::move(str)) {}
#if CSTR_HAS_STRING_VIEW
	cStr(std::string_view sv) : m_str(sv) {}
#endif
	cStr(int iValue) : m_str(std::to_string(iValue)) {}
	cStr(unsigned int iValue) : m_str(std::to_string(iValue)) {}
	cStr(long iValue) : m_str(std::to_string(iValue)) {}
	cStr(unsigned long iValue) : m_str(std::to_string(iValue)) {}
	cStr(long long iValue) : m_str(std::to_string(iValue)) {}
	cStr(unsigned long long iValue) : m_str(std::to_string(iValue)) {}
	cStr(float fValue)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "%.2f", fValue);
		m_str = buf;
	}
	cStr(double dValue)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "%.2f", static_cast<float>(dValue));
		m_str = buf;
	}

	~cStr();

	// Assignment operators
	cStr operator=(const cStr& other);
	cStr& operator=(cStr&& other) noexcept
	{
		m_str = std::move(other.m_str);
		return *this;
	}
	cStr& operator=(std::nullptr_t)
	{
		m_str.clear();
		return *this;
	}
	cStr& operator=(const char* other);
	cStr& operator=(char* other)
	{
		if (other) m_str = other; else m_str.clear();
		return *this;
	}
	cStr& operator=(const std::string& other)
	{
		m_str = other;
		return *this;
	}
	cStr& operator=(std::string&& other) noexcept
	{
		m_str = std::move(other);
		return *this;
	}
#if CSTR_HAS_STRING_VIEW
	cStr& operator=(std::string_view other)
	{
		m_str = other;
		return *this;
	}
#endif
	cStr& operator=(int val)
	{
		m_str = std::to_string(val);
		return *this;
	}
	cStr& operator=(unsigned int val)
	{
		m_str = std::to_string(val);
		return *this;
	}
	cStr& operator=(long val)
	{
		m_str = std::to_string(val);
		return *this;
	}
	cStr& operator=(unsigned long val)
	{
		m_str = std::to_string(val);
		return *this;
	}
	cStr& operator=(long long val)
	{
		m_str = std::to_string(val);
		return *this;
	}
	cStr& operator=(unsigned long long val)
	{
		m_str = std::to_string(val);
		return *this;
	}
	cStr& operator=(float val)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "%.2f", val);
		m_str = buf;
		return *this;
	}
	cStr& operator=(double val)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "%.2f", static_cast<float>(val));
		m_str = buf;
		return *this;
	}

	// Compound assignment
	cStr& operator+=(const cStr& other);
	cStr& operator+=(const char* other)
	{
		if (other) m_str += other;
		return *this;
	}
	cStr& operator+=(char* other)
	{
		if (other) m_str += other;
		return *this;
	}
	cStr& operator+=(const std::string& other)
	{
		m_str += other;
		return *this;
	}
#if CSTR_HAS_STRING_VIEW
	cStr& operator+=(std::string_view other)
	{
		m_str += other;
		return *this;
	}
#endif
	cStr& operator+=(char c)
	{
		m_str += c;
		return *this;
	}
	cStr& operator+=(int val)
	{
		m_str += std::to_string(val);
		return *this;
	}
	cStr& operator+=(unsigned int val)
	{
		m_str += std::to_string(val);
		return *this;
	}
	cStr& operator+=(long val)
	{
		m_str += std::to_string(val);
		return *this;
	}
	cStr& operator+=(unsigned long val)
	{
		m_str += std::to_string(val);
		return *this;
	}
	cStr& operator+=(long long val)
	{
		m_str += std::to_string(val);
		return *this;
	}
	cStr& operator+=(unsigned long long val)
	{
		m_str += std::to_string(val);
		return *this;
	}
	cStr& operator+=(float val)
	{
		*this += cStr(val);
		return *this;
	}
	cStr& operator+=(double val)
	{
		*this += cStr(val);
		return *this;
	}

	// Concatenation
	cStr operator+(const cStr& other);
	cStr operator+(const cStr& other) const;
	cStr operator+(const char* other);
	cStr operator+(const char* other) const;
	cStr operator+(char* other);
	cStr operator+(char* other) const;
	cStr operator+(const std::string& other);
	cStr operator+(const std::string& other) const;
#if CSTR_HAS_STRING_VIEW
	cStr operator+(std::string_view other);
	cStr operator+(std::string_view other) const;
#endif
	cStr operator+(char c);
	cStr operator+(char c) const;
	cStr operator+(int iValue);
	cStr operator+(int iValue) const;
	cStr operator+(unsigned int iValue);
	cStr operator+(unsigned int iValue) const;
	cStr operator+(long iValue);
	cStr operator+(long iValue) const;
	cStr operator+(unsigned long iValue);
	cStr operator+(unsigned long iValue) const;
	cStr operator+(long long iValue);
	cStr operator+(long long iValue) const;
	cStr operator+(unsigned long long iValue);
	cStr operator+(unsigned long long iValue) const;
	cStr operator+(float fValue);
	cStr operator+(float fValue) const;
	cStr operator+(double dValue);
	cStr operator+(double dValue) const;

	friend cStr operator+(const char* lhs, const cStr& rhs)
	{
		return cStr((lhs ? std::string(lhs) : std::string()) + rhs.m_str);
	}
	friend cStr operator+(char* lhs, const cStr& rhs)
	{
		return cStr((lhs ? std::string(lhs) : std::string()) + rhs.m_str);
	}
	friend cStr operator+(const std::string& lhs, const cStr& rhs)
	{
		return cStr(lhs + rhs.m_str);
	}
#if CSTR_HAS_STRING_VIEW
	friend cStr operator+(std::string_view lhs, const cStr& rhs)
	{
		std::string res(lhs);
		res += rhs.m_str;
		return cStr(std::move(res));
	}
#endif
	friend cStr operator+(char lhs, const cStr& rhs)
	{
		std::string res(1, lhs);
		res += rhs.m_str;
		return cStr(std::move(res));
	}
	friend cStr operator+(int lhs, const cStr& rhs)
	{
		return cStr(lhs) + rhs;
	}

	// Equality / Comparison
	bool operator==(const cStr& s);
	bool operator==(const cStr& s) const noexcept { return m_str == s.m_str; }
	bool operator==(const char* s);
	bool operator==(const char* s) const noexcept { return m_str == (s ? s : ""); }
	bool operator==(char* s);
	bool operator==(char* s) const noexcept { return m_str == (s ? s : ""); }
	bool operator==(const std::string& s);
	bool operator==(const std::string& s) const noexcept { return m_str == s; }
#if CSTR_HAS_STRING_VIEW
	bool operator==(std::string_view sv);
	bool operator==(std::string_view sv) const noexcept { return std::string_view(m_str) == sv; }
#endif
	bool operator==(int val);
	bool operator==(int val) const noexcept { return m_str == std::to_string(val) || Val() == val; }
	bool operator==(unsigned int val);
	bool operator==(unsigned int val) const noexcept { return m_str == std::to_string(val); }
	friend bool operator==(const char* lhs, const cStr& rhs) noexcept { return rhs == lhs; }
	friend bool operator==(char* lhs, const cStr& rhs) noexcept { return rhs == lhs; }
	friend bool operator==(const std::string& lhs, const cStr& rhs) noexcept { return rhs == lhs; }
#if CSTR_HAS_STRING_VIEW
	friend bool operator==(std::string_view lhs, const cStr& rhs) noexcept { return rhs == lhs; }
#endif
	friend bool operator==(int lhs, const cStr& rhs) noexcept { return rhs == lhs; }
	friend bool operator==(unsigned int lhs, const cStr& rhs) noexcept { return rhs == lhs; }

	bool operator!=(const cStr& s);
	bool operator!=(const cStr& s) const noexcept { return m_str != s.m_str; }
	bool operator!=(const char* s);
	bool operator!=(const char* s) const noexcept { return m_str != (s ? s : ""); }
	bool operator!=(char* s);
	bool operator!=(char* s) const noexcept { return m_str != (s ? s : ""); }
	bool operator!=(const std::string& s);
	bool operator!=(const std::string& s) const noexcept { return m_str != s; }
#if CSTR_HAS_STRING_VIEW
	bool operator!=(std::string_view sv);
	bool operator!=(std::string_view sv) const noexcept { return std::string_view(m_str) != sv; }
#endif
	bool operator!=(int val);
	bool operator!=(int val) const noexcept { return !(*this == val); }
	bool operator!=(unsigned int val);
	bool operator!=(unsigned int val) const noexcept { return !(*this == val); }
	friend bool operator!=(const char* lhs, const cStr& rhs) noexcept { return rhs != lhs; }
	friend bool operator!=(char* lhs, const cStr& rhs) noexcept { return rhs != lhs; }
	friend bool operator!=(const std::string& lhs, const cStr& rhs) noexcept { return rhs != lhs; }
#if CSTR_HAS_STRING_VIEW
	friend bool operator!=(std::string_view lhs, const cStr& rhs) noexcept { return rhs != lhs; }
#endif
	friend bool operator!=(int lhs, const cStr& rhs) noexcept { return rhs != lhs; }
	friend bool operator!=(unsigned int lhs, const cStr& rhs) noexcept { return rhs != lhs; }

	bool operator<(const cStr& other) const noexcept { return m_str < other.m_str; }
	bool operator<(const char* other) const noexcept { return m_str < (other ? other : ""); }
	bool operator<(char* other) const noexcept { return m_str < (other ? other : ""); }
	bool operator<(const std::string& other) const noexcept { return m_str < other; }
#if CSTR_HAS_STRING_VIEW
	bool operator<(std::string_view other) const noexcept { return std::string_view(m_str) < other; }
#endif
	friend bool operator<(const char* lhs, const cStr& rhs) noexcept { return (lhs ? lhs : "") < rhs.m_str; }
	friend bool operator<(char* lhs, const cStr& rhs) noexcept { return (lhs ? lhs : "") < rhs.m_str; }
	friend bool operator<(const std::string& lhs, const cStr& rhs) noexcept { return lhs < rhs.m_str; }
#if CSTR_HAS_STRING_VIEW
	friend bool operator<(std::string_view lhs, const cStr& rhs) noexcept { return lhs < std::string_view(rhs.m_str); }
#endif

	// Indexing
	char operator[](int index) const noexcept
	{
		return (index >= 0 && static_cast<size_t>(index) < m_str.size()) ? m_str[index] : '\0';
	}
	char& operator[](int index)
	{
		return m_str[index];
	}
	char operator[](size_t index) const noexcept
	{
		return (index < m_str.size()) ? m_str[index] : '\0';
	}
	char& operator[](size_t index)
	{
		return m_str[index];
	}

	// Conversions
	operator const char*() const noexcept { return m_str.c_str(); }
	operator std::string() const { return m_str; }
	operator const std::string&() const noexcept { return m_str; }
	operator std::string&() noexcept { return m_str; }
#if CSTR_HAS_STRING_VIEW
	operator std::string_view() const noexcept { return std::string_view(m_str); }
#endif

	// Accessors
	int Len(void);
	int Len() const noexcept { return static_cast<int>(m_str.length()); }
	const char* c_str() const noexcept { return m_str.c_str(); }
	// Writable access replaces existing characters only. Use assignment or str()
	// for changes to length; external buffer writers need their own sized buffer.
	char* Get(void);
	const char* Get() const noexcept { return m_str.c_str(); }
	char* GetMutable() noexcept { return &m_str[0]; }
	int Val(void);
	int Val() const noexcept { return m_str.empty() ? 0 : atoi(m_str.c_str()); }
	float ValF(void);
	float ValF() const noexcept { return m_str.empty() ? 0.0f : static_cast<float>(atof(m_str.c_str())); }

	size_t size() const noexcept { return m_str.size(); }
	size_t length() const noexcept { return m_str.length(); }
	bool empty() const noexcept { return m_str.empty(); }
	void clear() noexcept { m_str.clear(); }
	std::string& str() noexcept { return m_str; }
	const std::string& str() const noexcept { return m_str; }

	cStr Upper(void);
	cStr Upper() const
	{
		std::string res = m_str;
		for (char& c : res) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
		return cStr(std::move(res));
	}

	cStr Lower(void);
	cStr Lower() const
	{
		std::string res = m_str;
		for (char& c : res) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
		return cStr(std::move(res));
	}

	cStr Left(int iCount);
	cStr Left(int iCount) const
	{
		if (iCount <= 0 || m_str.empty()) return cStr("");
		if (static_cast<size_t>(iCount) >= m_str.length()) return *this;
		return cStr(m_str.substr(0, iCount));
	}

	cStr Right(int iCount);
	cStr Right(int iCount) const
	{
		if (iCount <= 0 || m_str.empty()) return cStr("");
		if (static_cast<size_t>(iCount) >= m_str.length()) return *this;
		return cStr(m_str.substr(m_str.length() - iCount));
	}

	cStr Mid(int iPoint);
	cStr Mid(int point) const
    {
        if (point < 0 || static_cast<size_t>(point) >= m_str.size()) return cStr("");
        return cStr(m_str.substr(static_cast<size_t>(point), 1));
    }
    cStr Mid(int start, int count) const
	{
		if (m_str.empty()) return cStr("");
		if (start <= 1)
		{
			if (count < 0) return *this;
			return cStr(m_str.substr(0, count));
		}
		if (start > static_cast<int>(m_str.length())) return cStr("");
		if (count < 0)
			return cStr(m_str.substr(start - 1));
		else
			return cStr(m_str.substr(start - 1, count));
	}
};

inline cStr cStr::operator+(const char* other) { return cStr(m_str + (other ? other : "")); }
inline cStr cStr::operator+(const char* other) const { return cStr(m_str + (other ? other : "")); }
inline cStr cStr::operator+(char* other) { return cStr(m_str + (other ? other : "")); }
inline cStr cStr::operator+(char* other) const { return cStr(m_str + (other ? other : "")); }
inline cStr cStr::operator+(const std::string& other) { return cStr(m_str + other); }
inline cStr cStr::operator+(const std::string& other) const { return cStr(m_str + other); }
#if CSTR_HAS_STRING_VIEW
inline cStr cStr::operator+(std::string_view other) { std::string res = m_str; res += other; return cStr(std::move(res)); }
inline cStr cStr::operator+(std::string_view other) const { std::string res = m_str; res += other; return cStr(std::move(res)); }
#endif
inline cStr cStr::operator+(char c) { return cStr(m_str + c); }
inline cStr cStr::operator+(char c) const { return cStr(m_str + c); }
inline cStr cStr::operator+(int iValue) { return cStr(m_str + std::to_string(iValue)); }
inline cStr cStr::operator+(int iValue) const { return cStr(m_str + std::to_string(iValue)); }
inline cStr cStr::operator+(unsigned int iValue) { return cStr(m_str + std::to_string(iValue)); }
inline cStr cStr::operator+(unsigned int iValue) const { return cStr(m_str + std::to_string(iValue)); }
inline cStr cStr::operator+(long iValue) { return cStr(m_str + std::to_string(iValue)); }
inline cStr cStr::operator+(long iValue) const { return cStr(m_str + std::to_string(iValue)); }
inline cStr cStr::operator+(unsigned long iValue) { return cStr(m_str + std::to_string(iValue)); }
inline cStr cStr::operator+(unsigned long iValue) const { return cStr(m_str + std::to_string(iValue)); }
inline cStr cStr::operator+(long long iValue) { return cStr(m_str + std::to_string(iValue)); }
inline cStr cStr::operator+(long long iValue) const { return cStr(m_str + std::to_string(iValue)); }
inline cStr cStr::operator+(unsigned long long iValue) { return cStr(m_str + std::to_string(iValue)); }
inline cStr cStr::operator+(unsigned long long iValue) const { return cStr(m_str + std::to_string(iValue)); }
inline cStr cStr::operator+(float fValue) { return *this + cStr(fValue); }
inline cStr cStr::operator+(float fValue) const { return *this + cStr(fValue); }
inline cStr cStr::operator+(double dValue) { return *this + cStr(dValue); }
inline cStr cStr::operator+(double dValue) const { return *this + cStr(dValue); }

inline bool cStr::operator==(char* s) { return m_str == (s ? s : ""); }
inline bool cStr::operator==(const std::string& s) { return m_str == s; }
#if CSTR_HAS_STRING_VIEW
inline bool cStr::operator==(std::string_view sv) { return std::string_view(m_str) == sv; }
#endif
inline bool cStr::operator==(int val) { return m_str == std::to_string(val) || Val() == val; }
inline bool cStr::operator==(unsigned int val) { return m_str == std::to_string(val); }

inline bool cStr::operator!=(char* s) { return m_str != (s ? s : ""); }
inline bool cStr::operator!=(const std::string& s) { return m_str != s; }
#if CSTR_HAS_STRING_VIEW
inline bool cStr::operator!=(std::string_view sv) { return std::string_view(m_str) != sv; }
#endif
inline bool cStr::operator!=(int val) { return !(*this == val); }
inline bool cStr::operator!=(unsigned int val) { return !(*this == val); }

#endif // __C_STR__

#ifndef __DBP_LIKE_ARRAYS__
#define __DBP_LIKE_ARRAYS__

#define Dim( name , size ) do { (name).resize ( (size) < 0 ? 0 : ((size) + 1) ); } while(0)
#define Redim Dim

#define Dim2( name , a , b ) do { \
	(name).resize( (a) < 0 ? 0 : ((a) + 1) ); \
	for ( size_t _i = 0 ; _i < (name).size() ; ++_i ) { \
		(name)[_i].resize ( (b) < 0 ? 0 : ((b) + 1) ); \
	} \
} while(0)
#define Redim2 Dim2

#define Dim3( name , a , b , c ) do { \
	(name).resize( (a) < 0 ? 0 : ((a) + 1) ); \
	for ( size_t _i = 0 ; _i < (name).size() ; ++_i ) { \
		(name)[_i].resize ( (b) < 0 ? 0 : ((b) + 1) ); \
		for ( size_t _j = 0 ; _j < (name)[_i].size() ; ++_j ) { \
			(name)[_i][_j].resize ( (c) < 0 ? 0 : ((c) + 1) ); \
		} \
	} \
} while(0)
#define Redim3 Dim3

#define Undim( name ) do { (name).clear(); (name).shrink_to_fit(); } while(0)
#define UnDim Undim

#define Undim2( name ) do { \
	for ( size_t _i = 0 ; _i < (name).size() ; ++_i ) { \
		(name)[_i].clear(); (name)[_i].shrink_to_fit(); \
	} \
	(name).clear(); (name).shrink_to_fit(); \
} while(0)
#define UnDim2 Undim2

#define Undim3( name ) do { \
	for ( size_t _i = 0 ; _i < (name).size() ; ++_i ) { \
		for ( size_t _j = 0 ; _j < (name)[_i].size() ; ++_j ) { \
			(name)[_i][_j].clear(); (name)[_i][_j].shrink_to_fit(); \
		} \
		(name)[_i].clear(); (name)[_i].shrink_to_fit(); \
	} \
	(name).clear(); (name).shrink_to_fit(); \
} while(0)
#define UnDim3 Undim3

// DBPro command array count
#define ArrayCount( name ) (static_cast<int>((name).size()) - 1)

int ArrayCount2 ( const std::vector<std::vector<cstr>>& array );
cstr ArrayAt ( const std::vector<std::vector<cstr>>& array, int l );

void LoadArray ( const char* fileName , std::vector<cstr>& array );
inline void LoadArray ( char* fileName , std::vector<cstr>& array ) { LoadArray((const char*)fileName, array); }

void SaveArray ( const char* fileName , const std::vector<cstr>& array );
inline void SaveArray ( char* fileName , const std::vector<cstr>& array ) { SaveArray((const char*)fileName, array); }

void LoadArray ( const char* fileName , std::vector<std::vector<cstr>>& array );
inline void LoadArray ( char* fileName , std::vector<std::vector<cstr>>& array ) { LoadArray((const char*)fileName, array); }

void SaveArray ( const char* fileName , const std::vector<std::vector<cstr>>& array );
inline void SaveArray ( char* fileName , const std::vector<std::vector<cstr>>& array ) { SaveArray((const char*)fileName, array); }

float atan2deg( float fA , float fB );
void replaceAll(std::string& str, const std::string& from, const std::string& to);
std::string soundexall(std::string all);

#endif // __DBP_LIKE_ARRAYS__
