/* ======== sample_mm ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): David "BAILOPAN" Anderson
* ============================
*/

#ifndef _INCLUDE_ZombiePlugin_H
#define _INCLUDE_ZombiePlugin_H

#define ANTISTICK

#define ADDSTRING_ISSERVER true

#include <ISmmPlugin.h>
#include <sourcehook/sourcehook.h>
#include <igameevents.h>
#include "bitbuf.h"
#define GAME_DLL 1
#define CSTRIKE_DLL 1
#define ENGINE_DLL 1
#include "zm_includesdk.h"
#include "team.h"
#include "MRecipients.h"
#include "sourcehook/sh_string.h"
#include <time.h>
#include "weapon_csbase.h"
#include "datamap.h"
#include "shake.h"
#include "IEffects.h"
#include "itempents.h"
#include "model_types.h"
#include "te_effect_dispatch.h"
#include "ivoiceserver.h"
#include "apparent_velocity_helper.h"
#include "soundent.h"
#include "vguiscreen.h"
#include "ZM_Util.h"
#include "sourcehook/sh_list.h"
#include "dt_send.h"
#include "VFunc.h"
#include "zm_linuxutils.h"
#include "sh_vector.h"
#include "sm_trie.h"
#include <IGameConfigs.h>
#include "IThreader.h"
#include "css_offsets.h"
#include "ZMClass.h"

#define ZMGlobals g_SMAPI->GetCGlobals()

#define WEAPONINFO_COUNT 29

#include "Timers.h"

class ZombieClass;

struct dmpatch_t
{
	dmpatch_t()
	{
		patch[0] = 0;
		bytes = 0;
	}
	unsigned char patch[20];
	size_t bytes;
};

#if !defined ORANGEBOX_BUILD
	class CCommand
	{
	public:
		const char *ArgS()
		{
			return m_Engine->Cmd_Args();
		}
		int ArgC()
		{
			return m_Engine->Cmd_Argc();
		}
	 
		const char *Arg(int index)
		{
			return args.Arg(index);
		}
	};
#endif


#if !defined ORANGEBOX_BUILD
	CCommand args;
	extern CCommand args;
#endif


// - LDUKE
class CTakeDamageInfoHack : public CTakeDamageInfo
{
public:
	inline int GetAttacker() { return CTakeDamageInfo::m_hAttacker.GetEntryIndex(); }
	inline int GetInflictor() { return CTakeDamageInfo::m_hInflictor.GetEntryIndex(); }
};
// - LDUKE

typedef SourceHook::String myString;

#define PVFN( classptr , offset ) ((*(DWORD*) classptr ) + offset)
#define VFN( classptr , offset ) *(DWORD*)PVFN( classptr , offset )

#define IN_ATTACK		(1 << 0)
#define IN_JUMP			(1 << 1)
#define IN_DUCK			(1 << 2)
#define IN_FORWARD		(1 << 3)
#define IN_BACK			(1 << 4)
#define IN_USE			(1 << 5)
#define IN_CANCEL		(1 << 6)
#define IN_LEFT			(1 << 7)
#define IN_RIGHT		(1 << 8)
#define IN_MOVELEFT		(1 << 9)
#define IN_MOVERIGHT	(1 << 10)
#define IN_ATTACK2		(1 << 11)
#define IN_RUN			(1 << 12)
#define IN_RELOAD		(1 << 13)
#define IN_ALT1			(1 << 14)
#define IN_ALT2			(1 << 15)
#define IN_SCORE		(1 << 16)   	// Used by client.dll for when scoreboard is held down
#define IN_SPEED		(1 << 17)	// Player is holding the speed key
#define IN_WALK			(1 << 18)	// Player holding walk key
#define IN_ZOOM			(1 << 19)	// Zoom key for HUD zoom
#define IN_WEAPON1		(1 << 20)	// weapon defines these bits
#define IN_WEAPON2		(1 << 21)	// weapon defines these bits
#define IN_BULLRUSH		(1 << 22)
#define IN_GRENADE1		(1 << 23)	// grenade 1
#define IN_GRENADE2		(1 << 24)	// grenade 2

#define MAX_CLIENTS ZMGlobals->maxClients //g_SMAPI->GetCGlobals()->maxClients

