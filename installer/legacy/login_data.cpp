#include "stdafx.h"

#include "login_data.h"
#include "stream.h"
#include "md5.h"
#include "registry.h"
#include "const.h"

namespace
{
    const wchar_t* EXT_DATA_PREFIX = L"####ext:";
}


#define LOGIN_LENGTH		128
#define PASS_LENGTH			64
#define CS_MRA_LOGINS L"magent_logins2"
#define CS_MRA_LOGINS3 L"magent_logins3"
#define MAX_LOGIN_AS 5



MAKFC_CLoginData::MAKFC_CLoginData( bool bConvertToMd5 )
{
	m_sLogin = L"";
	m_sProtocolUID = m_sLogin;
	m_nLoginType = TLoginType::LT_Undefined;
	m_sPassword = L"";	
	m_sIMName = L"";
	m_bMD5 = bConvertToMd5;
	m_client_id = 0;
}

MAKFC_CLoginData::MAKFC_CLoginData( MAKFC_CString sLogin, MAKFC_CString sPassword, MAKFC_CLoginData::TLoginType loginType, bool bConvertToMd5 )
{
	m_sLogin = sLogin;
	m_sProtocolUID = m_sLogin;

	m_nLoginType = loginType;
	m_sPassword = sPassword;
	m_lpsPassword = MAKFC_CLPS((LPCWSTR)sPassword);

	m_bMD5 = bConvertToMd5;

	if ( bConvertToMd5 )
		ConvertPasswordToMd5(true);

	m_client_id = 0;
}

MAKFC_CLoginData::MAKFC_CLoginData( const MAKFC_CLoginData& src )
{
	*this = src;
}

MAKFC_CLoginData::~MAKFC_CLoginData()
{
	ClearIMLogins();
}

void MAKFC_CLoginData::SetMD5(bool bMD5)
{
	m_bMD5 = bMD5;
}

const MAKFC_CString &MAKFC_CLoginData::GetLogin() const
{
	return m_sLogin;
}

void MAKFC_CLoginData::SetLogin(const MAKFC_CString &login)
{
	m_sLogin = login;
}

void MAKFC_CLoginData::SetPassword(const MAKFC_CString &password)
{
	m_sPassword = password;
}

const MAKFC_CString &MAKFC_CLoginData::GetPassword() const
{
	return m_sPassword;
}

const MAKFC_CLPS &MAKFC_CLoginData::GetLpsToken() const
{
	return m_lpsToken;
}

void MAKFC_CLoginData::SetLpsToken(const MAKFC_CLPS &lps)
{
	m_lpsToken = lps;
}

void MAKFC_CLoginData::DecodeLpsToken(BLOWFISHKEY key)
{
	if (m_lpsToken.getDataSize() == 0)
	{
		m_sToken.Empty();
		return;
	}

	auto rw = Blowfish_DecodeData(
		m_lpsToken.getdata(),
		m_lpsToken.getDataSize(),
		key
	);

	const int len = rw->readInt();
	m_sToken = MAKFC_CLPS(rw->getBuffer() + sizeof(DWORD), len).ToStringW();

	delete rw;
}

void MAKFC_CLoginData::EncodeLpsToken(BLOWFISHKEY key)
{
	m_lpsToken.clear();

	if (m_sToken.IsEmpty())
	{
		return;
	}

	MAKFC_CLPS lpsToken(m_sToken);
	auto rw = Blowfish_EncodeData(
		lpsToken.getLPS(),
		lpsToken.getsize(),
		key
		);
	m_lpsToken = MAKFC_CLPS(rw->getBuffer(),rw->size());
	delete rw;
}

MAKFC_CLoginData::TLoginType MAKFC_CLoginData::GetLoginType(MAKFC_CString sName)
{
	if (sName == L"email")
	{
		return TLoginType::LT_Email;
	}
	else if (sName == L"uin")
	{
		return TLoginType::LT_UIN;
	}
	else if (sName == L"phone")
	{
		return TLoginType::LT_Phone;
	}
	else if (sName == L"fb")
	{
		return TLoginType::LT_Facebook;
	}
	else if (sName == L"odkl")
	{
		return TLoginType::LT_Odkl;
	}
	else if (sName == L"vk")
	{
		return TLoginType::LT_Vkontakte;
	}
	else if (sName == L"token")
	{
		return TLoginType::LT_Token;
	}
	
	return TLoginType::LT_Undefined;
}

MAKFC_CString MAKFC_CLoginData::GetLoginTypeName(MAKFC_CLoginData::TLoginType loginType)
{
	//assert(loginType >= TLoginType::LT_Min);
	assert(loginType <= TLoginType::LT_Max);

	MAKFC_CString result;

	switch (loginType)
	{
	case TLoginType::LT_Email:
		result = L"email";
		break;
	case TLoginType::LT_UIN:
		result = L"uin";
		break;
	case TLoginType::LT_Phone:
		result = "phone";
		break;
	case TLoginType::LT_Facebook:
		result = L"fb";
		break;
	case TLoginType::LT_Odkl:
		result = L"odkl";
		break;
	case TLoginType::LT_Vkontakte:
		result = L"vk";
		break;
	case TLoginType::LT_Token:
		result = L"token";
		break;
	}
	
	assert (result.Len() < MaxLoginTypeNameLength);

	return result;
}

MAKFC_CString MAKFC_CLoginData::GetLoginTypeName() const
{
	return GetLoginTypeName(m_nLoginType);
}

void MAKFC_CLoginData::SetLoginType(const TLoginType loginType)
{
	//assert(loginType >= TLoginType::LT_Min);
	assert(loginType <= TLoginType::LT_Max);
	m_nLoginType = loginType;
}

MAKFC_CLoginData::TLoginType MAKFC_CLoginData::GetDefaultLoginType(MAKFC_CString sLogin)
{
	if (sLogin.IsEmpty())
	{
		return TLoginType::LT_Undefined;
	}

	if (makfc_IsEmail(sLogin))
	{
		return TLoginType::LT_Email;
	}

	if (MAKFC_CString::IsUin(sLogin))
	{
		return TLoginType::LT_UIN;
	}

	if (makfc_IsPhone(sLogin))
	{
		return TLoginType::LT_Phone;
	}

	return TLoginType::LT_Email;
}

MAKFC_CString MAKFC_CLoginData::GetDefaultLoginTypeName(MAKFC_CString sLogin)
{
	return GetLoginTypeName(GetDefaultLoginType(sLogin));
}

bool MAKFC_CLoginData::IsFederatedLoginType(MAKFC_CLoginData::TLoginType nLoginType)
{
	return (nLoginType == TLoginType::LT_Facebook || nLoginType == TLoginType::LT_Odkl || nLoginType == TLoginType::LT_Vkontakte);
}

BOOL MAKFC_CLoginData::operator<(const MAKFC_CLoginData &ld) const
{
	if (m_sLogin < ld.m_sLogin)
		return TRUE;
	if (m_sLogin > ld.m_sLogin)
		return FALSE;
	return m_nLoginType < ld.m_nLoginType;
}

