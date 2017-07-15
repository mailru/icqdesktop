#pragma once

#define USE_NAMESPACES

#include "../../external/gigabase_377/gigabase.h"

class CHttpCookiesList;
class MAKFC_CContactInfo;

typedef std::basic_string<TCHAR> tstring;
typedef std::basic_stringstream<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > tstringstream;

typedef unsigned __int64 ARCHIVE_ID;

// System parameters
#define SYSTEM_PARAMETER_DTABASE_CONVERTED	_T( "DatabaseConverted" )	// DWORD

//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// Parameters tables list
// class MRAParamsTables
//////////////////////////////////////////////////////////////////////////
struct MRAParamsTables
{
	MRAParamsTables();

	MRAParamsTables( 
		IN tstring sName, 
		IN gigabase::oid_t oid )
			:	m_sName( sName ), 
				m_oid( oid )
	{
	}

	tstring				m_sName;	// parameters table name
	gigabase::oid_t		m_oid;		// table oid

	TYPE_DESCRIPTOR( ( 
		FIELD( m_sName ),
		FIELD( m_oid )
		) );
};

struct HistoryManagerContext;

struct MRAHistoryCursor;
struct Condition
{
	virtual bool operator()(MRAHistoryCursor&) = 0;
};

struct MRAParamsTables_O : MRAParamsTables
{
	MRAParamsTables_O() {};

	MRAParamsTables_O( IN tstring sName, IN gigabase::oid_t oid )
		:	MRAParamsTables( sName, oid  )
	{
	}

	TYPE_DESCRIPTOR( ( 
		FIELD( m_sName ),
		FIELD( m_oid )
		) );
};


//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE 
// History tables list
// class MRAHistoryTables
//////////////////////////////////////////////////////////////////////////
struct MRAHistoryTables
{
	MRAHistoryTables();
	MRAHistoryTables( tstring sName, gigabase::oid_t oid )
		:	m_sName( sName ), m_oid( oid )
	{
	}

	tstring				m_sName;	// history table name
	gigabase::oid_t		m_oid;		// table oid

	TYPE_DESCRIPTOR( ( 
		FIELD( m_sName ),
		FIELD( m_oid )
		) );
	
};

//-------------------------------------------------------------

//------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// Keys list ( login )
// class MRAKeys
//////////////////////////////////////////////////////////////////////////
struct MRAKeys
{
	MRAKeys();
	MRAKeys( tstring sName );

	tstring	m_sName;	// key name (login)

	TYPE_DESCRIPTOR( ( 
		KEY( m_sName, gigabase::INDEXED | gigabase::HASHED | gigabase::UNIQUE )
		) );
};





//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// Items list ( item - contact )
// class MRAItems
//////////////////////////////////////////////////////////////////////////
struct MRAItems
{
	tstring	m_sName;
	
	MRAItems();
	MRAItems( tstring sName );	// item name (contact)

	TYPE_DESCRIPTOR( ( 
		KEY( m_sName, gigabase::INDEXED | gigabase::HASHED | gigabase::UNIQUE )
		) );
};



//////////////////////////////////////////////////////////////////////////
// Parameters classes
//////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// Parameters table prototype ( not used )
// for each key ( login ) new table will be created
// class MRAParams
//////////////////////////////////////////////////////////////////////////
struct MRAParams
{
	enum eMRAParamType
	{
		eTypeUnknown	= 0,	
		eTypeDWORD		= 1,	
		eTypeString		= 2,
		eTypeRawData	= 3
	};

	const TCHAR*						m_sName;	// parameter name
	gigabase::byte						m_Type;		// parameters type - eMRAParamType
	gigabase::dbArray<gigabase::byte>	m_Value;	// value as binary
	const TCHAR*						m_sItem;	// item name (contact name)


	TYPE_DESCRIPTOR( (
		KEY( m_sName, gigabase::HASHED | gigabase::INDEXED ),
		FIELD( m_Type ),
		FIELD( m_Value ),
		KEY( m_sItem, ( gigabase::HASHED | gigabase::INDEXED ) )
		) );
};



struct MRAParams_O : MRAParams
{
	TYPE_DESCRIPTOR( (
		KEY( m_sName, gigabase::HASHED | gigabase::INDEXED ),
		FIELD( m_Type ),
		FIELD( m_Value ),
		KEY( m_sItem, ( gigabase::HASHED | gigabase::INDEXED ) )
		) );
};








struct MRAParamsCursor;

//////////////////////////////////////////////////////////////////////////
// 
// Parameters table descriptor derived from MRAParams table descriptor
// class MRAParamsdbTableDescriptor
//
//////////////////////////////////////////////////////////////////////////
struct MRAParamsdbTableDescriptor : gigabase::dbTableDescriptor
{
	friend struct MRAParamsCursor;
	friend struct MRAParamsCursor_O;

	MRAParamsdbTableDescriptor( 
		gigabase::char_t const* tableName, 
		gigabase::dbDatabase* db, 
		size_t objSize,
		describeFunc func,
		dbTableDescriptor* original = NULL )
		:	gigabase::dbTableDescriptor( tableName, db, objSize, func, original )
	{

	}

	void Clear()
	{
		gigabase::dbFieldDescriptor* fd;

		for ( fd = firstField; fd != NULL; fd = fd->nextField )
		{
			fd->bTree = 0;
			fd->hashTable = 0;
			fd->attr &= ~gigabase::dbFieldDescriptor::Updated;
		}

		nRows = 0;
		firstRow = 0;
		lastRow = 0;
	}
};


//////////////////////////////////////////////////////////////////////////
// Parameters tables descriptors hash_map
//////////////////////////////////////////////////////////////////////////
typedef std::unordered_map<tstring, CAutoPtr<MRAParamsdbTableDescriptor> > MRAParamsDescs;


//////////////////////////////////////////////////////////////////////////
// Parameters table cursor. Need for parameters tables operations
// class MRAParamsCursor
//////////////////////////////////////////////////////////////////////////
struct MRAParamsCursor : gigabase::dbCursor<MRAParams>
{
	MRAParamsCursor( MRAParamsdbTableDescriptor* lpDesc, gigabase::dbCursorType type = gigabase::dbCursorViewOnly )
		:	gigabase::dbCursor<MRAParams>( lpDesc->db, type )
	{
		table = lpDesc;
	}
};