#define TYPE_TEXT								0  // just display this plain text
#define TYPE_INDEX								1 // lookup text & title in stringtable
#define TYPE_URL								2   // show this URL
#define TYPE_FILE								3  // show this local file

#define SPECTATOR								1 // Team index number
#define TERRORISTS								2 // Team index number
#define COUNTERTERRORISTS						3 // Team index number

#define SLOT_PRIMARY							0 // weapon slots
#define SLOT_SECONDARY							1
#define SLOT_KNIFE								2
#define SLOT_NADE								3
#define SLOT_BOMB								4

#define Target_Bombed							0
#define VIP_Escaped								1 // Round end type.
#define VIP_Assassinated						2
#define Terrorists_Escaped						3
#define CTs_PreventEscape						4
#define Escaping_Terrorists_Neutralized			5
#define Bomb_Defused							6
#define CTs_Win									7
#define Terrorists_Win							8
#define Round_Draw								9
#define All_Hostages_Rescued					10
#define Target_Saved							11
#define Hostages_Not_Rescued					12
#define Terrorists_Not_Escaped					13
#define VIP_Not_Escaped							14

#define JetpackSound							"npc/manhack/mh_engine_loop2.wav"

#define ZOMBIE_VERSION	"3.0.1 a"

#define AUTORESPAWN_CHECK_OFFSET	22969394
#define AUTORESPAWN_PROTECT_OFFSET	23969358
#define ZOMBIECHECK_TIMER_ID 		69785474
#define RANDOM_ZOMBIE_TIMER_ID 		16723963
#define MAX_SERVERCMD_LENGTH		128
#define NUM_ZOMBIE_SOUNDS			14
#define MAX_MAP_NAME				32	


typedef unsigned char *PBYTE;
typedef short *PSHORT;
typedef long *PLONG;    
typedef void *PVOID;
typedef char *PCHAR;
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
#define VOID		void
typedef signed char         INT8;
typedef signed short        INT16;
typedef signed int          INT32;
typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
#define LPCSTR				const char *
#define WINAPI				__stdcall
#define ERROR_INVALID_DATA               13L

class vEmptyClass { };

typedef void			  ( *NoClipFunction )( void );
typedef void			  ( *RemoveFunction )( CBaseEntity * );
typedef FileWeaponInfo_t *( * GetFileWeaponInfoFromHandleFunc)( WEAPON_FILE_INFO_HANDLE handle );

typedef void ( * ClientPrint_Func )( CBasePlayer *, int, const char *, const char *, const char *, const char *, const char *);

extern RemoveFunction				g_UtilRemoveFunc;

#ifdef WIN32
	typedef void ( * SetMinMaxSize )( CBaseEntity *, const Vector&, const Vector& );

	typedef CBaseEntity *(* FindEntityByClassname_Func )( CBaseEntity *pStartEntity, const char *szName );
	typedef void (*CCSPlayer_RoundRespawnFunc)( );
	typedef int (*OnTakeDamageFunction)( CTakeDamageInfo &inputInfo );
	typedef bool (*WantsLagFunction)( const CBasePlayer *, const CUserCmd *, const CBitVec<MAX_EDICTS> * );
	typedef void (*TermRoundFunction)(float, int);
	typedef CBaseEntity	*( *GiveNamedItem_Func) ( const char *, int );
	typedef CBaseEntity *( *FindEntityByClassname_Func )( CBaseEntity *, const char * );
	typedef void (*WeaponDropFunction)(CBaseCombatWeapon *, const Vector *, const Vector * );
	typedef CBaseCombatWeapon *(*WeaponSlotFunction)(int);
	typedef char *(*GetNameFunction)(void);
	typedef void (*WeaponDeleteFunction)(void);
#else
	#include <sys/socket.h>
	typedef CBaseEntity *(* FindEntityByClassname_Func )( CGlobalEntityList *, CBaseEntity *pStartEntity, const char *szName );
	typedef void (* CCSPlayer_RoundRespawnFunc)( CBasePlayer* );
	typedef int (*OnTakeDamageFunction)( CBasePlayer* , CTakeDamageInfo &inputInfo );
	typedef bool (*WantsLagFunction)( CBasePlayer *, const CBasePlayer *, const CUserCmd *, const CBitVec<MAX_EDICTS> * );
	typedef void (*TermRoundFunction)(CGameRules *, float, int);
	typedef CBaseEntity	*( *GiveNamedItem_Func) ( CBasePlayer *, const char *, int );
	typedef CBaseEntity *( *FindEntityByClassname_Func )( CGlobalEntityList *, CBaseEntity *, const char * );
	typedef void (*WeaponDropFunction)( CBasePlayer *, CBaseCombatWeapon *, const Vector *, const Vector * );
	typedef CBaseCombatWeapon *(*WeaponSlotFunction)(CBaseCombatCharacter *, int);
	typedef char*(*GetNameFunction)(CBaseCombatWeapon *);
	typedef void (*WeaponDeleteFunction)(CBaseCombatWeapon *);
	typedef void (*SetCollisionBounds_Func)( CBaseEntity *, const Vector&, const Vector & );
