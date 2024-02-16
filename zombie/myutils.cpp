
#include "myutils.h"
#include "VFunc.h"

//*****************************************************************
// edict utilities

edict_t *EdictOfUserId( int UserId )
{
	int i;
	edict_t *pEnt;
	for  (i=0; i<MAX_PLAYERS; i++)
	{
		pEnt = g_ZombiePlugin.m_Engine->PEntityOfEntIndex(i);
		if (pEnt && !pEnt->IsFree())
		{
			if (FStrEq(pEnt->GetClassName(), "player"))
			{
				if (g_ZombiePlugin.m_Engine->GetPlayerUserId(pEnt)==UserId)
				{
					return pEnt;
				}
			}
		}
	}

	return NULL;
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
						META_LOG( g_PLAPI, "%s", dmap->dataDesc[i].fieldName );
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
	ServerClass *sc = g_ZombiePlugin.m_ServerDll->GetAllServerClasses();
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
	ServerClass *sc = g_ZombiePlugin.m_ServerDll->GetAllServerClasses();
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

	ServerClass *sc = g_ZombiePlugin.m_ServerDll->GetAllServerClasses();
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