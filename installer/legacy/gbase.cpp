#include "stdafx.h"


#include "gbase.h"
#include "contact_info.h"
#include "stream.h"

#define GIGABASE_EXCEPTION_ID exc
#define GIGABASE_TRY_BEGIN try {
#define GIGABASE_TRY_END } catch(const gigabase::dbException &GIGABASE_EXCEPTION_ID) { GIGABASE_EXCEPTION_ID; } catch(...) { MRABase::SetCriticalError(); }
#define GIGABASE_EXCEPTION() GIGABASE_EXCEPTION_ID

MRAOptionsBase	g_options_base;
MRAHistoryBase	g_history_base( &g_options_base );

REGISTER_IN( MRAParams, &g_history_base );
REGISTER_IN( MRAItems, &g_history_base );
REGISTER_IN( MRAKeys, &g_history_base );
REGISTER_IN( MRAHistory, &g_history_base );
REGISTER_IN( MRAParamsTables, &g_history_base );
REGISTER_IN( MRAHistoryTables, &g_history_base );

REGISTER_IN( MRATrash, &g_history_base );
REGISTER_IN( MRAUnreadMessages, &g_history_base );
REGISTER_IN( MRAFriends, &g_history_base );
REGISTER_IN( MRACookie, &g_history_base );
REGISTER_IN( MRAGEOInfo, &g_history_base );
REGISTER_IN( MRAContactGEOInfo, &g_history_base );
REGISTER_IN( MRAMashineGEOInfo, &g_history_base );
REGISTER_IN( MRA_RB_IDs, &g_history_base );
REGISTER_IN( MRAHistoryConvertationInfo, &g_history_base );
REGISTER_IN( MRAICQMicroblogActivityIDInfo, &g_history_base );

REGISTER_IN( MRAParams_O, &g_options_base );
REGISTER_IN( MRAParamsTables_O, &g_options_base );
REGISTER_IN( MRAFriends_O, &g_options_base );			// - converted
REGISTER_IN( MRAUnreadMessages_O, &g_options_base );	// - converted
REGISTER_IN( MRACookie_O, &g_options_base );			// - converted
REGISTER_IN( MRAGEOInfo_O, &g_options_base );			// - converted
REGISTER_IN( MRAContactGEOInfo_O, &g_options_base );	// - converted
REGISTER_IN( MRAMashineGEOInfo_O, &g_options_base );	// - converted
REGISTER_IN( MRA_RB_IDs_O, &g_options_base );			// - converted
REGISTER_IN( MRAICQMicroblogActivityID, &g_options_base );
REGISTER_IN( MRA_Active_Chats, &g_options_base );
REGISTER_IN( MRA_ContactList, &g_options_base );
//#define NEWSTAT
#ifdef NEWSTAT
	REGISTER_IN( MRAFileSharing, &g_options_base );
#endif

#define DATABASE_POOL_SIZE		10000						// pages
#define DATABASE_FILE_EXTEND	32*1024*1024				// bytes
#define DATABASE_INIT_INDEX		64*1024						// typical number of objects

#define OPTIONS_DATABASE_POOL_SIZE		1000						// pages
#define OPTIONS_DATABASE_FILE_EXTEND	1*1024*1024			// bytes
#define OPTIONS_DATABASE_INIT_INDEX		64*1024				// typical number of objects


#define SYSTEM_PARAMETERS_KEY	_T( "MRASystem" )
#define SYSTEM_PARAMETERS_ITEM	_T( "MRASystem" )


void MRABase::SetCriticalError()
{
}

DWORD MRAHistory::GetType() const
{
	return (m_dwType & ~UTC_DATE_FLAG);
}

void MRAHistory::SetType(const DWORD type)
{
	m_dwType = (type | UTC_DATE_FLAG);
}

bool MRAHistory::IsTimeInUtcFormat() const
{
	return ((m_dwType & UTC_DATE_FLAG) != 0);
}

__int64 MRAHistory::GetUtcFiletime() const
{
	if (IsTimeInUtcFormat())
	{
		return m_Time;
	}

	__int64 localFiletime = 0;
	::LocalFileTimeToFileTime(
		reinterpret_cast<const FILETIME*>(&m_Time), 
		reinterpret_cast<FILETIME*>(&localFiletime));
	return localFiletime;
}

//////////////////////////////////////////////////////////////////////////
// class MRAParamsTables
//////////////////////////////////////////////////////////////////////////
MRAParamsTables::MRAParamsTables()
	:	m_oid( 0 )
{
}




//////////////////////////////////////////////////////////////////////////
// class MRAHistoryTables
//////////////////////////////////////////////////////////////////////////
MRAHistoryTables::MRAHistoryTables()
	:	m_oid( 0 )
{
}

//----------------------------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// class MRAKeys
//////////////////////////////////////////////////////////////////////////
MRAKeys::MRAKeys()
{
}

MRAKeys::MRAKeys( tstring sName )
	:	m_sName( sName )
{
}





//////////////////////////////////////////////////////////////////////////
// class MRAItems
//////////////////////////////////////////////////////////////////////////
MRAItems::MRAItems()
{
}

MRAItems::MRAItems( tstring sName )
	:	m_sName( sName )
{
}








//////////////////////////////////////////////////////////////////////////
// class MRAParamsCache
//////////////////////////////////////////////////////////////////////////
MRAParamsCache::MRAParamsCache()
	:	m_bChanged( false )
{
}

std::shared_ptr<MRAParamShortInfo> MRAParamsCache::FindParameter( 
								 IN const tstring sKey, 
								 IN const tstring sItem, 
								 IN const tstring sParamName )
{
	iterator iter2 = find( sKey );
	if ( iter2 == end() )
		return 0;

	ParamsMap1::iterator iter1 = iter2->second.find( sItem );
	if ( iter1 == iter2->second.end() )
		return 0;

	ParamsMap0::iterator iter0 = iter1->second.find( sParamName );
	if ( iter0 == iter1->second.end() )
		return 0;

	return iter0->second;
}