#endif

extern char					g_CurrentMap[MAX_MAP_NAME];
extern myString				g_MapEntities;


extern unsigned int			iGlobalVoteTimer;
extern int					iPlayerCount;
extern bool					bLanguagesLoaded;
extern KeyValues			*kLanguage;
extern myString				sLangError;

extern ConVar				*mp_restartgame;

extern IGameEventManager2	*m_GameEventManager;	
extern IEngineTrace			*m_EngineTrace;
extern IVEngineServer		*m_Engine;
extern IServerGameDLL		*m_ServerDll;
extern IPlayerInfoManager	*m_PlayerInfoManager;
extern IServerGameEnts		*m_GameEnts;
extern STimers				*g_Timers;
extern int g_MaxClip[WEAPONINFO_COUNT];
extern int g_DefaultClip[WEAPONINFO_COUNT];
extern ISmmPlugin			*g_PLAPI;
extern GetFileWeaponInfoFromHandleFunc g_GetFileWeaponInfoFromHandle;
extern void					*g_EntList;
extern IServerGameEnts		*m_GameEnts;
extern int					entInfoOffset;
extern CSharedEdictChangeInfo		*g_pSharedChangeInfo;
extern bool g_bZombieClasses;

struct OffsetInfo_t
{
	int		m_iHealth;
	int		m_iAccount;
	int		m_bHasNightVision;
	int		m_iTeamNum;
	int		m_iTeamTeamNum;
	int		m_iScore;
	int		m_lifeState;
	int		m_iFOV;
	int		m_Local;
	int		m_fFlags;
	int		m_bNightVisionOn;
	int		m_bDrawViewmodel;
	int		m_iHideHUD;
	int		m_iFrags;
	int		m_iDeaths;
	int		m_bInBuyZone;
	int		m_vecOrigin;
	int		m_MoveType;
	int		m_Collision;
	int		m_clrRender;
	int		m_nRenderMode;
	int		m_hRagdoll;
	int		m_iDefaultFOV;
	int		m_vecBaseVelocity;
	int		localdata;
	int		m_flMaxspeed;
	int		m_hActiveWeapon;

	int		m_clrRenderWeapon;
	int		m_nRenderModeWeapon;
};

struct TimerInfo_t
{
	unsigned int iTimer;
	int iPlayer;
};

struct ModelInfo_t
{
	myString				sModel;
	int						iPrecache;
	bool					bHasHead;
	myString				sHeadLess;
	myString				sHead;
	int						iHeadPrecache;
};

/*
struct ZombieClasses_t
{
	myString	sClassname;
	myString	sModel;
	int			iHealth;
	float		fSpeed;
	float		fSpeedDuck;
	float		fSpeedRun;
	int			iJumpHeight;
	float		fKnockback;
	bool		bInUse;
	int			iPrecache;
	int			iHeadshots;
	bool		bHeadShotsOnly;
	int			iRegenHealth;
	float		fRegenTimer;
	float		fGrenadeMultiplier;
	float		fGrenadeKnockback;
	int			iHealthBonus;
	bool		bFallDamage;
};
*/

struct BuyMenuInfo_t
{
	bool		bPistols;
	bool		bSMGs;
	bool		bRifles;
	bool		bMGs;
	bool		bEquip;
	myString	iPistols[9];
	myString	iSMGs[9];
	myString	iRifles[9];
	myString	iMGs[9];
	myString	iEquip[9];
	myString	sPistols[9];
	myString	sSMGs[9];
	myString	sRifles[9];
	myString	sMGs[9];
	myString	sEquip[9];
};

