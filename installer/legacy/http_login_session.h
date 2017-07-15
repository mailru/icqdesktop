#include "string.h"
#include <atltime.h>

class MRABase;
class MAKFC_CLoginData;

bool LoadExported(
    IN MRABase& _db,
    IN MAKFC_CString sKey, 
    OUT MAKFC_CString& devID, 
    OUT MAKFC_CString& aolToken, 
    OUT MAKFC_CString& sessionKey, 
    OUT time_t& expirationDate, 
    OUT __int64& time_offset );

bool LoadSessionData(IN MAKFC_CString databaseKey, 
                     IN MRABase& _db,
                     OUT MAKFC_CString& sDevId, 
                     OUT MAKFC_CString& sA, 
                     OUT MAKFC_CString& sSessionKey, 
                     OUT MAKFC_CString& iTimeOffset);

struct wim_auth_parameters
{
    MAKFC_CString m_aimid;
    MAKFC_CString m_a_token;
    MAKFC_CString m_session_key;
    MAKFC_CString m_dev_id;
    time_t m_exipired_in;
    __int64 m_time_offset;
    MAKFC_CString m_aim_sid;
    MAKFC_CString m_fetch_url;
    __int64 m_next_fetch_time;
    __int64 m_active_session_id;
    MAKFC_CString m_database_key;

    wim_auth_parameters() : 
        m_exipired_in(::time(0)),
        m_time_offset(0), 
        m_next_fetch_time(CFileTime::GetCurrentTime().GetTime()),
        m_active_session_id(0)
    {

    }
};

std::shared_ptr<wim_auth_parameters> load_auth_params_from_db(IN MRABase& _db, const MAKFC_CLoginData& _ld);
MAKFC_CString GetDatabaseKey(const MAKFC_CLoginData& _ld);