struct MRAParamsCursor_O : gigabase::dbCursor<MRAParams_O>
{
	MRAParamsCursor_O( MRAParamsdbTableDescriptor* lpDesc, gigabase::dbCursorType type = gigabase::dbCursorViewOnly )
		:	gigabase::dbCursor<MRAParams_O>( lpDesc->db, type )
	{
		table = lpDesc;
	}
};






//////////////////////////////////////////////////////////////////////////
// History and Trash classes
//////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// Trash records
// class MRATrash
//////////////////////////////////////////////////////////////////////////
struct MRATrash
{
	__int64								m_Time;
	DWORD								m_dwType;

	LPCTSTR								m_sFrom;

	gigabase::dbArray<gigabase::byte>	m_Message;
	LPCTSTR								m_sSimple;

	gigabase::byte						m_bDirection;
	gigabase::byte						m_bUnread;

	LPCTSTR								m_sKey;
	LPCTSTR								m_sItem;
	LPCTSTR								m_sProtocol;


	MRATrash()
		:	m_Time( 0 ),
		m_dwType( 0 ),
		m_bDirection( FALSE ),
		m_bUnread( FALSE )
	{
	}

	MRATrash( const MRATrash& history )
		:	m_Time( history.m_Time ),
			m_dwType( history.m_dwType ),
			m_sFrom( history.m_sFrom ),
			m_Message( 
			history.m_Message.get(), 
			history.m_Message.length(), 
			history.m_Message.length() ),
			m_sSimple( history.m_sSimple ),
			m_bDirection( history.m_bDirection ),
			m_bUnread( history.m_bUnread ),
			m_sItem( history.m_sItem ),
			m_sKey( history.m_sKey ),
			m_sProtocol( history.m_sProtocol )
	{
	}

	TYPE_DESCRIPTOR( (
		FIELD( m_Time ),
		FIELD( m_dwType ),
		FIELD( m_sFrom ),
		FIELD( m_bDirection ),
		FIELD( m_bUnread ),
		FIELD( m_Message ),
		FIELD( m_sSimple ),
		FIELD( m_sKey ),
		FIELD( m_sItem ),
		FIELD( m_sProtocol )
		)		);
};





//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// History table prototype ( not used )
// for each pair key, item new table will be created
// class MRAHistory
//////////////////////////////////////////////////////////////////////////
struct MRAHistory
{
	__int64								m_Time;

	gigabase::byte						m_bDirection;
	gigabase::byte						m_bUnread;

	LPCTSTR								m_sFrom;

	LPCTSTR								m_sSimple;
	gigabase::dbArray<gigabase::byte>	m_Message;

	gigabase::nat8						m_archiveId;
		
	MRAHistory()
		:	m_Time( 0 ),
			m_dwType( 0 ),
			m_bDirection( 0 ),
			m_bUnread( 0 ),
			m_sSimple( 0 ),
			m_sFrom( 0 ),
			m_archiveId( 0 )
	{
	}

	TYPE_DESCRIPTOR( (
		FIELD( m_Time ),
		FIELD( m_dwType ),
		FIELD( m_bDirection ),
		FIELD( m_bUnread ),
		FIELD( m_sFrom ),
		FIELD( m_sSimple ),
		FIELD( m_Message )
		// temporarily disabled for server history is disabled at all!
		// FIELD( m_archiveId )
		) );

	DWORD GetType() const;
	void SetType(const DWORD type);
	bool IsTimeInUtcFormat() const;
	__int64 GetUtcFiletime() const;

private:
	static const DWORD UTC_DATE_FLAG = static_cast<DWORD>(1 << 31);

	DWORD m_dwType;

};

//////////////////////////////////////////////////////////////////////////
// History table descriptor derived from MRAHistory table descriptor
// class MRAHistorydbTableDescriptor
//////////////////////////////////////////////////////////////////////////
struct MRAHistorydbTableDescriptor : gigabase::dbTableDescriptor
{
	friend struct MRAHistoryCursor;

	MRAHistorydbTableDescriptor( 
		gigabase::char_t const* tableName, 
		gigabase::dbDatabase* db, 
		size_t objSize,
		describeFunc func,
		dbTableDescriptor* original = NULL )
		:	gigabase::dbTableDescriptor( tableName, db, objSize, func, original )
	{

	}

	~MRAHistorydbTableDescriptor()
	{

	}

	void Clear()
	{
		gigabase::dbFieldDescriptor* fd;

		for ( fd = firstField; fd != NULL; fd = fd->nextField )
		{
			fd->bTree = 0;
			fd->hashTable = 0;
			fd->attr &= ~gigabase::dbFieldDescriptor::Updated;
		}

		nRows = 0;
		firstRow = 0;
		lastRow = 0;
	}
};


//////////////////////////////////////////////////////////////////////////
// Parameters tables descriptors hash_map
//////////////////////////////////////////////////////////////////////////
typedef std::unordered_map<tstring, CAutoPtr<MRAHistorydbTableDescriptor> > MRAHistoryDescs;




//////////////////////////////////////////////////////////////////////////
// History table cursor. Need for history tables operations
// class MRAParamsCursor
//////////////////////////////////////////////////////////////////////////
struct MRAHistoryCursor : gigabase::dbCursor<MRAHistory>
{
	MRAHistoryCursor( MRAHistorydbTableDescriptor* lpDesc, gigabase::dbCursorType type = gigabase::dbCursorViewOnly )
		:	gigabase::dbCursor<MRAHistory>( lpDesc->db, type )
	{
		table = lpDesc;
	}

	bool FastNext() { return gotoNext(); }
};



//////////////////////////////////////////////////////////////////////////
// History record class need for cache and records exchange
// class MRAHistoryCache
//////////////////////////////////////////////////////////////////////////
struct MRAHistoryCache
{
	__int64								m_Time;
	DWORD								m_dwType;

	tstring								m_sFrom;

	gigabase::dbArray<gigabase::byte>	m_Message;
	tstring								m_sSimple;

	gigabase::byte						m_bDirection;
	gigabase::byte						m_bUnread;

	tstring								m_sKey;
	tstring								m_sItem;
	tstring								m_sProtocol;
	ARCHIVE_ID							archive_id_;

