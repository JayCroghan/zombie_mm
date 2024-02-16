#include "myutils.h"
#include "VFunc.h"

extern IVEngineServer		*m_Engine;
extern IServerGameDLL		*m_ServerDll;
extern IPlayerInfoManager	*m_PlayerInfoManager;
extern PlayerInfo_t			g_Players[65];

#undef GetProp
#undef GetClassName

Trie *m_pClasses;
List<DataTableInfo *> m_Tables;
THash<datamap_t *, DataMapTrie> m_Maps;


void InitZMUtils()
{
	m_pClasses = sm_trie_create();
}

void UnloadZMUtils()
{
	sm_trie_destroy(m_pClasses);

	SourceHook::List<DataTableInfo *>::iterator iter;
	DataTableInfo *pInfo;
	for (iter=m_Tables.begin(); iter!=m_Tables.end(); iter++)
	{
		pInfo = (*iter);
		delete pInfo;
	}

	m_Tables.clear();
}

bool SetClrRender(CBaseEntity *pBase, color32 clr)
{
	int Offset = g_ZombiePlugin.g_Offsets.m_clrRender;

	//if (Offset==0)
	//{
		//Offset = UTIL_FindOffsetMap( pBase, "CBaseEntity", "m_clrRender" );
	//}

	if (Offset!=0)
	{
		*(color32 *)((char *)pBase + Offset ) = clr;
		edict_t *pEntity = m_GameEnts->BaseEntityToEdict( pBase );
		if ( pEntity )
		{
			SetEdictStateChanged(pEntity, Offset);
		}
	}
	else
	{
		return false;
	}

	return true;
	
}

bool SetRenderMode(CBaseEntity *pBase, RenderMode_t mode)
{
	int Offset = g_ZombiePlugin.g_Offsets.m_nRenderMode;

	//if (Offset==0)
	//{
	//	Offset = UTIL_FindOffsetMap( pBase, "CBaseEntity", "m_nRenderMode" );
	//}

	//Offset = g_ZombiePlugin.g_Offsets.m_nRenderMode;
	edict_t *pEntity = m_GameEnts->BaseEntityToEdict( pBase );
	if (Offset!=0 && pEntity)
	{
		*(unsigned char *)((char *)pBase + Offset ) = (unsigned char)mode;
		//if ( UTIL_SetProperty( Offset, pEntity, mode ) )
		//{
			SetEdictStateChanged(pEntity, Offset);
		/*}
		else
		{
			return false;
		}*/
	}
	else
	{
		return false;
	}
	//UTIL_SetProperty(entity, Prop_Send, prop, mode, 1);

	return true;
}

RenderMode_t GetRenderMode(CBaseEntity *pBase)
{
	int Offset = g_ZombiePlugin.g_Offsets.m_nRenderMode;
	RenderMode_t ret1;
	/*if (Offset==0)
	{
		Offset = UTIL_FindOffsetMap( pBase, "CBaseEntity", "m_nRenderMode" );
	}*/
	edict_t *pEntity = m_GameEnts->BaseEntityToEdict( pBase );
	if (Offset!=0 && pEntity)
	{
		unsigned char ret;
		if ( UTIL_GetProperty( Offset, pEntity, &ret ) )
		{
			ret1 = (RenderMode_t)ret;
			return ret1;
		}

		//unsigned char ret = *(unsigned char *)((char *)pBase + Offset );
		//return (RenderMode_t)ret;
	}

	return kRenderNormal;
}

//*****************************************************************
// edict utilities

bool IsValidPlayer( int iPlayer, edict_t **pEntity, CBaseEntity **pBaseEntity )
{
	edict_t *pTmp = m_Engine->PEntityOfEntIndex( iPlayer );
	int *iTmp = NULL;
	if ( IsValidPlayer( pTmp, iTmp, pBaseEntity ) )
	{
		if ( pEntity )
		{
			*pEntity = pTmp;
		}
		return true;
	}
	if ( pBaseEntity )
	{
		*pBaseEntity = NULL;
	}
	if ( pEntity )
	{
		*pEntity = NULL;
	}
	return false;
}