void MRAParamsCache::PutParameter( 
								  IN const tstring sKey, 
								  IN const tstring sItem, 
								  IN const tstring sParamName, 
								  IN MRAParams::eMRAParamType param_type,
								  IN const gigabase::byte* param_value,
								  IN DWORD dwValueSize )
{
	std::shared_ptr<MRAParamShortInfo> ptrParam( new MRAParamShortInfo );

	ptrParam->m_bChanged = true;
	ptrParam->m_Type = (gigabase::byte)param_type;
	ptrParam->m_Value.assign( param_value, dwValueSize );

	(*this)[sKey][sItem][sParamName] = ptrParam;

	m_bChanged = true;
}


void MRAParamsCache::PutParameter( 
								  IN const tstring sKey, 
								  IN const tstring sItem, 
								  IN const tstring sParamName, 
								  IN std::shared_ptr<MRAParamShortInfo>& ptrInfo )
{
	(*this)[sKey][sItem][sParamName] = ptrInfo;
}






bool MRADataBase::OpenDatabase()
{
	if ( m_osfile.open( m_sFileName.c_str(), 0 ) != gigabase::dbFile::ok )
		return FALSE;

	gigabase::dbDatabase::OpenParameters params;
	GetOpenParams( params );

	DWORD dwErrors = 0;
	
	bool bRes = false;

	try
	{
		if ( dwErrors < 3 )
			bRes = !!open( params );
	}
	catch ( ... )
	{
		bRes = false;
	}

	if ( !bRes )
	{
		m_osfile.close();
	}
    
    return bRes;
}


void MRADataBase::Close()
{
	close();
	m_osfile.close();
}



void MRADataBase::GetOpenParams( OUT gigabase::dbDatabase::OpenParameters& params )
{
	params.file = &m_osfile;
	params.deleteFileOnClose = false;
	params.accessType = gigabase::dbDatabase::dbAllAccess;
	params.extensionQuantum = DATABASE_FILE_EXTEND;
	params.initIndexSize = DATABASE_INIT_INDEX;
	params.poolSize = DATABASE_POOL_SIZE;
	params.nThreads = 0;
	params.doNotReuseOidAfterClose = true;
	params.transactionCommitDelay = 0;
}


BOOL MRADataBase::TryAttach()
{
	if ( threadContext.get() == NULL ) 
	{
		__super::attach();

		return TRUE;
	}

	return FALSE;
}

void MRADataBase::Attach()
{
	attach();
}

void MRADataBase::Detach()
{
	detach();
}

void MRADataBase::BeginTransaction()
{
GIGABASE_TRY_BEGIN
	beginTransaction(gigabase::dbExclusiveLock);
GIGABASE_TRY_END
}

void MRADataBase::PreCommit()
{
GIGABASE_TRY_BEGIN
	precommit();
GIGABASE_TRY_END
}

void MRADataBase::Commit()
{
GIGABASE_TRY_BEGIN
	commit();
GIGABASE_TRY_END
}

void MRADataBase::Rollback()
{
GIGABASE_TRY_BEGIN
	rollback();
GIGABASE_TRY_END
}

//////////////////////////////////////////////////////////////////////////
// class MRABase::AutoDetachObject
//////////////////////////////////////////////////////////////////////////
MRADataBase::AutoDetachObject::AutoDetachObject( MRADataBase* p )
	:	m_p( p ) 
{
	m_bAttached = m_p->TryAttach();
}

MRADataBase::AutoDetachObject::~AutoDetachObject()
{ 
	if ( m_bAttached )
	{
		m_p->Detach();
	}
}



//////////////////////////////////////////////////////////////////////////
// MRAHistoryBase
//////////////////////////////////////////////////////////////////////////

MRAHistoryBase::MRAHistoryBase( MRAOptionsBase* lpOptionsBase )
	:	m_lpOptionsBase( lpOptionsBase )
{
}

MRAHistoryBase::~MRAHistoryBase()
{
}

void MRAHistoryBase::Close()
{
	__super::Close();

	m_descs_history.clear();
}


void MRAHistoryBase::GetNames( std::list<tstring>& lstNames )
{
	gigabase::dbCursor<MRAHistoryTables> cur_tables;

	if ( cur_tables.select() )
	{
		do 
		{
			lstNames.push_back( cur_tables->m_sName );
		} 
		while ( cur_tables.next() );
	}

	precommit();
}

MRAHistorydbTableDescriptor* MRAHistoryBase::GetHistoryTable( IN tstring sName, BOOL bRead )
{
	MRAHistorydbTableDescriptor* lpDesc = 0;

	if ( !LinkMraHistoryTable( sName, TRUE, &lpDesc ) )
		return 0;

	if ( lpDesc || bRead )
		return lpDesc;

	do 
	{
		beginTransaction( gigabase::dbExclusiveLock );

		CAutoPtr<MRAHistorydbTableDescriptor> ptrDesc( new 
			MRAHistorydbTableDescriptor( sName.c_str(), NULL, sizeof( MRAHistory ), &dbDescribeComponentsOfMRAHistory )
			);

		ptrDesc->Clear();
		ptrDesc->setFlags();

		gigabase::oid_t oid = addNewTable( ptrDesc.m_p );
		modified = true;

		addIndices( ptrDesc.m_p );

		if ( !ptrDesc->checkRelationship() )
			break;

		lpDesc = ptrDesc.Detach();

		insert( MRAHistoryTables( sName, oid ) );
	} 
	while ( false );

	commit();

	return lpDesc;
}