	MRAHistoryCache()
		:	m_Time( 0 ),
		m_dwType( 0 ),
		m_bDirection( FALSE ),
		m_bUnread( FALSE ),
		archive_id_(0)
	{
	}

	MRAHistoryCache( const MRAHistory& history )
		:	m_Time( history.GetUtcFiletime() ),
		m_dwType( history.GetType() ),
		m_sFrom( history.m_sFrom ),

		m_Message( 
			history.m_Message.get(), 
			history.m_Message.length(), 
			history.m_Message.length() ),

		m_sSimple( history.m_sSimple ),
		m_bDirection( history.m_bDirection ),
		m_bUnread( history.m_bUnread ),
		archive_id_(history.m_archiveId)
	{
	}

	MRAHistoryCache& operator = ( gigabase::dbCursor<MRATrash>& cur_trash )
	{
		m_Time = cur_trash->m_Time;
		m_dwType = cur_trash->m_dwType;
		m_sFrom = cur_trash->m_sFrom;
		m_Message.assign( cur_trash->m_Message.get(), cur_trash->m_Message.length(), true );
		m_sSimple = cur_trash->m_sSimple;
		m_bDirection = cur_trash->m_bDirection;
		m_bUnread = cur_trash->m_bUnread;
		m_sItem = cur_trash->m_sItem;
		m_sKey = cur_trash->m_sKey;
		m_sProtocol = cur_trash->m_sProtocol;
		archive_id_ = 0;

		return *this;
	}
};


struct IM_Contact_Info
{
	tstring		m_sKeyName;
	tstring		m_sProtocol;
	tstring		m_sItemName;

	IM_Contact_Info( tstring sKeyName, tstring sProtocol, tstring sItemName )
		:	m_sKeyName( sKeyName ), m_sProtocol( sProtocol ), m_sItemName( sItemName )
	{}
};






//////////////////////////////////////////////////////////////////////////
// Short format history record. Used in cache. 
// class MRAHistoryShort
//////////////////////////////////////////////////////////////////////////

struct MRAHistoryShort
{
	gigabase::oid_t	m_oid;
	__int64			m_Time;
	tstring			m_sFrom;
	tstring			m_sIMLogin;
	tstring			m_sSimple;
	gigabase::byte	m_bDirection;
	UINT			m_uiAlertType;
	
	
	MRAHistoryShort()
		:	m_oid( 0 ),
			m_Time( 0 ),
			m_bDirection( 0 )
	{
		m_uiAlertType = 0;
	}

	MRAHistoryShort( MRAHistoryCursor& cur_history )
		:	m_oid( cur_history.currentId().getOid() ),
			m_Time( cur_history.get()->GetUtcFiletime() ),
			m_sFrom( cur_history.get()->m_sFrom ),
			m_sSimple( cur_history.get()->m_sSimple ),
			m_bDirection( cur_history.get()->m_bDirection )
	{
		m_uiAlertType = cur_history.get()->GetType();
		assert(m_uiAlertType);
	}

	MRAHistoryShort& operator = ( MRAHistoryCursor& cur_history )
	{
		m_oid = cur_history.currentId().getOid();
		m_Time = cur_history.get()->GetUtcFiletime();
		m_sFrom = cur_history.get()->m_sFrom;
		m_sSimple = cur_history.get()->m_sSimple;
		m_bDirection = cur_history.get()->m_bDirection;
		m_uiAlertType = cur_history.get()->GetType();
		assert(m_uiAlertType);

		return *this;
	}

	MRAHistoryShort& operator = ( gigabase::dbCursor<MRATrash>& cur_trash )
	{
		m_oid = cur_trash.currentId().getOid();
		m_Time = cur_trash.get()->m_Time;
		m_sFrom = cur_trash.get()->m_sFrom;
		m_sIMLogin = cur_trash.get()->m_sKey;
		m_sSimple = cur_trash.get()->m_sSimple;
		m_bDirection = cur_trash.get()->m_bDirection;
		m_uiAlertType = cur_trash.get()->m_dwType;

		return *this;
	}
};





//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// Unread messages table. Need for fast exchange information about unread messages
// class MRAUnreadMessages
//////////////////////////////////////////////////////////////////////////
struct MRAUnreadMessages
{
	gigabase::db_int4	m_nCount;

	LPCTSTR				m_sKeyName;
	LPCTSTR				m_sItemName;

	MRAUnreadMessages()
		:	m_nCount( 0 ),
			m_sKeyName( 0 ),
			m_sItemName( 0 )
	
	{
	}

	TYPE_DESCRIPTOR( (
		FIELD( m_nCount ),
		FIELD( m_sKeyName ),
		FIELD( m_sItemName )
		) );
};

struct MRAUnreadMessages_O : MRAUnreadMessages
{
	MRAUnreadMessages_O() {}
	MRAUnreadMessages_O( const MRAUnreadMessages& u ) : MRAUnreadMessages( u ) {}

	TYPE_DESCRIPTOR( (
		FIELD( m_nCount ),
		FIELD( m_sKeyName ),
		FIELD( m_sItemName )
		) );
};




//////////////////////////////////////////////////////////////////////////
// gigabase oids list
//////////////////////////////////////////////////////////////////////////
struct HistoryOIDsList : std::vector<gigabase::oid_t>
{
};



//////////////////////////////////////////////////////////////////////////
// class MRAParamShortInfo
// parameter info, need for cache
//////////////////////////////////////////////////////////////////////////
struct MRAParamShortInfo
{
	gigabase::byte m_Type;
	gigabase::dbArray<gigabase::byte> m_Value;

	bool m_bChanged;

	MRAParamShortInfo()
		:	m_bChanged( false ),
			m_Type( 0 )
	{
	}
};

// hash - level 0, by parameter name
struct ParamsMap0 : std::unordered_map< tstring, std::shared_ptr< MRAParamShortInfo > >
{
};

// hash - level 1, by item name ( contact )
struct ParamsMap1 : std::unordered_map< tstring, ParamsMap0 >
{
};

// hash - level2, by key name ( login )
struct MRAParamsCache : std::unordered_map< tstring, ParamsMap1 >
{
	bool	m_bChanged;

	MRAParamsCache();
	