struct PlayerInfo_t
{
	CBasePlayer				*pPlayer;
	CBaseCombatWeapon 		*pKnife;
	int						iBulletCount;
	bool					isZombie;
	bool					isBot;
	bool					isHooked;
	bool					isChangingTeams;
	bool					isHeadshot;
	Vector					vLastHit;
	Vector					vDirection;
	int						iModel;
	int						iHeadShots;
	bool					bHeadCameOff;
	int						iJetPack;
	bool					bJetPack;
	int						iUserID;
	bool					bSentWelcome;
	bool					bConnected;
	bool					bClientCommandsRestrited;
	bool					bProtected;
	int						iShownMOTD;
	unsigned int			iProtectTimer;
	unsigned int			iVisionTimer;
	bool					bAsZombie;
	myString				sUserName;
	RenderMode_t			tRenderMode;
	int						iStuckRemain;
	bool					bHasSpawned;
	int						iVictimList[MAX_PLAYERS + 1];
	int						iHealth;
	int						iZombieHealth;
	bool					bFaded;
	int						iClass;
	int						iClassMenu;
	int						iMenu;
	int						iBuyMenu;
	int						iChangeToClass;
	bool					bShowClassMenu;
	bool					bChoseClass;
	unsigned int			iRegenTimer;
	Vector					vSpawn;
	QAngle					qSpawn;
	int						iBuyWeapons[5];
	bool					bSameWeapons;
	CCollisionProperty		*m_Collision;
	float					fOriginalKnifeSpeed;
	bool					bShowZombieVision;

#ifdef _ICEPICK
	float					fKnockback;
#endif
};


class ZombiePlugin : public ISmmPlugin 
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	void AllPluginsLoaded();
	bool Pause(char *error, size_t maxlen)
	{
		return true;
	}
	bool Unpause(char *error, size_t maxlen)
	{
		return true;
	}
public:
	int GetApiVersion() { return METAMOD_PLAPI_VERSION; }
public:
	const char *GetAuthor()
	{
		return "c0ldfyr3";
	}
	const char *GetName()
	{
		return "ZombieMod";
	}
	const char *GetDescription()
	{
		return "Zombie Mod for CS:S";
	}
	const char *GetURL()
	{
		return "http://www.zombiemod.com/";
	}
	const char *GetLicense()
	{
		return "none";
	}
	const char *GetVersion()
	{
		return ZOMBIE_VERSION;
	}
	const char *GetDate()
	{
		return __DATE__;
	}
	const char *GetLogTag()
	{
		return "ZOMBIE";
	}
public:

	float g_fBuyTimeLimit;

	bool LevelInit_Pre(const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background);
	bool LevelInit(const char *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background);
	void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
	void GameFrame(bool simulating);
	void LevelShutdown(void);
	void ClientActive(edict_t *pEntity, bool bLoadGame);
	void ClientDisconnect(edict_t *pEntity);
	void ClientPutInServer(edict_t *pEntity, char const *playername);
	void SetCommandClient(int index);
	bool ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);

#if defined ORANGEBOX_BUILD
	void ClientCommand(edict_t *pEntity, const CCommand &args);
#else
	void ClientCommand(edict_t *pEntity);
