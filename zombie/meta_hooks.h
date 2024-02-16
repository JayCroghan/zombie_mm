/* ======== sample_mm ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): David "BAILOPAN" Anderson
* ============================
*/

#ifndef _INCLUDE_META_HOOKS_H
#define _INCLUDE_META_HOOKS_H

#ifdef WIN32
    #define VTABLE_OFFSET 0
#else
    #define VTABLE_OFFSET 1
#endif

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);
SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);
SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, 0);
//SH_DECL_HOOK2_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, edict_t *, bool);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char*, const char *, char *, int);
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);

SH_DECL_HOOK14_void( IEngineSound, EmitSound, SH_NOATTRIB, 0, IRecipientFilter&, int, int, const char *, float , float, int, int, const Vector *, const Vector *, CUtlVector< Vector >*, bool, float, int );
SH_DECL_HOOK0_void(ConCommand, Dispatch, SH_NOATTRIB, 0);

SH_DECL_HOOK3( IVoiceServer, SetClientListening, SH_NOATTRIB, 0, bool, int, int, bool );
//SH_DECL_HOOK1_void( CBaseEntity, Touch, SH_NOATTRIB, 0, CBaseEntity * );
//SH_DECL_HOOK1( CBasePlayer, OnTakeDamage, SH_NOATTRIB, 0, int, const CTakeDamageInfo & );

//H_DECL_MANUALHOOK2(

//bool CBasePlayer::Weapon_CanUse( CBaseCombatWeapon *pWeapon )
//bool CBaseCombatCharacter::Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon )

//SH_DECL_MANUALHOOK2_void( PickupObject_hook, 314 + VTABLE_OFFSET, 0, 0, CBaseEntity *, bool );
SH_DECL_MANUALHOOK1_void( Touch_hook, 83 + VTABLE_OFFSET, 0, 0, CBaseEntity * );
SH_DECL_MANUALHOOK0_void( DoMuzzleFlash_hook, 236 + VTABLE_OFFSET, 0, 0 );
SH_DECL_MANUALHOOK1( OnTakeDamage_hook, 54 + VTABLE_OFFSET, 0, 0, int, const CTakeDamageInfo & );
SH_DECL_MANUALHOOK1_void( FireBullets_hook, 95 + VTABLE_OFFSET, 0, 0, const FireBulletsInfo_t & );
SH_DECL_MANUALHOOK1(Weapon_CanSwitchTo_hook, 206 + VTABLE_OFFSET, 0, 0, bool, CBaseCombatWeapon *);
SH_DECL_MANUALHOOK1(Weapon_CanUse_hook, 200 + VTABLE_OFFSET, 0, 0, bool, CBaseCombatWeapon *);
SH_DECL_MANUALHOOK0(GetMaxSpeed_hook, 301 + VTABLE_OFFSET, 0, 0, float);
SH_DECL_MANUALHOOK0_void(Delete_hook, 285 + VTABLE_OFFSET, 0, 0);
SH_DECL_MANUALHOOK3_void(UpdateStepSound_hook, 272 + VTABLE_OFFSET, 0, 0, surfacedata_t *, const Vector &, const Vector &);
SH_DECL_MANUALHOOK4_void(PlayStepSound_hook, 273 + VTABLE_OFFSET, 0, 0, Vector &, surfacedata_t *, float, bool );
SH_DECL_MANUALHOOK3_void(TraceAttack_hook, 52 + VTABLE_OFFSET, 0, 0, const CTakeDamageInfo &, const Vector &, trace_t *);

#endif //_INCLUDE_META_HOOKS_H