bool MAKFC_CLoginData::IsEmptyLogin() const
{
	return m_sLogin.IsEmpty();
}

bool MAKFC_CLoginData::IsEmptyToken() const
{
	return m_sToken.IsEmpty();
}

bool MAKFC_CLoginData::IsEmptySessionKey() const
{
	return m_sSessionKey.IsEmpty();
}

const MAKFC_CString &MAKFC_CLoginData::GetToken() const
{
	return m_sToken;
}

void MAKFC_CLoginData::SetProtocolUID(MAKFC_CString sProtocolUID, bool bEmptyIsValid)
{
	if (!sProtocolUID.IsEmpty() || bEmptyIsValid)
	{
		m_sProtocolUID = sProtocolUID;
	}
}

void MAKFC_CLoginData::SetLoginTypeWithDefaultType(MAKFC_CString sLogin)
{
	sLogin = sLogin.Trim();

	m_sLogin = sLogin;
	m_sProtocolUID = m_sLogin;

	m_nLoginType = GetDefaultLoginType(sLogin);
}

void MAKFC_CLoginData::SetLogin(const MAKFC_CString &sLogin, MAKFC_CLoginData::TLoginType loginType)
{
	m_sLogin = sLogin;
	m_sProtocolUID = sLogin;

	m_nLoginType = loginType;
}

void MAKFC_CLoginData::SetLogin(const MAKFC_CString &sLogin, const MAKFC_CString &protoUID, TLoginType loginType)
{
	m_sLogin = sLogin;

	m_sProtocolUID = protoUID;
	m_nLoginType = loginType;
}

void MAKFC_CLoginData::SetToken(const MAKFC_CString &sToken)
{
	m_sToken = sToken;
}

void MAKFC_CLoginData::SetSessionKey(const MAKFC_CString &sessionKey)
{
	m_sSessionKey = sessionKey;
}

const MAKFC_CLoginData& MAKFC_CLoginData::operator=( const MAKFC_CLoginData &src )
{
	m_sLogin=src.m_sLogin;
	m_nLoginType = src.m_nLoginType;
	m_sProtocolUID = src.m_sProtocolUID;

	m_sPassword=src.m_sPassword;
	m_sIMName=src.m_sIMName;
	m_lpsPassword=src.m_lpsPassword;
	m_password_md5 = src.m_password_md5;
	m_bMD5 = src.m_bMD5;
	m_sToken = src.m_sToken;
	m_sSessionKey = src.m_sSessionKey;
	m_client_id = src.m_client_id;

	ClearIMLogins();

	LOGINRECORDSPTR::const_iterator it;
	for (it = src.m_imLogins.begin(); it != src.m_imLogins.end(); it++)
	{
		MAKFC_CLoginData *lpLogin = new MAKFC_CLoginData((*it)->GetMD5());
		*lpLogin = *(*it);
		m_imLogins.push_back(lpLogin);
	}

	return *this;
}

BOOL MAKFC_CLoginData::ConvertPasswordToMd5( bool bClearPassword )
{
	if (m_password_md5.empty())
		m_password_md5.resize( 16 );

	MAKFC_md5( m_sPassword.NetStrA( 1251 ), m_sPassword.NetSizeA( 1251 ), (char*) &m_password_md5[0] );

	if ( bClearPassword )
		m_sPassword = L"";

	return TRUE;
}

BOOL MAKFC_CLoginData::IsPasswordEmpty() const
{
	if(!m_bMD5)
	{
		assert(m_sPassword != MD5_PASSWORD_STRING);
		return !m_lpsPassword.getDataSize();
	}
	if ( !m_password_md5.size() )	
		return TRUE;

	char md5[16];
	MAKFC_md5( "", 0, md5 );

	if ( memcmp( &m_password_md5[0], md5, 16 ) == 0 )
		return TRUE;

	return FALSE;
}


bool MAKFC_CLoginData::IsPasswordOrTokenEmpty() const
{
	if ( GetLoginType() == TLoginType::LT_Email || GetLoginType() == TLoginType::LT_UIN )
		return IsPasswordEmpty();
	else if ( GetLoginType() == TLoginType::LT_Token )
		return GetToken().IsEmpty();

	return false;
}

BOOL MAKFC_CLoginData::IsPasswordMd5String() const
{
	return (m_sPassword == MD5_PASSWORD_STRING);
}

void MAKFC_CLoginData::MakePasswordEmpty()
{
	if(m_bMD5)
	{
		char md5[16];
		MAKFC_md5( "", 0, md5 );
		memcpy( &m_password_md5[0], md5, 16 );
	}
	else
	{
		//##SHRF##  - clear password
		m_password_md5.clear();
	}
	m_lpsPassword = MAKFC_CLPS();
}

BOOL MAKFC_CLoginData::EncodePasswordMD5(const BLOWFISHKEY &key, BOOL bUnicode)
{
	if (!m_bMD5)
		return EncodePasswordSimple(key, bUnicode);

	if ((m_sPassword.IsEmpty() && m_password_md5.empty()) || (m_sPassword == MD5_PASSWORD_STRING))
	{
		m_lpsPassword.clear();
		m_password_md5.clear();
		return FALSE;
	}

	if (m_password_md5.empty())
	{
		m_password_md5.resize(16);
	}

	MAKFC_CLPS lpsPass;
	lpsPass = MAKFC_CLPS((byte*) &m_password_md5[0], m_password_md5.size() );

	auto rw = Blowfish_EncodeData(
		lpsPass.getLPS(),
		lpsPass.getsize(),
		key
		);
	m_lpsPassword = MAKFC_CLPS(rw->getBuffer(),rw->size());
	delete rw;

	return TRUE;
}

BOOL MAKFC_CLoginData::EncodePasswordSimple(const BLOWFISHKEY &key, BOOL bUnicode)
{
	if(!m_sPassword.GetLength() || (m_sPassword == MD5_PASSWORD_STRING))
	{
		return FALSE;
	}

	assert (!m_bMD5);

	MAKFC_CLPS lpsPass;
	if(bUnicode)
	{
		lpsPass=MAKFC_CLPS((byte*)(LPCTSTR)m_sPassword,m_sPassword.GetLength()*sizeof(TCHAR));
	}
	else
	{
		lpsPass=MAKFC_CLPS((byte*)m_sPassword.NetStrA(),m_sPassword.GetLength());

	}
	MAKFC_CReaderWriter *rw = Blowfish_EncodeData(
		lpsPass.getLPS(),
		lpsPass.getsize(),
		key
		);
	m_lpsPassword=MAKFC_CLPS(rw->getBuffer(),rw->size());
	delete rw;

	return TRUE;
}

BOOL MAKFC_CLoginData::DecodePasswordMD5(const BLOWFISHKEY &key, BOOL bUnicode)
{
	if (!m_bMD5)
		return DecodePasswordSimple(key, bUnicode);

	MAKFC_CReaderWriter *rw = Blowfish_DecodeData(
		m_lpsPassword.getdata(),
		m_lpsPassword.getDataSize(),
		key
		);

	int len = rw->readInt();
	if ( len == 16 ) 
	{
		if ( !m_password_md5.size() )
			m_password_md5.resize( 16 );

		memcpy( &m_password_md5[0], rw->getBuffer() + sizeof( ULONG ), 16 );
	}

	delete rw;
	return TRUE;
}