#endif

	bool SetClientListening(int iReceiver, int iSender, bool bListen);
	void WriteByte( int val );
	bool WriteString( const char *pStr );
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	int OnTakeDamage( CTakeDamageInfoHack &info );
	void UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity );
	void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	bool Weapon_CanUse( CBaseCombatWeapon *pWeapon );
	bool Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon );
	void Delete( void );
	void PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize );
	void Touch( CBaseEntity *pOther );
	void ChangeTeam( int iTeam );
	void AddTeamScore( CTeam *cTeam );
	
	void CommitSuicide( bool bExplode, bool bForce );
	
	META_RES CommitSuicideHook( );
	void PerformSuicide( CBasePlayer *pBase, bool bExplode, bool bForce );
	void Event_Killed( const CTakeDamageInfo &info );
	void PreThink( void );
	bool FireEvent( IGameEvent *pEvent, bool bDontBroadcast = false );
	bf_write *UserMessageBegin( IRecipientFilter *filter, int msg_type );

	void ChangeRunHook( int iOffset );

	void RestrictWeapon( const char *sWhoFor, const char *sWeapon, const char *sCount, const char *sWholeString );

	void ShowMOTD(int index, char *title, char *msg, int type, char *cmd);
	void ZombieDropWeapons( CBasePlayer *pPlayer, bool bHook = false, bool bRespawn = false );
	bool LoadSignatures( void* laddr, char *error, size_t maxlen );
	bool LoadZombieModelList(const char *filename, const char *downloadfile);
	bool LoadZombieHeads( int iIndex, char *sModel );
	void MakeZombie( CBasePlayer *pPlayer, int iHealth, bool bFirst = false, int iAttacker = -1 );
	float GetMaxSpeed(void);
	void ZombieOn();
	void ZombieOff();
	bool IsAlive( edict_t *pEntity );
	void ShowZomb();
	void RemoveHead( CBasePlayer *pPlayer, int iIndex );
	const char *ParseAndFilter( const char *map, const char *ents );
	int UserMessageIndex(const char *messageName);
	void DispatchEffect( const char *pName, const CEffectData &data );
	void ZombieEffect( Vector vLocation );
	void TransmitShakeEvent( CBasePlayer *pPlayer, float localAmplitude, float frequency, float duration, ShakeCommand_t eCommand );
	void UTIL_ScreenShake( CBasePlayer *pPlayer, float amplitude, float frequency, float duration, float radius, ShakeCommand_t eCommand, bool bAirShake );
	float ComputeShakeAmplitude( const Vector &center, const Vector &shakePt, float amplitude, float radius ) ;
	void UTIL_BloodSpray( const Vector &pos, const Vector &dir, int color, int amount, int flags );
	void Slap( int iPlayer );
	Vector ClosestPlayer( int iPlayer, Vector vecPlayer );

	void ZombieVision( edict_t *pEntity, bool bEnable );
	void SetWeaponProtection( CBasePlayer *pPlayer, bool bProtect, RenderMode_t tRenderMode, color32 cColour );
	void SetFOV( edict_t* cEntity, int iFOV, bool bChangeCheats = true );

	void Dissolve( edict_t *pEntity );

	void DoAmmo(  bool bEnable );
	void RestrictWeapons();

	void ShowDamageMenu( edict_t *pEntity, edict_t *pKiller = NULL );

	void HookPlayer( int iPlayer );
	void UnHookPlayer( int iPlayer );

	void UTIL_DrawMenu( int player_index, int   time, int range, const char *menu_string, bool final = false );
	void UTIL_ClientPrintf(edict_t *pEdict, const char *szMsg);

	void ShowMainMenu( edict_t * pEntity );
	bool CanUseWeapon( CBaseEntity* pPlayer, CBaseEntity *pEnt, bool bDrop = true );
	bool AllowWeapon( CBasePlayer *pPlayer, int WeaponID, int &nCount );
	void CheckAutobuy( int nIndex, CBasePlayer* pPlayer );
	int EntIdxOfUserIdx(int useridx);
	void Hud_Print( edict_t *pEntity, const char *sMsg, ... );

	void HintTextMsg( edict_t *pEntity, const char *sMsg, ... );

	int GetTeam(edict_t *pEntity);

	void Set_sv_cheats( bool bEnable, bool *bOldValue );
	void GiveWeapons( int iPlayer, edict_t *cEntity );

	bool g_ZombieRoundOver;

	void FindCVars();

	bool LoadLanguages();

	bool LoadBuyMenus( );

	void GiveMoney( edict_t *pEntity, int iMoney );

	const char *GetGameDescription( void );

	const char* GetLang( const char *sKeyName );

	STimers				*g_Timers;
	int					iCurrentPlayer;

	OffsetInfo_t		g_Offsets;
	IEngineSound		*m_EngineSound;
	IEffects			*m_Effects;
	IVModelInfo			*m_ModelInfo;
	IFileSystem			*m_FileSystem;
	INetworkStringTableContainer			*m_NetworkStringTable; 

	int GetStringTable( const char *sTable, const char *sMaterial );
	int SetStringTable( const char *sTable, const char *sString );
	void DoFade( CBaseEntity* cBase, bool bEnable = true, int iPlayer = -1 );
	void SetProtection( int iPlayer, bool bProtect = true, bool bReport = true, bool bRoundEnd = false );
	void RemoveAllSpawnTimers();

	// - Events
	void Event_Round_Freeze_End( );
	META_RES Event_Player_Say( int iPlayer, const char *sText );
	void Event_Player_Spawn( int iPlayer );
	void Event_Player_Death( int iVic, int iAtt, const char*sWeapon );
	void Event_Round_End( int iWinner );
	void Event_Hostage_Follows( int iHosti );
	void Event_Round_Start( );
	void Event_Player_Jump( int iPlayer );
	void Event_Weapon_Fire( int iUser, const char *sWeapon );
	void Event_Player_Hurt( int iUser, const char *sWeapon );

	bool						bAllowedToJetPack;
	bool						bZombieDone;
	bool						bRoundStarted;
	bool						bGameOn;
	bool						bClientConnecting;
	int							iUseSpawn;
	int							iNextSpawn ;

	Vector						vSpawnVec[MAX_PLAYERS + 1];

	void RoundEndOverlay( bool bEnable, int iWinners );
	bool LoadZombieClasses();
	void ShowClassMenu( edict_t * pEntity );

	void HudMessage( int EdictId, const hudtextparms_t &textparms, const char *pMessage );
	void HudMessage( int EdictId, int channel, float x, float y, byte r1, byte g1, byte b1, byte a1, byte r2, byte g2, byte b2, byte a2, int effect, float fadeinTime, float fadeoutTime, float holdTime, float fxTime, const char *pMessage);

	void ZombieModHelpText( INetworkStringTable *pInfoPanel );

	void DisplayHelp( INetworkStringTable *pInfoPanel );

	void SelectZombieClass(edict_t *pEntity, int iPlayer, ZombieClass *zClass, bool bNow = false, bool bMessages = true);

	void DisplayClassHelp( INetworkStringTable *pInfoPanel );

	void ZombificationEvent( int iUserId, int iClass, int iAttacker = -1);

	META_RES MenuSelect( int iPlayer, edict_t *pEntity, int iMenu );

	void ShowBuyMenu( edict_t * pEntity, int iMenu );

	void DisplayBuyMenu( edict_t * pEntity, int iPlayer, int iMenu );

	void ClearBuyMenus( );

	void DropWeapon( CBasePlayer *pPlayer, bool bPrimary );

	void GiveSameWeapons( edict_t *pEntity, int iPlayer );

	bool BuyTimeExceeded();

	void GiveAmmo( CBaseCombatCharacter *pPlayer, int iCount, const char *sWeapon );

	void LoadZombieEvents( void );

	void UnLoadZombieEvents( void );

	void RemoveObjectives( void );

	void PrecacheSounds( void );

	//bool ChangeRoundEndSounds( void );

