#ifndef _INCLUDE_VFUNC_H
#define _INCLUDE_VFUNC_H
// ** vfuncs **

#ifdef WIN32
    #define VTABLE_OFFSET 0
#else
    #define VTABLE_OFFSET 1
#endif

#define VFUNC_OFFSET_GETMODELINDEX			6
#define VFUNC_OFFSET_SETMODELINDEX			8
#define VFUNC_OFFSET_GETDATADESCMAP			13
#define VFUNC_OFFSET_SETOWNERENTITY 		33
#define VFUNC_OFFSET_SETMODEL				25
#define VFUNC_OFFSET_KEYVALUE				29
#define VFUNC_OFFSET_SETPARENT				33
#define VFUNC_OFFSET_ACCEPTINPUT			35
#define VFUNC_OFFSET_EVENT_KILLED			62
#define VFUNC_OFFSET_CHANGETEAM				81
#define	VFUNC_OFFSET_TELEPORT				98
#define VFUNC_OFFSET_GETVELOCITY			126
#define VFUNC_OFFSET_WORLDSPACECENTER		137
#define VFUNC_OFFSET_IGNITE					188
#define VFUNC_OFFSET_GIVEAMMO				213
#define VFUNC_OFFSET_WEAPON_SWITCH			220
#define VFUNC_OFFSET_EVENT_DYING			241
#define VFUNC_OFFSET_GIVENAMEDITEM			329
#define VFUNC_OFFSET_SETPLAYERUNDERWATER	308

#define VFUNC_OFFSET_GETSLOT				224

#define VFUNC_OFFSET_GETVIEWVECTORS			5

bool			CBaseCombatCharacter_Weapon_Switch( CBaseCombatCharacter *pCombat, CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
CBaseCombatWeapon* CBaseCombatCharacter_WeaponSlot( CBaseCombatCharacter *pCombat, int slot );
void			CBaseAnimating_Teleport( CBaseEntity *pThisPtr, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );
datamap_t *		CBaseEntity_GetDataDescMap( CBaseEntity *pThisPtr );
bool			CBaseEntity_KeyValue( CBaseEntity *pThisPtr, const char *szKeyName, const char *szValue );
void			CBaseEntity_ChangeTeam (CBaseEntity *pThisPtr, int TeamNum );
void			CBaseEntity_Ignite( CBaseEntity *pThisPtr, float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner );
CBaseEntity *	CCSPlayer_GiveNamedItem( CBaseEntity *pThisPtr, const char *ItemName, int iSubType = 0 );
void			CBaseEntity_Event_Killed( CBaseEntity *pThisPtr, const CTakeDamageInfo &info );
void			CBasePlayer_Event_Dying( CBasePlayer *pThisPtr );
int				CBaseEntity_GetModelIndex( CBasePlayer *pThisPtr );
void			CBaseEntity_SetModelIndex( CBasePlayer *pThisPtr, int Index );
void			CBaseEntity_SetModel( CBasePlayer *pThisPtr, const char *sModel );
const Vector&	CBaseEntity_WorldSpaceCenter( CBasePlayer *pThisPtr );
void			CBaseEntity_GetVelocity( CBasePlayer *pThisPtr, Vector *vVelocity, AngularImpulse *vAngVelocity = NULL );
void			CBaseEntity_SetParent( CBaseEntity *pThisPtr, CBaseEntity* pNewParent, int iAttachment = -1 );
void			CBaseEntity_SetOwnerEntity( CBaseEntity *pThisPtr, CBaseEntity* pOwner );
int				CBaseCombatCharacter_GiveAmmo( CBaseCombatCharacter *pThisPtr, int iCount, int iAmmoIndex, bool bSuppressSound = false );

bool			CBaseEntity_AcceptInput( CBaseEntity *pThisPtr, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID );

const CViewVectors* CGameRules_GetViewVectors( CGameRules *pThisPtr );
//CViewVectors*	CGameRules_GetViewVectors()


#endif
// end ** vfuncs **