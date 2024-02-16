#include "myutils.h"
#include "VFunc.h"

#ifndef TYRANT_MM_EXPORTS
	extern IVEngineServer		*m_Engine;
	extern IServerGameDLL		*m_ServerDll;
	extern IPlayerInfoManager	*m_PlayerInfoManager;
	extern PlayerInfo_t			g_Players[65];
#else
	#include "../hidden_mm/TyrantPlugin.h"
#endif

#undef GetProp

bool SetClrRender(CBaseEntity *pBase, color32 clr)
{
	static int Offset = 0;

	if (Offset==0)
	{
		Offset = UTIL_FindOffsetMap( pBase, "CBaseEntity", "m_clrRender" );
	}

	if (Offset!=0)
	{
		*(color32 *)((char *)pBase + Offset ) = clr;
	}
	else
	{
		return false;
	}

	return true;
	
}

bool SetRenderMode(CBaseEntity *pBase, RenderMode_t mode)
{
	static int Offset = 0;

	if (Offset==0)
	{
		Offset = UTIL_FindOffsetMap( pBase, "CBaseEntity", "m_nRenderMode" );
	}

	if (Offset!=0)
	{
		*(unsigned char *)((char *)pBase + Offset ) = (unsigned char)mode;
	}
	else
	{
		return false;
	}

	return true;
}

RenderMode_t GetRenderMode(CBaseEntity *pBase)
{
	static int Offset = 0;

	if (Offset==0)
	{
		Offset = UTIL_FindOffsetMap( pBase, "CBaseEntity", "m_nRenderMode" );
	}

	if (Offset!=0)
	{
		unsigned char ret = *(unsigned char *)((char *)pBase + Offset );
		return (RenderMode_t)ret;
	}

	return kRenderNormal;
}

//*****************************************************************
// edict utilities

bool IsValidPlayer( int iPlayer, CBaseEntity **pEntity )
{
	edict_t *pEnt;
	if ( IsValidPlayer( iPlayer, &pEnt ) )
	{
		CBaseEntity *pBase = pEnt->GetUnknown()->GetBaseEntity();
		if ( IsValidPlayer( pBase ) )
		{
			*pEntity = pBase;
			return true;
		}
	}
	*pEntity = NULL;
	return false;
}

bool IsValidPlayer( int iPlayer, edict_t **pEntity )
{
	if ( iPlayer > 0 && iPlayer <= MAX_PLAYERS )
	{
		edict_t *pEnt = m_Engine->PEntityOfEntIndex( iPlayer );
		if ( IsValidPlayer( pEnt ) )
		{
			if ( pEntity )
			{
				*pEntity = pEnt;
			}
			return true;
		}
	}
	*pEntity = NULL;
	return false;
}

bool IsValidPlayer( CBaseEntity *pEntity, int *iIndex )
{
	if ( pEntity )
	{
		if ( IsValidPlayer( m_GameEnts->BaseEntityToEdict( pEntity ), iIndex ) )
		{
			return true;
		}
	}
	return false;
}

bool IsValidPlayer( edict_t *pEntity, int *iIndex )
{
	if ( pEntity && !pEntity->IsFree() && m_Engine->GetPlayerUserId( pEntity ) != -1 )
	{
		int iPlayer = m_Engine->IndexOfEdict( pEntity );
		
		if ( iPlayer > 0 && iPlayer <= MAX_PLAYERS )
		{
			if ( iIndex )
			{
				*iIndex = iPlayer;
			}
			return true;
		}
	}
	return false;
}

bool IsValidEnt( edict_t *pEntity )
{
	if ( pEntity && !pEntity->IsFree() )
	{
		return true;
	}
	return false;
}

bool IsValidEnt( CBaseEntity *pEntity )
{
	if ( pEntity )
	{
		return IsValidEnt( pEntity );
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