bool IsValidPlayer( int iPlayer, CBaseEntity **pBaseEntity, edict_t **pEntity )
{
	edict_t *pTmpEnt = m_Engine->PEntityOfEntIndex( iPlayer );;
	if ( IsValidPlayer( pTmpEnt, NULL, pBaseEntity ) )
	{
		if ( pEntity )
		{
			*pEntity = pTmpEnt;
		}
		return true;
	}
	if ( pBaseEntity )
	{
		*pBaseEntity = NULL;
	}
	if ( pEntity )
	{
		*pEntity = NULL;
	}
	return false;
}

bool IsValidPlayer( CBaseEntity *pBaseEntity, int *iIndex, edict_t **pEntity )
{
	if ( pBaseEntity )
	{
		edict_t *pTmp = m_GameEnts->BaseEntityToEdict( pBaseEntity );
		if (IsValidPlayer( pTmp, iIndex ) )
		{
			if ( pEntity )
			{
				*pEntity = pTmp;
			}
			return true;
		}
	}
	if ( iIndex )
	{
		*iIndex = -1;
	}
	if ( pEntity )
	{
		*pEntity = NULL;
	}
	return false;
}

bool IsValidPlayer( edict_t *pEntity, int *iIndex, CBaseEntity **pBaseEntity )
{
	if ( pEntity && !pEntity->IsFree() && m_Engine->GetPlayerUserId( pEntity ) != -1 )
	{
		int iPlayer = m_Engine->IndexOfEdict( pEntity );
		CBaseEntity *pTmpEnt = NULL;
		
		if ( iPlayer > 0 && iPlayer <= MAX_PLAYERS )
		{
			pTmpEnt = m_GameEnts->EdictToBaseEntity( pEntity );
			if ( pTmpEnt )
			{
				if ( iIndex )
				{
					*iIndex = iPlayer;
				}
				if ( pBaseEntity )
				{
					*pBaseEntity = pTmpEnt;
				}
				return true;
			}
		}
	}
	if ( iIndex )
	{
		*iIndex = -1;
	}
	if ( pBaseEntity )
	{
		*pBaseEntity = NULL;
	}
	return false;
}

bool IsValidEnt( edict_t *pEntity )
{
	return ( pEntity && !pEntity->IsFree() );
}

bool IsValidEnt( CBaseEntity *pEntity )
{
	if ( pEntity )
	{
		return IsValidEnt( m_GameEnts->BaseEntityToEdict( pEntity ) );
	}
	return false;
}


//*****************************************************************
// find offsets for Get/Set Property Templates
int UTIL_FindOffsetMap( CBaseEntity *pBaseEntity, const char *ClassName, const char *PropertyName )
{
	//edict_t *pEntity = g_DeadVox.m_Engine->PEntityOfEntIndex( 1 );
	if ( pBaseEntity )
	{
		datamap_t *dmap = CBaseEntity_GetDataDescMap( pBaseEntity );
		if (dmap)
		{
			while (dmap)
			{
				if ( Q_stricmp( ClassName, dmap->dataClassName ) == 0 )
				{
					for ( int i = 0; i < dmap->dataNumFields; i++ )
					{
						//META_LOG( g_PLAPI, "%s", dmap->dataDesc[i].fieldName );
						if ( Q_stricmp( dmap->dataDesc[i].fieldName, PropertyName ) == 0 )
						{
							return dmap->dataDesc[i].fieldOffset[TD_OFFSET_NORMAL];
						}
					}
					return 0;
				}
				dmap = dmap->baseMap;
			}
		}
	}
	return 0;
}

int UTIL_FindOffset(const char *ClassName, const char *PropertyName)
{
	ServerClass *sc = m_ServerDll->GetAllServerClasses();
	while (sc)
	{
		if (FStrEq(sc->GetName(), ClassName))
		{
			int NumProps = sc->m_pTable->GetNumProps();
			for (int i=0; i<NumProps; i++)
			{
				if (stricmp(sc->m_pTable->GetProp(i)->GetName(), PropertyName) == 0)
				{
					return sc->m_pTable->GetProp(i)->GetOffset();
				}
			}
			META_LOG( g_PLAPI, "ERROR: FindOffset(\"%s\", \"%s\");", ClassName, PropertyName );
			return 0;
		}
		sc = sc->m_pNext;
	}
	META_LOG( g_PLAPI, "ERROR: FindOffset(\"%s\", \"%s\");", ClassName, PropertyName );
	return 0;
}