MRAHistorydbTableDescriptor* MRAHistoryBase::GetHistoryTable( IN const tstring sKey, IN const tstring sItem, 
	BOOL bRead, BOOL bCommit )
{
	std::vector<TCHAR> sKeyBuffer(sKey.length() + 1);
	_tcscpy( &sKeyBuffer[0], sKey.c_str() );
	std::vector<TCHAR> sItemBuffer(sItem.length() + 1);
	_tcscpy( &sItemBuffer[0], sItem.c_str() );

	tstring sTableName = tstring( _T( "mrahistory_" ) ) + CharLower( &sKeyBuffer[0] ) + _T( "_" ) + 
		CharLower( &sItemBuffer[0] );

	MRAHistorydbTableDescriptor* lpDesc = 0;

	GIGABASE_TRY_BEGIN

		MRAHistoryDescs::iterator iter = m_descs_history.find( sTableName );

	if ( iter != m_descs_history.end() )
		return iter->second.m_p;

	if ( !LinkMraHistoryTable( sTableName, bCommit, &lpDesc ) )
		return 0;

	if ( lpDesc || bRead )
		return lpDesc;

	do 
	{
		beginTransaction( gigabase::dbExclusiveLock );

		CAutoPtr<MRAHistorydbTableDescriptor> ptrDesc( new 
			MRAHistorydbTableDescriptor( sTableName.c_str(), NULL, sizeof( MRAHistory ), &dbDescribeComponentsOfMRAHistory )
			);

		ptrDesc->Clear();
		ptrDesc->setFlags();

		gigabase::oid_t oid = addNewTable( ptrDesc.m_p );
		modified = true;

		addIndices( ptrDesc.m_p );

		if ( !ptrDesc->checkRelationship() )
			break;

		lpDesc = ptrDesc.m_p;

		m_descs_history[sTableName] = ptrDesc;

		insert( MRAHistoryTables( sTableName, oid ) );
	} 
	while ( false );

	GIGABASE_TRY_END

	if (bCommit)
	{
		Commit();
	}
	else
	{
		PreCommit();
	}

	return lpDesc;
}


BOOL MRAHistoryBase::get_history_tables(std::unordered_map<tstring, MRAHistorydbTableDescriptor*> &descriptors)
{
	BOOL bRes = TRUE;

	beginTransaction( gigabase::dbUpdateLock );

	gigabase::dbCursor<MRAHistoryTables> cur_tables;
	gigabase::dbGetTie tie;
	
	if (cur_tables.select())
	{
		do 
		{
			if (descriptors.find(cur_tables->m_sName) == descriptors.end())
			{
				continue;
			}
			CAutoPtr<MRAHistorydbTableDescriptor> ptrDesc( new 
				MRAHistorydbTableDescriptor( _tcsdup(cur_tables->m_sName.c_str()), NULL, sizeof( MRAHistory ), &dbDescribeComponentsOfMRAHistory )
				);

			ptrDesc->Clear();

			gigabase::dbTable* lpTable = (gigabase::dbTable*) getRow( tie, cur_tables->m_oid );

			if ( !lpTable )
				continue;
			
			if ( !ptrDesc->equal( lpTable ) )
			{
				beginTransaction( gigabase::dbExclusiveLock );

				modified = true;

				if ( lpTable->nRows == 0 )
				{
					ptrDesc->match( lpTable, true, false, false);
					updateTableDescriptor( ptrDesc.m_p, cur_tables->m_oid, lpTable );
				} 
				else
				{
					allowColumnsDeletion();
					reformatTable( cur_tables->m_oid, ptrDesc.m_p );
					unlinkTable(ptrDesc.m_p);
				}
			}

			linkTable( ptrDesc.m_p, cur_tables->m_oid );

			ptrDesc->setFlags();

			addIndices( ptrDesc.m_p );

			bRes = ptrDesc->checkRelationship();

			if ( bRes )
			{
				descriptors[cur_tables->m_sName] = ptrDesc.Detach();
			}
		}
		while ( cur_tables.next() );
	}

	Commit();
//	endTransaction(threadContext.get());

	return bRes;
}


BOOL MRAHistoryBase::LinkMraHistoryTable( 
								  IN IN tstring& sTableName, 
								  IN BOOL bCommit, 
								  OUT MRAHistorydbTableDescriptor** lpDesc)
{
	BOOL bRes = TRUE;

	beginTransaction( gigabase::dbUpdateLock );

	{
		CAtlString lowered_table = sTableName.c_str();
		sTableName = (LPCTSTR)lowered_table.MakeLower();
	}
	gigabase::dbQuery qSelTable;
	qSelTable = _T( "lower( m_sName )=" ), sTableName;

	gigabase::dbCursor<MRAHistoryTables> cur_tables;

	gigabase::dbGetTie tie;

	if ( cur_tables.select( qSelTable ) )
	{
		do 
		{
			CAutoPtr<MRAHistorydbTableDescriptor> ptrDesc( new 
				MRAHistorydbTableDescriptor( /*_tcsdup*/(cur_tables->m_sName.c_str()), NULL, sizeof( MRAHistory ), &dbDescribeComponentsOfMRAHistory )
				);

			ptrDesc->Clear();

			gigabase::dbTable* lpTable = (gigabase::dbTable*) getRow( tie, cur_tables->m_oid );
			if ( !lpTable )
			{
				bRes = FALSE;
				break;
			}

			if ( !ptrDesc->equal( lpTable ) )
			{
				beginTransaction( gigabase::dbExclusiveLock );

				modified = true;
				if ( lpTable->nRows == 0 )
				{
					ptrDesc->match( lpTable, true, false, false);
					updateTableDescriptor( ptrDesc.m_p, cur_tables->m_oid, lpTable );
				} 
				else
				{
					allowColumnsDeletion();
					reformatTable( cur_tables->m_oid, ptrDesc.m_p );
					unlinkTable(ptrDesc.m_p);
				}
			}

			linkTable( ptrDesc.m_p, cur_tables->m_oid );

			ptrDesc->setFlags();

			addIndices( ptrDesc.m_p );

			bRes = ptrDesc->checkRelationship();

			if ( bRes )
			{
				*lpDesc = ptrDesc.m_p;
				m_descs_history[sTableName] = ptrDesc;
			}
		}
		while ( false );
	}

	if (bCommit)
		Commit();
	else
		PreCommit();

	return bRes;
}


//////////////////////////////////////////////////////////////////////////
// MRAOptionsBase
//////////////////////////////////////////////////////////////////////////

MRAOptionsBase::MRAOptionsBase()
{

}

MRAOptionsBase::~MRAOptionsBase()
{

}

void MRAOptionsBase::GetOpenParams( OUT gigabase::dbDatabase::OpenParameters& params )
{
	params.file = &m_osfile;
	params.deleteFileOnClose = false;
	params.accessType = gigabase::dbDatabase::dbAllAccess;
	params.extensionQuantum = OPTIONS_DATABASE_FILE_EXTEND;
	params.initIndexSize = OPTIONS_DATABASE_INIT_INDEX;
	params.poolSize = OPTIONS_DATABASE_POOL_SIZE;
	params.nThreads = 0;
	params.doNotReuseOidAfterClose = true;
	params.transactionCommitDelay = 0;
}