	std::shared_ptr<MRAParamShortInfo> FindParameter( 
		IN const tstring sKey, 
		IN const tstring sItem, 
		IN const tstring sParamName );
	
	// From write
	void PutParameter( 
		IN const tstring sKey, 
		IN const tstring sItem, 
		IN const tstring sParamName, 
		IN MRAParams::eMRAParamType param_type, 
		IN const gigabase::byte* param_value,
		IN DWORD dwValueSize );
	
	// From read
	void PutParameter( 
		IN const tstring sKey, 
		IN const tstring sItem, 
		IN const tstring sParamName, 
		IN std::shared_ptr<MRAParamShortInfo>& ptrInfo );
};





//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// MRAFriends
// class MRAFiends
//////////////////////////////////////////////////////////////////////////
struct MRAFriends
{
	LPCTSTR		m_sKeyName;
	LPCTSTR		m_sItemName;	// email

	LPCTSTR		m_sLocation;
	LPCTSTR		m_sNick;

	UINT		m_nReason;
	UINT		m_nRaiting;

	UINT		m_nStatus;
	UINT		m_nFlags;

	LPCTSTR     m_sBirthday;

	LPCTSTR     m_sFirstName;
	LPCTSTR     m_sLastName;
	UINT        m_sex;

	MRAFriends()
		:
		m_sKeyName( 0 ),
		m_sItemName( 0 ),
		m_sLocation( 0 ),
		m_nReason( 0 ),
		m_nRaiting( 0 ),
		m_nFlags( 0 ),
		m_sNick( 0 ),
		m_sBirthday( 0 ),
		m_sFirstName( 0 ),
		m_sLastName( 0 ),
		m_sex(0)
	{
	}

	TYPE_DESCRIPTOR( (
		FIELD( m_sKeyName ),
		KEY( m_sItemName, gigabase::HASHED | gigabase::INDEXED ),
		FIELD( m_sLocation ),
		FIELD( m_nReason ),
		FIELD( m_nRaiting ),
		FIELD( m_nFlags ),
		FIELD( m_sNick ),
		FIELD( m_sBirthday ),
		FIELD( m_sFirstName ),
		FIELD( m_sLastName ),
		FIELD( m_sex )
		) );
};


struct MRAFriends_O : MRAFriends
{
	MRAFriends_O() {}
	MRAFriends_O( const MRAFriends& f ) : MRAFriends( f ) {}

	TYPE_DESCRIPTOR( (
		FIELD( m_sKeyName ),
		KEY( m_sItemName, gigabase::HASHED | gigabase::INDEXED ),
		FIELD( m_sLocation ),
		FIELD( m_nReason ),
		FIELD( m_nRaiting ),
		FIELD( m_nFlags ),
		FIELD( m_sNick ),
		FIELD( m_sBirthday ),
		FIELD( m_sFirstName ),
		FIELD( m_sLastName ),
		FIELD( m_sex )
		) );
};


//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// MRAGEOInfo
// class MRAGEOInfo
//////////////////////////////////////////////////////////////////////////
struct MRAGEOInfo
{
	int			m_nId;
	LPCTSTR		m_sLattitude;
	LPCTSTR		m_sLongitude;
	LPCTSTR		m_sType;
	LPCTSTR		m_sObject;
	LPCTSTR		m_sSpeed;
	LPCTSTR		m_sTypeCode;
	LPCTSTR		m_sObjectCode;
	LPCTSTR		m_sAzimut;

	int			m_bFromMrim; // if geo status received from server
	int			m_bContact;
	__int64		m_Date;

	
	MRAGEOInfo()
		:
		m_nId( 0 ),
		m_sLattitude( 0 ),
		m_sLongitude( 0 ),
		m_sType( 0 ),
		m_sObject( 0 ),
		m_sSpeed( 0 ),
		m_sTypeCode( 0 ),
		m_sObjectCode( 0 ),
		m_sAzimut( 0 ),
		m_bFromMrim( 0 ),
		m_Date( 0 ),
		m_bContact( 0 )
	{
	}

	TYPE_DESCRIPTOR( (
		KEY( m_nId, gigabase::AUTOINCREMENT | gigabase::INDEXED ),
		FIELD( m_sLattitude ),
		FIELD( m_sLongitude ),
		FIELD( m_sType ),
		FIELD( m_sObject ),
		FIELD( m_sSpeed ),
		FIELD( m_sTypeCode ),
		FIELD( m_sObjectCode ),
		FIELD( m_sAzimut ),
		FIELD( m_bFromMrim ),
		FIELD( m_Date ),
		FIELD( m_bContact )
		) );
};


struct MRAGEOInfo_O : MRAGEOInfo
{
	MRAGEOInfo_O() {}
	MRAGEOInfo_O( const MRAGEOInfo& g ) : MRAGEOInfo( g ) {}

	TYPE_DESCRIPTOR( (
		KEY( m_nId, gigabase::AUTOINCREMENT | gigabase::INDEXED ),
		FIELD( m_sLattitude ),
		FIELD( m_sLongitude ),
		FIELD( m_sType ),
		FIELD( m_sObject ),
		FIELD( m_sSpeed ),
		FIELD( m_sTypeCode ),
		FIELD( m_sObjectCode ),
		FIELD( m_sAzimut ),
		FIELD( m_bFromMrim ),
		FIELD( m_Date ),
		FIELD( m_bContact )
		) );

};


//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// MRAContactsGEOInfo
// class MRAContactsGEOInfo
//////////////////////////////////////////////////////////////////////////
struct MRAContactGEOInfo
{
	LPCTSTR				m_sKeyName;		// Contact email
	__int64				m_Date;
	int					m_nGeoInfoId;	

	MRAContactGEOInfo()
		:
	m_sKeyName( 0 ),
		m_Date( 0 ),
		m_nGeoInfoId( -1 )
	{
	}

	TYPE_DESCRIPTOR( (
		FIELD( m_sKeyName ),
		FIELD( m_Date ),
		FIELD( m_nGeoInfoId )
		) );
};


struct MRAContactGEOInfo_O : MRAContactGEOInfo
{
	MRAContactGEOInfo_O() {}
	MRAContactGEOInfo_O( const MRAContactGEOInfo& info ) : MRAContactGEOInfo( info ) {}