#ifdef ANTISTICK
	void UnstickPlayer( int iPlayer );
#endif

#ifdef ENDSOUND
	bool AddSound( const char *soundname, const char *scriptfile, const CSoundParametersInternal& params );
	bool GetParametersForSound( const char *soundname, CSoundParameters& params, gender_t gender, bool isbeingemitted );
	const char		*GetWavFileForSoundInt1( const char *soundname, char const *actormodel );
	const char		*GetWavFileForSoundInt2( const char *soundname, gender_t gender );
	const char		*GetWaveName( CUtlSymbol& sym );
	CUtlSymbol		AddWaveName( const char *name );
	bool			GetParametersForSoundEx( const char *soundname, HSOUNDSCRIPTHANDLE& handle, CSoundParameters& params, gender_t gender, bool isbeingemitted );
#endif

	ISoundEmitterSystemBase *m_SoundEmitterSystem;
private:

	void AddDownload( const char *file );
	IVoiceServer		*m_VoiceServer;
	ITempEntsSystem		*m_TempEnts;
	ICvar				*m_CVar;
	IServerGameClients	*m_ServerClients;
};

extern ZombiePlugin g_ZombiePlugin;

#if defined ORANGEBOX_BUILD
	void CVar_CallBack(IConVar *var, const char *pOldString, float flOldValue);
#else
	void CVar_CallBack( IConVar *var, char const *pOldString );
#endif

void FindGameRules(void *addr);