bool MRAOptionsBase::OpenDatabase()
{
	if ( !CreateCommitThread() )
		return FALSE;

	if ( !__super::OpenDatabase() )
		return false;

	::ResumeThread( m_commit_thread );

	return !!LinkMraParamsTables();
}

void MRAOptionsBase::Close()
{
	SetEvent( m_commit_thread_sync );
	::WaitForSingleObject( m_commit_thread, INFINITE );

	__super::Close();

	m_descs_params.clear();
}

BOOL MRAOptionsBase::LinkMraParamsTables()
{
GIGABASE_TRY_BEGIN

	beginTransaction( gigabase::dbUpdateLock );

	gigabase::dbCursor<MRAParamsTables_O> cur_tables;

	gigabase::dbGetTie tie;

	if ( cur_tables.select() )
	{
		do 
		{
			CAutoPtr<MRAParamsdbTableDescriptor> ptrDesc( new 
				MRAParamsdbTableDescriptor( cur_tables->m_sName.c_str(), NULL, sizeof( MRAParams_O ), &dbDescribeComponentsOfMRAParams )
				);

			ptrDesc->Clear();

			gigabase::dbTable* lpTable = (gigabase::dbTable*) getRow( tie, cur_tables->m_oid );
			if ( !lpTable )
				continue;

			if ( !ptrDesc->equal( lpTable ) )
			{
				beginTransaction( gigabase::dbExclusiveLock );

				modified = true;
				if ( lpTable->nRows == 0 )
				{
					//					TRACE_MSG( ( STRLITERAL( "Replace definition of table '%s'\n"), ptrDesc->name ) );
					ptrDesc->match( lpTable, true, false, false);

					updateTableDescriptor( ptrDesc.m_p, cur_tables->m_oid, lpTable );
				} 
				else
				{
					allowColumnsDeletion();
					reformatTable( cur_tables->m_oid, ptrDesc.m_p );
					unlinkTable(ptrDesc.m_p);
				}
			}

			linkTable( ptrDesc.m_p, cur_tables->m_oid );

			ptrDesc->setFlags();

			addIndices( ptrDesc.m_p );

			if ( !ptrDesc->checkRelationship() )
				continue;

			m_descs_params[cur_tables->m_sName.c_str()] = ptrDesc;

		}
		while ( cur_tables.next() );
	}

GIGABASE_TRY_END

	Commit();

	return TRUE;
}

MRAParamsdbTableDescriptor* MRAOptionsBase::GetParamsTable_( IN const tstring sTableName, BOOL bRead )
{
	MRAParamsdbTableDescriptor* lpDesc = 0;

GIGABASE_TRY_BEGIN

	MRAParamsDescs::iterator iter = m_descs_params.find( sTableName );

	if ( iter != m_descs_params.end() )
		return iter->second.m_p;

	if ( bRead )
		return 0;

	do 
	{
		beginTransaction( gigabase::dbExclusiveLock );

		CAutoPtr<MRAParamsdbTableDescriptor> ptrDesc( new 
			MRAParamsdbTableDescriptor( sTableName.c_str(), NULL, sizeof( MRAParams_O ), &dbDescribeComponentsOfMRAParams_O )
			);

		ptrDesc->Clear();
		ptrDesc->setFlags();

		gigabase::oid_t oid = addNewTable( ptrDesc.m_p );
		modified = true;

		addIndices( ptrDesc.m_p );

		if ( !ptrDesc->checkRelationship() )
			break;

		lpDesc = ptrDesc.m_p;

		m_descs_params[sTableName] = ptrDesc;

		insert( MRAParamsTables_O( sTableName, oid ) );
	} 
	while ( false );

GIGABASE_TRY_END

	Commit();

	return lpDesc;
}

MRAParamsdbTableDescriptor* MRAOptionsBase::GetParamsTable( IN const tstring sKey, BOOL bRead )
{
	tstring sTableName = tstring( _T( "MRAParams_" ) ) + sKey;

	return GetParamsTable_( sTableName, bRead );
}

BOOL MRAOptionsBase::CreateCommitThread()
{
	m_commit_thread_sync.Close();
	m_commit_thread_sync.Attach( ::CreateEvent( NULL, TRUE, FALSE, NULL ) );

    m_commit_thread_flush.Close();
    m_commit_thread_flush.Attach( ::CreateEvent( NULL, FALSE, FALSE, NULL ) );

	DWORD dwThreadId = 0;
	m_commit_thread.Close();
	m_commit_thread.Attach( ::CreateThread( 0, 0, CommitThread, this, CREATE_SUSPENDED, &dwThreadId ) );

	return ( m_commit_thread.m_h != 0 );
}

DWORD WINAPI MRAOptionsBase::CommitThread( LPVOID lpParams )
{
	if (lpParams == NULL)
	{
		assert(!"CommitThread was started with invalid parameters!");
		return 0;
	}

	MRAOptionsBase* lpDB = ( MRAOptionsBase* ) lpParams;

	DWORD dwWaitResult = 0;

	AutoDetachObject so( lpDB );

	while ( true )
	{
        assert(lpDB);

        const HANDLE events[] = { lpDB->m_commit_thread_sync, lpDB->m_commit_thread_flush };
		dwWaitResult = (::WaitForMultipleObjects)( _countof(events), events, FALSE, 10000 );
        assert(dwWaitResult != WAIT_FAILED);

		lpDB->SaveCachedObjects();

		if ( dwWaitResult == WAIT_OBJECT_0 )
			break;
	}

	return 0;
}

BOOL MRAOptionsBase::SaveCachedObjects()
{
	return TRUE;
}