	TYPE_DESCRIPTOR( (
		FIELD( m_sKeyName ),
		FIELD( m_Date ),
		FIELD( m_nGeoInfoId )
		) );
};

enum THistoryConvertationFlags {
	HCF_Started = 0x1, 
	HCF_Finished = 0x2
};

struct MRAHistoryConvertationInfo
{
	MRAHistoryConvertationInfo() :
		m_Version(0), 
		m_Flags(0), 
		m_LastDate(0)
	{
	}

	LPCWSTR m_Login;
	DWORD m_Version;
	LPCWSTR m_Other;
	DWORD m_Flags;
	__int64 m_LastDate;

	TYPE_DESCRIPTOR( (
		FIELD( m_Login ),
		FIELD( m_Version ),
		FIELD( m_Other ), 
		FIELD( m_Flags ), 
		FIELD( m_LastDate )
		) );
};

struct MRAICQMicroblogActivityIDInfo
{
	LPCWSTR m_Login;
	LPCWSTR m_Other;
	LPCWSTR m_ActivityID;
	__int64 m_Date;
	gigabase::oid_t m_HistoryOID;
	LPCWSTR m_SourceURL;

	TYPE_DESCRIPTOR( (
		FIELD( m_Login ), 
		FIELD( m_Other ), 
		FIELD( m_ActivityID ), 
		FIELD( m_Date ), 
		FIELD( m_HistoryOID ), 
		FIELD( m_SourceURL )
		) );
};

//--------------------------------------------------------------------------
struct MRAICQMicroblogActivityID
{
	LPCWSTR m_ActivityID;
	LPCWSTR m_UIN;
	__int64 m_Date;

	TYPE_DESCRIPTOR( (
		FIELD( m_ActivityID ), 
		FIELD( m_UIN ), 
		FIELD( m_Date )
		) );
};

//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// MRAMashineGEOInfo
// class MRAMashineGEOInfo
//////////////////////////////////////////////////////////////////////////

struct MRAMashineGEOInfo
{
	int			m_nClassificatorType;
	LPCTSTR		m_sClassificator;
	int			m_nGeoInfoId;

	MRAMashineGEOInfo()
		:
		m_nClassificatorType( -1 ),
		m_sClassificator( 0 ),
		m_nGeoInfoId( -1 )
	{
	}
    
	TYPE_DESCRIPTOR( (
		FIELD( m_nClassificatorType ),
		FIELD( m_sClassificator ),
		FIELD( m_nGeoInfoId )
		) );
};



struct MRAMashineGEOInfo_O : MRAMashineGEOInfo
{
	MRAMashineGEOInfo_O() {}
	MRAMashineGEOInfo_O( const MRAMashineGEOInfo& info ) : MRAMashineGEOInfo( info ) {}

	TYPE_DESCRIPTOR( (
		FIELD( m_nClassificatorType ),
		FIELD( m_sClassificator ),
		FIELD( m_nGeoInfoId )
		) );
};




//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// MRACookie
// class MRACookie
//////////////////////////////////////////////////////////////////////////

struct MRACookie
{
	LPCTSTR		m_sName;
	LPCTSTR		m_sValue;
	LPCTSTR		m_sDomain;
	LPCTSTR		m_sPath;
	__int64		m_nDate;

	MRACookie()
	: 
		m_sName( 0 ),
		m_sValue( 0 ),
		m_sDomain( 0 ),
		m_sPath( 0 ),
		m_nDate( 0 )
	{
	}

	TYPE_DESCRIPTOR( (
		FIELD( m_sName ),
		FIELD( m_sValue ),
		FIELD( m_sDomain ),
		FIELD( m_sPath ),
		FIELD( m_nDate )
	) );
};


struct MRACookie_O : MRACookie
{
	MRACookie_O() {}
	MRACookie_O( const MRACookie& c ) : MRACookie( c ) {}

	TYPE_DESCRIPTOR( (
		FIELD( m_sName ),
		FIELD( m_sValue ),
		FIELD( m_sDomain ),
		FIELD( m_sPath ),
		FIELD( m_nDate )
		) );
};


//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// MRA_RB_IDs
// class MRA_RB_IDs
//////////////////////////////////////////////////////////////////////////

struct MRA_RB_IDs
{
	int		m_nType;	// 1 - for news
	int		m_nId;
	
	MRA_RB_IDs()
		: 
		m_nType( -1 ),
		m_nId( -1 )
	{
	}

	TYPE_DESCRIPTOR( (
		FIELD( m_nType ),
		FIELD( m_nId )
		) );
};


struct MRA_RB_IDs_O : MRA_RB_IDs
{
	MRA_RB_IDs_O() {}
	MRA_RB_IDs_O( const MRA_RB_IDs& r ) : MRA_RB_IDs( r ) {}

	TYPE_DESCRIPTOR( (
		FIELD( m_nType ),
		FIELD( m_nId )
		) );
};

struct ActiveChatInfo
{
	tstring	m_sItemName;

	__int64			m_iLastInteraction;
	__int64			m_iActivationTime;
};


struct MRA_Active_Chats
{
	LPCTSTR		m_sKeyName;
	LPCTSTR		m_sItemName;

	__int64		m_iLastInteraction;
	__int64		m_iActivationTime;

	MRA_Active_Chats() : m_iLastInteraction( 0 ), m_iActivationTime( 0 ) {}

	TYPE_DESCRIPTOR( (
		FIELD( m_sKeyName ),
		FIELD( m_sItemName ),
		FIELD( m_iLastInteraction ),
		FIELD( m_iActivationTime )
		) );
};


//////////////////////////////////////////////////////////////////////////
// Contact list params
//////////////////////////////////////////////////////////////////////////
struct MRA_ContactList
{
	LPCWSTR		m_sKeyName;
	LPCWSTR		m_sItemName;

	DWORD		m_dwLoadTime_22x22; 
	DWORD		m_dwServerTime_22x22;
	DWORD		m_dwHttpStatus_22x22;

	DWORD		m_dwLoadTime_32x32;
	DWORD		m_dwServerTime_32x32;
	DWORD		m_dwHttpStatus_32x32;

	DWORD		m_dwLoadTime_45x45;
	DWORD		m_dwServerTime_45x45;
	DWORD		m_dwHttpStatus_45x45;