BOOL MAKFC_CLoginData::DecodePasswordSimple( const BLOWFISHKEY &key, BOOL bUnicode )
{
	if(m_lpsPassword.getDataSize()<=0)
	{
		return FALSE;
	}

	m_sPassword.Empty();

	MAKFC_CReaderWriter *rw = Blowfish_DecodeData(
		m_lpsPassword.getdata(),
		m_lpsPassword.getDataSize(),
		key
	);

	if(bUnicode)
	{
		if (rw->size() && rw->size() <= IM_PASSWORD_LENGTH_MAX*sizeof(TCHAR) + sizeof(DWORD))
		{
			rw->readLpsAsStringW(m_sPassword);
		}
	}
	else
	{
		if (rw->size() && rw->size() <= IM_PASSWORD_LENGTH_MAX + sizeof(DWORD))
		{
			rw->readLpsAsStringA(m_sPassword);
		}
	}
	delete rw;

	if (m_sPassword == MD5_PASSWORD_STRING)
	{
		m_lpsPassword.clear();
		m_sPassword.Empty();
		return FALSE;
	}

	m_bMD5 = FALSE;
	return TRUE;
}

void MAKFC_CLoginData::ClearIMLogins()
{
	LOGINRECORDSPTR::const_iterator it;
	for (it = m_imLogins.begin(); it != m_imLogins.end(); it++)
	{
		delete *it;
	}
	m_imLogins.clear();

}

void MAKFC_CLoginData::UnPackLogin()
{
	int nIndexPos=m_sLogin.Find(_T(":"));
	if(nIndexPos != -1)
	{
		m_sIMName = m_sLogin.Mid(0,nIndexPos);
	}
	else
	{
		m_sIMName = m_sLogin.Mid(0,3);
		nIndexPos = 0;
	}
	MAKFC_CLoginData IMLogin(m_bMD5);
	int nLoginPos=m_sLogin.Find(_T("#"));
	if(nLoginPos!=-1)
	{
		m_sLogin=m_sLogin.Mid(nLoginPos+1);
	}

	m_sProtocolUID = m_sLogin;

	m_nLoginType = GetDefaultLoginType(m_sLogin);
}

void MAKFC_CLoginData::PackLogin( MAKFC_CString sIMName, DWORD dwPos /*= 0*/ )
{
	m_sLogin = sIMName 
		+ _T(":") 
		+ MAKFC_CString::ToString(dwPos,_T("%03d")) 
		+ _T("#")
		+ m_sLogin;
}

void MAKFC_CLoginData::CopyPassword( MAKFC_CLoginData &src )
{
	m_password_md5 = src.m_password_md5;
	m_sPassword = L"";
}

BOOL MAKFC_CLoginData::operator==( const MAKFC_CLoginData &ld ) const
{
	return (m_sLogin == ld.m_sLogin) 
		&& (m_nLoginType == ld.m_nLoginType)
		&& (m_password_md5 == ld.m_password_md5);
}

BOOL MAKFC_CLoginData::DebugEq(const MAKFC_CLoginData &ld) const
{
	BOOL bRes = (m_sLogin == ld.m_sLogin) 
		&& (m_nLoginType == ld.m_nLoginType)
		&& (m_sPassword == ld.m_sPassword);

	if (!bRes)
		return FALSE;

	if (m_imLogins.size() != ld.m_imLogins.size())
		return FALSE;

	std::list<MAKFC_CLoginData *>::const_iterator it1 = m_imLogins.begin();
	std::list<MAKFC_CLoginData *>::const_iterator it2 = ld.m_imLogins.begin();

	while ( it1 != m_imLogins.end() && it2 != ld.m_imLogins.begin())
	{
		if (!(*it1)->DebugEq(*(*it2)))
		{
			return FALSE;
		}

		++it1;
		++it2;
	}

	return TRUE;
}

BOOL MAKFC_CLoginData::operator!=( const MAKFC_CLoginData &ld ) const
{
	return !(*this == ld);
}

std::vector<BYTE> MAKFC_CLoginData::GetChildsKey() const
{
	if(m_password_md5.empty())
	{
		std::vector<BYTE> md5(16);
		MAKFC_CString empty =  m_sPassword;
		MAKFC_md5( empty.NetStrA( 1251 ), empty.NetSizeA( 1251 ), (char*) &md5[0] );
		return md5;
	}

	return m_password_md5;
}

void MAKFC_CLoginData::SetClientId( unsigned int _client_id )
{
	m_client_id = _client_id;
}

unsigned int MAKFC_CLoginData::GetClientId() const
{
	return m_client_id;
}

LOGINRECORDS LOGIN_GetOldLogins(BOOL bVer2, const BLOWFISHKEY& key)
{
	LOGINRECORDS aLogins;	//login records list


	MAKFC_CString sKey = MAKFC_CString(legacy::cs_mra_key) + ((bVer2) ? (MAKFC_CString)_T("\\magent_logins") : (MAKFC_CString)_T("\\mra_logins"));
	TCHAR szEmail[1024];
	char szPass[1024];
	BYTE lpsPass[1024];
	DWORD dwLoginLength = LOGIN_LENGTH + 1;
	DWORD dwPassLength = PASS_LENGTH + sizeof(ULONG);

	
	//read all logins from old registry branch
	HKEY hKeysSet = NULL;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, sKey, 0, KEY_ALL_ACCESS, &hKeysSet) == ERROR_SUCCESS)
	{
		DWORD dwKeys = 0;
		if (RegQueryInfoKey(hKeysSet, NULL, NULL, NULL, NULL, NULL, NULL, &dwKeys, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			if (dwKeys > 100)
				dwKeys = 100; //to avoid infinite cycle
			DWORD dwIndex = 0;
			dwLoginLength = 1023;
			dwPassLength = 1023;
			DWORD dwType = 0;
			while (dwIndex <= dwKeys
				&& RegEnumValue(hKeysSet, dwIndex, szEmail,
					&dwLoginLength, NULL, &dwType, (BYTE*)lpsPass, &dwPassLength) == ERROR_SUCCESS)
			{
				if (dwType == REG_BINARY && dwLoginLength > 0 && dwLoginLength < LOGIN_LENGTH)
				{
					if (
						(!bVer2 && STR_IsEmail(szEmail))
						|| (bVer2 && _tcslen(szEmail) > 4 && STR_IsEmail(szEmail + 4))
						)
					{
						_tcslwr(szEmail);
						MAKFC_CLoginData login(true);
						login.SetLoginTypeWithDefaultType(szEmail);
						login.SetPassword(L"");
						if (bVer2)
						{
							MAKFC_CReaderWriter *rw = Blowfish_DecodeData((BYTE *)lpsPass, dwPassLength, key); 
							if (rw->size() && rw->size() <= PASS_LENGTH + sizeof(DWORD))
							{
								lps2szA(szPass, rw->getBuffer());
								login.SetPassword(szPass);
							}
							delete rw;
						}
						else
						{
							if (LPSLENGTH(lpsPass) <= PASS_LENGTH)
							{
								//decode old style password coding 
								mra_decode(lpsPass);
								//store password in sz
								lps2szA(szPass, lpsPass);
								//store pair in list
								login.SetPassword(szPass);
							}
						}
						aLogins.push_back(login);
					}
				}
				dwIndex++;
				dwType = 0;
				dwLoginLength = 1023;
				dwPassLength = 1023;
			}
		}
		RegCloseKey(hKeysSet);
	}
	if (bVer2)
	{
		aLogins.sort();
	}
	return aLogins;
}