const char *GetPlayerClassName( edict_t *pEntity );
CBaseEntity *UTIL_FindEntityByClassName( CGlobalEntityList *EntList, CBaseEntity *pStartEntity, const char *szName );
CBaseEntity *FindEntityByClassName( CGlobalEntityList *EntList, CBaseEntity *pStartEntity, const char *szName );
bool CCSPlayer_IsInBuyZone( CBasePlayer *pPlayer );
void CBasePlayer_ApplyAbsVelocityImpulse( CBasePlayer *pPlayer, Vector &vecImpulse );
void CBasePlayer_SwitchTeam( CBasePlayer *pPlayer, int iTeam, bool bIgnorePlayerCount = false );
bool CBasePlayer_SetFOV( CBasePlayer *pPlayer, CBaseEntity *pRequester, int FOV, float zoomRate = 0.0f );
void UTIL_RestartRound( CBasePlayer *pPlayer );
void UTIL_WeaponDrop( CBasePlayer *pBasePlayer, CBaseCombatWeapon *pWeapon, const Vector *target, const Vector *velocity );
char* UTIL_GetName(CBaseCombatWeapon *pWeapon);
void UTIL_WeaponDelete(CBaseCombatWeapon *pWeapon);
void UTIL_TermRound(float delay, int reason);
void UTIL_SetModel( CBaseEntity *pEntity, const char *pModelName );
CBaseEntity	*UTIL_GiveNamedItem( CBasePlayer *pPlayer, const char *pszName, int iSubType );
int UTIL_GetNumPlayers( CTeam *pTeam );
CCollisionProperty *UTIL_GetCollision( edict_t *pEntity );
void UTIL_MemProtect( void *addr, int length, int prot );

void UTIL_StringToFloatArray( float *pVector, int count, const char *pString );
Vector UTIL_MidPoint(Vector p1,Vector p2);
void UTIL_StringToVector( float *pVector, const char *pString );

void Timed_Dissolve( void **params );

CBaseEntity *UTIL_GetPlayerRagDoll( edict_t *pEntity );
CBaseEntity *UTIL_GetPlayerWeapon( edict_t *pEntity );
//void UTIL_TraceLines( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter *pFilter, trace_t *ptr, bool bRed );
void FixSlashes( char *str );
int LookupBuyID(const char *name);
int LookupRebuyID(const char *name);
const char *LookupWeaponName(int id);
float DamageForce(const Vector &size, float damage, float fKnockback );
void CheckZombies(void **params);
void ZombieLevelInit( void **params );
void RandomZombie(void **params);
void CheckZombieWeapons( void **params );
void CheckRespawn( void **params );
void Timed_SetFOV( void **params );
void Timed_SetModel( void **params );
void Timed_RoundTimer( void **params );
void Timed_SetBuyZone( void **params );
void Timed_ConsGreet( void **params );
void TimerProtection(void **params);
void TimerShowVictims(void **params);
void TimerVision(void **params);

void Timed_Remove( void **params );

void Timed_Bot( void **params );

void Timed_Regen( void **params );

void ForceScreenFades( void **params );

void DumpMapEnts();
void UTIL_PathFmt(char *buffer, size_t len, const char *fmt, ...);
void ClearRegen();
void ShowZomb();

#if defined ORANGEBOX_BUILD
	void cmdKill( const CCommand &command );
	void cmdTeamSay( const CCommand &command );
	void cmdSay( const CCommand &command );
	void cmdAutoBuy( const CCommand &command );
#else
	void cmdKill( void );
#endif

void ClientPrint_hook( CBasePlayer *player, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 );

void FFA_Enable();
void FFA_Disable();
void DM_ProtectMemory(void *addr, int length, int prot);
void DM_ApplyPatch(void *address, int offset, const dmpatch_t *patch, dmpatch_t *restore);
size_t DM_StringToBytes(const char *str, unsigned char buffer[], size_t maxlength);
void CBaseEntity_Teleport( CBaseEntity *pThisPtr, Vector *newPosition, QAngle *newAngles, Vector *newVelocity );
void KickStartEnd( void **params ) ;

int GetAmmoIndex( const char *sWeapon );

int OnIPointsForKill(CBasePlayer *pl1, CBasePlayer *pl2);
void LoadLinuxSigs(void);

#if !defined ( WIN32 )
	void UTIL_GetGamePath ( char *path );
	void GetLinuxBins ( char *game, char *engine ) ;
#endif

// + DDOS
void ClearDosAddrList();
void ReHookRecvFrom();
int MyRecvFromHook(int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen);
void UnHookRecvFrom();
void AddDoSCount(int ip1, int ip2, int ip3, int ip4);
// - DDOS

void IterateSigs(Trie *pTrie, const char *key, void **value, void *data);