	DWORD		m_dwLoadTime_50x50;
	DWORD		m_dwServerTime_50x50;
	DWORD		m_dwHttpStatus_50x50;

	DWORD		m_dwLoadTime_60x60;
	DWORD		m_dwServerTime_60x60;
	DWORD		m_dwHttpStatus_60x60;

	DWORD		m_dwLoadTime_90x90;
	DWORD		m_dwServerTime_90x90;
	DWORD		m_dwHttpStatus_90x90;

	DWORD		m_dwLoadTime_120x120;
	DWORD		m_dwServerTime_120x120;
	DWORD		m_dwHttpStatus_120x120;

	DWORD		m_dwLoadTime_180x180;
	DWORD		m_dwServerTime_180x180;
	DWORD		m_dwHttpStatus_180x180;

	DWORD		m_dwLoadTimeIcq_64x64;
	DWORD		m_dwServerTimeIcq_64x64;
	DWORD		m_dwHttpStatusIcq_64x64;

	DWORD		m_dwLoadTimeIcq_100x100;
	DWORD		m_dwServerTimeIcq_100x100;
	DWORD		m_dwHttpStatusIcq_100x100;

	gigabase::dbArray<gigabase::byte>	m_rci;

	MRA_ContactList() : m_sKeyName( 0 ), m_sItemName( 0 ),
		m_dwLoadTime_22x22( 0 ), m_dwServerTime_22x22( 0 ), m_dwHttpStatus_22x22( 0 ), 
		m_dwLoadTime_32x32( 0 ), m_dwServerTime_32x32( 0 ), m_dwHttpStatus_32x32( 0 ), 
		m_dwLoadTime_45x45( 0 ), m_dwServerTime_45x45( 0 ), m_dwHttpStatus_45x45( 0 ),
		m_dwLoadTime_50x50( 0 ), m_dwServerTime_50x50( 0 ), m_dwHttpStatus_50x50( 0 ),
		m_dwLoadTime_60x60( 0 ), m_dwServerTime_60x60( 0 ), m_dwHttpStatus_60x60( 0 ),
		m_dwLoadTime_90x90( 0 ), m_dwServerTime_90x90( 0 ), m_dwHttpStatus_90x90( 0 ),
		m_dwLoadTime_120x120( 0 ), m_dwServerTime_120x120( 0 ), m_dwHttpStatus_120x120( 0 ),
		m_dwLoadTime_180x180( 0 ), m_dwServerTime_180x180( 0 ), m_dwHttpStatus_180x180( 0 ),
		m_dwLoadTimeIcq_64x64( 0 ), m_dwServerTimeIcq_64x64( 0 ), m_dwHttpStatusIcq_64x64( 0 ),
		m_dwLoadTimeIcq_100x100( 0 ), m_dwServerTimeIcq_100x100( 0 ), m_dwHttpStatusIcq_100x100( 0 )

		{
		}

	TYPE_DESCRIPTOR( (
		KEY( m_sKeyName, gigabase::HASHED | gigabase::INDEXED ), FIELD( m_sItemName ),
		FIELD( m_dwLoadTime_22x22 ), FIELD( m_dwServerTime_22x22 ), FIELD( m_dwHttpStatus_22x22 ),
		FIELD( m_dwLoadTime_32x32 ), FIELD( m_dwServerTime_32x32 ), FIELD( m_dwHttpStatus_32x32 ),
		FIELD( m_dwLoadTime_45x45 ), FIELD( m_dwServerTime_45x45 ),	FIELD( m_dwHttpStatus_45x45 ),
		FIELD( m_dwLoadTime_50x50 ), FIELD( m_dwServerTime_50x50 ), FIELD( m_dwHttpStatus_50x50 ),
		FIELD( m_dwLoadTime_60x60 ), FIELD( m_dwServerTime_60x60 ), FIELD( m_dwHttpStatus_60x60 ),
		FIELD( m_dwLoadTime_90x90 ), FIELD( m_dwServerTime_90x90 ), FIELD( m_dwHttpStatus_90x90 ),
		FIELD( m_dwLoadTime_120x120 ), FIELD( m_dwServerTime_120x120 ),	FIELD( m_dwHttpStatus_120x120 ),
		FIELD( m_dwLoadTime_180x180 ), FIELD( m_dwServerTime_180x180 ),	FIELD( m_dwHttpStatus_180x180 ),
		FIELD( m_dwLoadTimeIcq_64x64 ), FIELD( m_dwServerTimeIcq_64x64 ),	FIELD( m_dwHttpStatusIcq_64x64 ),
		FIELD( m_dwLoadTimeIcq_100x100 ), FIELD( m_dwServerTimeIcq_100x100 ),	FIELD( m_dwHttpStatusIcq_100x100 ),
		FIELD( m_rci )
		) );
};

struct ContactListParams
{
	tstring		m_sKeyName;
	tstring		m_sItemName;

	DWORD		m_dwLoadTime_22x22; 
	DWORD		m_dwServerTime_22x22;
	DWORD		m_dwHttpStatus_22x22;

	DWORD		m_dwLoadTime_32x32;
	DWORD		m_dwServerTime_32x32;
	DWORD		m_dwHttpStatus_32x32;

	DWORD		m_dwLoadTime_45x45;
	DWORD		m_dwServerTime_45x45;
	DWORD		m_dwHttpStatus_45x45;

	DWORD		m_dwLoadTime_50x50;
	DWORD		m_dwServerTime_50x50;
	DWORD		m_dwHttpStatus_50x50;

	DWORD		m_dwLoadTime_60x60;
	DWORD		m_dwServerTime_60x60;
	DWORD		m_dwHttpStatus_60x60;

	DWORD		m_dwLoadTime_90x90;
	DWORD		m_dwServerTime_90x90;
	DWORD		m_dwHttpStatus_90x90;

	DWORD		m_dwLoadTime_120x120;
	DWORD		m_dwServerTime_120x120;
	DWORD		m_dwHttpStatus_120x120;

	DWORD		m_dwLoadTime_180x180;
	DWORD		m_dwServerTime_180x180;
	DWORD		m_dwHttpStatus_180x180;