void LOGIN_GetChildLogins2( HKEY hLoginKey, MAKFC_CString sPassword, MAKFC_CLoginData *login ) 
{
	DWORD dwKeys=0;
	if (!RegQueryInfoKey(hLoginKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwKeys, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		return;
	}
	if (dwKeys > 100)
		dwKeys = 100; //to avoid infinite cycle
	BLOWFISHKEY keyChilds={0};
	LOGINS_GetKey(keyChilds, sPassword);
	TCHAR szLogin[1024];
	BYTE lpsPass[1024];
	DWORD dwIndex = 0;
	DWORD dwLoginLength = 1023;
	DWORD dwPassLength = 1023;
	DWORD dwType = 0;
	while (
		dwIndex <= dwKeys
		&& RegEnumValue(
		hLoginKey,
		dwIndex,
		szLogin,
		&dwLoginLength,
		NULL,
		&dwType,
		lpsPass,
		&dwPassLength
		) == ERROR_SUCCESS
		)
	{
		if (MAKFC_CString(szLogin).GetLength() >= 4 && MAKFC_CString(szLogin).Mid(0, 4) != L"####")
		{
			MAKFC_CLoginData *loginChild = new MAKFC_CLoginData(true);
			loginChild->SetLoginTypeWithDefaultType(szLogin);

			if(sPassword.GetLength())
			{
				loginChild->m_lpsPassword=MAKFC_CLPS(lpsPass,dwPassLength);
				if (dwPassLength>0)
				{
					loginChild->DecodePasswordSimple(keyChilds);
				}
			}

			login->m_imLogins.push_back(loginChild);
		}
		dwIndex++;
		dwType = 0;
		dwLoginLength = 1023;
		dwPassLength = 1023;
	}
}

LOGINRECORDS LOGIN_GetLogins2(const BLOWFISHKEY& key)
{
	LOGINRECORDS aLogins;	//login records list

	MAKFC_CString sKey = MAKFC_CString(legacy::cs_mra_key) + (MAKFC_CString)_T("\\") + CS_MRA_LOGINS;
	TCHAR szEmail[1024];
	BYTE lpsPass[1024];
	DWORD dwLoginLength = LOGIN_LENGTH + 1;
	DWORD dwPassLength = PASS_LENGTH + sizeof(ULONG);
    	
	//read all logins from old registry branch
	HKEY hKeysSet = NULL;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, sKey, 0, KEY_ALL_ACCESS, &hKeysSet) == ERROR_SUCCESS)
	{
		DWORD dwKeys = 0;
		if (RegQueryInfoKey(hKeysSet, NULL, NULL, NULL, &dwKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			if (dwKeys > 100)
				dwKeys = 100; //to avoid infinite cycle
			DWORD dwIndex = 0;
			dwLoginLength = 1023;
			dwPassLength = 1023;
			DWORD dwType = 0;
			HKEY hLoginKey=NULL;
			while (
				dwIndex <= dwKeys
				&& RegEnumKey(
					hKeysSet,
					dwIndex,
					szEmail,
					dwLoginLength
				) == ERROR_SUCCESS
			)
			{
				if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER, sKey + _T("\\") + szEmail, &hLoginKey))
				{
					if (_tcslen(szEmail) > 4 && STR_IsEmail(szEmail + 4))
					{
						_tcslwr(szEmail);
						::RegQueryValueEx(hLoginKey,_T("####password"),NULL,&dwType,lpsPass,&dwPassLength);
						MAKFC_CLoginData login(true);
						login.SetLoginTypeWithDefaultType(szEmail);
						login.m_lpsPassword=MAKFC_CLPS(lpsPass,dwPassLength);
						login.SetPassword(L"");
						if (dwType==REG_BINARY && dwPassLength>0)
						{
							login.DecodePasswordSimple(key,FALSE);
						}
						LOGIN_GetChildLogins2(hLoginKey, login.GetPassword(), &login);
						aLogins.push_back(login);
					}
				}
				dwIndex++;
				dwType = 0;
				dwLoginLength = 1023;
				dwPassLength = 1023;
				RegCloseKey(hLoginKey);
			}
		}
		RegCloseKey(hKeysSet);
	}
	aLogins.sort();

	return aLogins;
}

void LOGIN_GetLogins2(MAKFC_CLoginData *login)
{
	MAKFC_CString sKey = MAKFC_CString(legacy::cs_mra_key) + (MAKFC_CString)_T("\\") + CS_MRA_LOGINS;
	TCHAR szEmail[1024];
	DWORD dwLoginLength = LOGIN_LENGTH + 1;

	//read all logins from old registry branch
	HKEY hKeysSet = NULL;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, sKey, 0, KEY_ALL_ACCESS, &hKeysSet) != ERROR_SUCCESS)
	{
		return;
	}
	DWORD dwKeys = 0;
	if (RegQueryInfoKey(hKeysSet, NULL, NULL, NULL, &dwKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
	{
		return;
	}
	if (dwKeys > 100)
		dwKeys = 100; //to avoid infinite cycle
	DWORD dwIndex = 0;
	dwLoginLength = 1023;
	DWORD dwType = 0;
	HKEY hLoginKey=NULL;
	while (
		dwIndex <= dwKeys
		&& RegEnumKey(
			hKeysSet,
			dwIndex,
			szEmail,
			dwLoginLength
		) == ERROR_SUCCESS
	)
	{
		if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER, sKey + _T("\\") + szEmail, &hLoginKey))
		{
			if (_tcslen(szEmail) > 4 && STR_IsEmail(szEmail + 4))
			{
				if(login->GetLogin().CompareNoCase(szEmail+4) == 0)
				{
					login->m_imLogins.clear();
					LOGIN_GetChildLogins2(hLoginKey, login->GetPassword(), login);
					return;

				}
			}
		}
		dwIndex++;
		dwType = 0;
		dwLoginLength = 1023;
		RegCloseKey(hLoginKey);
	}
	RegCloseKey(hKeysSet);
}

#define ACCMERGE_LOG_ID L"accmerge"
#define ACCMERGE_LOG(FMT, ...) { _LF(WriteLog2(ACCMERGE_LOG_ID, L"%s\n"FMT, __FUNCTIONW__, __VA_ARGS__)); }
#define ACCMERGE_MSG(MSG) { _LF(WriteLog2(ACCMERGE_LOG_ID, L"%s\n%s", __FUNCTIONW__, (MSG))); }