class CPlayerMatchList
{
private:
	CUtlVector<edict_t*>	m_pEdictList;
public:
	bool 			m_bAliveOnly;
	bool			m_bDeadOnly;
	bool 			m_bAllPlayers;
	bool			m_bMatchBots;
	bool			m_bMatchOne;
	bool			m_bSteamID;
	bool			m_bUserID;
	int				m_nUserID;
	int 			m_iTeam;
	char			m_szMatchString[128];
	
	CPlayerMatchList(const char *pMatchString, bool bAliveOnly = false, bool bDeadOnly = false, bool bMatchBots = true, bool bMatchOne = false)
	{
		Q_strncpy((char*)&m_szMatchString, pMatchString, sizeof(m_szMatchString));
		m_bAliveOnly = bAliveOnly;
		m_bDeadOnly = bDeadOnly;
		m_bMatchBots = bMatchBots;
		m_bMatchOne = bMatchOne;
        m_bAllPlayers = (!Q_strnicmp(pMatchString, "#a", 2));
		m_bSteamID = ((!Q_strnicmp(pMatchString, "STEAM_", 6) && pMatchString[7] == ':' && pMatchString[9] == ':') || !Q_strnicmp(pMatchString, "BOT", 3));
		m_iTeam = ((Q_strnicmp(pMatchString, "#t", 2) == 0) ? 2 : ((Q_strnicmp(pMatchString, "#c", 2) == 0) ? 3 : ((Q_strnicmp(pMatchString, "#s", 2) == 0) ? 0 : -1)));
		m_bUserID = (m_iTeam == -1 && pMatchString[0] == '#' && atoi(pMatchString+1));
		if (m_bUserID)
			m_nUserID = atoi(pMatchString+1);
        
		for ( int i = 1; i <= MAX_CLIENTS; i++ )
		{
			edict_t *pEdict = m_Engine->PEntityOfEntIndex(i);
			CBasePlayer *pPlayer = (CBasePlayer *)m_GameEnts->EdictToBaseEntity( pEdict );
			if (!pEdict || pEdict->IsFree() || !pPlayer || (m_Engine->GetPlayerUserId(pEdict) < 0))
			{
				continue;
			}
			if ( !bMatchBots && CBasePlayer_IsBot( pPlayer ) )
			{
				continue;
			}

			bool bAlive = g_ZombiePlugin.IsAlive( pEdict );
			const char *pName = m_Engine->GetClientConVarValue(i, "name");
			if (m_bAliveOnly && !bAlive )
			{
				continue;
			}
			else if (m_bDeadOnly && bAlive )
			{
				continue;
			}
			else if (m_bSteamID && m_Engine->GetPlayerNetworkIDString(pEdict) && !Q_stricmp(m_Engine->GetPlayerNetworkIDString(pEdict), m_szMatchString))
			{
				m_pEdictList.Purge();
				m_pEdictList.AddToTail(pEdict);
				return;
			}
			else if (m_bUserID && (m_Engine->GetPlayerUserId(pEdict) == m_nUserID))
			{
				m_pEdictList.Purge();
				m_pEdictList.AddToTail(pEdict);
				return;
			}
			else if (m_bMatchOne && m_Engine->GetClientConVarValue(i, "name") && !Q_stricmp(m_szMatchString, pName))
			{
				m_pEdictList.Purge();
				m_pEdictList.AddToTail(pEdict);
				return;
			}
			else if (m_bAllPlayers)
			{
				m_pEdictList.AddToTail(pEdict);
				continue;
			}
			else if (m_iTeam > -1)
			{
				int team = g_ZombiePlugin.GetTeam( pEdict );
				if ( ( m_iTeam == team ) || ( ( m_iTeam == 0 ) && ( team == 1 ) ) )
				{
					m_pEdictList.AddToTail(pEdict);
				}
				continue;
			}
			if (pName && Q_stristr(pName, m_szMatchString))
			{
				m_pEdictList.AddToTail(pEdict);
			}
		}
	}
	int Count() { return (int)m_pEdictList.Count(); }
	edict_t *GetMatch(int num) { 
		if ((num < 0) || (num >= m_pEdictList.Count())) 
			return NULL;
		return m_pEdictList[num];
	}
};


PLUGIN_GLOBALVARS();

#endif //_INCLUDE_ZombiePlugin_H
