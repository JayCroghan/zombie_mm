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
SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char*, const char *, char *, int);
SH_DECL_HOOK2_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, edict_t *, bool);
SH_DECL_HOOK2( IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool );
SH_DECL_HOOK2( IVEngineServer, UserMessageBegin, SH_NOATTRIB, 0, bf_write *, IRecipientFilter *, int );
SH_DECL_HOOK0( IServerGameDLL, GetGameDescription, SH_NOATTRIB, 0, const char * );

#ifdef ENDSOUND
SH_DECL_HOOK2( ISoundEmitterSystemBase, GetWavFileForSound, SH_NOATTRIB, 0, const char *, const char *, gender_t );
SH_DECL_HOOK2( ISoundEmitterSystemBase, GetWavFileForSound, SH_NOATTRIB, 1, const char *, char const *, char const * );
SH_DECL_HOOK3( ISoundEmitterSystemBase, AddSound, SH_NOATTRIB, 0, bool, const char *, const char *, const CSoundParametersInternal& );
SH_DECL_HOOK4( ISoundEmitterSystemBase, GetParametersForSound, SH_NOATTRIB, 0, bool, const char *, CSoundParameters&, gender_t, bool );
SH_DECL_HOOK1( ISoundEmitterSystemBase, GetWaveName, SH_NOATTRIB, 0, const char *, CUtlSymbol& );
SH_DECL_HOOK1( ISoundEmitterSystemBase, AddWaveName, SH_NOATTRIB, 0, CUtlSymbol, const char * );
SH_DECL_HOOK5( ISoundEmitterSystemBase, GetParametersForSoundEx, SH_NOATTRIB, 0, bool, const char *, HSOUNDSCRIPTHANDLE&, CSoundParameters&, gender_t, bool );
#endif

#if defined ORANGEBOX_BUILD
	SH_DECL_HOOK1_void(ConCommand, Dispatch, SH_NOATTRIB, 0, const CCommand &);
	SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *, const CCommand &);
#else
	SH_DECL_HOOK0_void(ConCommand, Dispatch, SH_NOATTRIB, 0);
	SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
#endif


SH_DECL_HOOK3( IVoiceServer, SetClientListening, SH_NOATTRIB, 0, bool, int, int, bool );


SH_DECL_MANUALHOOK1			( OnTakeDamage_hook,			0, 0, 0, int, CTakeDamageInfoHack & );
SH_DECL_MANUALHOOK1			( Weapon_CanSwitchTo_hook,		0, 0, 0, bool, CBaseCombatWeapon * );
SH_DECL_MANUALHOOK1			( Weapon_CanUse_hook,			0, 0, 0, bool, CBaseCombatWeapon * );
SH_DECL_MANUALHOOK3_void	( TraceAttack_hook,				0, 0, 0, const CTakeDamageInfo &, const Vector &, trace_t * );
SH_DECL_MANUALHOOK1_void	( ChangeTeam_hook,				0, 0, 0, int );
SH_DECL_MANUALHOOK2_void	( CommitSuicide_hook,			0, 0, 0, bool, bool );
SH_DECL_MANUALHOOK1_void	( Event_Killed_hook,			0, 0, 0, const CTakeDamageInfo & );
SH_DECL_MANUALHOOK3_void	( Teleport_Hook,				0, 0, 0, const Vector *, const QAngle *, const Vector * ); 
SH_DECL_MANUALHOOK0_void	( PreThink_hook,				0, 0, 0);
SH_DECL_MANUALHOOK2			( CGameRules_IPointsForKill,	0, 0, 0, int, CBasePlayer *, CBasePlayer *);
SH_DECL_MANUALHOOK0			( GetPlayerMaxSpeed,			0, 0, 0, float);

#define Teleport( ThisPtr, newPosition, newAngles, newVelocity ) \
	SH_MCALL( ThisPtr, Teleport_Hook)( newPosition, newAngles, newVelocity ); \

#define Suicide( ThisPtr, Dir, Explode, Force ) \
	SH_MCALL( ThisPtr, CommitSuicide_hook ) ( Explode, Force );

#endif //_INCLUDE_META_HOOKS_H