void LOGIN_GetChildLogins( HKEY hLoginKey, MAKFC_CLoginData &loginData ) 
{
	BLOWFISHKEY keyChilds={0};
	LOGINS_GetKey(keyChilds, loginData.GetChildsKey());
	

	DWORD dwKeys=0;
	if (RegQueryInfoKey(hLoginKey, NULL, NULL, NULL, NULL, NULL, NULL, &dwKeys, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
	{
		return;
	}

	if (dwKeys > 100)
	{
		dwKeys = 100; //to avoid infinite cycle
	}

	std::map<MAKFC_CString, MAKFC_CLPS> tokens;

	wchar_t szLogin[1024] = { 0 };
	BYTE lpsPass[1024] = { 0 };
	DWORD dwIndex = 0;
	DWORD dwLoginLength = 1023;
	DWORD dwPassLength = 1023;
	DWORD dwType = 0;

	auto advance = [&] {
		dwIndex++;
		dwType = 0;
		dwLoginLength = 1023;
		dwPassLength = 1023;
	};

	while (dwIndex <= dwKeys)
	{
		if (RegEnumValue(
			hLoginKey,
			dwIndex,
			szLogin,
			&dwLoginLength,
			NULL,
			&dwType,
			lpsPass,
			&dwPassLength
			) != ERROR_SUCCESS)
		{
			break;
		}

		const bool isToken = MAKFC_CString(szLogin).StartsWith(EXT_DATA_PREFIX);
		if (isToken)
		{
			MAKFC_CString login = szLogin;
			login.ReplaceFirst(EXT_DATA_PREFIX, L"");
			tokens[login] = MAKFC_CLPS(lpsPass, dwPassLength);
		}

		advance();
	}

	advance();
	dwIndex = 0;

	while (
		dwIndex <= dwKeys
		&& RegEnumValue(
		hLoginKey,
		dwIndex,
		szLogin,
		&dwLoginLength,
		NULL,
		&dwType,
		lpsPass,
		&dwPassLength
		) == ERROR_SUCCESS
		)
	{
		MAKFC_CString login = szLogin;

		const bool isAdditionalAccount = ((login.GetLength() >= 4) && (login.Mid(0, 4) != L"####"));
		if(isAdditionalAccount)
		{
			MAKFC_CString sIMName = szLogin;
			int nIMLen = sIMName.Find(L":");
			if(nIMLen != -1)
			{
				sIMName = sIMName.Left(nIMLen);
			}

			MAKFC_CLoginData *loginChild = new MAKFC_CLoginData(sIMName == L"Agent");
			loginChild->SetLoginTypeWithDefaultType(szLogin);
			loginChild->m_lpsPassword = MAKFC_CLPS(lpsPass, dwPassLength);
			loginChild->m_sIMName = sIMName;

			auto token = tokens.find(szLogin);
			if (token != tokens.end())
			{
				loginChild->SetLpsToken(token->second);
				loginChild->DecodeLpsToken(keyChilds);
			}

			if ( dwPassLength > 0 && loginChild->GetLogin().Len() > 3 )
			{
				loginChild->DecodePasswordMD5(keyChilds);
			}


			loginData.m_imLogins.push_back(loginChild);
		}

		advance();
	}
}

LOGINRECORDS LOGIN_GetLogins(const BLOWFISHKEY& key, BOOL bConvertLogins)
{
	LOGINRECORDS aLogins;	//login records list

	TCHAR szEmail[1024];
	BYTE lpsPass[1024];
	BYTE lpsToken[1024];
	DWORD dwLoginLength = LOGIN_LENGTH + 1;
	DWORD dwPassLength = PASS_LENGTH + sizeof(ULONG);
	DWORD dwTokenLength = 1023;

	
	//read all logins from old registry branch
	HKEY hKeysSet = NULL;
	MAKFC_CString sKey = MAKFC_CString(legacy::cs_mra_key) + _T("\\") + CS_MRA_LOGINS3;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, sKey, 0, KEY_ALL_ACCESS, &hKeysSet) == ERROR_SUCCESS)
	{
		DWORD dwKeys = 0;
		if (RegQueryInfoKey(hKeysSet, NULL, NULL, NULL, &dwKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			if (dwKeys > 100)
				dwKeys = 100; //to avoid infinite cycle
			DWORD dwIndex = 0;
			dwLoginLength = 1023;
			dwPassLength = 1023;
			DWORD dwType = 0;
			HKEY hLoginKey=NULL;
			while (
				dwIndex <= dwKeys
				&& RegEnumKey(
				hKeysSet,
				dwIndex,
				szEmail,
				dwLoginLength
				) == ERROR_SUCCESS
				)
			{
				if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER, sKey + _T("\\") + szEmail, &hLoginKey))
				{
					if (_tcslen(szEmail) > 4
#ifdef _AGENT
						&& STR_IsEmail(szEmail + 4)
#endif
						)
					{
						//////////////////////////////////////////////////////////////////////////
						_tcslwr(szEmail);
						MAKFC_CString sLogin = szEmail;
						MAKFC_CString sDefaultLoginType = MAKFC_CLoginData::GetDefaultLoginTypeName(sLogin.Mid(4));

						MAKFC_CLoginData::TLoginType nLoginType = MAKFC_CLoginData::GetLoginType(
							makfc_REG_GetStringData(hLoginKey, L"####login_type", sDefaultLoginType));
						
						if (::RegQueryValueEx(hLoginKey,_T("####password"),NULL,&dwType, lpsPass, &dwPassLength) != ERROR_SUCCESS)
						{
							dwPassLength = 0;
						}

						//////////////////////////////////////////////////////////////////////////
						 MAKFC_CLoginData login( MAKFC_LoginDataRootIsMD5() && (nLoginType != MAKFC_CLoginData::TLoginType::LT_Token) );
						login.m_nLoginType = nLoginType;
						login.SetLogin(sLogin);
						login.SetProtocolUID(makfc_REG_GetStringData(hLoginKey, L"####proto_uid", login.GetLogin().Mid(4)));

						login.m_lpsPassword=MAKFC_CLPS(lpsPass,dwPassLength);
						login.SetPassword(L"");
						
						if (dwType==REG_BINARY && dwPassLength>0)
						{
							login.DecodePasswordMD5(key);
							if (!MAKFC_LoginDataRootIsMD5())
							{
								login.ConvertPasswordToMd5( false );
							}
						}
						
						//////////////////////////////////////////////////////////////////////////
						dwTokenLength = 1023;
						if (::RegQueryValueEx(hLoginKey,_T("####token"),NULL,&dwType, lpsToken, &dwTokenLength) != ERROR_SUCCESS)
						{
							dwTokenLength = 0;
						}

						login.SetLpsToken(MAKFC_CLPS(lpsToken,dwTokenLength));
						
						if (dwType==REG_BINARY && dwTokenLength>0)
							login.DecodeLpsToken(key);

						//////////////////////////////////////////////////////////////////////////
						DWORD dw_client_id = 0;
						dwTokenLength = 1023;
						if (::RegQueryValueEx( hLoginKey, L"####client_id", NULL, &dwType, lpsToken, &dwTokenLength ) == ERROR_SUCCESS )
						{
							if ( REG_DWORD == dwType && dwTokenLength == sizeof( DWORD ) )
							{
								dw_client_id = *( (DWORD*)( lpsToken ) );
								login.SetClientId( dw_client_id );
							}

							dwTokenLength = 0;
						}
								

						LOGIN_GetChildLogins( hLoginKey, login );
						aLogins.push_back(login);
					}
				}
				dwIndex++;
				dwType = 0;
				dwLoginLength = 1023;
				dwPassLength = 1023;
				RegCloseKey(hLoginKey);
			}
		}
		RegCloseKey(hKeysSet);
	}
	aLogins.sort();
	if(bConvertLogins)
	{
		LOGINRECORDS::iterator it;
		for(it=aLogins.begin();it!=aLogins.end(); ++it)
		{
			it->SetLogin(it->GetLogin().Mid(4), it->GetProtocolUID(), it->m_nLoginType);
		}
	}

	return aLogins;
}

