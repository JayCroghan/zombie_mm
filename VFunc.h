#ifndef _INCLUDE_VFUNC_H
#define _INCLUDE_VFUNC_H
// ** vfuncs **

#ifdef WIN32
    #define VTABLE_OFFSET 0
#else
    #define VTABLE_OFFSET 1
#endif

//#include "CTakeDamageInfo.h"

ICollideable	*CBaseEntity_GetCollideable( CBaseEntity *pThisPtr );
int				CBaseEntity_GetModelIndex( CBaseEntity *pThisPtr );
void			CBaseEntity_SetModelIndex( CBaseEntity *pThisPtr, int Index );
datamap_t *		CBaseEntity_GetDataDescMap( CBaseEntity *pThisPtr );
//void			CBaseEntity_SetOwnerEntity( CBaseEntity *pThisPtr, CBaseEntity* pOwner );
void			CBasePlayer_SetModel( CBasePlayer *pThisPtr, const char *sModel );
bool			CBaseEntity_KeyValue( CBaseEntity *pThisPtr, const char *szKeyName, const char *szValue );
bool			CBaseEntity_KeyValueFloat( CBaseEntity *pThisPtr, const char *szKeyName, float *flValue );
bool			CBaseEntity_KeyValueVector( CBaseEntity *pThisPtr, const char *szKeyName, const Vector &vecValue );
bool			CBaseEntity_GetKeyValue( CBaseEntity *pThisPtr, const char *szKeyName, char *szValue, int iMaxLen );

void			CBaseEntity_SetParent( CBaseEntity *pThisPtr, CBaseEntity* pNewParent, int iAttachment = -1 );
bool			CBaseEntity_AcceptInput( CBaseEntity *pThisPtr, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID );
void			CCSPlayer_Event_Killed( CCSPlayer *pThisPtr, const CTakeDamageInfo &info );
void			CCSPlayer_ChangeTeam (CCSPlayer *pThisPtr, int TeamNum );
void			CBaseAnimating_GetVelocity( CBaseAnimating *pThisPtr, Vector *vVelocity, AngularImpulse *vAngVelocity = NULL );
const Vector&	CBaseEntity_WorldSpaceCenter( CBaseEntity *pThisPtr );
void			CBaseAnimating_Ignite( CBaseAnimating *pThisPtr, float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner );
int				CBaseCombatCharacter_GiveAmmo( CBaseCombatCharacter *pThisPtr, int iCount, int iAmmoIndex, bool bSuppressSound = false );
bool			CCSPlayer_Weapon_Switch( CCSPlayer *pCombat, CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
void			CBasePlayer_Event_Dying( CBasePlayer *pThisPtr );
CBaseEntity *	CCSPlayer_GiveNamedItem( CCSPlayer *pThisPtr, const char *ItemName, int iSubType = 0 );
CBaseCombatWeapon* CBaseCombatCharacter_Weapon_GetSlot( CBaseCombatCharacter *pCombat, int slot );
bool			CBasePlayer_IsBot( CBasePlayer *pThisPtr );
const CViewVectors* CGameRules_GetViewVectors( CGameRules *pThisPtr );

#endif
// end ** vfuncs **