int UTIL_FindOffsetTable(const char *ClassName, const char *DataTableName, const char *PropertyName, const char *sLocal )
{

	SendTable *m_DataTable = NULL;
	SendTable *m_Local = NULL;
	ServerClass *sc = m_ServerDll->GetAllServerClasses();
	while (sc)
	{
		if (FStrEq(sc->GetName(), ClassName))
		{
			int NumProps = sc->m_pTable->GetNumProps();
			for (int i=0; i<NumProps; i++)
			{
				if (stricmp(sc->m_pTable->GetProp(i)->GetName(), DataTableName) == 0)
				{
					m_DataTable = sc->m_pTable->GetProp(i)->GetDataTable();
					if ( Q_strlen( PropertyName ) == 0 )
					{
						return m_DataTable->GetProp(i)->GetOffset();
					}
					break;
				}
			}
		}
		sc = sc->m_pNext;
	}
	bool bLocal = (sLocal && Q_strlen(sLocal) > 0);
	if (m_DataTable)
	{
		int Props = m_DataTable->GetNumProps();
		int i;
		for (i=0; i<Props; i++)
		{
			if ( bLocal )
			{
				if ( FStrEq(m_DataTable->GetProp(i)->GetName(), PropertyName ) )
				{
					m_Local = m_DataTable->GetProp(i)->GetDataTable();
					if ( m_Local )
					{
						int z;
						int mProps = m_Local->GetNumProps();
						for (z=0; z<mProps; z++)
						{
							if (FStrEq(m_Local->GetProp(z)->GetName(), sLocal))
							{
								return m_Local->GetProp(z)->GetOffset();
							}
						}
					}
					return 0;
				}
			}
			else
			{
				if (FStrEq(m_DataTable->GetProp(i)->GetName(), PropertyName))
				{
					return m_DataTable->GetProp(i)->GetOffset();
				}
			}
		}
		return 0;
	}
	else
	{
		return 0;
	}
}

ArrayLengthSendProxyFn UTIL_FindOffsetArray(const char *ClassName, const char *ArrayProp, void *vTeam )
{

	ServerClass *sc = m_ServerDll->GetAllServerClasses();
	while (sc)
	{
		if (FStrEq(sc->GetName(), ClassName))
		{
			int NumProps = sc->m_pTable->GetNumProps();
			for (int i=0; i<NumProps; i++)
			{
				if (stricmp(sc->m_pTable->GetProp(i)->GetName(), ArrayProp) == 0)
				{
					return sc->m_pTable->GetProp(i)->GetArrayLengthProxy();
				}
			}
		}
		sc = sc->m_pNext;
	}
	return NULL;
}


bool FindSendPropInfo(const char *classname, const char *offset, sm_sendprop_info_t *info)
{
	DataTableInfo *pInfo;
	sm_sendprop_info_t *prop;

	if ((pInfo = _FindServerClass(classname)) == NULL)
	{
		return false;
	}

	if ((prop = pInfo->lookup.retrieve(offset)) == NULL)
	{
		sm_sendprop_info_t temp_info;

		if (!UTIL_FindInSendTable(pInfo->sc->m_pTable, offset, &temp_info, 0))
		{
			return false;
		}

		pInfo->lookup.insert(offset, temp_info);
		*info = temp_info;
	}
	else
	{
		*info = *prop;
	}
	
	return true;
}

