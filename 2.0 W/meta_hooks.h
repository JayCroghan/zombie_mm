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
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);
//SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, edict_t *);
//SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char*, const char *, char *, int);
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK2_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, edict_t *, bool);

SH_DECL_HOOK2( IVEngineServer, UserMessageBegin, SH_NOATTRIB, 0, bf_write *, IRecipientFilter *, int );
//SH_DECL_HOOK14_void( IEngineSound, EmitSound, SH_NOATTRIB, 0, IRecipientFilter&, int, int, const char *, float , float, int, int, const Vector *, const Vector *, CUtlVector< Vector >*, bool, float, int );
SH_DECL_HOOK0_void(ConCommand, Dispatch, SH_NOATTRIB, 0);
SH_DECL_HOOK3( IVoiceServer, SetClientListening, SH_NOATTRIB, 0, bool, int, int, bool );
SH_DECL_HOOK2( IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool );

SH_DECL_HOOK5_void( IServerGameDLL, OnQueryCvarValueFinished, SH_NOATTRIB, 0, QueryCvarCookie_t, edict_t *, EQueryCvarValueStatus, const char *, const char * );


#define CBASEENTITY_TOUCH				( 89  + VTABLE_OFFSET )
#define CBASEENTITY_MUZZLEFLASH			( 256 + VTABLE_OFFSET )
#define CBASEENTITY_ONTAKEDAMAGE		( 60  + VTABLE_OFFSET )
#define CBASEENTITY_FIREBULLETS			( 101  + VTABLE_OFFSET )
#define CBASEENTITY_WEAPON_CANSWITCHTO	( 222 + VTABLE_OFFSET )
#define CBASEENTITY_WEAPON_CANUSE		( 216 + VTABLE_OFFSET )

#define CBASECOMBATWEAPON_GETMAXSPEED	( 322 + VTABLE_OFFSET )
#define CBASECOMBATWEAPON_DELETE		( 306 + VTABLE_OFFSET )

#define CBASEENTITY_TRACEATTACK			( 58  + VTABLE_OFFSET )
#define CBASEENTITY_CANHEARCHATFROM		( 349 + VTABLE_OFFSET )
#define CCSPLAYER_CHANGETEAM			( 81  + VTABLE_OFFSET )
#define CBASEPLAYER_COMMITSUICIDE		( 357 + VTABLE_OFFSET )
#define CBASEPLAYER_EVENT_KILLED		( 62  + VTABLE_OFFSET )
#define CBASEENTITY_GETMODELINDEX		( 6   + VTABLE_OFFSET )
#define CBASEENTITY_SETMODELINDEX		( 8   + VTABLE_OFFSET )
#define	CBASEANIMATING_TELEPORT			( 98  + VTABLE_OFFSET )

SH_DECL_MANUALHOOK1_void	( Touch_hook,				CBASEENTITY_TOUCH,				0, 0, CBaseEntity * );
//SH_DECL_MANUALHOOK0_void	( DoMuzzleFlash_hook,		CBASEENTITY_MUZZLEFLASH,		0, 0 );
SH_DECL_MANUALHOOK1			( OnTakeDamage_hook,		CBASEENTITY_ONTAKEDAMAGE,		0, 0, int, const CTakeDamageInfo & );
//SH_DECL_MANUALHOOK1_void	( FireBullets_hook,			CBASEENTITY_FIREBULLETS,		0, 0, const FireBulletsInfo_t & );
SH_DECL_MANUALHOOK1			( Weapon_CanSwitchTo_hook,	CBASEENTITY_WEAPON_CANSWITCHTO, 0, 0, bool, CBaseCombatWeapon * );
SH_DECL_MANUALHOOK1			( Weapon_CanUse_hook,		CBASEENTITY_WEAPON_CANUSE,		0, 0, bool, CBaseCombatWeapon * );
SH_DECL_MANUALHOOK0			( GetMaxSpeed_hook,			CBASECOMBATWEAPON_GETMAXSPEED,	0, 0, float );
SH_DECL_MANUALHOOK0_void	( Delete_hook,				CBASECOMBATWEAPON_DELETE,		0, 0);
//SH_DECL_MANUALHOOK3_void	( UpdateStepSound_hook,		272 + VTABLE_OFFSET,			0, 0, surfacedata_t *, const Vector &, const Vector & );
//SH_DECL_MANUALHOOK4_void	( PlayStepSound_hook,		273 + VTABLE_OFFSET,			0, 0, Vector &, surfacedata_t *, float, bool );
SH_DECL_MANUALHOOK3_void	( TraceAttack_hook,			CBASEENTITY_TRACEATTACK,		0, 0, const CTakeDamageInfo &, const Vector &, trace_t * );
SH_DECL_MANUALHOOK1_void	( ChangeTeam_hook,			CCSPLAYER_CHANGETEAM,			0, 0, int );
SH_DECL_MANUALHOOK0_void	( CommitSuicide_hook,		CBASEPLAYER_COMMITSUICIDE,		0, 0 );
SH_DECL_MANUALHOOK1_void	( Event_Killed_hook,		CBASEPLAYER_EVENT_KILLED,		0, 0, const CTakeDamageInfo & );

SH_DECL_MANUALHOOK3_void	( Teleport_Hook,			CBASEANIMATING_TELEPORT,		0, 0, const Vector *, const QAngle *, const Vector * ); 

#define Teleport( ThisPtr, newPosition, newAngles, newVelocity ) \
	SH_MCALL( ThisPtr, Teleport_Hook)( newPosition, newAngles, newVelocity ); \

#endif //_INCLUDE_META_HOOKS_H