BOOL STR_IsEmailName(const TCHAR *szStr)
{
    const TCHAR *p = szStr;
    int iLen = _tcslen(szStr);
    for (int i = 0; i < iLen; i++)
    {
        if (!isemailname(*p))
            return FALSE;
        p++;
    }
    return TRUE;
}


BOOL STR_IsEmail(const MAKFC_CString& stStr)
{
    MAKFC_CString sDomain;
    //check for @
    int iSobaka = stStr.Find(_T("@"));
    if (iSobaka == -1 || iSobaka == 0 || (stStr.Len()-1) - iSobaka < 4)
        return FALSE;
    //analyze name
    MAKFC_CString sName = stStr.Mid(0, iSobaka);
    if (!STR_IsEmailName(sName))
        return FALSE;
    //analyze domain
    sDomain = stStr.Mid(iSobaka+1);	
    _tcslwr(sDomain.GetBuffer());
    BOOL bAlpha = FALSE, bDot = FALSE;
    int iLen = sDomain.Len();
    if (iLen <= 1)
        return FALSE;
    TCHAR *p = sDomain.GetBuffer();
    for (int i = 0; i < iLen; i++)
    {
        if (_istalpha(*p))// || isrussian(*p)) can not be in email
            bAlpha = TRUE;
        if (*p == _T('.'))
            bDot = TRUE;
        p++;
    }
    if (bAlpha && bDot && (_istalpha(*(p-1)) || _istdigit(*(p-1)))) //at least 1 alpha symbol, exactly 1 dot and last alpha symbol or digit
        return TRUE;
    return FALSE;
}


LOGINRECORDS LOGIN_GetLoginsAgent(HANDLE hBFFile, BOOL bConvertLogins)
{
	LOGINRECORDS aLogins;	//login records list

	MAKFC_CString sKey = MAKFC_CString(legacy::str_cs_agent_mra_key) + (MAKFC_CString)_T("\\") + CS_MRA_LOGINS3;
	TCHAR szEmail[1024];
	BYTE lpsPass[1024];
	DWORD dwLoginLength = LOGIN_LENGTH + 1;
	DWORD dwPassLength = PASS_LENGTH + sizeof(ULONG);

	BLOWFISHKEY key = {0};
	BOOL bKeyRead =  Blowfish_ReadKey(hBFFile, &key);

	//read all logins from old registry branch
	HKEY hKeysSet = NULL;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, sKey, 0, KEY_ALL_ACCESS, &hKeysSet) == ERROR_SUCCESS)
	{
		DWORD dwKeys = 0;
		if (RegQueryInfoKey(hKeysSet, NULL, NULL, NULL, &dwKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			if (dwKeys > 100)
				dwKeys = 100; //to avoid infinite cycle
			DWORD dwIndex = 0;
			dwLoginLength = 1023;
			dwPassLength = 1023;
			DWORD dwType = 0;
			HKEY hLoginKey=NULL;
			while (
				dwIndex <= dwKeys
				&& RegEnumKey(
				hKeysSet,
				dwIndex,
				szEmail,
				dwLoginLength
				) == ERROR_SUCCESS
				)
			{
				if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER, sKey + _T("\\") + szEmail, &hLoginKey))
				{
					if (_tcslen(szEmail) > 4
						&& STR_IsEmail(szEmail + 4)
						)
					{
						_tcslwr(szEmail);
						::RegQueryValueEx(hLoginKey,_T("####password"),NULL,&dwType,lpsPass,&dwPassLength);
						MAKFC_CLoginData login(TRUE);
						
						login.SetLogin(szEmail);

						MAKFC_CString sDefaultLoginType = MAKFC_CLoginData::GetDefaultLoginTypeName(login.GetLogin().Mid(4));
						login.m_nLoginType = MAKFC_CLoginData::GetLoginType(
							makfc_REG_GetStringData(hLoginKey, L"####login_type", sDefaultLoginType));
						login.SetProtocolUID(makfc_REG_GetStringData(hLoginKey, L"####proto_uid", login.GetLogin().Mid(4)));

						login.m_lpsPassword=MAKFC_CLPS(lpsPass,dwPassLength);
						login.SetPassword(L"");
						if (bKeyRead && dwType==REG_BINARY && dwPassLength>0)
						{
							login.DecodePasswordMD5(key);
						}
						LOGIN_GetChildLogins( hLoginKey, login );
						aLogins.push_back(login);
					}
				}
				dwIndex++;
				dwType = 0;
				dwLoginLength = 1023;
				dwPassLength = 1023;
				RegCloseKey(hLoginKey);
			}
		}
		RegCloseKey(hKeysSet);
	}
	aLogins.sort();
	if(bConvertLogins)
	{
		LOGINRECORDS::iterator it;
		for(it=aLogins.begin();it!=aLogins.end(); ++it)
		{
			it->SetLogin(it->GetLogin().Mid(4), it->GetProtocolUID(), it->m_nLoginType);
		}
	}
	memset(&key,0,sizeof(key));
	return aLogins;
}

