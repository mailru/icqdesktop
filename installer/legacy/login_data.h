#pragma once

#include "string.h"
#include "lps.h"
#include "makarov_blowfish.h"

#define IM_LOGIN_LENGTH_MAX				128
#define IM_PASSWORD_LENGTH_MAX			128
#define MD5_PASSWORD_STRING		L"4$q'/H(!k?"
#define TOKEN_PASSWORD_STRING	L"_4$q'/H(!k?_"

class MAKFC_CLoginData
{
public:

	enum class TLoginType
	{
		LT_Undefined = -1, 
		LT_Min,
		LT_Email = LT_Min, 
		LT_UIN, 
		LT_Phone, 
		LT_Facebook, 
		LT_Odkl, 
		LT_Vkontakte,
		LT_Token,
		LT_Max = LT_Token
	};

	static const int MaxLoginTypeNameLength = 10;

	static TLoginType GetLoginType(MAKFC_CString sName);
	TLoginType GetLoginType() const { return m_nLoginType; }
	void SetLoginType(const TLoginType loginType);

	static MAKFC_CString GetLoginTypeName(TLoginType loginType);
	MAKFC_CString GetLoginTypeName() const;
	static TLoginType GetDefaultLoginType(MAKFC_CString sLogin);
	static MAKFC_CString GetDefaultLoginTypeName(MAKFC_CString sLogin);
	static bool IsFederatedLoginType(TLoginType nLoginType);

	const bool IsUin() const { return m_nLoginType == TLoginType::LT_UIN; }
	const bool IsEmail() const { return m_nLoginType == TLoginType::LT_Email; }
	const bool IsPhone() const { return m_nLoginType == TLoginType::LT_Phone; }

	TLoginType m_nLoginType;

	MAKFC_CString m_sToken;
	MAKFC_CString m_sSessionKey;
	MAKFC_CString m_sIMName;
	MAKFC_CLPS m_lpsPassword;

	std::list<MAKFC_CLoginData *> m_imLogins;

	std::vector<BYTE> m_password_md5;

	void SetMD5(bool bMD5);
	bool GetMD5() const { return m_bMD5; }

	void SetLogin(const MAKFC_CString &login);

	void SetPassword(const MAKFC_CString &password);
	const MAKFC_CString &GetPassword() const;

	const MAKFC_CLPS &GetLpsToken() const;
	void SetLpsToken(const MAKFC_CLPS &lps);
	void DecodeLpsToken(BLOWFISHKEY key);
	void EncodeLpsToken(BLOWFISHKEY key);

protected:

	bool				m_bMD5;
	MAKFC_CString		m_sProtocolUID;
	MAKFC_CString		m_sLogin;
	MAKFC_CString		m_sPassword;
	MAKFC_CLPS			m_lpsToken;
	unsigned int		m_client_id;

public:
	explicit MAKFC_CLoginData(bool bConvertToMd5);
	MAKFC_CLoginData(MAKFC_CString sLogin, MAKFC_CString sPassword, 
		MAKFC_CLoginData::TLoginType loginType, bool bConvertToMd5);
	MAKFC_CLoginData(const MAKFC_CLoginData& src);
	virtual ~MAKFC_CLoginData();

	std::vector<BYTE> GetChildsKey() const;
	BOOL EncodePasswordMD5(const BLOWFISHKEY &key, BOOL bUnicode = TRUE);
	BOOL EncodePasswordSimple(const BLOWFISHKEY &key, BOOL bUnicode = TRUE);
	BOOL DecodePasswordMD5(const BLOWFISHKEY &key, BOOL bUnicode = TRUE);
	BOOL DecodePasswordSimple(const BLOWFISHKEY &key, BOOL bUnicode = TRUE);

	BOOL ConvertPasswordToMd5( bool bClearPassword = true );
	void CopyPassword(MAKFC_CLoginData &src);
	BOOL IsPasswordEmpty() const;
	BOOL IsPasswordMd5String() const;
	bool IsPasswordOrTokenEmpty() const;
	void MakePasswordEmpty();
	void PackLogin(MAKFC_CString sIMName, DWORD dwPos = 0);
	void UnPackLogin();
	void ClearIMLogins();

