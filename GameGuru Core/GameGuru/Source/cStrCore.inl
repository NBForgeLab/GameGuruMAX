// Neutralized legacy flag maintained for binary link compatibility
bool noDeleteCSTR = false;
uint32_t activecstr = 0;

// Exported member implementations for binary / ABI compatibility
cStr::~cStr() = default;

cStr cStr::operator=(const cStr& other)
{
	m_str = other.m_str;
	return *this;
}

cStr& cStr::operator=(const char* other)
{
	if (other) m_str = other; else m_str.clear();
	return *this;
}

cStr& cStr::operator+=(const cStr& other)
{
	m_str += other.m_str;
	return *this;
}

cStr cStr::operator+(const cStr& other)
{
	return cStr(m_str + other.m_str);
}

cStr cStr::operator+(const cStr& other) const
{
	return cStr(m_str + other.m_str);
}

bool cStr::operator==(const cStr& s)
{
	return m_str == s.m_str;
}

bool cStr::operator==(const char* s)
{
	return m_str == (s ? s : "");
}

bool cStr::operator!=(const cStr& s)
{
	return m_str != s.m_str;
}

bool cStr::operator!=(const char* s)
{
	return m_str != (s ? s : "");
}

int cStr::Len(void)
{
	return (int)m_str.length();
}

char* cStr::Get(void)
{
	return &m_str[0];
}

int cStr::Val(void)
{
	return m_str.empty() ? 0 : atoi(m_str.c_str());
}

float cStr::ValF(void)
{
	return m_str.empty() ? 0.0f : static_cast<float>(atof(m_str.c_str()));
}

cStr cStr::Upper(void)
{
	std::string res = m_str;
	for (char& c : res) c = (char)toupper((unsigned char)c);
	return cStr(std::move(res));
}

cStr cStr::Lower(void)
{
	std::string res = m_str;
	for (char& c : res) c = (char)tolower((unsigned char)c);
	return cStr(std::move(res));
}

cStr cStr::Left(int iCount)
{
	if (iCount <= 0 || m_str.empty()) return cStr("");
	if (static_cast<size_t>(iCount) >= m_str.length()) return *this;
	return cStr(m_str.substr(0, iCount));
}

cStr cStr::Right(int iCount)
{
	if (iCount <= 0 || m_str.empty()) return cStr("");
	if (static_cast<size_t>(iCount) >= m_str.length()) return *this;
	return cStr(m_str.substr(m_str.length() - iCount));
}

cStr cStr::Mid(int iPoint)
{
    return static_cast<const cStr&>(*this).Mid(iPoint);
}