LOGINRECORDS LOGIN_GetLoginsICQ(HANDLE hBFFile, BOOL bConvertLogins)
{
	LOGINRECORDS aLogins;	//login records list

	MAKFC_CString sKey = MAKFC_CString(legacy::str_cs_icq_mra_key) + L"\\" + CS_MRA_LOGINS3;
	TCHAR szEmail[1024];
	BYTE lpsPass[1024];
	DWORD dwLoginLength = LOGIN_LENGTH + 1;
	DWORD dwPassLength = PASS_LENGTH + sizeof(ULONG);

	BLOWFISHKEY key = {0};
	BOOL bKeyRead =  Blowfish_ReadKey(hBFFile, &key);

	//read all logins from old registry branch
	HKEY hKeysSet = NULL;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, sKey, 0, KEY_ALL_ACCESS, &hKeysSet) == ERROR_SUCCESS)
	{
		DWORD dwKeys = 0;
		if (RegQueryInfoKey(hKeysSet, NULL, NULL, NULL, &dwKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			if (dwKeys > 100)
				dwKeys = 100; //to avoid infinite cycle
			DWORD dwIndex = 0;
			dwLoginLength = 1023;
			dwPassLength = 1023;
			DWORD dwType = 0;
			HKEY hLoginKey=NULL;
			while (
				dwIndex <= dwKeys
				&& RegEnumKey(
				hKeysSet,
				dwIndex,
				szEmail,
				dwLoginLength
				) == ERROR_SUCCESS
				)
			{
				if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER, sKey + _T("\\") + szEmail, &hLoginKey))
				{
					if (_tcslen(szEmail) > 4
						)
					{
						_tcslwr(szEmail);
						::RegQueryValueEx(hLoginKey,_T("####password"),NULL,&dwType,lpsPass,&dwPassLength);
						MAKFC_CLoginData login(FALSE);
						login.SetLogin(szEmail);

						DWORD nType;
						DWORD nSize = MAKFC_CLoginData::MaxLoginTypeNameLength;
						wchar_t sLoginType[MAKFC_CLoginData::MaxLoginTypeNameLength];
						::memset(sLoginType, 0, sizeof(sLoginType));

						if (ERROR_SUCCESS != RegQueryValueEx(hLoginKey, L"####login_type", 0, &nType, 
							(BYTE*)sLoginType, &nSize)
							|| nSize <= 0 || nSize >= MAKFC_CLoginData::MaxLoginTypeNameLength || nType != REG_SZ)
						{
							login.m_nLoginType = MAKFC_CLoginData::GetDefaultLoginType(sKey);
						}
						else
						{
							login.m_nLoginType = MAKFC_CLoginData::GetLoginType(sLoginType);
						}

						login.m_lpsPassword=MAKFC_CLPS(lpsPass,dwPassLength);
						login.SetPassword(L"");
						if (bKeyRead && dwType==REG_BINARY && dwPassLength>0)
						{
							login.DecodePasswordMD5(key);
							login.ConvertPasswordToMd5( false );
						}
						LOGIN_GetChildLogins( hLoginKey, login );
						aLogins.push_back(login);
					}
				}
				dwIndex++;
				dwType = 0;
				dwLoginLength = 1023;
				dwPassLength = 1023;
				RegCloseKey(hLoginKey);
			}
		}
		RegCloseKey(hKeysSet);
	}
	aLogins.sort();
	if(bConvertLogins)
	{
		LOGINRECORDS::iterator it;
		for(it=aLogins.begin();it!=aLogins.end(); ++it)
		{
			it->SetLogin(it->GetLogin().Mid(4), it->m_nLoginType);
		}
	}
	memset(&key,0,sizeof(key));
	return aLogins;
}

BOOL IsMd5PasswordFormat( const MAKFC_CString& sLogin )
{
	MAKFC_CString sProtocol = L"Agent";

	return ( sLogin.Len() > sProtocol.Len() && ( sLogin.Mid( 0, sProtocol.Len() ) == sProtocol ) );
}

BOOL LOGIN_SetLogins(LOGINRECORDS aLogins, HANDLE hBFFile)
{
	if (!hBFFile || hBFFile == INVALID_HANDLE_VALUE)
	{
		assert(FALSE);
		return FALSE;
	}

	//delete branch
	MAKFC_CString sKey = MAKFC_CString(legacy::cs_mra_key) + L"\\" + CS_MRA_LOGINS3;
	SHDeleteKey(HKEY_CURRENT_USER, sKey);
	//create empty branch
	HKEY hKeyLogins = NULL;
	if (ERROR_SUCCESS == RegCreateKey(HKEY_CURRENT_USER, sKey, &hKeyLogins) && hKeyLogins)
	{
		RegCloseKey(hKeyLogins);
	}
	hKeyLogins=NULL;
	//write values
	LOGINRECORDS::iterator it;
	BLOWFISHKEY key;
	//read blowfish key
	if (!Blowfish_ReadKey(hBFFile, &key, FALSE))
		return FALSE;
	for (it = aLogins.begin(); it != aLogins.end(); ++it)
	{
		if (MAKFC_CLoginData::IsFederatedLoginType(it->m_nLoginType))
		{
			MAKFC_CString sTmpLogin = (it->GetLogin().GetLength() <= 4) ? L"" : it->GetLogin().Mid(4);

			LOGINS_FixAutoLogin(sTmpLogin);							// apply fix from 25.April.2015 

			bool bAutoLogin = LOGINS_GetAutoLogin(sTmpLogin);
			if (!bAutoLogin)
			{
				continue;
			}
		}

		if (ERROR_SUCCESS == RegCreateKey(HKEY_CURRENT_USER, sKey + _T("\\") + it->GetLogin(), &hKeyLogins) && hKeyLogins)
		{
			RegCloseKey(hKeyLogins);
		}

		it->EncodePasswordMD5( key );
		it->EncodeLpsToken( key );

		MAKFC_CString sRootKey = sKey + _T("\\") + it->GetLogin();
		makfc_REG_SetNValueData(HKEY_CURRENT_USER, sRootKey, _T("####password"), it->m_lpsPassword.getdata(), it->m_lpsPassword.getDataSize());
		makfc_REG_SetNValueData(HKEY_CURRENT_USER, sRootKey, _T("####token"), it->GetLpsToken().getdatac(), it->GetLpsToken().getDataSize());
		
		MAKFC_CString sLoginType = MAKFC_CLoginData::GetLoginTypeName(it->m_nLoginType);
		makfc_REG_SetStringData(HKEY_CURRENT_USER, sRootKey, L"####login_type", sLoginType);

		makfc_REG_SetStringData(HKEY_CURRENT_USER, sRootKey, L"####proto_uid", it->GetProtocolUID());
		makfc_REG_SetDWORDValue(HKEY_CURRENT_USER, sRootKey, L"####client_id", it->GetClientId());

		BLOWFISHKEY keyChilds={0};
		LOGINS_GetKey( keyChilds, it->GetChildsKey() );

		for (auto &childLogin : it->m_imLogins)
		{
			if ( !IsMd5PasswordFormat( childLogin->GetLogin() ) )
			{
				childLogin->EncodePasswordSimple( keyChilds );
			}
			else
			{
				childLogin->EncodePasswordMD5( keyChilds );
			}
			
			childLogin->EncodeLpsToken(keyChilds);

			const MAKFC_CString sRootLoginKey = sKey + L"\\" + it->GetLogin();

			makfc_REG_SetNValueData(
				HKEY_CURRENT_USER, 
				sRootLoginKey, 
				childLogin->GetLogin(), 
				childLogin->m_lpsPassword.getdata(), childLogin->m_lpsPassword.getDataSize());

			assert(!childLogin->m_sIMName.IsEmpty());

			const bool isAgent = (childLogin->m_sIMName == L"Agent");
			if (!childLogin->GetLpsToken().empty() && isAgent)
			{
				makfc_REG_SetNValueData(
					HKEY_CURRENT_USER, 
					sRootLoginKey, 
					EXT_DATA_PREFIX + childLogin->GetLogin(), 
					childLogin->GetLpsToken().getdatac(), childLogin->GetLpsToken().getDataSize());
			}
		}
	}

	return TRUE;
}