BOOL MRAOptionsBase::SaveParameter(
							IN const tstring sKeyName, 
							IN const tstring sItemName, 
							IN const tstring sParamName,
							IN std::shared_ptr<MRAParamShortInfo> lpParam,
							IN bool/* bRecursive*/ )
{
	BOOL bRes = FALSE;

	if ( !isOpen() )
		return FALSE;

	MRAParamsdbTableDescriptor* lpDesc = GetParamsTable( sKeyName, FALSE );
	if ( lpDesc == 0 )
	{
		assert( FALSE );
		return FALSE;
	}

GIGABASE_TRY_BEGIN

	do
	{
		// if parameter exist
		gigabase::dbQuery qSelectParamer;
		qSelectParamer = _T( "m_sItem=" ), sItemName.c_str(), _T( "and m_sName=" ), sParamName.c_str();

		MRAParamsCursor_O cur_upd_params( lpDesc, gigabase::dbCursorForUpdate );

		if ( cur_upd_params.select( qSelectParamer ) )
		{
			cur_upd_params->m_Type = lpParam->m_Type;
			cur_upd_params->m_Value = lpParam->m_Value;

			bRes = cur_upd_params.update();
			break;
		}

		// Insert the parameter
		MRAParams_O params;

		params.m_sItem = sItemName.c_str();
		params.m_sName = sParamName.c_str();
		params.m_Type = lpParam->m_Type;
		params.m_Value = lpParam->m_Value;

		gigabase::dbReference<MRAParams_O> ref_params;

		if ( !insertRecord( lpDesc, &ref_params, &params, false ) )
		{
			assert( FALSE );
			break;
		}

		lpParam->m_bChanged = false;

		bRes = TRUE;

	}
	while ( false );

GIGABASE_TRY_END

	PreCommit();

	return bRes;
}


BOOL MRAOptionsBase::SaveParameter( 
							IN const tstring sKeyName, 
							IN const tstring sItemName, 
							IN const tstring sParamName, 
							IN MRAParams::eMRAParamType param_type, 
							IN const gigabase::byte* param_value,
							IN DWORD dwValueSize )
{
	m_params_cache.PutParameter( sKeyName, sItemName, sParamName, param_type, param_value, dwValueSize );

	return TRUE;
}

BOOL MRAOptionsBase::ReadParameter(
							IN const tstring sKeyName, 
							IN const tstring sItemName, 
							IN const tstring sParamName, 
							IN MRAParams::eMRAParamType param_type, 
							OUT gigabase::dbArray<gigabase::byte>& param_value )
{
	std::shared_ptr<MRAParamShortInfo> lpInfo = m_params_cache.FindParameter( sKeyName, sItemName, sParamName );

	if ( lpInfo )
	{
		param_value = lpInfo->m_Value;

		return TRUE;
	}

	if ( !isOpen() )
		return FALSE;

	BOOL bAttached = FALSE;

	bAttached = TryAttach();

	BOOL bRes = FALSE;

GIGABASE_TRY_BEGIN

	do 
	{
		MRAParamsdbTableDescriptor* lpDesc = GetParamsTable( sKeyName, TRUE );
		if ( lpDesc == 0 )
			break;

		gigabase::dbQuery qSelectParamer;
		qSelectParamer = _T( "m_sItem=" ), sItemName.c_str(), _T( "and m_sName=" ), sParamName.c_str();

		MRAParamsCursor_O cur_sel_params( lpDesc );

		if ( !cur_sel_params.select( qSelectParamer ) )
			break;

		if ( cur_sel_params->m_Type != param_type && 
			cur_sel_params->m_Type != MRAParams::eTypeUnknown )
			break;

		param_value = cur_sel_params->m_Value;

		/*CAutoPtr<MRAParamShortInfo> ptrInfo( new MRAParamShortInfo );
		ptrInfo->m_Type = (gigabase::byte)param_type;
		ptrInfo->m_Value = param_value;
		ptrInfo->m_bChanged = false;*/

		bRes = TRUE;
	} 
	while ( false );

GIGABASE_TRY_END

	if ( bAttached )
		Detach();
	else
		PreCommit();

	return bRes;
}



BOOL MRAOptionsBase::FlushParametersToDatabase()
{
    const BOOL result = ::SetEvent(m_commit_thread_flush);
    assert(result != 0);

    return (result != 0);
}



//////////////////////////////////////////////////////////////////////////
// Class MRABase
//////////////////////////////////////////////////////////////////////////

MRABase::MRABase()
{
}

MRABase::~MRABase()
{
}

//////////////////////////////////////////////////////////////////////////
// MRA Base private functions
//////////////////////////////////////////////////////////////////////////



BOOL MRABase::OpenDatabase()
{
    MRAOptionsBase* lpBase = GetOptionsBase();
	if ( !lpBase )
		return FALSE;

	DWORD dwValue = 0;
	if ( !lpBase->ReadDWORD( L"", L"", L"database_converted", dwValue ) || !dwValue )
	{
		if ( !ConvertDatabase() )
			return FALSE;

		lpBase->SaveDWORD( L"", L"", L"database_converted", (DWORD) 1 );
	}

	return TRUE;
}


BOOL MRABase::Open(IN tstring sOptionsFileName)
{
GIGABASE_TRY_BEGIN

	g_options_base.m_sFileName = sOptionsFileName;
	
	return OpenDatabase();

GIGABASE_TRY_END

	return FALSE;
}