	DWORD		m_dwLoadTimeIcq_64x64;
	DWORD		m_dwServerTimeIcq_64x64;
	DWORD		m_dwHttpStatusIcq_64x64;

	DWORD		m_dwLoadTimeIcq_100x100;
	DWORD		m_dwServerTimeIcq_100x100;
	DWORD		m_dwHttpStatusIcq_100x100;

	gigabase::dbArray<gigabase::byte>	m_rci;

	ContactListParams( const MRA_ContactList& cl ) : 
		m_sKeyName( cl.m_sKeyName ), m_sItemName( cl.m_sItemName ),
		m_dwLoadTime_22x22( cl.m_dwLoadTime_22x22 ), 
		m_dwServerTime_22x22( cl.m_dwServerTime_22x22 ), 
		m_dwHttpStatus_22x22( cl.m_dwHttpStatus_22x22 ), 
		m_dwLoadTime_32x32( cl.m_dwLoadTime_32x32 ), 
		m_dwServerTime_32x32( cl.m_dwServerTime_32x32 ), 
		m_dwHttpStatus_32x32( cl.m_dwHttpStatus_32x32 ), 
		m_dwLoadTime_45x45( cl.m_dwLoadTime_45x45 ), 
		m_dwServerTime_45x45( cl.m_dwServerTime_45x45 ), 
		m_dwHttpStatus_45x45( cl.m_dwHttpStatus_45x45 ),
		m_dwLoadTime_50x50( cl.m_dwLoadTime_50x50 ), 
		m_dwServerTime_50x50( cl.m_dwServerTime_50x50 ), 
		m_dwHttpStatus_50x50( cl.m_dwHttpStatus_50x50 ),
		m_dwLoadTime_60x60( cl.m_dwLoadTime_60x60 ), 
		m_dwServerTime_60x60( cl.m_dwServerTime_60x60 ), 
		m_dwHttpStatus_60x60( cl.m_dwHttpStatus_60x60 ),
		m_dwLoadTime_90x90( cl.m_dwLoadTime_90x90 ), 
		m_dwServerTime_90x90( cl.m_dwServerTime_90x90 ), 
		m_dwHttpStatus_90x90( cl.m_dwHttpStatus_90x90 ),
		m_dwLoadTime_120x120( cl.m_dwLoadTime_120x120 ), 
		m_dwServerTime_120x120( cl.m_dwServerTime_120x120 ), 
		m_dwHttpStatus_120x120( cl.m_dwHttpStatus_120x120 ),
		m_dwLoadTime_180x180( cl.m_dwLoadTime_180x180 ), 
		m_dwServerTime_180x180( cl.m_dwServerTime_180x180 ), 
		m_dwHttpStatus_180x180( cl.m_dwHttpStatus_180x180 ),
		m_dwLoadTimeIcq_64x64( cl.m_dwLoadTimeIcq_64x64 ), 
		m_dwServerTimeIcq_64x64( cl.m_dwServerTimeIcq_64x64 ), 
		m_dwHttpStatusIcq_64x64( cl.m_dwHttpStatusIcq_64x64 ),
		m_dwLoadTimeIcq_100x100( cl.m_dwLoadTimeIcq_100x100 ), 
		m_dwServerTimeIcq_100x100( cl.m_dwServerTimeIcq_100x100 ), 
		m_dwHttpStatusIcq_100x100( cl.m_dwHttpStatusIcq_100x100 )
	{
		m_rci.assign( cl.m_rci.get(), cl.m_rci.length() );
	}
};


//////////////////////////////////////////////////////////////////////////
// DATABASE TABLE
// MRAFileSharing
// class MRAFileSharing
//////////////////////////////////////////////////////////////////////////
struct MRAFileSharing
{
	__int64		m_nTime;
	UINT		m_nDirection;
	LPCTSTR		m_sKeyNameFrom;
	LPCTSTR		m_sKeyNameTo;
	UINT		m_nState;
	UINT		m_nType;
	__int64		m_nFileSize;



	MRAFileSharing()
		:
	m_nTime(0),
	m_nDirection(0),
	m_sKeyNameFrom(0),
	m_sKeyNameTo(0),
	m_nState(0),
	m_nType(0),
	m_nFileSize(0)
	{
	}

	TYPE_DESCRIPTOR( (
		FIELD( m_nTime ),
		FIELD( m_nDirection ),
		KEY( m_sKeyNameFrom, gigabase::INDEXED ),
		KEY( m_sKeyNameTo, gigabase::INDEXED ),
		FIELD( m_nState ),
		FIELD( m_nType ),
		FIELD( m_nFileSize )
		) );

private:
	MRAFileSharing(const MRAFileSharing&) {}
	MRAFileSharing& operator=(const MRAFileSharing&) {}
};
//////////////////////////////////////////////////////////////////////////

struct MRADataBase : gigabase::dbDatabase
{
	tstring				m_sFileName;
	gigabase::dbOSFile	m_osfile;

private:

	virtual void GetOpenParams( OUT gigabase::dbDatabase::OpenParameters& params );

public:

	struct AutoDetachObject
	{
		MRADataBase*	m_p;
		BOOL			m_bAttached;

		AutoDetachObject( MRADataBase* p );
		~AutoDetachObject();
	};

	virtual bool OpenDatabase();
	virtual void Close();

	BOOL TryAttach();
	void Attach();
	void Detach();

	void BeginTransaction();
	void Commit();
	void PreCommit();
	void Rollback();

	virtual ~MRADataBase()
	{

	}
};

struct MRAOptionsBase;

struct MRAHistoryBase : MRADataBase
{
private:
	MRAOptionsBase*		m_lpOptionsBase;

	BOOL LinkMraHistoryTable( IN tstring& sTableName, IN BOOL bCommit, OUT MRAHistorydbTableDescriptor** lpDesc);
    	
protected:

	struct ConditionIsUnread : Condition
	{
		bool operator()(MRAHistoryCursor&);
	};
	
	MRAHistoryDescs		m_descs_history;	// hash_map history

public:

	BOOL get_history_tables(std::unordered_map<tstring, MRAHistorydbTableDescriptor*> &descriptors);
	MRAHistorydbTableDescriptor* GetHistoryTable( IN tstring sName, BOOL bRead );
	MRAHistorydbTableDescriptor* GetHistoryTable( IN const tstring sKey, IN const tstring sItem, 
		BOOL bRead = TRUE, BOOL bCommit = TRUE );