LOGINRECORDS::iterator LOGIN_FindLogin(LOGINRECORDS &aLogins, MAKFC_CString sLogin)
{
	LOGINRECORDS::iterator it;
	_tcslwr(sLogin.GetBuffer());
	for (it = aLogins.begin(); it != aLogins.end(); ++it)
	{
		if (it->GetLogin() == sLogin)
			return it;
	}
	return it;
}

LOGINRECORDS::iterator LOGIN_FindLogin(LOGINRECORDS &aLogins, MAKFC_CString sLogin, MAKFC_CLoginData::TLoginType loginType)
{
	LOGINRECORDS::iterator it;
	_tcslwr(sLogin.GetBuffer());
	for (it = aLogins.begin(); it != aLogins.end(); ++it)
	{
		if (it->GetLogin() == sLogin && it->m_nLoginType == loginType)
			return it;
	}
	return it;
}

LOGINRECORDS LOGIN_ConvertPasswords(const BLOWFISHKEY& key)
{
	MAKFC_CString szKeyOld  = MAKFC_CString(legacy::cs_mra_key) + _T("\\mra_logins");    //logins should be moved from
	MAKFC_CString szKeyOld2 = MAKFC_CString(legacy::cs_mra_key) + _T("\\magent_logins"); //new login registry branch

	LOGINRECORDS aLogins = LOGIN_GetLogins(key, FALSE);

	if ( aLogins.size() )
	{
		return aLogins;
	}

	aLogins=LOGIN_GetLogins2(key); //don't convert if we have logins in current format
	if ( !aLogins.size() )
		aLogins = LOGIN_GetOldLogins(TRUE, key);
		
	while (aLogins.size() > MAX_LOGIN_AS)
		aLogins.pop_back();

	return aLogins;
}

ULONG GetMraID()
{
    DWORD id = 0;
    TCHAR str[150];

    if (makfc_REG_GetValueData(HKEY_CURRENT_USER, MAKFC_CString(legacy::cs_mra_key),_T("ID"),(BYTE*)str))
    {
        id = _ttol(str);
    }
    return id;
}


BOOL mra_encode(BYTE *lpsData)
{
	DWORD id = GetMraID();
	BYTE xor_char = LOBYTE(LOWORD(id));
	GetLpsX(lpsData, xor_char);
	return 1;
}

BOOL mra_decode(BYTE *lpsData)
{
	return mra_encode(lpsData);
}

BOOL LOGINS_StoreLoginDataToStorage(BOOL/* bStorePassword*/, HANDLE/* hBFFile*/)
{
	//##SHRF## - changed name of variable
/*	auto lpMain = g_IMs.GetRootIM();
	if(!lpMain)
	{
		assert(FALSE);
		return FALSE;
	}

	LOGINRECORDS aLogins = LOGIN_GetLogins(hBFFile, FALSE);

	MAKFC_CLoginData login(lpMain->GetLoginData());
	login.SetLogin(L"000#" + login.GetLogin());
	if(!bStorePassword)
	{
		login.SetPassword(L"");
		login.MakePasswordEmpty();
		login.SetToken(L"");
	}

	//update our current login in the root login list and place it to the beginning of the list
	LOGINRECORDS::iterator it;
	for (it = aLogins.begin(); it != aLogins.end(); ++it)
	{
		if (it->GetLogin().GetLength() <= 4)
		{
			continue;
		}

		if (it->GetLogin().Mid(4) == login.GetLogin().Mid(4)
			&& (it->m_nLoginType == MAKFC_CLoginData::TLoginType::LT_Undefined || 
			    it->m_nLoginType == MAKFC_CLoginData::TLoginType::LT_Token || 
				 it->m_nLoginType == login.m_nLoginType))
		{
			aLogins.erase(it);
			break;
		}
	}

	aLogins.push_front(login);

	int iIndex = 0;
	for (it = aLogins.begin(); it != aLogins.end(); ++it)
	{
		MAKFC_CString s;
		s.Format( L"%.3d#%s", iIndex, (LPCTSTR)it->GetLogin().Mid(4) );

		it->SetLogin(s);
		iIndex++;
	}

	while (aLogins.size() > MAX_LOGIN_AS)
		aLogins.pop_back();

	return LOGIN_SetLogins(aLogins, hBFFile);*/

    return TRUE;
}

UINT LOGINS_GetKey(BLOWFISHKEY &key, const MAKFC_CString &sPass)
{
	char md5[16];
	MAKFC_md5((const char*)(LPCTSTR)sPass,sPass.GetLength()*sizeof(TCHAR),md5);
	memcpy(key.key,md5,16);
	memcpy(key.key+16,md5,16);
	memcpy(key.key+32,md5,16);
	memcpy(key.key+48,md5,8);
	return TRUE;
}

UINT LOGINS_GetKey(BLOWFISHKEY &key, std::vector<BYTE>& md5)
{
	if ( md5.size() != 16 )
		return FALSE;

	memcpy( key.key, &md5[0], 16 );
	memcpy( key.key+16, &md5[0], 16 );
	memcpy( key.key+32, &md5[0], 16 );
	memcpy( key.key+48, &md5[0], 8 );

	return TRUE;
}

/*UINT LOGINS_GetKey(BLOWFISHKEY &key)
{
	CGeneralIM *lpMain=g_IMs.GetRootIM();
	return LOGINS_GetKey(key, lpMain->GetLoginData().GetChildsKey());
}*/

void LOGINS_SetAutoLogin(MAKFC_CString login, BOOL/* autologin*/)
{
/*	DWORD p = autologin ? 1 : 0;
	tc->m_db.SaveDWORD(tstring((LPCWSTR)login), L"", L"AUTOLOGIN", p);*/
}

bool LOGINS_GetAutoLogin(MAKFC_CString login)
{
	DWORD autologin = 0;
/*	if (!tc->m_db.ReadDWORD(tstring(static_cast<LPCWSTR>(login)), L"", L"AUTOLOGIN", autologin))
	{
		autologin = 0;
	}*/

	return (autologin != 0);
}

void LOGINS_FixAutoLogin(MAKFC_CString login)
{
	// Note: для старых версий ICQ в поле автологин для ***federated*** логинов может быть прописан FALSE
	//		 поэтому на первом проходе мы этот FALSE должны переписать, а в последущих - не трогать.

	MAKFC_CString sAlreadyFixed = login + L"_already_fixed";		// looking for a mark in DB
	bool bAlreadyFixed = LOGINS_GetAutoLogin(sAlreadyFixed);		

	if (!bAlreadyFixed)												// if not marked yet ..
	{																
		LOGINS_SetAutoLogin(login, TRUE);							// fix crap; overwrite value with a TRUE
		LOGINS_SetAutoLogin(sAlreadyFixed, TRUE);					// put a mark to DB
	}
}