bool MRABase::ConvertDatabase()
{
	MRAHistoryBase* lpHistoryBase = GetHistoryBase();
	MRAOptionsBase* lpOptionsBase = GetOptionsBase();

	if ( !lpHistoryBase || !lpOptionsBase )
		return false;

	
	//////////////////////////////////////////////////////////////////////////
	//  Convert the Friends table
	//////////////////////////////////////////////////////////////////////////
	gigabase::dbCursor<MRAFriends> cur_friends;
	if ( cur_friends.select() )
	{
		do
		{
			MRAFriends_O o( *cur_friends.get() );
			lpOptionsBase->insert( o );
		}
		while ( cur_friends.next() );
	}

	//////////////////////////////////////////////////////////////////////////
	// Convert the MRACookie table
	//////////////////////////////////////////////////////////////////////////
	gigabase::dbCursor<MRACookie> cur_cookie;
	if ( cur_cookie.select() )
	{
		do 
		{
			MRACookie_O o( *cur_cookie.get() );
			lpOptionsBase->insert( o );
		} 
		while ( cur_cookie.next() );
	}

	//////////////////////////////////////////////////////////////////////////
	// Convert the MRA_RB_IDs table
	//////////////////////////////////////////////////////////////////////////
	gigabase::dbCursor<MRA_RB_IDs> cur_rb;
	if ( cur_rb.select() )
	{
		do 
		{
			MRA_RB_IDs_O o( *cur_rb.get() );
			lpOptionsBase->insert( o );
		} 
		while ( cur_rb.next() );
	}

	//////////////////////////////////////////////////////////////////////////
	// Convert the MRAUnreadMessages table
	//////////////////////////////////////////////////////////////////////////
	gigabase::dbCursor<MRAUnreadMessages> cur_unread;
	if ( cur_unread.select() )
	{
		do 
		{
			MRAUnreadMessages_O o( *cur_unread.get() );
			lpOptionsBase->insert( o );
		} 
		while ( cur_unread.next() );
	}

	//////////////////////////////////////////////////////////////////////////
	// Convert GEO tables
	//////////////////////////////////////////////////////////////////////////

	std::map<int, int> ids_map;

	gigabase::dbCursor<MRAGEOInfo> cur_info;

	if ( cur_info.select() )
	{
		do 
		{
			int nIdOld = cur_info->m_nId;

			MRAGEOInfo_O o( *cur_info.get() );
			o.m_nId = 0;

			lpOptionsBase->insert( o );

			ids_map[nIdOld] = o.m_nId;
		} 
		while ( cur_info.next() );
	}

	gigabase::dbCursor<MRAContactGEOInfo> cur_cgi;

	if ( cur_cgi.select() )
	{
		do 
		{
			std::map<int, int>::iterator iter = ids_map.find( cur_cgi->m_nGeoInfoId );

			if ( iter != ids_map.end() )
			{
				MRAContactGEOInfo_O o( *cur_cgi.get() );

				o.m_nGeoInfoId = iter->second;

				lpOptionsBase->insert( o );
			}
		}
		while ( cur_cgi.next() );
	}

	gigabase::dbCursor<MRAMashineGEOInfo> cur_mgi;

	if ( cur_mgi.select() )
	{
		do 
		{
			std::map<int, int>::iterator iter = ids_map.find( cur_mgi->m_nGeoInfoId );

			if ( iter != ids_map.end() )
			{
				MRAMashineGEOInfo_O o( *cur_mgi.get() );

				o.m_nGeoInfoId = iter->second;

				lpOptionsBase->insert( o );
			}
		} 
		while ( cur_mgi.next() );
	}

	//////////////////////////////////////////////////////////////////////////
	// Convert Parameters tables
	//////////////////////////////////////////////////////////////////////////

	typedef std::list< std::pair<tstring, gigabase::oid_t> > ParametersTablesList;
	ParametersTablesList tables_list;

	gigabase::dbCursor<MRAParamsTables> cur_tables;
	
	if ( cur_tables.select() )
	{
		do 
		{
			tables_list.push_back( std::make_pair( cur_tables->m_sName, cur_tables->m_oid ) );
		} 
		while ( cur_tables.next() );
	}

	for ( ParametersTablesList::iterator iter_p = tables_list.begin(); iter_p != tables_list.end(); iter_p++ )
	{
		MRAParamsdbTableDescriptor* lpDescOld = lpHistoryBase->GetParamsTableOld( iter_p->first, iter_p->second );

		if ( lpDescOld )
		{
			MRAParamsdbTableDescriptor* lpDescNew = lpOptionsBase->GetParamsTable_( iter_p->first, FALSE );

			if ( !lpDescNew )
				continue;

		//	lpOptionsBase->m_descs_params[iter_p->first] = CAutoPtr<MRAParamsdbTableDescriptor>(lpDescNew);

			MRAParamsCursor cur_sel_params( lpDescOld );

			if ( cur_sel_params.select() )
			{
				do 
				{
					gigabase::dbQuery qSelectParamer;
					qSelectParamer = _T( "m_sItem=" ), cur_sel_params->m_sItem, _T( "and m_sName=" ), cur_sel_params->m_sName;

					MRAParamsCursor_O cur_upd_params( lpDescNew, gigabase::dbCursorForUpdate );

					if ( cur_upd_params.select( qSelectParamer ) )
					{
						cur_upd_params->m_Type = cur_sel_params->m_Type;
						cur_upd_params->m_Value = cur_sel_params->m_Value;

						cur_upd_params.update();

						continue;
					}

					MRAParams_O params;

					params.m_sItem = cur_sel_params->m_sItem;
					params.m_sName = cur_sel_params->m_sName;
					params.m_Type = cur_sel_params->m_Type;
					params.m_Value = cur_sel_params->m_Value;

					gigabase::dbReference<MRAParams_O> ref_params;

					if ( !lpOptionsBase->insertRecord( lpDescNew, &ref_params, &params, false ) )
					{
						assert( FALSE );
						break;
					}
				} 
				while ( cur_sel_params.next() );
			}
		}
	}

	lpHistoryBase->Commit();
	lpOptionsBase->Commit();
		
	return true;
}





__int64 MRABase::GetSize()
{
	return ( (__int64) GetHistoryBase()->getDatabaseSize() + (__int64) GetOptionsBase()->getDatabaseSize() );
}




//////////////////////////////////////////////////////////////////////////
// Restore database from backup

void MRABase::Close()
{
	if ( g_options_base.isOpen() )
	{
		MRAOptionsBase *ob = GetOptionsBase();
		if (ob) { ob->Close(); }
	}
	
	if (g_history_base.isOpen())
	{
		MRAHistoryBase *hb = GetHistoryBase();
		if (hb) { hb->Close(); }
	}
}


MRAHistoryBase* MRABase::GetHistoryBase()
{
	if ( !g_history_base.isOpen() )
	{
	//	MB( L"Open History Base" );

		if ( !g_history_base.OpenDatabase() )
			return 0;
	}

	return &g_history_base;
}

MRAOptionsBase* MRABase::GetOptionsBase()
{
	if ( !g_options_base.isOpen() )
	{
		if ( !g_options_base.OpenDatabase() )
			return 0;
	}

	return &g_options_base;
}

BOOL MRAOptionsBase::SaveDWORD( 
						IN const tstring sKeyName, 
						IN const tstring sItemName, 
						IN const tstring sParamName, 
						IN const DWORD dwValue )
{
	return SaveParameter( sKeyName, sItemName, sParamName, MRAParams::eTypeDWORD, 
		(gigabase::byte*) &dwValue, sizeof( dwValue ) );
}