	MRAParamsdbTableDescriptor* GetParamsTableOld( IN const tstring sTableName, gigabase::oid_t oid );

	void GetNames( std::list<tstring>& lstNames );
	virtual void Close();

	MRAHistoryBase( MRAOptionsBase* lpOptionsBase );
	virtual ~MRAHistoryBase();
};

struct MRAOptionsBase : MRADataBase
{
	friend MRAHistoryBase;

	MRAParamsDescs		m_descs_params;		// hash_map parameters tables descriptors

private:

	MRAParamsCache		m_params_cache;		// parameters cache

    CHandle				m_commit_thread_sync;
    CHandle				m_commit_thread_flush;
	CHandle				m_commit_thread;

public:

	MRAParamsdbTableDescriptor* GetParamsTable( IN const tstring sKey, BOOL bRead = TRUE );
	MRAParamsdbTableDescriptor* GetParamsTable_( IN const tstring sTableName, BOOL bRead = TRUE );

private:

	BOOL LinkMraParamsTables();

	BOOL CreateCommitThread();

	virtual void GetOpenParams( OUT gigabase::dbDatabase::OpenParameters& params );

	BOOL SaveParameter( 
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		IN MRAParams::eMRAParamType param_type, 
		IN const gigabase::byte* param_value,
		IN DWORD dwValueSize );

	BOOL SaveParameter( 
		IN const tstring sKeyName, 
		IN const tstring sItemName, 
		IN const tstring sParamName,
		IN std::shared_ptr<MRAParamShortInfo> lpParam,
		IN bool bRecursive = false );

	BOOL ReadParameter(
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		IN MRAParams::eMRAParamType param_type, 
		OUT gigabase::dbArray<gigabase::byte>& param_value );

	BOOL ResetUnread_in(
		IN const tstring sKeyName,
		IN const tstring sItemName
		);

    BOOL SaveCachedObjects();
public:

	MRAOptionsBase();
	virtual ~MRAOptionsBase();

	virtual bool OpenDatabase();
	virtual void Close();
	
	static DWORD WINAPI CommitThread( LPVOID lpParams );
    BOOL FlushParametersToDatabase();

	BOOL SaveDWORD( 
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		IN const DWORD dwValue );

	BOOL ReadDWORD( 
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		OUT DWORD& dwValue );

	//////////////////////////////////////////////////////////////////////////
	BOOL SaveInt64( 
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		IN const __int64 iValue );

	BOOL ReadInt64( 
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		OUT __int64& iValue );

	//////////////////////////////////////////////////////////////////////////

	BOOL SaveString( 
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		IN const tstring sValue );

	BOOL ReadString( 
		IN const tstring sKeyName, 
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		OUT tstring& sValue );

	//////////////////////////////////////////////////////////////////////////

	BOOL SaveRawData( 
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		IN const gigabase::dbArray<gigabase::byte>& data );

	BOOL ReadRawData( 
		IN const tstring sKeyName, 
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		OUT gigabase::dbArray<gigabase::byte>& data );

	//////////////////////////////////////////////////////////////////////////

	BOOL SaveUnknown( 
		IN const tstring sKeyName, 
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		IN gigabase::dbArray<gigabase::byte>& data );
 
    bool RCI_Get( IN const tstring sKeyName, IN const tstring sItemName, OUT MAKFC_CContactInfo& rci );
    bool RCI_GetNoCache( IN const tstring sKeyName, IN const tstring sItemName, OUT MAKFC_CContactInfo& rci );
    void ReadMutedChats(const tstring& sKeyName, std::list<std::string>& _chats);
};



extern MRAHistoryBase	g_history_base;
extern MRAOptionsBase	g_options_base;

//////////////////////////////////////////////////////////////////////////
// Database API
// class MRABase
//////////////////////////////////////////////////////////////////////////
class MRABase
{
	BOOL OpenDatabase();

	bool ConvertDatabase();

public:

	static void SetCriticalError();

	MRAHistoryBase* GetHistoryBase();
	MRAOptionsBase* GetOptionsBase();
	
	MRABase();
	virtual ~MRABase();

	void CommitHistory();
	void CommitOptions();

	static DWORD GetVersion() { return 1; }

	BOOL Open( 
		IN tstring sOptionsFileName );

	void Close();

	BOOL Restore( IN LPCTSTR sFromFile, IN HWND hWndParent );
	BOOL BeforeConvert();

	void StoreToFile( IN LPCTSTR sFileName );
	
	__int64 GetSize();

	//////////////////////////////////////////////////////////////////////////
	// MRA parameters functions
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////

	BOOL SaveDWORD( 
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		IN const DWORD dwValue );

	BOOL ReadDWORD( 
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		OUT DWORD& dwValue );
    	
	//////////////////////////////////////////////////////////////////////////
	BOOL SaveInt64( 
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		IN const __int64 iValue );

	BOOL ReadInt64( 
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		OUT __int64& iValue );

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	BOOL SaveString( 
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		IN const tstring sValue );

	BOOL ReadString( 
		IN const tstring sKeyName, 
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		OUT tstring& sValue );

	//////////////////////////////////////////////////////////////////////////

	BOOL SaveRawData( 
		IN const tstring sKeyName,
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		IN const gigabase::dbArray<gigabase::byte>& data );

	BOOL ReadRawData( 
		IN const tstring sKeyName, 
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		OUT gigabase::dbArray<gigabase::byte>& data );

	//////////////////////////////////////////////////////////////////////////

	BOOL SaveUnknown( 
		IN const tstring sKeyName, 
		IN const tstring sItemName, 
		IN const tstring sParamName, 
		IN gigabase::dbArray<gigabase::byte>& data );

    bool RCI_Get(IN const tstring sKeyName, IN const tstring sItemName, OUT MAKFC_CContactInfo& rci);
    bool RCI_GetNoCache( IN const tstring sKeyName, IN const tstring sItemName, OUT MAKFC_CContactInfo& rci );

    void ReadMutedChats(const tstring& sKeyName, std::list<std::string>& _chats);

	//////////////////////////////////////////////////////////////////////////
};