DataTableInfo *_FindServerClass(const char *classname)
{
	DataTableInfo *pInfo = NULL;

	if (!sm_trie_retrieve(m_pClasses, classname, (void **)&pInfo))
	{
		ServerClass *sc = m_ServerDll->GetAllServerClasses();
		while (sc)
		{
			if (strcmp(classname, sc->GetName()) == 0)
			{
				pInfo = new DataTableInfo;
				pInfo->sc = sc;
				sm_trie_insert(m_pClasses, classname, pInfo);
				m_Tables.push_back(pInfo);
				break;
			}
			sc = sc->m_pNext;
		}
		if (!pInfo)
		{
			return NULL;
		}
	}

	return pInfo;
}


bool UTIL_FindInSendTable(SendTable *pTable, 
						  const char *name,
						  sm_sendprop_info_t *info,
						  unsigned int offset)
{
	const char *pname;
	int props = pTable->GetNumProps();
	SendProp *prop;

	for (int i=0; i<props; i++)
	{
		prop = pTable->GetProp(i);
		pname = prop->GetName();
		if (pname && strcmp(name, pname) == 0)
		{
			info->prop = prop;
			info->actual_offset = offset + info->prop->GetOffset();
			return true;
		}
		if (prop->GetDataTable())
		{
			if (UTIL_FindInSendTable(prop->GetDataTable(), 
				name,
				info,
				offset + prop->GetOffset())
				)
			{
				return true;
			}
		}
	}

	return false;
}




bool IndexToAThings(cell_t num, CBaseEntity **pEntData, edict_t **pEdictData)
{
	CBaseEntity *pEntity = ReferenceToEntity(num);

	if (!pEntity)
	{
		return false;
	}

	int index = ReferenceToIndex(num);
	if (index > 0 && index <= MAX_PLAYERS)
	{
		edict_t * pEnt = NULL;
		if ( !IsValidPlayer( index, &pEnt ) )
		{
			return false;
		}
	}

	if (pEntData)
	{
		*pEntData = pEntity;
	}

	if (pEdictData)
	{
		edict_t *pEdict = m_GameEnts->BaseEntityToEdict(pEntity);
		if (!pEdict || pEdict->IsFree())
		{
			pEdict = NULL;
		}

		*pEdictData = pEdict;
	}

	return true;
}


CBaseEntity *ReferenceToEntity(cell_t entRef)
{
	CEntInfo *pInfo = NULL;

	if (entRef & (1<<31))
	{
		/* Proper ent reference */
		int hndlValue = entRef & ~(1<<31);
		CBaseHandle hndl(hndlValue);

		pInfo = LookupEntity(hndl.GetEntryIndex());
		if (pInfo->m_SerialNumber != hndl.GetSerialNumber())
		{
			return NULL;
		}
	}
	else
	{
		/* Old style index only */
		pInfo = LookupEntity(entRef);
	}

	if (!pInfo)
	{
		return NULL;
	}

	IServerUnknown *pUnk = static_cast<IServerUnknown *>(pInfo->m_pEntity);
	if (pUnk)
	{
		return pUnk->GetBaseEntity();
	}

	return NULL;
}


int ReferenceToIndex(cell_t entRef)
{
	if ((unsigned)entRef == INVALID_EHANDLE_INDEX)
	{
		return INVALID_EHANDLE_INDEX;
	}

	if (entRef & (1<<31))
	{
		/* Proper ent reference */
		int hndlValue = entRef & ~(1<<31);
		CBaseHandle hndl(hndlValue);

		CEntInfo *pInfo = LookupEntity(hndl.GetEntryIndex());

		if (pInfo->m_SerialNumber != hndl.GetSerialNumber())
		{
			return INVALID_EHANDLE_INDEX;
		}

		return hndl.GetEntryIndex();
	}

	return entRef;
}


CEntInfo *LookupEntity(int entIndex)
{
	if ( !g_EntList || entInfoOffset == -1 )
	{
		/* Attempt to use engine interface instead */
		static CEntInfo tempInfo;
		tempInfo.m_pNext = NULL;
		tempInfo.m_pPrev = NULL;

		edict_t *pEdict = m_Engine->PEntityOfEntIndex(entIndex);

		if (!pEdict)
		{
			return NULL;
		}

		IServerUnknown *pUnk = pEdict->GetUnknown();

		if (!pUnk)
		{
			return NULL;
		}

		tempInfo.m_pEntity = pUnk;
		tempInfo.m_SerialNumber = pUnk->GetRefEHandle().GetSerialNumber();

		return &tempInfo;
	}

	CEntInfo *pArray = (CEntInfo *)(((unsigned char *)g_EntList) + entInfoOffset);
	return &pArray[entIndex];
}