BOOL MRAOptionsBase::ReadDWORD( 
						IN const tstring sKeyName, 
						IN const tstring sItemName, 
						IN const tstring sParamName, 
						OUT DWORD& dwValue )
{
	gigabase::dbArray<gigabase::byte> param_value;

	if ( !ReadParameter( sKeyName, sItemName, sParamName, MRAParams::eTypeDWORD, param_value ) )
		return FALSE;

	if ( sizeof( dwValue ) != param_value.length() )
		return FALSE;

	memcpy( &dwValue, param_value.get(), sizeof( dwValue ) );

	return TRUE;
}

BOOL MRAOptionsBase::SaveInt64( 
			   IN const tstring sKeyName,
			   IN const tstring sItemName, 
			   IN const tstring sParamName, 
			   IN const __int64 iValue )
{
	return SaveParameter( sKeyName, sItemName, sParamName, MRAParams::eTypeRawData, 
		(gigabase::byte*) &iValue, sizeof( iValue ) );
}

BOOL MRAOptionsBase::ReadInt64( 
			   IN const tstring sKeyName,
			   IN const tstring sItemName, 
			   IN const tstring sParamName, 
			   OUT __int64& iValue )
{
	gigabase::dbArray<gigabase::byte> param_value;

	if ( !ReadParameter( sKeyName, sItemName, sParamName, MRAParams::eTypeRawData, param_value ) )
		return FALSE;

	if ( sizeof( iValue ) != param_value.length() )
		return FALSE;

	memcpy( &iValue, param_value.get(), sizeof( iValue ) );

	return TRUE;
}




BOOL MRAOptionsBase::SaveString( 
						 IN const tstring sKeyName, 
						 IN const tstring sItemName, 
						 IN const tstring sParamName, 
						 IN const tstring sValue )
{
	return SaveParameter( sKeyName, sItemName, sParamName, MRAParams::eTypeString, 
		(gigabase::byte*) sValue.c_str(), sValue.length() * sizeof( TCHAR ) );
}

BOOL MRAOptionsBase::ReadString( 
						 IN const tstring sKeyName, 
						 IN const tstring sItemName, 
						 IN const tstring sParamName, 
						 OUT tstring& sValue )
{
	gigabase::dbArray<gigabase::byte> param_value;

	if ( !ReadParameter( sKeyName, sItemName, sParamName, MRAParams::eTypeString, param_value ) )
		return FALSE;

	sValue.assign( ( LPCTSTR ) param_value.get(), param_value.length()/sizeof( TCHAR ) );

	return TRUE;
}

BOOL MRAOptionsBase::SaveRawData( 
						  IN const tstring sKeyName,
						  IN const tstring sItemName, 
						  IN const tstring sParamName, 
						  IN const gigabase::dbArray<gigabase::byte>& data )
{
	return SaveParameter( sKeyName, sItemName, sParamName, MRAParams::eTypeRawData, 
		data.get(), data.length() );
}

BOOL MRAOptionsBase::ReadRawData( 
						  IN const tstring sKeyName, 
						  IN const tstring sItemName, 
						  IN const tstring sParamName, 
						  OUT gigabase::dbArray<gigabase::byte>& data )
{
	return ReadParameter( sKeyName, sItemName, sParamName, MRAParams::eTypeRawData, data );
}


BOOL MRAOptionsBase::SaveUnknown( 
						  IN const tstring sKeyName, 
						  IN const tstring sItemName, 
						  IN const tstring sParamName, 
						  IN gigabase::dbArray<gigabase::byte>& data )
{
	return SaveParameter( sKeyName, sItemName, sParamName, MRAParams::eTypeUnknown, 
		data.get(), data.length() );
}


bool MRAOptionsBase::RCI_Get( IN const tstring sKeyName, IN const tstring sItemName, OUT MAKFC_CContactInfo& rci )
{
    if (sItemName.empty())
    {
        return false;
    }

    if ( !isOpen() )
    {
        return false;
    }

    AutoDetachObject so( this );

    bool bRes = false;

    GIGABASE_TRY_BEGIN

        do 
        {
            gigabase::dbQuery qSelect;
            qSelect = _T( "m_sKeyName=" ), sKeyName.c_str(), _T( "and m_sItemName=" ), sItemName.c_str();

            gigabase::dbCursor<MRA_ContactList> cur_sel;

            int records_count = cur_sel.select( qSelect );

            if ( records_count )
            {
                if ( cur_sel->m_rci.length() )
                {
                    MAKFC_CReaderWriter rw;
                    rw.write( cur_sel->m_rci.get(), cur_sel->m_rci.length() );

                    bRes = !!rci.unserialize( ( MAKFC_CBufferInputStream* ) &rw );
                }

            }
        }
        while ( false );

        GIGABASE_TRY_END

            if ( !so.m_bAttached )
                PreCommit();

        return bRes;
}

void MRAOptionsBase::ReadMutedChats(const tstring& sKeyName, std::list<std::string>& _chats)
{
    if ( !isOpen() )
        return;

    BOOL bAttached = FALSE;

    bAttached = TryAttach();

GIGABASE_TRY_BEGIN

    do 
    {
        MRAParamsdbTableDescriptor* lpDesc = GetParamsTable(sKeyName, TRUE);
        if (lpDesc == 0)
            break;

        gigabase::dbQuery qSelectParamer;
        qSelectParamer = L"m_sName=", L"NOTIFY_ABOUT_MESSAGES";

        MRAParamsCursor_O cur_sel_params( lpDesc );

        if (!cur_sel_params.select(qSelectParamer))
        {
            break;
        }

        do 
        {
            if (MRAParams::eTypeDWORD != cur_sel_params->m_Type)
                continue;

            if (sizeof(DWORD) != cur_sel_params->m_Value.length())
                continue;

            DWORD dwValue = 0;
            memcpy(&dwValue, cur_sel_params->m_Value.get(), sizeof(dwValue));

            if (dwValue)
                continue;

            _chats.push_back(MAKFC_CString(cur_sel_params->m_sItem).NetStrA(CP_UTF8));
        } 
        while (cur_sel_params.next());

    } 
    while (false);

GIGABASE_TRY_END

    if (bAttached)
    {
        Detach();
    }
    else
    {
        PreCommit();
    }
}


