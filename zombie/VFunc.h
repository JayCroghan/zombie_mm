#ifndef _INCLUDE_VFUNC_H
#define _INCLUDE_VFUNC_H
// ** vfuncs **

#ifdef WIN32
    #define VTABLE_OFFSET 0
#else
    #define VTABLE_OFFSET 1
#endif
#define VFUNC_OFFSET_GETDATADESCMAP 12
#define VFUNC_OFFSET_KEYVALUE 27
#define VFUNC_OFFSET_CHANGETEAM 75
#define VFUNC_OFFSET_IGNITE 172
#define VFUNC_OFFSET_GIVENAMEDITEM 306
#define VFUNC_OFFSET_SETPLAYERUNDERWATER 308
#define VFUNC_OFFSET_EVENT_KILLED 56
#define VFUNC_OFFSET_EVENT_DYING 225

datamap_t *		CBaseEntity_GetDataDescMap( CBaseEntity *pThisPtr );
bool			CBaseEntity_KeyValue( CBaseEntity *pThisPtr, const char *szKeyName, const char *szValue );
void			CBaseEntity_ChangeTeam (CBaseEntity *pThisPtr, int TeamNum );
void			CBaseEntity_Ignite( CBaseEntity *pThisPtr, float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner );
CBaseEntity *	CCSPlayer_GiveNamedItem( CBaseEntity *pThisPtr, const char *ItemName, int iSubType = 0 );
void			CBaseEntity_Event_Killed( CBaseEntity *pThisPtr, const CTakeDamageInfo &info );
void			CBasePlayer_Event_Dying( CBasePlayer *pThisPtr );

#endif
// end ** vfuncs **