typedescription_t *FindInDataMap(datamap_t *pMap, const char *offset)
{
	typedescription_t *td = NULL;
	DataMapTrie &val = m_Maps[pMap];

	if (!val.trie)
	{
		val.trie = sm_trie_create();
	}
	if (!sm_trie_retrieve(val.trie, offset, (void **)&td))
	{
		if ((td = UTIL_FindInDataMap(pMap, offset)) != NULL)
		{
			sm_trie_insert(val.trie, offset, td);
		}
	}

	return td;
}


void SetEdictStateChanged(edict_t *pEdict, unsigned short offset)
{
	if ( g_pSharedChangeInfo != NULL )
	{
		if ( offset )
		{
			pEdict->StateChanged(offset);
		}
	}
	else
	{
		pEdict->m_fStateFlags |= FL_EDICT_CHANGED;
	}
}


typedescription_t *UTIL_FindInDataMap(datamap_t *pMap, const char *name)
{
	while (pMap)
	{
		for (int i=0; i<pMap->dataNumFields; i++)
		{
			if (pMap->dataDesc[i].fieldName == NULL)
			{
				continue;
			}
			if (strcmp(name, pMap->dataDesc[i].fieldName) == 0)
			{
				return &(pMap->dataDesc[i]);
			}
			if (pMap->dataDesc[i].td)
			{
				typedescription_t *_td;
				if ((_td=UTIL_FindInDataMap(pMap->dataDesc[i].td, name)) != NULL)
				{
					return _td;
				}
			}
		}
		pMap = pMap->baseMap;
	}

	return NULL; 
}

IChangeInfoAccessor *CBaseEdict::GetChangeAccessor()
{
	return m_Engine->GetChangeAccessor( (const edict_t *)this );
}

namespace SourceHook
{
	template<>
	int HashFunction<datamap_t *>(datamap_t * const &k)
	{
		return reinterpret_cast<int>(k);
	}

	template<>
	int Compare<datamap_t *>(datamap_t * const &k1, datamap_t * const &k2)
	{
		return (k1-k2);
	}
}

//bool GetEntPropVector(IPluginContext *pContext, const cell_t *params)
bool GetEntPropVector(int iEntIndex, PropType eSendType, const char *sProp, Vector &vVector)
{
	CBaseEntity *pEntity;
	int offset;
	const char *class_name;
	edict_t *pEdict;
	char error[200];
	int maxlength = sizeof( error );
	
	if (!IndexToAThings(iEntIndex, &pEntity, &pEdict))
	{
		//META_LOG( g_PLAPI, "Entity %d (%d) is invalid", ReferenceToIndex(iEntIndex), iEntIndex );
		return false;
	}

	if (!pEdict || (class_name = pEdict->GetClassName()) == NULL)
	{
		class_name = "";
	}

	switch (eSendType)
	{
	case Prop_Data:
		{
			typedescription_t *td;
			
			FIND_PROP_DATA(td);

			if (td->fieldType != FIELD_VECTOR
				&& td->fieldType != FIELD_POSITION_VECTOR)
			{
				META_LOG( g_PLAPI, "Data field %s is not a vector (%d != [%d,%d])", sProp, td->fieldType, FIELD_VECTOR, FIELD_POSITION_VECTOR );
				return false;
			}

			offset = td->fieldOffset[TD_OFFSET_NORMAL];
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_Vector)
			{
				META_LOG( g_PLAPI, "SendProp %s is not a vector (%d != %d)",
					sProp,
					info.prop->GetType(),
					DPT_Vector);
				return false;
			}

			offset = info.actual_offset;
			break;
		}
	default:
		{
			META_LOG( g_PLAPI, "Invalid Property type %d", eSendType );
			return false;
		}
	}

	Vector *v = (Vector *)((uint8_t *)pEntity + offset);

	vVector.x = v->x;
	vVector.y = v->y;
	vVector.z = v->z;

	return 1;
}