bool MRAOptionsBase::RCI_GetNoCache( IN const tstring sKeyName, IN const tstring sItemName, OUT MAKFC_CContactInfo& rci )
{
    gigabase::dbArray<gigabase::byte> param_value;
    ReadParameter(sKeyName, sItemName, L"RCI", MRAParams::eTypeRawData, param_value);

    MAKFC_CReaderWriter rw;
    rw.write(param_value.get(), param_value.length());

    if ( !rci.unserialize( ( MAKFC_CBufferInputStream *) &rw ) )
    {
        return false;
    }

    return true;
}






MRAParamsdbTableDescriptor* MRAHistoryBase::GetParamsTableOld( IN const tstring sTableName, gigabase::oid_t oid )
{
	beginTransaction( gigabase::dbUpdateLock );

	gigabase::dbGetTie tie;

	MRAParamsdbTableDescriptor* lpDesk = 0;

	CAutoPtr<MRAParamsdbTableDescriptor> ptrDesc( new MRAParamsdbTableDescriptor( sTableName.c_str(), NULL, sizeof( MRAParams ), 
		&dbDescribeComponentsOfMRAParams ) );

	ptrDesc->Clear();

	gigabase::dbTable* lpTable = (gigabase::dbTable*) getRow( tie, oid );

	do 
	{
		if ( !lpTable )
			break;

		if ( !ptrDesc->equal( lpTable ) )
		{
			beginTransaction( gigabase::dbExclusiveLock );

			modified = true;

			if ( lpTable->nRows == 0 )
			{
				ptrDesc->match( lpTable, true, false, false);

				updateTableDescriptor( ptrDesc.m_p, oid, lpTable );
			} 
			else
			{
				allowColumnsDeletion();
				reformatTable( oid, ptrDesc.m_p );
				unlinkTable(ptrDesc.m_p);
			}
		}

		linkTable( ptrDesc.m_p, oid );

		ptrDesc->setFlags();

		addIndices( ptrDesc.m_p );

		if ( !ptrDesc->checkRelationship() )
			continue;

		lpDesk = ptrDesc.Detach();

	} 
	while ( false );

	Commit();
	
	return lpDesk;
}


																																						 



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////



void MRABase::CommitHistory()
{
	GetHistoryBase()->Commit();
}

void MRABase::CommitOptions()
{
	GetOptionsBase()->Commit();
}

//////////////////////////////////////////////////////////////////////////
// MRABase public functions
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// MRA parameters functions
//////////////////////////////////////////////////////////////////////////

BOOL MRABase::SaveDWORD( 
						IN const tstring sKeyName, 
						IN const tstring sItemName, 
						IN const tstring sParamName, 
						IN const DWORD dwValue )
{
	return GetOptionsBase()->SaveDWORD( sKeyName, sItemName, sParamName, dwValue );
}

BOOL MRABase::ReadDWORD( 
						IN const tstring sKeyName, 
						IN const tstring sItemName, 
						IN const tstring sParamName, 
						OUT DWORD& dwValue )
{
	return GetOptionsBase()->ReadDWORD( sKeyName, sItemName, sParamName, dwValue );
}

BOOL MRABase::SaveInt64( 
			   IN const tstring sKeyName,
			   IN const tstring sItemName, 
			   IN const tstring sParamName, 
			   IN const __int64 iValue )
{
	return GetOptionsBase()->SaveInt64( sKeyName, sItemName, sParamName, iValue );
}

BOOL MRABase::ReadInt64( 
			   IN const tstring sKeyName,
			   IN const tstring sItemName, 
			   IN const tstring sParamName, 
			   OUT __int64& iValue )
{
	return GetOptionsBase()->ReadInt64( sKeyName, sItemName, sParamName, iValue );
}


BOOL MRABase::SaveString( 
						 IN const tstring sKeyName, 
						 IN const tstring sItemName, 
						 IN const tstring sParamName, 
						 IN const tstring sValue )
{
	return GetOptionsBase()->SaveString( sKeyName, sItemName, sParamName, sValue );
}

BOOL MRABase::ReadString( 
						 IN const tstring sKeyName, 
						 IN const tstring sItemName, 
						 IN const tstring sParamName, 
						 OUT tstring& sValue )
{
	return GetOptionsBase()->ReadString( sKeyName, sItemName, sParamName, sValue );
}

BOOL MRABase::SaveRawData( 
						  IN const tstring sKeyName,
						  IN const tstring sItemName, 
						  IN const tstring sParamName, 
						  IN const gigabase::dbArray<gigabase::byte>& data )
{
    return GetOptionsBase()->SaveRawData( sKeyName, sItemName, sParamName, data );
}

BOOL MRABase::ReadRawData( 
						  IN const tstring sKeyName, 
						  IN const tstring sItemName, 
						  IN const tstring sParamName, 
						  OUT gigabase::dbArray<gigabase::byte>& data )
{
	return GetOptionsBase()->ReadRawData( sKeyName, sItemName, sParamName, data );
}


BOOL MRABase::SaveUnknown( 
							IN const tstring sKeyName, 
							IN const tstring sItemName, 
							IN const tstring sParamName, 
							IN gigabase::dbArray<gigabase::byte>& data )
{
	return GetOptionsBase()->SaveUnknown( sKeyName, sItemName, sParamName, data );
}

bool MRABase::RCI_Get(IN const tstring sKeyName, IN const tstring sItemName, OUT MAKFC_CContactInfo& rci)
{
    return GetOptionsBase()->RCI_Get( sKeyName, sItemName, rci );
}

bool MRABase::RCI_GetNoCache( IN const tstring sKeyName, IN const tstring sItemName, OUT MAKFC_CContactInfo& rci )
{
    return GetOptionsBase()->RCI_GetNoCache( sKeyName, sItemName, rci );
}

void MRABase::ReadMutedChats(const tstring& sKeyName, std::list<std::string>& _chats)
{
    return GetOptionsBase()->ReadMutedChats(sKeyName, _chats);
}