	bool IsEmptyLogin() const;
	bool IsEmptyToken() const;
	bool IsEmptySessionKey() const;
	const MAKFC_CString &GetLogin() const;
	const MAKFC_CString &GetToken() const;
	const MAKFC_CString GetProtocolUID() const { return m_sProtocolUID; }
	void SetProtocolUID(MAKFC_CString sProtocolUID, bool bEmptyIsValid = false);
	void SetLoginTypeWithDefaultType(MAKFC_CString sLogin);
	void SetLogin(const MAKFC_CString &sLogin, TLoginType loginType);
	void SetLogin(const MAKFC_CString &sLogin, const MAKFC_CString &protoUID, TLoginType loginType);
	void SetToken(const MAKFC_CString &sToken);
	void SetSessionKey(const MAKFC_CString &sessionKey);
	void SetClientId( unsigned int _client_id );
	unsigned int GetClientId() const;

	BOOL operator < (const MAKFC_CLoginData &ld) const;
	BOOL operator == (const MAKFC_CLoginData &ld) const;
	BOOL operator != (const MAKFC_CLoginData &ld) const;
	const MAKFC_CLoginData& operator = (const MAKFC_CLoginData &src);
	BOOL DebugEq(const MAKFC_CLoginData &ld) const;
};

typedef std::vector<MAKFC_CLoginData> LOGINDATAVECTOR;
typedef std::list<MAKFC_CLoginData *> LOGINRECORDSPTR; 

typedef std::list<MAKFC_CLoginData> LOGINRECORDS;

LOGINRECORDS LOGIN_GetOldLogins(BOOL bNew, const BLOWFISHKEY& key);
LOGINRECORDS LOGIN_GetLogins2(const BLOWFISHKEY& key);
void LOGIN_GetLogins2(MAKFC_CLoginData *login);

inline BOOL MAKFC_LoginDataRootIsMD5()
{
    if (build::is_agent())
        return TRUE;
    else
        return FALSE;
}

LOGINRECORDS LOGIN_GetLogins(const BLOWFISHKEY& key, BOOL bConvertLogins);
LOGINRECORDS LOGIN_GetLoginsAgent(HANDLE hBFFile, BOOL bConvertLogins=FALSE);
LOGINRECORDS LOGIN_GetLoginsICQ(HANDLE hBFFile, BOOL bConvertLogins=FALSE);

BOOL LOGIN_SetLogins(LOGINRECORDS aLogins, HANDLE hBFFile);
LOGINRECORDS::iterator LOGIN_FindLogin(LOGINRECORDS &aLogins, MAKFC_CString sLogin);
LOGINRECORDS::iterator LOGIN_FindLogin(LOGINRECORDS &aLogins, MAKFC_CString sLogin, MAKFC_CLoginData::TLoginType loginType);
LOGINRECORDS LOGIN_ConvertPasswords(const BLOWFISHKEY& key);
void LOGIN_GetChildLogins( HKEY hLoginKey, MAKFC_CLoginData &login );
void LOGIN_GetChildLogins2( HKEY hLoginKey, MAKFC_CString sPassword, MAKFC_CLoginData *login ) ;

BOOL mra_encode(BYTE *lps);
BOOL mra_decode(BYTE *lps);

BOOL LOGINS_StoreLoginDataToStorage(BOOL bStorePassword, HANDLE hBFFile);
UINT LOGINS_GetKey(BLOWFISHKEY &key, const MAKFC_CString &sPass);
UINT LOGINS_GetKey(BLOWFISHKEY &key, std::vector<BYTE>& md5);
//UINT LOGINS_GetKey(BLOWFISHKEY &key);

void LOGINS_SetAutoLogin(MAKFC_CString login, BOOL autologin);
bool LOGINS_GetAutoLogin(MAKFC_CString login);
void LOGINS_FixAutoLogin(MAKFC_CString login);
BOOL STR_IsEmail(const MAKFC_CString& stStr);