bool SetEntPropVector(int iEntIndex, PropType eSendType, const char *sProp, Vector vVector)
{
	CBaseEntity *pEntity;
	int offset;
	const char *class_name;
	edict_t *pEdict;
	char error[200];
	int maxlength = sizeof( error );
	
	if (!IndexToAThings(iEntIndex, &pEntity, &pEdict))
	{
		//META_LOG( g_PLAPI, "Entity %d (%d) is invalid", ReferenceToIndex(iEntIndex), iEntIndex );
		return false;
	}

	if ( !pEdict || ( class_name = pEdict->GetClassName( )) == NULL)
	{
		class_name = "";
	}

	switch (eSendType)
	{
	case Prop_Data:
		{
			typedescription_t *td;
			
			FIND_PROP_DATA(td);

			if (td->fieldType != FIELD_VECTOR
				&& td->fieldType != FIELD_POSITION_VECTOR)
			{
				META_LOG( g_PLAPI, "Data field %s is not a vector (%d != [%d,%d])", sProp, td->fieldType, FIELD_VECTOR, FIELD_POSITION_VECTOR );
				return false;
			}

			offset = td->fieldOffset[TD_OFFSET_NORMAL];
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_Vector)
			{
				META_LOG( g_PLAPI, "SendProp %s is not a vector (%d != %d)",
					sProp,
					info.prop->GetType(),
					DPT_Vector);
				return false;
			}

			offset = info.actual_offset;
			break;
		}
	default:
		{
			META_LOG( g_PLAPI, "Invalid Property type %d", eSendType );
			return false;
		}
	}

	Vector *v = (Vector *)((uint8_t *)pEntity + offset);
	v->x = vVector.x;
	v->y = vVector.y;
	v->z = vVector.z;

	if (eSendType == Prop_Send && (pEdict != NULL))
	{
		SetEdictStateChanged(pEdict, offset);
	}

	return true;
}


int MatchFieldAsInteger(int field_type)
{
	switch (field_type)
	{
	case FIELD_TICK:
	case FIELD_MODELINDEX:
	case FIELD_MATERIALINDEX:
	case FIELD_INTEGER:
	case FIELD_COLOR32:
		return 32;
	case FIELD_SHORT:
		return 16;
	case FIELD_CHARACTER:
		return 8;
	case FIELD_BOOLEAN:
		return 1;
	default:
		return 0;
	}

	return 0;
}


bool SetEntPropClr( int iEntIndex, PropType eSendType, const char *sProp, color32 NewValue )
{
	CBaseEntity *pEntity;
	int offset;
	const char *class_name;
	edict_t *pEdict;
	int bit_count;
	char error[200];
	int maxlength = sizeof( error );


	if (!IndexToAThings(iEntIndex, &pEntity, &pEdict))
	{
		//META_LOG( g_PLAPI, "Entity %d (%d) is invalid", ReferenceToIndex(iEntIndex), iEntIndex );
		return false;
	}

	if ( !pEdict || ( class_name = pEdict->GetClassName( )) == NULL)
	{
		class_name = "";
	}

	switch (eSendType)
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			/*if ((bit_count = MatchFieldAsInteger(NewValue)) == 0)
			{
				META_LOG( g_PLAPI, "Data field %s is not an integer (%d)", 
					sProp,
					td->fieldType);
			}*/
			offset = td->fieldOffset[TD_OFFSET_NORMAL];
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_Int)
			{
				META_LOG( g_PLAPI, "SendProp %s is not an integer ([%d,%d] != %d)",
					sProp,
					info.prop->GetType(),
					info.prop->m_nBits,
					DPT_Int);
			}

			bit_count = info.prop->m_nBits;
			offset = info.actual_offset;
			break;
		}
	default:
		{
			META_LOG( g_PLAPI, "Invalid Property type %d", eSendType );
		}
	}

	*(color32 *)((uint8_t *)pEntity + offset) = NewValue;

	return 0;
}