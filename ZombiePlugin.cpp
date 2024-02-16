/* ======== ZombieMod_mm ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): David "BAILOPAN" Anderson
* ============================
*/

#include "ZombiePlugin.h"
#include "cvars.h"
#include "myutils.h"
#include "eiface.h"
#include "KeyValues.h"
#include "VFunc.h"
#include "sourcehook.h"
#include "tier0/vcrmode.h"
#include "mathlib.h"
#include "zm_linuxutils.h"
#include "CSigManager.h"
#include "css_offsets.h"

#include <sm_trie_tpl.h>
#include "sm_trie.h"

#undef CreateEvent
#undef GetClassName

#if defined ( WIN32 )
	#define PATH_SLASH '\\'
#else
	#define PATH_SLASH '/'
#endif

myString					sRestrictedWeapons;

CUtlVector<TimerInfo_t>		g_RespawnTimers;

ZombiePlugin				g_ZombiePlugin;
PlayerInfo_t				g_Players[65];
BuyMenuInfo_t				g_tBuyMenu;
bool						g_bBuyMenu;
//ZombieClasses_t				g_ZombieClasses[100];
//int							g_iZombieClasses = -1;
int							g_RestrictT[CSW_MAX];
int							g_RestrictCT[CSW_MAX];
int							g_RestrictW[CSW_MAX];

IGameEventManager2			*m_GameEventManager;	
IEngineTrace				*m_EngineTrace;
IServerGameDLL				*m_ServerDll;
IVEngineServer				*m_Engine;
IAchievementMgr				*m_AchievementMgr;
IPlayerInfoManager			*m_PlayerInfoManager;
IServerPluginHelpers		*m_Helpers;
IServerGameEnts				*m_GameEnts;

#ifndef WIN32
	SetCollisionBounds_Func		g_SetCollisionBounds = NULL;
#else
	SetMinMaxSize				g_SetMinMaxSize = NULL;
#endif

FindEntityByClassname_Func  g_FindEnt = NULL;
RemoveFunction				g_UtilRemoveFunc = NULL;
NoClipFunction				g_NoClipOff = NULL;
TermRoundFunction			g_TermRoundFunc = NULL;
GiveNamedItem_Func			g_GiveNamedItemFunc = NULL;
GetNameFunction				g_GetNameFunc = NULL;
WeaponDropFunction			g_WeaponDropFunc = NULL;
WantsLagFunction			g_WantsLag = NULL;
OnTakeDamageFunction		g_TakeDamage = NULL;

static void					*g_WantsLagPatch = NULL;
static void					*g_TakeDamagePatch = NULL;

GetFileWeaponInfoFromHandleFunc g_GetFileWeaponInfoFromHandle = NULL;
CCSPlayer_RoundRespawnFunc  g_CCSPlayer_RoundRespawn = NULL;
IStaticPropMgrServer		*staticpropmgr = NULL;

void						*v_SetFOV = NULL;
void						*v_SwitchTeam = NULL;
void						*v_ApplyAbsVelocity = NULL;
void						*v_KeyValues = NULL;


void						*g_LevelShutdown = NULL;
void						*g_EntList = NULL;
void						**g_gamerules_addr = NULL;

CBaseEntityList				*g_pEntityList = NULL;
CUtlVector<ModelInfo_t>		g_ZombieModels;
//CUtlVector< char * >		g_Offsets;


ConCommand					*pKillCmd = NULL;
ConCommand					*pSayCmd = NULL;
ConCommand					*pTeamSayCmd = NULL;
ConCommand					*pAutoBuyCmd = NULL;
ConCommand					*pReBuyCmd = NULL;
ConCommand					*pNoClip = NULL;


bool						bUsingCVAR = false;
bool						bChangeCheats = false;

bool						bRoundTerminating = false;

/* Lagcomp */
static dmpatch_t g_lagcomp_patch;
static dmpatch_t g_lagcomp_restore;

/* Takedamage */
static dmpatch_t g_takedmg_patch[2];
static dmpatch_t g_takedmg_restore[2];

static void *g_domrev_addr = NULL;
static dmpatch_t g_domrev_patch;
static dmpatch_t g_domrev_restore;

bool						g_ffa = false;
CTeam						*teamCT = NULL;
CTeam						*teamT = NULL;
ArrayLengthSendProxyFn		TeamCount;
bool						bFirstEverRound = false;
bool						bLoadedLate = false;
bool						bFirstRound = false;

unsigned int				g_RoundTimer = 0;

static int					g_oldFFValue;
static int					g_oldSpawnProtectionTime;
static int					g_oldTKPunish;
static int					g_oldAlltalkValue;
static int					g_oldAmmoBuckshotValue;

static char					g_sZombieSounds[][20] = {"zombie_voice_idle1", "zombie_voice_idle2", "zombie_voice_idle3", "zombie_voice_idle4", "zombie_voice_idle5", "zombie_voice_idle6", "zombie_voice_idle7", "zombie_voice_idle8", "zombie_voice_idle9", "zombie_voice_idle10", "zombie_voice_idle11", "zombie_voice_idle12", "zombie_voice_idle13", "zombie_voice_idle14"};
char						g_CurrentMap[MAX_MAP_NAME];
myString					g_MapEntities;
CBaseEntity					*pFogController = NULL;
KeyValues					*kvPlayerClasses;
KeyValues					*kvPlayerVision;

bool bBotAdded				= false;
bool						bKickStart[MAX_PLAYERS];
unsigned int				iKickTimer = 0;

unsigned int				iGlobalVoteTimer = 0;

bool						bLanguagesLoaded = false;

KeyValues					*kLanguage = NULL;
myString					sLangError;

float						fDefaultSpeed = 0.0;

char						linux_game_bin[256];
char						linux_engine_bin[256];

unsigned int ibTimer = 0;

int							entInfoOffset = 4;

CSharedEdictChangeInfo		*g_pSharedChangeInfo = NULL;

char						m_WantsLagPatch[1024];
char						m_TakeDamagePatch1[1024];
char						m_TakeDamagePatch2[1024];
char						m_CalcDomRevengePatch[1024];

char						*m_sGameDescription = NULL;


PLUGIN_EXPOSE( ZombiePlugin, g_ZombiePlugin );

INetworkStringTable *g_pStringTableZM;

#include "meta_hooks.h"

#define GET_CFG_ENDPARAM(var, maxlen) \
		if (c >= l) continue; \
		while ((line[c] == ' ') || (line[c] == '\t')) { \
			c++; \
			if (c-1 >= l) continue; \
		} \
		if (((line[c] == '/') && (line[c+1] == '/')) || (line[c] == '\r' || line[c] == '\n')) continue; \
		x = 0; \
		while (line[c] != '\n' && line[c] != '\r' && ((line[c] != '/') || ((l-c < 2) || (line[c+1] != '/')))) { \
			if (x < (maxlen-1)) { \
				var[x] = line[c]; \
				x++; \
			} \
			c++; \
			if (c >= l) break; \
		} \
		var[x] = 0; \
		if (!strlen(var)) { \
		        META_LOG( g_PLAPI, "Error parsing line %d in %s", lineno, filename); \
		        continue; \
		}

#define	FIND_IFACE(func, assn_var, num_var, name, type) \
	do { \
		if ( (assn_var=(type)((ismm->func())(name, NULL))) != NULL ) { \
			num_var = 0; \
			break; \
		} \
		if (num_var >= 999) \
			break; \
	} while ( num_var=ismm->FormatIface(name, sizeof(name)-1) ); \
	if (!assn_var) { \
		if (error) \
		{ \
			g_SMAPI->Format( error, maxlen, "Could not find interface %s", name ); \
		} \
		return false; \
	}

#define GETCLASS( iClass ) \
	ZombieClass *zClass; \
	zClass = m_ClassManager.GetClass( iClass );

inline bool IsBadPtr( void *p ) { return ( (p==NULL)||(p==(void*)(-1)) ); }

#define	Debug() return zombie_debug.GetBool()


ConVar *mp_friendlyfire = NULL;
ConVar *mp_limitteams = NULL;
ConVar *mp_autoteambalance = NULL;
ConVar *mp_spawnprotectiontime = NULL;
ConVar *mp_tkpunish = NULL;
ConVar *sv_forcepreload = NULL;
ConVar *sv_alltalk = NULL;
ConVar *sv_cheats = NULL;
ConVar *mp_roundtime = NULL;
ConVar *mp_restartgame = NULL;
ConVar *mp_buytime = NULL;
ConVar *ammo_buckshot_max = NULL;

int g_MaxClip[WEAPONINFO_COUNT];
int g_DefaultClip[WEAPONINFO_COUNT];

int iPlayerCount = 0;
int iZombieCount = 0;
int iHealthDecrease = 0;
int g_iDefaultClass = 0;
bool g_bZombieClasses = false;
bool g_bOverlayEnabled = false;

CBaseEntity				*pJumper[MAX_PLAYERS];
int						iJumpers = -1;
int						iJumper[MAX_PLAYERS];

int iNextHealth = 0;

const char *g_SettingsFile = "cfg/zombiemod/zombie_classes.cfg";
const char *g_SigFile = "cfg/zombiemod/signatures.kv";

class ZombiePlugin;

#if !defined ORANGEBOX_BUILD
	extern CCommand args;
#endif

#define CVAR_ORIGINAL 1
#define CVAR_SHORT 2

#if defined ORANGEBOX_BUILD
	void CVar_CallBack(IConVar *var, const char *pOldString, float flOldValue)
#else
	void CVar_CallBack( IConVar *var, char const *pOldString )
#endif
{
	IConVar *pOtherVar = NULL;
	int lZombieVar = 0;
	const char *sName = var->GetName();

	if ( FStrEq( sName, zombie_unlimited_ammo.GetName() ) )
	{
		g_ZombiePlugin.DoAmmo( zombie_unlimited_ammo.GetBool() );
		lZombieVar = CVAR_ORIGINAL;
	}
	else if ( FStrEq( sName, zombie_restrictions.GetName() ) )
	{
		g_ZombiePlugin.RestrictWeapons();
		lZombieVar = CVAR_ORIGINAL;
	}
	else if ( zombie_enabled.GetBool() )
	{

		if ( FStrEq( sName, zombie_enabled.GetName() ) & !bUsingCVAR )
		{
			lZombieVar = CVAR_ORIGINAL;
			bUsingCVAR = true;
			zombie_enabled.SetValue( pOldString );
			bUsingCVAR = false;
			META_LOG( g_PLAPI, "ERROR: ZombieMod must be enabled/disabled via the 'zombie_mode' command." );
		}
		else if ( FStrEq( mp_restartgame->GetName(), var->GetName() ) )
		{
			g_ZombiePlugin.Event_Round_End( 0 );
		}
		else if ( ( FStrEq( sName, mp_friendlyfire->GetName() ) || ( zombie_teams.GetBool() &&  ( FStrEq( sName, mp_limitteams->GetName() ) || FStrEq( sName, mp_autoteambalance->GetName() ) ) ) ) )
		{
			if ( mp_friendlyfire->GetInt() != 0 )
			{
				mp_friendlyfire->SetValue( 0 );
			}
			if ( mp_limitteams->GetInt() != 0 )
			{
				mp_limitteams->SetValue( 0 );
			}
			if ( mp_autoteambalance->GetInt() != 0 )
			{
				mp_autoteambalance->SetValue( 0 );
			}
		}
		else if ( FStrEq( zombie_ffa_enabled.GetName(), sName ) )
		{
			lZombieVar = CVAR_ORIGINAL;
			if ( zombie_ffa_enabled.GetBool() )
			{
				FFA_Enable();
			}
			else
			{
				FFA_Disable();
			}
		}
		else if ( Q_strncmp( sName, "zm_", 3 ) == 0 )
		{
			lZombieVar = CVAR_SHORT;
		}
		else if ( Q_strncmp( sName, "zombie_", 7 ) == 0 )
		{
			lZombieVar = CVAR_ORIGINAL;
		}
	}
	if ( lZombieVar > 0 )
	{
		char sTmp[100];
		char sNewVarName[100];
		char sPrefix[8];

		if ( lZombieVar == CVAR_ORIGINAL )
		{
			Q_strncpy( sPrefix, "zm_", sizeof( sPrefix ) );
			Q_StrRight( sName, Q_strlen( sName ) - 7, sTmp, 100 );
		}
		else // CVAR_SHORT
		{
			Q_strncpy( sPrefix, "zombie_", sizeof( sPrefix ) );
			Q_StrRight( sName, Q_strlen( sName ) - 3, sTmp, 100 );
		}
		Q_snprintf( sNewVarName, sizeof( sNewVarName ), "%s%s", sPrefix, sTmp );
		
		ConVarRef cRef = ConVarRef( sName );
		ConVarRef cNewRef = ConVarRef( sNewVarName );

		cNewRef.m_pConVarState->m_fValue = cRef.m_pConVarState->GetFloat();
		cNewRef.m_pConVarState->m_nValue = cRef.m_pConVarState->GetInt();
		char sString[5120] = "";
		Q_strncpy(sString, (char*)cRef.m_pConVarState->GetString(), 5120 );
		if ( Q_strlen( sString ) > 0 )
		{
			if ( cNewRef.m_pConVarState->m_pszString )
			{
				delete[] cNewRef.m_pConVarState->m_pszString;
			}
			int lLen = Q_strlen(sString) + 1;
			cNewRef.m_pConVarState->m_pszString	= new char[lLen];
			cNewRef.m_pConVarState->m_StringLength = lLen;
			memcpy( cNewRef.m_pConVarState->m_pszString, sString, lLen );
		}
		
		if ( lZombieVar == 2 )
		{
			char sCommand[1024];
			if ( Q_strlen(sNewVarName) == 0 || Q_strlen(sString) == 0 )
			{
				Assert( false );
			}
			Q_snprintf( sCommand, sizeof( sCommand ), "%s \"%s\"\n", sNewVarName, sString );
			m_Engine->ServerCommand( sCommand );
		}
	}
	return;
}

void ZombiePlugin::DoAmmo( bool bEnable )
{
	if ( !zombie_enabled.GetBool() )
	{
		return;
	}
	bool disable = false;
	for ( int x = 0; x < WEAPONINFO_COUNT; x++ )
	{
        CCSWeaponInfo *info = (CCSWeaponInfo *)g_GetFileWeaponInfoFromHandle(x);

		if ( !info || !info->szClassName || !strlen( info->szClassName ) || Q_stristr( info->szClassName, "hegrenade" ) )
		{
			continue;
		}
		if ( !bEnable )
		{
			disable = true;
			if ( info->iMaxClip1 == 1000000 )
			{
				info->iDefaultClip1 = g_DefaultClip[x];
				info->iMaxClip1 = g_MaxClip[x];
			}
		}
		else
		{
			if ( info->iMaxClip1 != 1000000 )
			{
				g_DefaultClip[x] = info->iDefaultClip1;
				g_MaxClip[x] = info->iMaxClip1;
				info->iDefaultClip1 = 1000000;
				info->iMaxClip1 = 1000000;
			}
		}
	}
	if ( disable )
	{
		META_LOG( g_PLAPI, "Unlimited-Ammo disabled." );
	}
	else 
	{
		META_LOG( g_PLAPI, "Unlimited-Ammo enabled." );
	}
}

void ZombiePlugin::RemoveHead( CBasePlayer *pPlayer, int iIndex )
{
	if ( iIndex > 0 && iIndex <= MAX_PLAYERS )
	{
		if ( g_Players[iIndex].isZombie && !g_Players[iIndex].bHeadCameOff )
		{
			int x = 0;
			for ( x = 0; x < g_ZombieModels.Count(); x++ )
			{
				if ( g_Players[iIndex].iModel == g_ZombieModels[x].iPrecache )
				{
					if ( g_ZombieModels[x].bHasHead )
					{
						g_Players[iIndex].bHeadCameOff = true;
						UTIL_SetModel( g_Players[iIndex].pPlayer, g_ZombieModels[x].sHeadLess.c_str() );
						MRecipientFilter mFilter;
						mFilter.AddAllPlayers( MAX_CLIENTS );
						Vector vLoc = g_Players[iIndex].vLastHit;
						QAngle qAng;
						qAng.Random( -90, 90 );
						Vector vDir;
						vDir = ( g_Players[iIndex].vDirection * 500 );
						m_TempEnts->PhysicsProp( mFilter, 0.0, g_ZombieModels[x].iHeadPrecache, 0, vLoc, qAng, vDir, 0, 0 );
						for ( x = 0; x < 3; x++ )
						{
							UTIL_BloodSpray( vLoc, Vector( 0, 0, 300 ), BLOOD_COLOR_RED, 10, FX_BLOODSPRAY_DROPS | FX_BLOODSPRAY_GORE );
						}
						
					}
					break;
				}
			}
		}
	}
}

void ZombiePlugin::Event_Killed( const CTakeDamageInfo &info )
{
	CBasePlayer *pPlayer = META_IFACEPTR( CBasePlayer );
	int iPlayer;
	if ( zombie_enabled.GetBool() && zombie_headshots.GetBool() && !IsValidPlayer( pPlayer, &iPlayer ) && g_Players[iPlayer].isHeadshot )
	{
		RemoveHead( pPlayer, iPlayer );
	}
	RETURN_META( MRES_IGNORED );
}

bool ZombiePlugin::FireEvent( IGameEvent *pEvent, bool bDontBroadcast )
{	
	if ( META_RESULT_STATUS == MRES_SUPERCEDE )
	{
		RETURN_META_VALUE( MRES_IGNORED, false );
	}
	if ( !pEvent )
	{
		RETURN_META_VALUE( MRES_IGNORED, false );
	}

	const char *sName = pEvent->GetName();

	if ( sName )
	{
		if (  zombie_teams.GetBool() && FStrEq( sName, "player_team" ) )
		{
			RETURN_META_VALUE_NEWPARAMS( MRES_SUPERCEDE, false, &IGameEventManager2::FireEvent, ( pEvent, true ) );
		}
	}
	
	RETURN_META_VALUE( MRES_IGNORED, true );
}

bool ZombiePlugin::LevelInit_Pre(const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background)
{	
	
	if ( zombie_enabled.GetBool() )
	{
		m_Engine->ServerCommand( "exec zombiemod/zombiemod.cfg\n" );
		if ( zombie_fog.GetBool() && pMapName != NULL )
		{
			if ( zombie_fog_sky.GetBool() )
			{
				char sTmp[100];
				const char *sArr[] = { "dn", "ft", "lf", "rt", "up", "bk"  };
				int x;
				for ( x = 0; x < 6; x++ )
				{
					Q_snprintf( sTmp, sizeof(sTmp), "materials/skybox/%s%s.vmt", zombie_fog_sky_material.GetString(), sArr[x] );
					AddDownload( sTmp );
				}
			}
			const char *ents = ParseAndFilter( g_CurrentMap, pMapEntities );
			RETURN_META_VALUE_NEWPARAMS( MRES_IGNORED, true, &IServerGameDLL::LevelInit, ( pMapName, ents, pOldLevel, pLandmarkName, loadGame, background) );
		}
	}
	if ( pMapName != NULL )
	{
		RETURN_META_VALUE( MRES_IGNORED, true );
	}
	return true;
}

bool ZombiePlugin::LevelInit(const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background)
{
	Q_strncpy( g_CurrentMap, STRING( m_PlayerInfoManager->GetGlobalVars()->mapname ) , sizeof(g_CurrentMap) );
	char sTmp[100];
	char sTmp1[100];
	Q_snprintf( sTmp, sizeof(sTmp), "cfg/zombiemod/maps/%s.cfg", g_CurrentMap );
	Q_snprintf( sTmp1, sizeof(sTmp1), "cfg/zombiemod/%s.cfg", g_CurrentMap );
	if ( m_FileSystem->FileExists( sTmp ) )
	{
		Q_snprintf( sTmp, sizeof(sTmp), "exec zombiemod/maps/%s.cfg\n", g_CurrentMap );
		m_Engine->ServerCommand( sTmp );
	}
	else if ( m_FileSystem->FileExists( sTmp1 ) )
	{
		Q_snprintf( sTmp1, sizeof(sTmp1), "exec zombiemod/%s.cfg\n", g_CurrentMap );
		m_Engine->ServerCommand( sTmp1 );
	}

	g_Offsets.m_iDeaths = 0;
	bFirstRound = true;
	int x;
	for ( x = 0; x < CSW_MAX; x++)
	{
		g_RestrictT[x] = -1;
		g_RestrictCT[x] = -1;
		g_RestrictW[x] = -1;
	}
	for ( x = 1; x <= MAX_PLAYERS; x++ )
	{
		g_Players[x].isZombie = false;
		g_Players[x].isBot = false;
		g_Players[x].isHooked = false;
		g_Players[x].pPlayer = NULL;
		g_Players[x].bHeadCameOff = false;
		g_Players[x].bJetPack = false;
		g_Players[x].iHeadShots = 0;
		g_Players[x].iJetPack = 0;
		g_Players[x].iModel = 0;
		g_Players[x].iUserID = -1;
		g_Players[x].pKnife = NULL;
		g_Players[x].vDirection.Init(0,0,0);
		g_Players[x].vLastHit.Init(0,0,0);
		g_Players[x].bConnected = false;
	}
	if ( g_MaxClip[0] == -1 && g_DefaultClip[0] == -1 )
	{
		for ( int x = 0; x < WEAPONINFO_COUNT; x++ )
		{
			CCSWeaponInfo *info = (CCSWeaponInfo *)g_GetFileWeaponInfoFromHandle(x);
			if ( !info || !info->szClassName || !strlen( info->szClassName ) || Q_stristr( info->szClassName, "hegrenade" ) )
			{
				continue;
			}
			g_DefaultClip[x] = info->iDefaultClip1;
			g_MaxClip[x] = info->iMaxClip1;
		}
	}
	#define SHOWVAR( var ) META_CONPRINTF("[ZOMBIE] %d\n", var);

	g_Offsets.m_bNightVisionOn = UTIL_FindOffset( "CCSPlayer", "m_bNightVisionOn" );
	g_Offsets.m_bHasNightVision = UTIL_FindOffset( "CCSPlayer", "m_bHasNightVision" );
	g_Offsets.m_iAccount = UTIL_FindOffset( "CCSPlayer", "m_iAccount" );
	g_Offsets.m_fFlags = UTIL_FindOffset( "CBasePlayer", "m_fFlags" );
	g_Offsets.m_iHealth = UTIL_FindOffset( "CBasePlayer", "m_iHealth" );
	g_Offsets.m_iTeamNum = UTIL_FindOffset( "CBaseEntity", "m_iTeamNum" );
	g_Offsets.m_iTeamTeamNum = UTIL_FindOffset( "CTeam", "m_iTeamNum" );
	g_Offsets.m_iScore = UTIL_FindOffset( "CTeam", "m_iScore" );
	g_Offsets.m_lifeState = UTIL_FindOffset( "CBasePlayer", "m_lifeState" );
	g_Offsets.m_Local = UTIL_FindOffsetTable( "CBasePlayer", "localdata", "m_Local" );
	g_Offsets.m_iFOV = UTIL_FindOffset( "CBasePlayer", "m_iFOV" );
	g_Offsets.m_bDrawViewmodel = UTIL_FindOffsetTable( "CBasePlayer", "localdata", "m_Local", "m_bDrawViewmodel" );
	g_Offsets.m_iHideHUD = UTIL_FindOffsetTable( "CBasePlayer", "localdata", "m_Local", "m_iHideHUD" );
	g_Offsets.m_vecOrigin = UTIL_FindOffset( "CBaseEntity", "m_vecOrigin" );
	g_Offsets.m_MoveType = UTIL_FindOffset( "CBaseEntity", "movetype" );
	g_Offsets.m_Collision = UTIL_FindOffset( "CBaseEntity", "m_Collision" );
	g_Offsets.m_clrRender = UTIL_FindOffset( "CBaseEntity", "m_clrRender" );
	g_Offsets.m_nRenderMode = UTIL_FindOffset( "CBaseEntity", "m_nRenderMode" );
	g_Offsets.m_hRagdoll = UTIL_FindOffset( "CCSPlayer", "m_hRagdoll" );
	g_Offsets.m_bInBuyZone = UTIL_FindOffset( "CCSPlayer", "m_bInBuyZone" );
	g_Offsets.m_vecBaseVelocity = UTIL_FindOffsetTable( "CBasePlayer", "localdata", "m_vecBaseVelocity" );
	g_Offsets.m_flMaxspeed = UTIL_FindOffset( "CBasePlayer", "m_flMaxspeed" );
	g_Offsets.m_hActiveWeapon = UTIL_FindOffset( "CBaseCombatCharacter", "m_hActiveWeapon" );

	for ( x = 0; x < m_Engine->GetEntityCount(); x++ )
	{
		edict_t *pTmp = m_Engine->PEntityOfEntIndex( x );
		if ( pTmp && !pTmp->IsFree() /*&& pTmp->GetClassName()*/ )
		{
			if ( FStrEq( pTmp->GetClassName(), "cs_team_manager" ) )
			{
				CBaseEntity *pBase = m_GameEnts->EdictToBaseEntity( pTmp ); //->GetUnknown()->GetBaseEntity();
				CTeam *tmpTeam = (CTeam *)pBase;
				int team_number;
				if ( UTIL_GetProperty( g_Offsets.m_iTeamTeamNum, pTmp, &team_number ) )
				{
					if ( team_number == TERRORISTS )
					{
						teamT = tmpTeam;
					}
					if ( team_number == COUNTERTERRORISTS )
					{
						teamCT = tmpTeam;
					}
				}
			}
		}
	}
	if ( teamCT )
	{
		TeamCount = UTIL_FindOffsetArray( "CTeam", "\"player_array\"", teamCT );
	}

	LoadZombieEvents();

	ibTimer = 0;
	bRoundStarted = false;

	if ( pMapName != NULL )
	{
		FindCVars();
	}
	 
	if ( g_Timers )
	{
		g_Timers->Reset();
		if ( zombie_enabled.GetBool() )
		{
			g_Timers->AddTimer(1.0, ZombieLevelInit);
			if ( !g_Timers->IsTimer( ZOMBIECHECK_TIMER_ID ) )
			{
				g_Timers->AddTimer(0.5, CheckZombies, NULL, 0, ZOMBIECHECK_TIMER_ID);
			}
			g_ZombieRoundOver = true;

		}

		#ifdef JC_BOT
			bBotAdded = false;
			g_Timers->AddTimer( 10.0, Timed_Bot );
		#endif
	}
	
	bLanguagesLoaded = LoadLanguages();

	g_bBuyMenu = LoadBuyMenus();

	m_Engine->ServerCommand( "sv_maxspeed 1000\n" );

	INetworkStringTable *pInfoPanel = m_NetworkStringTable->FindTable("InfoPanel");
	if (pInfoPanel)
	{
		bool save = m_Engine->LockNetworkStringTables(false);
		DisplayHelp( pInfoPanel );
		DisplayClassHelp( pInfoPanel );
		m_Engine->LockNetworkStringTables(save);
	}

	if ( pMapName != NULL )
	{
		RETURN_META_VALUE( MRES_IGNORED, true );
	}
	return true;
}

int Str_Chr( const char *sString, char sFind, int iLen, int iStart )
{
	int x = 0;
	for ( x = iStart; x < iLen; x++ )
	{
		if ( sString[x] == sFind )
		{
			return x;
		}
	}
	return -1;
}

int Str_rChr( const char *sString, char sFind, int iStart )
{
	int x = 0;
	for ( x = iStart; x >= 0; x-- )
	{
		if ( sString[x] == sFind )
		{
			return x;
		}
	}
	return -1;
}

myString Sub_Str( const char *sString, int iStart, int iLen )
{
	myString sTmp;
	int x = 0;
	int iMax = Q_strlen( sString );
	if ( iStart + iLen >= iMax )
	{
		iLen = iMax - iStart;
	}
	sTmp.Grow( iLen + 2 );
	for ( x = iStart; x < iStart + iLen; x++ )
	{
		sTmp.append( sString[x] );
	}
	return sTmp;
}
myString WorkOnCamera( const char *sCamera )
{
	myString sTmp;
	myString sFinal;
	sTmp.assign( "\"sky_camera\"\n" );
	sTmp.append( sCamera );
	sTmp.append( "\n" );
	KeyValues *kv = new KeyValues( "" );
	kv->LoadFromBuffer( "sky_camera", sTmp.c_str() );
	KeyValues *p = kv;
	sFinal.assign( "{\n" );
	bool bFog = false;
	bool bFogEnd = false;
	bool bFogStart = false;
	bool bFogDir = false;
	bool bFogC = false;
	bool bFogC2 = false;
	char buf[128] = "";
	for ( p = kv->GetFirstSubKey(); p; p = p->GetNextKey() )
	{
		if( FStrEq( p->GetName(), "origin" ) )
		{
			Q_snprintf( buf, sizeof(buf), "\"%s\" \"%s\"\n", p->GetName(), p->GetString() );
			sFinal.append( buf );
		}
		else if( FStrEq( p->GetName(), "angles" ) )
		{
			Q_snprintf( buf, sizeof(buf), "\"%s\" \"%s\"\n", p->GetName(), p->GetString() );
			sFinal.append( buf );
		}
		else if( FStrEq( p->GetName(), "fogenable" ) )
		{
			bFog = true;
			Q_snprintf( buf, sizeof(buf), "\"%s\" \"%s\"\n", p->GetName(), "1" );
			sFinal.append( buf );
		}
		else if( FStrEq( p->GetName(), "fogend" ) )
		{
			bFogEnd = true;
			Q_snprintf( buf, sizeof(buf), "\"%s\" \"%f\"\n", p->GetName(), zombie_fog_end.GetFloat() );
			sFinal.append( buf );
		}
		else if( FStrEq( p->GetName(), "fogstart" ) )
		{
			bFogStart = true;
			Q_snprintf( buf, sizeof(buf), "\"%s\" \"%f\"\n", p->GetName(), zombie_fog_start.GetFloat() );
			sFinal.append( buf );
		}
		else if( FStrEq( p->GetName(), "fogdir" ) )
		{
			bFogDir = true;
			Q_snprintf( buf, sizeof(buf), "\"%s\" \"%s\"\n", p->GetName(), p->GetString() );
			sFinal.append( buf );
		}
		else if( FStrEq( p->GetName(), "fogcolor2" ) )
		{
			bFogC2 = true;
			Q_snprintf( buf, sizeof(buf), "\"%s\" \"%s\"\n", p->GetName(), zombie_fog_colour2.GetString() );
			sFinal.append( buf );
		}
		else if( FStrEq( p->GetName(), "fogcolor" ) )
		{
			bFogC = true;
			Q_snprintf( buf, sizeof(buf), "\"%s\" \"%s\"\n", p->GetName(), zombie_fog_colour.GetString() );
			sFinal.append( buf );
		}
		else if( FStrEq( p->GetName(), "scale" ) )
		{
			Q_snprintf( buf, sizeof(buf), "\"%s\" \"%s\"\n", p->GetName(), p->GetString() );
			sFinal.append( buf );
		}
		else if( FStrEq( p->GetName(), "classname" ) )
		{
			Q_snprintf( buf, sizeof(buf), "\"%s\" \"%s\"\n", p->GetName(), "sky_camera" );
			sFinal.append( buf );
		}
		Q_strcpy( buf, "" );
	}

	if( !bFog )
	{
		Q_snprintf( buf, sizeof(buf), "\"fogenable\" \"%s\"\n", "1" );
		sFinal.append( buf );
		Q_strcpy( buf, "" );
	}
	if( !bFogEnd )
	{
		Q_snprintf( buf, sizeof(buf), "\"fogend\" \"%f\"\n", zombie_fog_end.GetFloat() );
		sFinal.append( buf );
		Q_strcpy( buf, "" );
	}
	if( !bFogStart )
	{
		Q_snprintf( buf, sizeof(buf), "\"fogstart\" \"%f\"\n", zombie_fog_start.GetFloat() );
		sFinal.append( buf );
		Q_strcpy( buf, "" );
	}
	if( !bFogDir )
	{
		Q_snprintf( buf, sizeof(buf), "\"fogdir\" \"%s\"\n", "1 0 0" );
		sFinal.append( buf );
		Q_strcpy( buf, "" );
	}
	if( !bFogC2 )
	{
		Q_snprintf( buf, sizeof(buf), "\"fogcolor2\" \"%s\"\n", zombie_fog_colour2.GetString() );
		sFinal.append( buf );
		Q_strcpy( buf, "" );
	}
	if( !bFogC )
	{
		Q_snprintf( buf, sizeof(buf), "\"fogcolor\" \"%s\"\n", zombie_fog_colour.GetString() );
		sFinal.append( buf );
		Q_strcpy( buf, "" );
	}
	sFinal.append( "}" );
	return sFinal;
}

myString ExtractLocationFromEnt( const char *sEntString, int iLen, const char *sEntity )
{
	char *Pos = Q_strstr( sEntString, sEntity );
	if ( Pos )
	{
		char sTmp[1024] = "";
		int iStart = Str_rChr( sEntString, '{',  Pos - sEntString );
		int iEnd = Str_Chr( sEntString, '}', iLen, Pos - sEntString );

		Q_snprintf( sTmp, sizeof(sTmp), "\"%s\"\n%s", sEntity, Sub_Str( sEntString, iStart, iEnd - iStart + 1 ).c_str() );

		KeyValues *kv = new KeyValues( "" );

		kv->LoadFromBuffer( sEntity, sTmp );
		return myString( kv->GetString( "origin" ) );
	}
	return myString("");
}

const char *ZombiePlugin::ParseAndFilter( const char *map, const char *ents )
{

	const char *sFog = 
						"{\n" \
							"\"origin\" \"168 584 136\"\n" \
							"\"angles\" \"0 0 0\"\n" \
							"\"farz\" \"20000\"\n" \
							"\"fogend\" \"%f\"\n" \
							"\"fogstart\" \"%f\"\n" \
							"\"fogdir\" \"1 0 0\"\n" \
							"\"fogcolor\" \"%s\"\n" \
							"\"fogcolor2\" \"%s\"\n" \
							"\"fogblend\" \"%d\"\n" \
							"\"fogenable\" \"1\"\n" \
							"\"maxdxlevel\" \"0\"\n" \
							"\"mindxlevel\" \"0\"\n" \
							"\"classname\" \"env_fog_controller\"\n" \
						"}\n";

	g_MapEntities.clear();
	char sFogString[1024] = "";
	char sSkyCam[1024] = "";
	Q_snprintf( sFogString, sizeof(sFogString), sFog, zombie_fog_end.GetFloat(), zombie_fog_start.GetFloat(), zombie_fog_colour.GetString(), zombie_fog_colour2.GetString(), zombie_fog_blend.GetInt() );
	int iSky[2];
	iSky[0] = 0;
	iSky[1] = 0;
	int iFog[4];
	iFog[0] = 0;
	iFog[1] = 0;
	iFog[2] = 0;
	iFog[3] = 0;
	int iLen = Q_strlen( ents );
	char *iPos = NULL;
	if ( zombie_fog_sky.GetBool() )
	{
		iPos = Q_strstr( ents, "\"skyname\"" );
		if ( iPos )
		{
			int iStart = (int)(iPos - ents);
			int iEnd = Str_Chr( ents, '"', iLen, ( iPos - ents + 12 ) );
			iSky[0] = iStart;
			iSky[1] = iEnd;
		}
		else
		{
			return ents;
		}
	}
	iPos = Q_strstr( ents, "env_fog_controller" );
	if ( iPos )
	{
		int iStart = Str_rChr( ents, '{',  iPos - ents );
		int iEnd = Str_Chr( ents, '}', iLen, iPos - ents );
		iFog[0] = iStart;
		iFog[1] = iEnd;
	} 
	iPos = Q_strstr( ents, "\"sky_camera\"" );
	if ( iPos )
	{
		int iStart = Str_rChr( ents, '{',  iPos - ents );
		int iEnd = Str_Chr( ents, '}', iLen, iPos - ents );
		Q_strcpy( sSkyCam, Sub_Str( ents, iStart, iEnd - iStart + 1 ).c_str() );
		if ( iStart < iFog[0] )
		{
			iFog[2] = iFog[0];
			iFog[3] = iFog[1];
			iFog[0] = iStart;
			iFog[1] = iEnd;
		}
		else if ( iFog[0] > 0 )
		{
			iFog[2] = iStart;
			iFog[3] = iEnd;
		}
		else 
		{
			iFog[0] = iStart;
			iFog[1] = iEnd;
		}
	}

	if ( iSky[0] > 0 )
	{
		g_MapEntities.assign( Sub_Str( ents, 0, iSky[0] + 11 ).c_str() );
		g_MapEntities.append( zombie_fog_sky_material.GetString() );
		g_MapEntities.append( "\"" );
	}
	else
	{
		iSky[1] = -1;
	}
	if ( iFog[2] > 0 && iFog[0] > 0 )
	{
		g_MapEntities.append( Sub_Str( ents, iSky[1] + 1, iFog[0] - iSky[1] - 1 ).c_str() );
		g_MapEntities.append( Sub_Str( ents, iFog[1] + 1, iFog[2] - iFog[1] - 1 ).c_str() );
		g_MapEntities.append( Sub_Str( ents, iFog[3] + 1, iLen - iFog[3] - 1 ).c_str() );
	}
	else if ( iFog[0] > 0 )
	{
		g_MapEntities.append( Sub_Str( ents, iSky[1] + 1, iFog[0] - iSky[1] - 1 ).c_str() );
		g_MapEntities.append( Sub_Str( ents, iFog[1] + 1, iLen - iFog[1] - 1 ).c_str() );
	}
	else
	{
		g_MapEntities.append( Sub_Str( ents, iSky[1] + 1, iLen - iSky[1] - 1 ).c_str() );
	}
	g_MapEntities.append( sFogString );
	if ( Q_strlen( sSkyCam ) > 0 && sSkyCam[0] != '\0' )
		g_MapEntities.append( WorkOnCamera( sSkyCam ).c_str() );
	
	return g_MapEntities.c_str();
}

void ZombiePlugin::PrecacheSounds()
{
	char buf[128];
	for (int x = 0; x < NUM_ZOMBIE_SOUNDS; x++)
	{
		Q_snprintf(buf, sizeof(buf), "npc/zombie/%s.wav", g_sZombieSounds[x]);
		m_EngineSound->PrecacheSound(buf, true);
	}
	for (int x = 1; x <= 6; x++)
	{
		Q_snprintf(buf, sizeof(buf), "npc/zombie/zombie_pain%d.wav", x);
		m_EngineSound->PrecacheSound(buf, true);
	}
	for (int x = 1; x <= 3; x++) 
	{
		Q_snprintf(buf, sizeof(buf), "npc/zombie/zombie_die%d.wav", x);
		m_EngineSound->PrecacheSound(buf, true);
	}
	
	if( zombie_undead_sound_enabled.GetBool() )
	{
		m_EngineSound->PrecacheSound( zombie_undead_sound.GetString(), true );
		Q_snprintf( buf, sizeof(buf), "sound/%s", zombie_undead_sound.GetString() );
		AddDownload( buf );
	}
	m_EngineSound->PrecacheSound( JetpackSound, true );
	m_EngineSound->PrecacheSound("npc/fast_zombie/fz_scream1.wav", true);

/*
	char sTmp[1024];
	char sOut[1024];
	static CUtlVector<char*> sOutStrings;
	for ( int x = 0; x <= 1; x++ )
	{
		const char *sSounds = ( x == 0 ? zombie_humans_win_sounds.GetString() : zombie_zombies_win_sounds.GetString() );
		if ( Q_strlen( sSounds ) > 0 )
		{
			Q_SplitString( sSounds, ",", sOutStrings );
			for ( int i = 0; i < sOutStrings.Count(); i++ )
			{
				if ( Q_strlen( sOutStrings[i] ) > 0 )
				{
					sOut[0] = '\0';
					if ( sOutStrings[i][0] == ' ' )
					{
						Q_StrRight( sOutStrings[i], Q_strlen( sOutStrings[i] ) - 1, sOut, 100 );
					}
					if ( sOutStrings[i][Q_strlen( sOutStrings[i] ) - 1] == ' ' )
					{
						Q_strncpy( sOut, (const char *)sOutStrings[i], Q_strlen( sOutStrings[i] ) );
					}
					if ( Q_strlen( sOut ) == 0 )
					{
						Q_strncpy( sOut, (const char *)sOutStrings[i], sizeof( sOut ) );
					}

					g_SMAPI->Format( sTmp, sizeof( sTmp ), "sound/radio/zombiemod/%s.wav", sOut );

					if ( !m_FileSystem->FileExists( sTmp, "MOD" ) )
					{
						META_LOG( g_PLAPI, " ***** ERRROR: Could not find '%s' sound file specified in cvar %s [%s].", sTmp, ( x == 0 ? "zombie_humans_win_sounds" : "zombie_zombies_win_sounds" ), sSounds );
					}
					else
					{
						g_SMAPI->Format( sTmp, sizeof( sTmp ), "radio/zombiemod/%s.wav", sOut );
						m_EngineSound->PrecacheSound( sTmp );
					}

					delete [] sOutStrings[i];
				}
			}

			sOutStrings.RemoveAll();
		}
	}
*/
}

void ZombiePlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	char buf[128];

	PrecacheSounds();

	if ( Q_strlen( zombie_vision_material.GetString() ) != 0 )
	{
		Q_snprintf( buf, sizeof(buf), "vgui/hud/zombiemod/%s_dx6", zombie_vision_material.GetString() );
		m_Engine->PrecacheDecal( buf, true );
		Q_snprintf( buf, sizeof(buf), "vgui/hud/zombiemod/%s", zombie_vision_material.GetString() );
		m_Engine->PrecacheDecal( buf, true );
	}

	g_bOverlayEnabled = false;
	if ( Q_strlen( zombie_end_round_overlay.GetString() ) != 0 )
	{
		g_bOverlayEnabled = true;
		Q_snprintf( buf, sizeof(buf), "vgui/hud/zombiemod/%s_humans", zombie_end_round_overlay.GetString() );
		m_Engine->PrecacheDecal( buf, true );
		Q_snprintf( buf, sizeof(buf), "vgui/hud/zombiemod/%s_zombies", zombie_end_round_overlay.GetString() );
		m_Engine->PrecacheDecal( buf, true );
	}

	if ( strlen( zombie_model_file.GetString() ) && !LoadZombieModelList( zombie_model_file.GetString(), zombie_download_file.GetString() ) )
	{
		META_LOG( g_PLAPI, "Unable to load zombie model list." );
	}

	g_bZombieClasses  =  ( zombie_classes.GetBool() &&  LoadZombieClasses() );

	RETURN_META(MRES_IGNORED);
}

void ZombiePlugin::GameFrame(bool simulating)
{ 
	if ( zombie_enabled.GetBool() )
	{
		if ( iJumpers > -1 )
		{
			for ( int x = iJumpers; x > -1; x-- )
			{
				Vector vVelocity;
				vVelocity.z = iJumper[x];
				vVelocity.x = 0.0;
				vVelocity.y = 0.0;
				CBasePlayer_ApplyAbsVelocityImpulse( (CBasePlayer *)pJumper[x], vVelocity );

				pJumper[x] = NULL;
				iJumpers--;
			}
			iJumpers = -1;
		}

		if ( g_Timers )
		{
			g_Timers->CheckTimers(simulating);
		}
	}
	RETURN_META(MRES_IGNORED);
}

bool ZombiePlugin::BuyTimeExceeded()
{
	if ( g_Timers )
	{
		float fTime = g_Timers->TheTime();
		if ( bRoundStarted && (g_fBuyTimeLimit > fTime) )
		{
			return true;
		}
	}
	return false;
}


void ZombiePlugin::LevelShutdown( void )
{
	int x = 0;
	for ( x = 1; x <= MAX_CLIENTS; x++ )
	{
		edict_t *pEntity = m_Engine->PEntityOfEntIndex( x );
		if ( pEntity && !pEntity->IsFree() && m_Engine->GetPlayerUserId( pEntity ) != -1 )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)m_GameEnts->EdictToBaseEntity( pEntity );
			if ( pPlayer )
			{
				g_Players[x].pPlayer = pPlayer;
				UnHookPlayer( x );
			}
		}
		g_Players[x].pPlayer = NULL;
		g_Players[x].isHooked = false;
		g_Players[x].isBot = false;
	}

	if ( zombie_save_classlist.GetBool() )
	{
		if ( kvPlayerClasses && kvPlayerClasses->GetFirstSubKey() )
		{
			kvPlayerClasses->SaveToFile( m_FileSystem, "cfg/zombiemod/player_classes.kv", "MOD" );
		}
	}
	if ( zombie_save_visionlist.GetBool() )
	{
		if ( kvPlayerVision && kvPlayerVision->GetFirstSubKey() )
		{
			kvPlayerVision->SaveToFile( m_FileSystem, "cfg/zombiemod/player_vision.kv", "MOD" );
		}
	}

	g_MapEntities.clear();

	g_Timers->Reset();

	UnLoadZombieEvents();

	RETURN_META(MRES_IGNORED);
}

void ZombiePlugin::ZombificationEvent( int iUserId, int iClass, int iAttacker)
{
	IGameEvent *pEvent = m_GameEventManager->CreateEvent( "zombification", true );
	if ( pEvent )
	{
		pEvent->SetInt( "userid", iUserId );
		pEvent->SetInt( "attacker", iAttacker );
		pEvent->SetInt( "class", iClass );
		SH_CALL( m_GameEventManager, &IGameEventManager2::FireEvent)( pEvent, false );
	}
}

int ZombiePlugin::OnTakeDamage( CTakeDamageInfoHack &info )
{
	if ( !zombie_enabled.GetBool() )
	{
		RETURN_META_VALUE( MRES_IGNORED, 0 );
	}
	CBasePlayer *pAttacker = NULL;
	CBaseEntity *pInflictor = NULL;
	bool bIsGrenade = false;
	float fOriginalDamage = 0;

	CBaseEntity *pVictim =  META_IFACEPTR( CBaseEntity );

	if ( pVictim )
	{
		edict_t *pEdict = m_GameEnts->BaseEntityToEdict( pVictim );

		if ( !pEdict || pEdict->IsFree() )
		{
			RETURN_META_VALUE( MRES_IGNORED, 0 );
		}

		edict_t *edict = NULL;
		edict_t *pAEdict = NULL;
		int iWeapIndex = -1;

		int nIndex = m_Engine->IndexOfEdict( pEdict );
		int AIndex = -1;
		const char *sWeapon = NULL;

		if ( info.GetAttacker() > 0 ) //info.GetAttacker() ) 
		{
			pAEdict = m_Engine->PEntityOfEntIndex( info.GetAttacker() );
			if ( IsValidPlayer( pAEdict, &AIndex ) )
			{
				pAttacker = (CBasePlayer*)m_GameEnts->EdictToBaseEntity( pAEdict ); //pAEdict->GetUnknown()->GetBaseEntity();
			}
		}
		if ( info.GetInflictor() > 0 )
		{
			edict = m_Engine->PEntityOfEntIndex( info.GetInflictor() );
			if ( IsValidEnt( edict )  )
			{
				pInflictor = m_GameEnts->EdictToBaseEntity( edict ); //edict->GetUnknown()->GetBaseEntity();
				iWeapIndex = m_Engine->IndexOfEdict( edict );
				sWeapon = edict->GetClassName();
				if ( sWeapon && FStrEq( sWeapon, "player" ) )
				{
					if ( IsValidPlayer( pAEdict ) )
					{
						IPlayerInfo *pInfo = m_PlayerInfoManager->GetPlayerInfo( pAEdict );
						sWeapon = ( ( pInfo && pInfo->GetWeaponName() ) ? pInfo->GetWeaponName() : NULL );
					}
					else
					{
						sWeapon = NULL;
					}
				}
			}
		}

		bool isZombie = g_Players[nIndex].isZombie;
		int iHealth = 0;
		float fDamage = 0.0f;

		if ( info.GetDamageType() == DMG_FALL && isZombie )
		{
			GETCLASS( g_Players[nIndex].iClass );
			if ( zClass )
			{
				if ( !zClass->bFallDamage )
				{
					RETURN_META_VALUE( MRES_SUPERCEDE, 1 );
				}
			}
			else if ( !zombie_falldamage.GetBool() )
			{
				RETURN_META_VALUE( MRES_SUPERCEDE, 1 );
			}
		}
		bool bAdjust = false;
		if ( pAttacker )
		{
			bool attackerZombie = g_Players[AIndex].isZombie;
			if ( attackerZombie && !isZombie )
			{
				if ( g_Players[nIndex].bProtected || ( sWeapon && Q_stristr( sWeapon, "nade" ) ) )
				{
					RETURN_META_VALUE( MRES_SUPERCEDE, 1 );
				}

				int iFrags = 0;
				if ( UTIL_GetProperty( g_Offsets.m_iFrags, pAEdict, &iFrags ) )
				{
					iFrags += 1;
					UTIL_SetProperty( g_Offsets.m_iFrags, pAEdict, iFrags );
				}
				int iDeaths = 0;
				if ( UTIL_GetProperty( g_Offsets.m_iDeaths, pEdict, &iDeaths ) )
				{
					iDeaths += 1;
					UTIL_SetProperty( g_Offsets.m_iDeaths, pEdict, iDeaths );
				}
				if ( zombie_notices.GetBool() )
				{
					IGameEvent *pEvent = m_GameEventManager->CreateEvent( "player_death", true );
					if ( pEvent )
					{
						pEvent->SetString( "weapon", zombie_weapon.GetString() );
						pEvent->SetBool( "headshot", false );
						pEvent->SetInt( "userid", m_Engine->GetPlayerUserId( pEdict ) );
						pEvent->SetInt( "attacker", m_Engine->GetPlayerUserId( pAEdict ) );
						SH_CALL( m_GameEventManager, &IGameEventManager2::FireEvent)( pEvent, false );
					}
				}

				if ( zombie_money.GetInt() > 0 )
				{
					GiveMoney( pAEdict, zombie_money.GetInt() );
				}

				int iBonus = -919;
				GETCLASS( g_Players[nIndex].iClass );
				
				if ( zClass )
				{
					iBonus = zClass->iHealthBonus;
				}

				if ( iBonus == -919 && zombie_health_bonus.GetInt() > 0 )
				{
					iBonus = zombie_health_bonus.GetInt();
				}

				if ( iBonus > 0 )
				{
					UTIL_GetProperty( g_Offsets.m_iHealth, pAEdict, &iHealth );
					iHealth += iBonus;
					UTIL_SetProperty( g_Offsets.m_iHealth, pAEdict, iHealth );
				}

				MakeZombie( (CBasePlayer *)pVictim, zombie_health.GetInt(), false, AIndex );


				//TODO-
				MRecipientFilter filter;
				filter.AddAllPlayers( MAX_CLIENTS );

				m_EngineSound->EmitSound( filter, nIndex, CHAN_VOICE, "npc/fast_zombie/fz_scream1.wav", RandomFloat( 0.6, 1.0 ), 0.5, 0, RandomInt( 70, 150 ) );

				if ( zombie_damagelist.GetBool() )
				{
					void *params[2];
					params[0] = (void *)pEdict;
					params[1] = (void *)pAEdict;
					g_Timers->AddTimer( 1.0, TimerShowVictims, params, 2 );
				}

				RETURN_META_VALUE( MRES_SUPERCEDE, 1 );
			}
			else if ( ( !isZombie && !attackerZombie ) || ( isZombie && attackerZombie ) )
			{
				RETURN_META_VALUE( MRES_SUPERCEDE, 1 );
			}
			else if ( isZombie )
			{

				UTIL_GetProperty( g_Offsets.m_iHealth, pEdict, &iHealth );

				float fMultiplier = 0.0;
				fDamage = info.GetDamage();
				fOriginalDamage = fDamage;
				
				if ( pAEdict )
				{ // Damage multipler for grenades.

					if ( sWeapon && Q_stristr( sWeapon, "nade" ) )
					{
						bIsGrenade = true;
						fMultiplier = zombie_grenade_damage_multiplier.GetFloat();
						GETCLASS( g_Players[nIndex].iClass );
						if ( zClass )
						{
							fMultiplier = zClass->fGrenadeMultiplier;
						}
						
						if ( fMultiplier > 0 )
						{
							bAdjust = true;
							fDamage *= fMultiplier;
						}
				
					}
#ifdef _PAID1

					else if ( sWeapon && FStrEq( sWeapon, "weapon_knife" ) )
					{ // Damage set for knife.
						bAdjust = true;
						fDamage = 2000;
					}
#endif
					if ( bAdjust )
					{
						info.SetDamage( fDamage );
					}
				}
				else
				{
					iHealth -= fDamage;	
				}
				
				g_Players[nIndex].iHealth = iHealth;
				g_Players[AIndex].iVictimList[nIndex] = g_Players[AIndex].iVictimList[nIndex] + fDamage;




				char sound[512];
				if ( zombie_show_health.GetInt() > 0 && isZombie )
				{
					if ( zombie_show_health.GetInt() == 1 )
					{
						Q_snprintf(sound, sizeof(sound), "%d", iHealth );
						g_ZombiePlugin.HudMessage( nIndex, 66, 0.03, 0.89, 188, 112, 0, 128, 188, 112, 0, 128, 0, 0.0, 0.0, 100.0, 0.0, sound );
					}
					else if ( pEdict )
					{
						HintTextMsg( pEdict, "%s %d", GetLang("hp_left"), iHealth );
					}
				}
				if ( zombie_show_attacker_health.GetInt() > 0 && isZombie )
				{
					if ( zombie_show_attacker_health.GetInt() == 1 )
					{
						Q_snprintf(sound, sizeof(sound), "%s - %d", g_Players[nIndex].sUserName.c_str(), iHealth );
						g_ZombiePlugin.HudMessage( AIndex, 66, 0.03, 0.89, 188, 112, 0, 128, 188, 112, 0, 128, 0, 0.0, 0.0, 100.0, 0.0, sound );
					}
					else if ( pEdict )
					{
						HintTextMsg( pAEdict, "%s - %d", g_Players[nIndex].sUserName.c_str(), iHealth );
					}
				}

				MRecipientFilter filter;
				filter.AddAllPlayers( MAX_CLIENTS );//g_SMAPI->GetCGlobals()->maxClients );
				
				Q_snprintf(sound, sizeof(sound), "npc/zombie/zombie_pain%d.wav", RandomInt(1,6));
				m_EngineSound->EmitSound( filter, nIndex, CHAN_VOICE, sound, RandomFloat(0.6, 1.0), 0.5, 0, RandomInt(70, 150) );

				float fKnockback = 0.0f;
				float fKnockbackMult = 0.0f;
				fKnockback = zombie_knockback.GetFloat();
				fKnockbackMult = zombie_grenade_knockback_multiplier.GetFloat();
				GETCLASS( g_Players[nIndex].iClass );
				if ( zClass )
				{
					float fTmp = ( zClass->fKnockback / 100 ) * zombie_knockback_percent.GetInt();
					fKnockback = fTmp;

					fKnockbackMult = zClass->fGrenadeKnockback;
				}

#ifdef _ICEPICK
				if ( g_Players[nIndex].fKnockback != 0.0 )
				{
					fKnockback = g_Players[nIndex].fKnockback;
				}
#endif

				if ( ( fKnockback > 0) && ( pInflictor && pVictim->GetMoveType() == MOVETYPE_WALK ) && ( !pAttacker->IsSolidFlagSet( FSOLID_TRIGGER ) ) )
				{
					if ( bIsGrenade )
					{
						fKnockback *= fKnockbackMult;
					}
					
					Vector vInflictorWorldSpaceCenter = CBaseEntity_WorldSpaceCenter( pInflictor );
					Vector vVictimWorldSpaceCenter = CBaseEntity_WorldSpaceCenter( pVictim );
					Vector vecDir = vInflictorWorldSpaceCenter - Vector (0, 0, 10) - vVictimWorldSpaceCenter;
					VectorNormalize( vecDir );

#ifdef _PAID1
					bAdjust = (zombie_knife_knockback.GetInt() == 0);
#endif

					if ( edict && iWeapIndex > -1 )
					{
						Vector vecMaxs;
						Vector vecMins;
						GetEntPropVector( iWeapIndex, Prop_Send, "m_vecMaxs", vecMaxs);
						GetEntPropVector( iWeapIndex, Prop_Send, "m_vecMins", vecMins);
						Vector &vWorldAlignSize = AllocTempVector();
						VectorSubtract( vecMaxs, vecMins, vWorldAlignSize );
						float fBaseDamage = ( bAdjust ? fOriginalDamage : ( ( info.GetBaseDamage() != BASEDAMAGE_NOT_SPECIFIED ) ? info.GetBaseDamage() : info.GetDamage() ) );
						Vector force = vecDir * -DamageForce( vWorldAlignSize, fBaseDamage, fKnockback );
						if ( force.z > 250.0f )
						{
								force.z = 250.0f;
						}
						CBasePlayer_ApplyAbsVelocityImpulse( (CBasePlayer *)pVictim, force );
					}
				}
			}
		}
		if ( bAdjust )
		{
			RETURN_META_VALUE( MRES_HANDLED, 1 );
		}
	}
	RETURN_META_VALUE( MRES_IGNORED, 0 );
}

bool ZombiePlugin::SetClientListening(int iReceiver, int iSender, bool bListen)
{
	if ( zombie_enabled.GetBool() && zombie_talk.GetBool() )
	{
		bool return_value;
		if ( g_Players[iReceiver].isZombie == g_Players[iSender].isZombie )
		{
			return_value = true;
		}
		else
		{
			return_value = false;
		}
		return_value = SH_CALL( m_VoiceServer, &IVoiceServer::SetClientListening)( iReceiver, iSender, return_value );
		RETURN_META_VALUE( MRES_SUPERCEDE, return_value );
	}
	RETURN_META_VALUE( MRES_IGNORED, true );
}

bool ZombiePlugin::Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon )
{
	if ( !zombie_enabled.GetBool() )
	{
		RETURN_META_VALUE( MRES_IGNORED, true );
	}
	CBasePlayer *pPlayer =  META_IFACEPTR( CBasePlayer );
	if ( !CanUseWeapon( pPlayer, pWeapon, false ) )
	{
		RETURN_META_VALUE( MRES_OVERRIDE, false );
	}
	RETURN_META_VALUE( MRES_IGNORED, true );
}

bool ZombiePlugin::Weapon_CanUse( CBaseCombatWeapon *pWeapon )
{
	if ( !zombie_enabled.GetBool() )
	{
		RETURN_META_VALUE( MRES_IGNORED, true );
	}
	CBasePlayer *pPlayer =  META_IFACEPTR( CBasePlayer );
	if ( !CanUseWeapon( pPlayer, pWeapon, false ) )
	{
		RETURN_META_VALUE( MRES_OVERRIDE, false );
	}
	RETURN_META_VALUE( MRES_IGNORED, true );
}

bool ZombiePlugin::IsAlive( edict_t *pEntity )
{
	int iPlayer = 0;
	if ( ! IsValidPlayer( pEntity, &iPlayer ) )
	{
		return false;
	}
	char iLifeState;
	if ( g_Players[iPlayer].bConnected && UTIL_GetProperty( g_Offsets.m_lifeState, pEntity, &iLifeState ) )
	{
		return ( iLifeState == LIFE_ALIVE );
	}
	else
	{
		return false;
	}
}

void ZombiePlugin::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( !zombie_enabled.GetBool() )
	{
		RETURN_META( MRES_IGNORED );
	}
	int iPlayer;
	edict_t *edict = NULL;
	CBaseEntity *pPlayer = META_IFACEPTR( CBaseEntity );
	if ( IsValidPlayer( pPlayer, &iPlayer ) )
	{
		int iAttacker = -1;
		CBaseEntity *pBase =  NULL;
		if ( !g_Players[iPlayer].isBot )
		{
			//
		}
		if ( info.m_hAttacker.GetEntryIndex() > 0 ) 
		{
			edict = m_Engine->PEntityOfEntIndex( info.m_hAttacker.GetEntryIndex() );
			if ( edict ) 
			{
				if ( !IsValidPlayer( edict, &iAttacker, &pBase ) )
				{
					RETURN_META( MRES_IGNORED );
				}
			}
		}
		if ( pBase && m_GameEnts->BaseEntityToEdict( pBase ) && m_Engine->GetPlayerUserId( m_GameEnts->BaseEntityToEdict( pPlayer ) ) != -1 )
		{
			bool bAttacker = g_Players[iAttacker].isZombie;
			bool bVictim = g_Players[iPlayer].isZombie;
			bool bHeadShotsOnly = false;
			GETCLASS( g_Players[iPlayer].iClass );
			if ( zClass )
			{
				bHeadShotsOnly = zClass->bHeadShotsOnly;
			}

			if ( g_Players[iPlayer].bProtected || ( bAttacker && bVictim ) || ( !bAttacker && !bVictim ) || 
					( ( zombie_headshots_only.GetBool() || bHeadShotsOnly ) && ptr && ptr->hitgroup == 1 ) )
			{
				RETURN_META( MRES_SUPERCEDE );
			}

			g_Players[iPlayer].isHeadshot = ( ptr && ptr->hitgroup == 1 && iAttacker > 0 && iAttacker <= MAX_CLIENTS );
			g_Players[iPlayer].vLastHit = ptr->endpos;
			g_Players[iPlayer].vDirection = vecDir;
			if ( g_Players[iPlayer].isHeadshot )
			{
				int iShots = 0;
				GETCLASS( g_Players[iPlayer].iClass );
				if ( zClass )
				{
					iShots = zClass->iHeadshots;
				}
				else
				{
					iShots = zombie_headshot_count.GetInt();
				}

				g_Players[iPlayer].iHeadShots++;
				if ( g_Players[iPlayer].iHeadShots >= iShots && iShots != 0 )
				{
					RemoveHead( (CBasePlayer *)pBase, iPlayer );
				}
				if ( g_Players[iPlayer].iHeadShots > 200 )
				{
					g_Players[iPlayer].iHeadShots = 0;
				}
			}
		}
	}
	RETURN_META( MRES_IGNORED );
}

void ZombiePlugin::ClientDisconnect( edict_t *pEntity )
{
	if ( pEntity && !pEntity->IsFree() )
	{
		int iPlayer = m_Engine->IndexOfEdict( pEntity );


		if ( zombie_disconnect_protection.GetBool() && g_Players[iPlayer].isZombie && IsAlive( pEntity ) )
		{
			int iZombies = 0;
			for ( int x = 0; x <= MAX_PLAYERS; x++ )
			{
				if ( x != iPlayer )
				{
					edict_t *pCheck = NULL;
					if ( IsValidPlayer( x, &pCheck ) )
					{
						if ( g_Players[x].isZombie && IsAlive( pCheck ) )
						{
							iZombies++;
						}
					}
				}
			}
			if ( iZombies == 0 )
			{
				RandomZombie( NULL );
			}
		}
		if ( g_Players[iPlayer].bConnected )
		{
			iPlayerCount--;
		}
		if ( g_Players[iPlayer].iRegenTimer != 0 )
		{
			g_Timers->RemoveTimer( g_Players[iPlayer].iRegenTimer  );
		}
		g_Players[iPlayer].bConnected = false;
		UnHookPlayer( iPlayer );
		g_Players[iPlayer].pPlayer = NULL;
		g_Players[iPlayer].bSentWelcome = false;
		g_Players[iPlayer].isHooked = false;
		g_Players[iPlayer].isBot = false;
		g_Players[iPlayer].isChangingTeams = false;
		g_Players[iPlayer].iProtectTimer = 99;
		g_Players[iPlayer].bProtected = false;
	}
	RETURN_META(MRES_IGNORED);
}

/*
float ZombiePlugin::GetMaxSpeed(void)
{
	if ( zombie_enabled.GetBool() )
	{
		CBaseCombatWeapon *pWeapon = META_IFACEPTR( CBaseCombatWeapon );
		if ( pWeapon )
		{
			int x;
			for ( x = 1; x <= MAX_CLIENTS; x++ )
			{
				if ( g_Players[x].pKnife == pWeapon && g_Players[x].isZombie ) 
				{
					if ( g_bZombieClasses && zombie_classes.GetBool() && g_Players[x].iClass > -1 && g_Players[x].iClass <= g_iZombieClasses )
					{
						int iSpeed = ( (float)g_ZombieClasses[ g_Players[x].iClass ].fSpeed / 100 ) * (float)zombie_speed_percent.GetInt();
						RETURN_META_VALUE( MRES_SUPERCEDE, iSpeed );
					}
					else
					{
						RETURN_META_VALUE( MRES_SUPERCEDE, zombie_speed.GetFloat() );
					}
				}
			}
		}
	}
	RETURN_META_VALUE( MRES_IGNORED, 0.0 );
}
*/

bf_write *ZombiePlugin::UserMessageBegin( IRecipientFilter *filter, int msg_type )
{
	if ( msg_type == 26 )
	{
		RETURN_META_VALUE( MRES_IGNORED, NULL );
	}
	char name[128] = "";
	int sizereturn = 0;
	bool boolrtn = false;
	boolrtn =  m_ServerDll->GetUserMessageInfo( msg_type, name, 128, sizereturn ); 
	if( name )
	{
		META_LOG( g_PLAPI, "Message: %d - %s", msg_type, name );
	}

	RETURN_META_VALUE( MRES_IGNORED, NULL );
}

void ZombiePlugin::PerformSuicide( CBasePlayer *pBase, bool bExplode, bool bForce )
{
	Vector vDir = vec3_origin;
	Suicide( pBase, vDir, bExplode, bForce );
}

void ZombiePlugin::CommitSuicide( bool bExplode, bool bForce )
{
	META_LOG( g_PLAPI, "CommitSuicide..." );
	RETURN_META( CommitSuicideHook() );
}

void ZombiePlugin::ChangeRunHook( int iOffset )
{
	int x = 0;
	for ( x = 1; x <= MAX_PLAYERS; x++ )
	{
		CBaseEntity *pBase = NULL;
		if ( IsValidPlayer( x, &pBase ) )
		{
			SH_REMOVE_MANUALHOOK( GetPlayerMaxSpeed, g_Players[x].pPlayer, SH_MEMBER( this, &ZombiePlugin::GetMaxSpeed ), false);
		}
	}
	SH_MANUALHOOK_RECONFIGURE( GetPlayerMaxSpeed, iOffset, 0, 0);
	for ( x = 1; x <= MAX_PLAYERS; x++ )
	{
		CBaseEntity *pBase = NULL;
		if ( IsValidPlayer( x, &pBase ) )
		{
			SH_ADD_MANUALHOOK( GetPlayerMaxSpeed, g_Players[x].pPlayer, SH_MEMBER( this, &ZombiePlugin::GetMaxSpeed ), false);
		}
	}
	META_LOG( g_PLAPI, "Changed run hook to %d.", iOffset );
	return;
}

META_RES ZombiePlugin::CommitSuicideHook( )
{
	CBasePlayer *pPlayer = META_IFACEPTR( CBasePlayer );
	//if ( IsBadPtr( pPlayer ) || IsBadPtr( m_GameEnts->BaseEntityToEdict( pPlayer ) ) )
	if ( !IsValidPlayer( pPlayer ) )
	{
		return MRES_IGNORED;
	}
	if ( !zombie_enabled.GetBool() || (!zombie_changeteam_block.GetBool() && !zombie_suicide.GetBool() ))
	{
		return MRES_IGNORED;
	}
	void *params[1];
	params[0] = (void *)pPlayer;
	g_Timers->AddTimer( 0.1, Timed_SetModel, params, 1 );
	return MRES_SUPERCEDE;
}

void ZombiePlugin::ChangeTeam( int iTeam )
{
	if ( !zombie_enabled.GetBool() || !zombie_changeteam_block.GetBool() )
	{
		RETURN_META( MRES_IGNORED );
	}
	CBasePlayer *pPlayer = META_IFACEPTR( CBasePlayer );
	int iPlayer = 0;
	if ( !IsValidPlayer( (CBaseEntity*)pPlayer, &iPlayer ) )
		RETURN_META( MRES_IGNORED );
	int ipTeam = GetTeam( m_GameEnts->BaseEntityToEdict( pPlayer ) );
	//Hud_Print( NULL, "Team To: %d // Team From %d // %s", iTeam, ipTeam, ( g_Players[iPlayer].isChangingTeams ? "Changing Teams" : "IS NOT Changing Teams" ) );
	if ( ( ipTeam == COUNTERTERRORISTS || ipTeam == TERRORISTS ) && iTeam != SPECTATOR )
	{
		void *params[1];
		params[0] = (void *)pPlayer;
		g_Timers->AddTimer( 0.1, Timed_SetModel, params, 1 );
		RETURN_META( MRES_SUPERCEDE );
	}
	if ( g_Players[iPlayer].isChangingTeams )
	{
		g_Players[iPlayer].isChangingTeams = false;
		RETURN_META( MRES_IGNORED );
	}
	RETURN_META( MRES_IGNORED );
}
/*
void ZombiePlugin::Touch( CBaseEntity *pOther )
{
	CBaseEntity *pPlayer = META_IFACEPTR( CBaseEntity );
	if (!CanUseWeapon( pPlayer, pOther, false ) )
	{
		RETURN_META( MRES_SUPERCEDE );
	}
	RETURN_META( MRES_IGNORED );
}
*/

void ZombiePlugin::PreThink( void )
{
	/*if ( zombie_enabled.GetBool() )
	{
		int iPlayer = 0;

		CBaseEntity *pPlayer = (CBaseEntity*)META_IFACEPTR( CBasePlayer );
		edict_t *pEntity = NULL;

		if ( !IsValidPlayer( pPlayer, &iPlayer, &pEntity ) || !g_Players[iPlayer].isZombie )
		{
			RETURN_META( MRES_IGNORED );
		}

		int wtf = 0;
		if ( ! GetEntProp(iPlayer, Prop_Data, "m_nButtons", &wtf ) )
		{
			META_LOG( g_PLAPI, "Could not get prop data for m_nButtons" );
			RETURN_META(MRES_IGNORED);
		}

		float fSpeed = zombie_speed.GetFloat();

		if(wtf <= 0)//Standing still give him default for giggles
			RETURN_META(MRES_IGNORED);

		if ( g_bZombieClasses && zombie_classes.GetBool() && g_Players[iPlayer].iClass > -1 && g_Players[iPlayer].iClass <= g_iZombieClasses )
		{
			if( wtf & IN_SPEED)//The user is walking yes the name is deciving
			{
				fSpeed = ( (float)g_ZombieClasses[ g_Players[iPlayer].iClass ].fSpeed / 100 ) * (float)zombie_speed_percent.GetFloat();
			}
			else if( wtf & IN_DUCK)//The user is crouched
			{
				fSpeed = ( (float)g_ZombieClasses[ g_Players[iPlayer].iClass ].fSpeedDuck / 100 ) * (float)zombie_speed_percent.GetFloat();
			}
			else//IDC what he is doing give him value he deserves
			{
				fSpeed = ( (float)g_ZombieClasses[ g_Players[iPlayer].iClass ].fSpeedRun / 100 ) * (float)zombie_speed_percent.GetFloat();
			}
		}
		else
		{
			if( wtf & IN_SPEED)//The user is walking yes the name is deciving
			{
				fSpeed = zombie_speed.GetFloat();
			}
			else if( wtf & IN_DUCK)//The user is crouched
			{
				fSpeed = zombie_speed_crouch.GetFloat();
			}
			else//IDC what he is doing give him value he deserves
			{
				fSpeed = zombie_speed_run.GetFloat();
			}
		}
		if ( ! UTIL_SetProperty( g_Offsets.m_flMaxspeed, pEntity, fSpeed ) )
		{
			META_LOG( g_PLAPI, "Could not set max speed for client." );
		}
		RETURN_META(MRES_HANDLED);

	}
	RETURN_META( MRES_IGNORED );*/
}

void ZombiePlugin::HookPlayer( int iPlayer )
{
	if ( iPlayer > 0 && iPlayer <= MAX_PLAYERS && !g_Players[iPlayer].isHooked && !IsBadPtr( g_Players[iPlayer].pPlayer ) )
	{
		SH_ADD_MANUALHOOK( Event_Killed_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::Event_Killed ), false );
		SH_ADD_MANUALHOOK( CommitSuicide_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::CommitSuicide ), false );
		//SH_ADD_MANUALHOOK( CommitSuicideVector_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::CommitSuicideVector ), false );
		SH_ADD_MANUALHOOK( ChangeTeam_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::ChangeTeam ), false );
		SH_ADD_MANUALHOOK( Weapon_CanUse_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::Weapon_CanUse ), false );
		SH_ADD_MANUALHOOK( OnTakeDamage_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::OnTakeDamage ), false );
		SH_ADD_MANUALHOOK( TraceAttack_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::TraceAttack ), false );
		//SH_ADD_MANUALHOOK( PreThink_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::PreThink ), false );
		SH_ADD_MANUALHOOK( Weapon_CanSwitchTo_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::Weapon_CanSwitchTo ), false );
		SH_ADD_MANUALHOOK( GetPlayerMaxSpeed, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::GetMaxSpeed ), false);
	}
	g_Players[iPlayer].isHooked = true;
}

void ZombiePlugin::UnHookPlayer( int iPlayer )
{
	if ( iPlayer > 0 && iPlayer <= MAX_PLAYERS && g_Players[iPlayer].isHooked && !IsBadPtr( g_Players[iPlayer].pPlayer ) )
	{
		SH_REMOVE_MANUALHOOK( Event_Killed_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::Event_Killed ), false );
		SH_REMOVE_MANUALHOOK( CommitSuicide_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::CommitSuicide ), false );
		//SH_REMOVE_MANUALHOOK( CommitSuicideVector_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::CommitSuicideVector ), false );
		SH_REMOVE_MANUALHOOK( ChangeTeam_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::ChangeTeam ), false );
		SH_REMOVE_MANUALHOOK( Weapon_CanUse_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::Weapon_CanUse ), false );
		SH_REMOVE_MANUALHOOK( OnTakeDamage_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::OnTakeDamage ), false );
		SH_REMOVE_MANUALHOOK( TraceAttack_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::TraceAttack ), false );
		//SH_REMOVE_MANUALHOOK( PreThink_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::PreThink ), false );
		SH_REMOVE_MANUALHOOK( Weapon_CanSwitchTo_hook, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::Weapon_CanSwitchTo ), false );
		SH_REMOVE_MANUALHOOK( GetPlayerMaxSpeed, g_Players[iPlayer].pPlayer, SH_MEMBER( this, &ZombiePlugin::GetMaxSpeed ), false);
	}
	g_Players[iPlayer].isHooked = false;
}

float ZombiePlugin::GetMaxSpeed( void )
{
	if ( zombie_enabled.GetBool() )
	{
		int iPlayer = 0;

		CBaseEntity *pPlayer = (CBaseEntity*)META_IFACEPTR( CBasePlayer );
		edict_t *pEntity = NULL;

		if ( !IsValidPlayer( pPlayer, &iPlayer, &pEntity ) || !g_Players[iPlayer].isZombie )
		{
			RETURN_META_VALUE( MRES_IGNORED, 0 );
		}

		int wtf = 0;
		if ( ! GetEntProp(iPlayer, Prop_Data, "m_nButtons", &wtf ) )
		{
			META_LOG( g_PLAPI, "Could not get prop data for m_nButtons" );
			RETURN_META_VALUE( MRES_IGNORED, 0 );
		}

		float fSpeed = zombie_speed.GetFloat();

		if(wtf <= 0)//Standing still give him default for giggles
			RETURN_META_VALUE( MRES_IGNORED, 0 );
		
		GETCLASS( g_Players[iPlayer].iClass );
		if ( zClass )
		{
			if( wtf & IN_SPEED)//The user is walking yes the name is deciving
			{
				fSpeed = ( (float)zClass->fSpeed / 100 ) * (float)zombie_speed_percent.GetFloat();
			}
			else if( wtf & IN_DUCK)//The user is crouched
			{
				fSpeed = ( (float)zClass->fSpeedDuck / 100 ) * (float)zombie_speed_percent.GetFloat();
			}
			else//IDC what he is doing give him value he deserves
			{
				fSpeed = ( (float)zClass->fSpeedRun / 100 ) * (float)zombie_speed_percent.GetFloat();
			}
		}
		else
		{
			if( wtf & IN_SPEED)//The user is walking yes the name is deciving
			{
				fSpeed = zombie_speed.GetFloat();
			}
			else if( wtf & IN_DUCK)//The user is crouched
			{
				fSpeed = zombie_speed_crouch.GetFloat();
			}
			else//IDC what he is doing give him value he deserves
			{
				fSpeed = zombie_speed_run.GetFloat();
			}
		}
		if ( fSpeed > 0 )
		{
			float fExSpeed = 0;
			if ( UTIL_GetProperty( g_Offsets.m_flMaxspeed, pEntity, &fExSpeed ) )
			{
				if ( ! UTIL_SetProperty( g_Offsets.m_flMaxspeed, pEntity, fSpeed ) )
				{
					META_LOG( g_PLAPI, "Could not set max speed for client." );
				}
				else
				{
					RETURN_META_VALUE( MRES_OVERRIDE, fSpeed );
				}
			}
			else
			{
				RETURN_META_VALUE( MRES_IGNORED, 0 );
			}
		}
	}
	RETURN_META_VALUE( MRES_IGNORED, 0 );
}

void ZombiePlugin::ClientActive(edict_t *pEntity, bool bLoadGame)
{
	int iPlayer = 0;
	#ifdef JC_BOT
		if ( !bBotAdded )
		{
			g_Timers->AddTimer( 30.0, Timed_Bot );
		}
	#endif
	if ( IsValidPlayer( pEntity, &iPlayer ) )
	{
		//es_menu 20 event_var(userid) "This Server Features Killer's Zombie Tools\nTo access restricted weapons say !buymeny\nTo Spawn in once connected type !zspawn\nWhen you respawn you have 20 seconds of\n  invisiblity to run to a new hiding spot"
		g_Players[iPlayer].bConnected = true;
		iPlayerCount++;
		if ( zombie_remove_radar.GetBool() )
		{
			m_Engine->ClientCommand( pEntity, "alias drawradar \"echo %s\"", GetLang("radar_disabled") ); //Radar Disabled, please restart your client to re-activate it. Sorry for the inconvenience.
		}
		
		m_Engine->ClientCommand( pEntity, "cl_forcepreload 1");

		if ( zombie_respawn.GetBool() && ( zombie_respawn_delay.GetInt() == 0 || zombie_respawn_onconnect.GetBool() ) ) 
		{
			myString sTmp;
			sTmp.assign( "\x03[ZOMBIE]\x01 " ); //To Spawn in once connected type !zspawn or zombie_respawn in console.
			sTmp.append( GetLang("zombie_spawn") );
			if ( zombie_respawn_protection.GetInt() > 0 )
			{
				char sPrint[100];
				Q_snprintf( sPrint, sizeof(sPrint), GetLang("invis_enabled"), zombie_respawn_protection.GetInt() ); //When you respawn you have %d seconds of invisiblity to run to a new hiding spot.
				sTmp.append( ' ' );
				sTmp.append( sPrint );
			}
			Hud_Print( pEntity, sTmp.c_str() );
		}
	}
	RETURN_META( MRES_IGNORED );
}

void ZombiePlugin::ClientPutInServer(edict_t *pEntity, char const *playername)
{
	int iPlayer;
	if ( IsValidPlayer( pEntity, &iPlayer ) )
	{
		if ( iPlayer > 0 )
		{
			const char *sSteamID = m_Engine->GetPlayerNetworkIDString( pEntity );
			g_Players[iPlayer].isBot = ( sSteamID ? ( Q_stricmp( sSteamID, "BOT" ) == 0 ) : true );
			CBasePlayer *pBase = (CBasePlayer *)m_GameEnts->EdictToBaseEntity( pEntity );
			if ( pBase  )
			{
				if ( g_Offsets.m_iDeaths == 0 )
				{
					g_Offsets.m_iDeaths = UTIL_FindOffsetMap( pBase, "CBasePlayer", "m_iDeaths" );
					g_Offsets.m_iFrags = UTIL_FindOffsetMap( pBase, "CBasePlayer", "m_iFrags" );
				}
				g_Players[iPlayer].pPlayer = pBase;
				UnHookPlayer( iPlayer );
				HookPlayer( iPlayer );
				g_Players[iPlayer].isZombie = false;
				g_Players[iPlayer].isChangingTeams = false;
				g_Players[iPlayer].bSentWelcome = false;
				g_Players[iPlayer].iShownMOTD = 0;
				if ( zombie_enabled.GetBool() && !g_Players[iPlayer].isBot )
				{
					if ( zombie_remove_radar.GetBool() )
					{
						m_Engine->ClientCommand( pEntity, "hideradar" );
					}
				}
				g_Players[iPlayer].iProtectTimer = 99;
				g_Players[iPlayer].sUserName.assign( playername );
				g_Players[iPlayer].iStuckRemain = zombie_teleportcount.GetInt();
				g_Players[iPlayer].bHasSpawned = false;
				g_Players[iPlayer].bFaded = false;
				g_Players[iPlayer].iClassMenu = -1;
				g_Players[iPlayer].iBuyMenu = -1;
				g_Players[iPlayer].iMenu = 0;
				g_Players[iPlayer].iClass = g_iDefaultClass;
				g_Players[iPlayer].iChangeToClass = -1;
				g_Players[iPlayer].bShowClassMenu = false;
				g_Players[iPlayer].bChoseClass = false;
				for ( int y = 0; y <= 4; y++ )
				{
					g_Players[iPlayer].iBuyWeapons[y] = 0;
				}

				IPlayerInfo *iInfo = m_PlayerInfoManager->GetPlayerInfo( pEntity );
				if ( iInfo )
				{
					if ( zombie_classes.GetBool() )
					{
						if ( zombie_classes_random.GetInt() > 0 )
						{
							ZombieClass *zClass = m_ClassManager.RandomClass();
							if ( zClass )
							{
								SelectZombieClass( pEntity, iPlayer, zClass, true, false );
							}
							else
							{
								META_LOG( g_PLAPI, "*** ERROR: ClassManager failed to get random class!!" );
							}
						}
						else
						{
							int iClass = kvPlayerClasses->GetInt( iInfo->GetNetworkIDString(), -1 );
							GETCLASS( iClass );
							if ( zClass )
							{
								SelectZombieClass( pEntity, iPlayer, zClass, true, false );
							}
							else
							{
								g_Players[iPlayer].bShowClassMenu = true;
								GETCLASS( g_iDefaultClass );
								SelectZombieClass( pEntity, iPlayer, zClass, true, false );
							}
						}
					}
					if ( zombie_save_visionlist.GetBool() )
					{
						g_Players[iPlayer].bShowZombieVision = ( kvPlayerVision->GetInt( iInfo->GetNetworkIDString(), 1 ) == 1 );
					}
				}
			}
		} 
	}
	if ( playername != NULL )
	{
		RETURN_META( MRES_IGNORED );
	}
}

void ZombiePlugin::SetCommandClient(int index)
{
	iCurrentPlayer = index + 1;
	RETURN_META(MRES_IGNORED);
}

/*
void ZombiePlugin::ClientSettingsChanged(edict_t *pEdict)
{
	RETURN_META(MRES_IGNORED);
}
*/
bool ZombiePlugin::ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	bClientConnecting = true;
	//int client = IndexOfEdict(pEntity);
	RETURN_META_VALUE(MRES_IGNORED, true);
}

//void ZombiePlugin::ClientCommand(edict_t *pEntity)
#if defined ORANGEBOX_BUILD
	void ZombiePlugin::ClientCommand(edict_t *pEntity, const CCommand &args)
#else
	void ZombiePlugin::ClientCommand(edict_t *pEntity)
#endif
{
	const char *sCmd = args.Arg(0);//args.Arg(0);
	CBaseEntity *pBase = NULL;
	pBase = m_GameEnts->EdictToBaseEntity( pEntity );
	if ( !pEntity || !pBase || !sCmd )
	{
		RETURN_META( MRES_IGNORED );
	}
	//Hud_Print( pEntity,"Command: '%s'", sCmd );
	//META_LOG( g_PLAPI, "Command:%s", sCmd );
	/*
	if ( FStrEq( sCmd, "distme" ) ) 
	{
		int iEnt = m_Engine->IndexOfEdict( pEntity );
		int x = 1;
		float fClosest = 500;
		int iPlayer;
		Vector vecPlayer;
		if ( !UTIL_GetProperty( g_Offsets.m_vecOrigin, g_Players[iEnt].pPlayer->edict(), &vecPlayer ) )
		{
			Hud_Print( pEntity, "Could not get your location..." );
			RETURN_META( MRES_SUPERCEDE );
		}
		for ( x = 1; x < MAX_PLAYERS; x++ )
		{
			edict_t *pEnt = NULL;
			if ( IsValidPlayer( x, &pEnt ) )
			{
				Vector vecLocation( 0, 0, 0 );
				if ( UTIL_GetProperty( g_Offsets.m_vecOrigin, g_Players[x].pPlayer->edict(), &vecLocation ) )
				{
					float fTmp = CalcDistance( vecLocation, vecPlayer );
					if ( fTmp > 0 && fTmp < fClosest )
					{
						fClosest = fTmp;
						iPlayer = x;
					}
				}
			}
		}
		if ( fClosest != 500 )
		{
			Hud_Print( pEntity, "Closest player(%d) is %f away from you....", iPlayer, fClosest );
		}
		else
		{
			Hud_Print( pEntity, "Could not find anyone closer than 500.0 away from your position." );
		}
		RETURN_META( MRES_SUPERCEDE );
	}
	else if ( FStrEq( sCmd, "slapme" ) )
	{
		Slap( m_Engine->IndexOfEdict( pEntity ) );
		RETURN_META( MRES_SUPERCEDE );
	}
	else if ( FStrEq( sCmd, "doteam" ) )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pEntity->GetUnknown()->GetBaseEntity();
		if ( pPlayer )
		{
			CBasePlayer_SwitchTeam( pPlayer, ( GetTeam(pEntity) == TERRORISTS ? COUNTERTERRORISTS : TERRORISTS )  );
		}
		RETURN_META( MRES_SUPERCEDE );
	}
	else if ( FStrEq( sCmd, "show_fov" ) )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pEntity->GetUnknown()->GetBaseEntity();
		int iFOV = 0;
		if ( m_Engine->Cmd_Argc() == 2 )
		{
			iFOV = Q_atoi( args.Arg(1) );
			//UTIL_SetProperty( g_Offsets.m_iFOV + g_Offsets.m_Local, pEntity, iFOV );
			CBasePlayer_SetFOV( pPlayer, pPlayer, iFOV );
		}
		UTIL_GetProperty( g_Offsets.m_iFOV + g_Offsets.m_Local, pEntity, &iFOV );
		Hud_Print( pEntity, "FOV:%d", iFOV );
		RETURN_META( MRES_SUPERCEDE );
	}
	*/
	/*if ( FStrEq( sCmd, "slapme" ) )
	{
		Hud_Print( pEntity, "You got slapped !" );
		Slap( m_Engine->IndexOfEdict( pEntity ) );
		RETURN_META( MRES_SUPERCEDE );
	}*/
	if ( zombie_enabled.GetBool() )
	{
		int iPlayer = m_Engine->IndexOfEdict( pEntity );
		CBasePlayer *pPlayer = (CBasePlayer *)pBase;
		if ( !pPlayer )
		{
			RETURN_META( MRES_IGNORED );
		}
		if ( FStrEq( sCmd, "zombie_vision_enabled" ) )
		{
			if ( args.ArgC() == 2 )
			{
				int iDecision = atoi( args.Arg( 1 ) );		
				g_Players[iPlayer].bShowZombieVision = ( iDecision == 1 );
				if ( zombie_save_visionlist.GetBool() )
				{
					IPlayerInfo *iInfo = m_PlayerInfoManager->GetPlayerInfo( pEntity );
					if ( iInfo )
					{
						kvPlayerVision->SetInt( iInfo->GetNetworkIDString(), iDecision );
					}
				}
				Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", ( iDecision == 1 ? GetLang("enabled_vision") : GetLang("disabled_vision") ) );
			}
			else
			{
				Hud_Print( pEntity, "\x03[ZOMBIE]\x01 *** Usage: zombie_vision_enabled 1 or 0" );
			}
			RETURN_META( MRES_SUPERCEDE );
		}
		else if ( ( args.ArgC() > 1 ) && FStrEq( sCmd, "buy") )
		{
			int WeaponID = LookupBuyID(args.Arg(1));
			int nCount = 0;
			if (!AllowWeapon(pPlayer, WeaponID, nCount))
			{
				if (nCount == 0)
				{
					char sPrint[100];
					Q_snprintf( sPrint, sizeof( sPrint ), GetLang("restricted_weap"), LookupWeaponName( WeaponID ) );
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 *** %s", sPrint );
				}
				RETURN_META( MRES_SUPERCEDE );
			}
			//if ( !CCSPlayer_IsInBuyZone( pPlayer ) )
			//{
				//int iTeam;
				//iTeam = g_ZombiePlugin.GetTeam( m_GameEnts->BaseEntityToEdict( pPlayer ) );
				//CBasePlayer_SwitchTeam( pPlayer, ( iTeam == TERRORISTS ? COUNTERTERRORISTS : TERRORISTS )  );
				/*if ( !CCSPlayer_IsInBuyZone( pPlayer ) )
				{
					CBasePlayer_SwitchTeam( pPlayer, iTeam );
				}
				else
				{
					UTIL_GiveNamedItem( pPlayer, args.Arg(1), 0 );
				}*/
			//}
		}
		else if ( ( ( zombie_jetpack.GetBool() && g_Players[iPlayer].isZombie ) || ( zombie_humans_jetpack.GetBool() && !g_Players[iPlayer].isZombie ) ) && FStrEq( sCmd, "+jetpack" ) )
		{
			int iJetTime = zombie_jetpack_timer.GetInt() * 2;
			if ( IsAlive( pEntity ) )
			{
				if ( !bAllowedToJetPack )
				{
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("jetpack_wait") ); //Sorry, you cannot use the jetpack until the first zombie has been chosen.
					RETURN_META( MRES_SUPERCEDE );
				}
				if ( iJetTime != 0 && g_Players[iPlayer].iJetPack >= iJetTime )
				{
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("jetpack_expired") );
					RETURN_META(MRES_SUPERCEDE);
				}
				g_Players[iPlayer].bJetPack = true;
				UTIL_SetProperty( g_Offsets.m_MoveType, pEntity, MOVETYPE_FLY );
				
				//Vector pos = pEntity->GetCollideable()->GetCollisionOrigin();
				CBaseEntity *pBase = m_GameEnts->EdictToBaseEntity( pEntity );
				ICollideable *cCol = CBaseEntity_GetCollideable( pBase );
				Vector pos = vec3_origin;
				if ( cCol )
				{
					pos = cCol->GetCollisionOrigin();
				}

				MRecipientFilter mrf;
				mrf.AddAllPlayers( MAX_CLIENTS );//g_SMAPI->GetCGlobals()->maxClients );
				m_EngineSound->EmitSound((IRecipientFilter &)mrf, m_Engine->IndexOfEdict(pEntity), CHAN_AUTO, JetpackSound, 0.7, ATTN_NORM, 0, PITCH_NORM, &pos, 0, 0, true, 0, m_Engine->IndexOfEdict(pEntity));
			}
			RETURN_META(MRES_SUPERCEDE);
		}
		else if ( ( zombie_jetpack.GetBool() || zombie_humans_jetpack.GetBool() ) && g_Players[iPlayer].bJetPack && FStrEq( sCmd, "-jetpack" ) )
		{
			if ( g_Players[iPlayer].bJetPack )
			{
				g_Players[iPlayer].bJetPack = false;
				m_EngineSound->StopSound(m_Engine->IndexOfEdict(pEntity), CHAN_AUTO, JetpackSound);
				if ( IsAlive( pEntity ) )
				{
					UTIL_SetProperty( g_Offsets.m_MoveType, pEntity, MOVETYPE_WALK);
				}
			}
			RETURN_META(MRES_SUPERCEDE);
		}
		else if ( zombie_respawn.GetBool() && FStrEq( sCmd, "zombie_respawn" ) )
		{
			Event_Player_Say( iPlayer, "!zspawn" );
			RETURN_META( MRES_SUPERCEDE );
		}
		else if ( FStrEq( sCmd, "zombie_vision" ) || FStrEq( sCmd, "nightvision" ) )
		{
			if ( zombie_allow_disable_nv.GetBool() && g_Players[iPlayer].isZombie )
			{
				IPlayerInfo *iInfo = m_PlayerInfoManager->GetPlayerInfo( pEntity );

				bool bValue = !g_Players[iPlayer].bFaded;
				DoFade( pPlayer, bValue, iPlayer );
				ZombieVision( pEntity, bValue );
				RETURN_META( MRES_SUPERCEDE );
			}
			else
			{
				RETURN_META( MRES_SUPERCEDE );
			}
		}
		else if ( FStrEq( sCmd, "menuselect" ) )
		{
			if ( g_Players[iPlayer].iMenu > 0 || g_Players[iPlayer].iClassMenu != -1 || g_Players[iPlayer].iBuyMenu != -1)
			{
				int iCmd = atoi( args.Arg(1) );
				RETURN_META( MenuSelect( iPlayer, pEntity, iCmd ) );
			}
			RETURN_META( MRES_IGNORED );
		}
		else if ( FStrEq( sCmd, "zombie_class_menu" ) )
		{
			if ( !m_ClassManager.Enabled() )
			{
				Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("classes_disabled") ); //Zombie Classes are disabled.
			}
			else
			{
				g_Players[iPlayer].iClassMenu = -1;
				ShowClassMenu( pEntity );
			}
			RETURN_META( MRES_SUPERCEDE );
		}
		else if ( FStrEq( sCmd, "scream" ) )
		{
			IPlayerInfo *info = m_PlayerInfoManager->GetPlayerInfo( pEntity );
			if ( !info || !g_Players[iPlayer].isZombie || !IsAlive( pEntity ) )
			{
				Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("cannot_scream") ); //Cannot scream when you are dead or when you are not a zombie.
				RETURN_META( MRES_SUPERCEDE );
			}
			MRecipientFilter filter;
			filter.AddAllPlayers( MAX_CLIENTS );//g_SMAPI->GetCGlobals()->maxClients );
			char sSound[128];
			Q_snprintf( sSound, sizeof(sSound), "npc/zombie/%s.wav", g_sZombieSounds[RandomInt(0, NUM_ZOMBIE_SOUNDS-1)] );
			m_EngineSound->EmitSound(filter, iPlayer, CHAN_VOICE, sSound, RandomFloat(0.1, 1.0), 0.5, 0, RandomInt(70, 150)/*PITCH_NORM*/);
			RETURN_META( MRES_SUPERCEDE );
		}
		else if ( FStrEq( sCmd, "buyammo1" ) )
		{
			int WeaponID = LookupBuyID( "primammo" );
			int nCount = 0;
			if ( !AllowWeapon( pPlayer, WeaponID, nCount ) )
			{
				if (nCount == 0)
				{
					char sPrint[100];
					Q_snprintf( sPrint, sizeof( sPrint ), GetLang("restricted_weap"), LookupWeaponName( WeaponID ) );
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 *** %s", sPrint );
				}
				RETURN_META( MRES_SUPERCEDE );
			}
		}
		if ( FStrEq( sCmd, "buyammo2" ) )
		{
			int WeaponID = LookupBuyID("secammo");
			int nCount = 0;
			if ( !AllowWeapon( pPlayer, WeaponID, nCount ) ) 
			{
				if (nCount == 0)
				{
					char sPrint[100];
					Q_snprintf( sPrint, sizeof( sPrint ), GetLang("restricted_weap"), LookupWeaponName( WeaponID ) );
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 *** %s", sPrint ); //Sorry, the %s is restricted.
				}
				RETURN_META( MRES_SUPERCEDE );
			}
		}
	}
	RETURN_META( MRES_IGNORED );
}

META_RES ZombiePlugin::MenuSelect( int iPlayer, edict_t *pEntity, int iMenu )
{
	// 1 Class
	// 2 Buymenu
	// 3 Spawn
	// 4 Tele
	// 5 Stuck
	if ( g_Players[iPlayer].iMenu > 0 )
	{
		switch ( iMenu )
		{
			case 1:
				if ( m_ClassManager.Enabled() )
				{
					// Class Menu
					ShowClassMenu( pEntity );
				}
				break;
			case 2:
				if ( zombie_buymenu.GetBool() && g_bBuyMenu )
				{
					// Buy Menu
					ShowBuyMenu( pEntity, -1 );
				}
				break;
			case 3:
				// Spawn
				Event_Player_Say( iPlayer, "!zspawn" );
				break;
			case 4:
				// Tele
				Event_Player_Say( iPlayer, "!ztele" );
				break;
			case 5:
				// Stuck
				Event_Player_Say( iPlayer, "!zstuck" );
				break;
			default:
				break;
		}
		//Hud_Print( pEntity, "Disabled normal menu." );
		g_Players[iPlayer].iMenu = 0;
		return MRES_SUPERCEDE;

	}
	else if ( g_Players[iPlayer].iBuyMenu > -1 )
	{
		ShowBuyMenu( pEntity, iMenu );
		return MRES_SUPERCEDE;
	}
	else if ( g_Players[iPlayer].iClassMenu != -1 )
	{
		int iCmd = iMenu; //atoi( args.Arg(1) );
		int iVal = -1;
		int iCommand = 0;
		
		iVal = g_Players[iPlayer].iClassMenu + iCmd -1;
		if ( iCmd == 8 )
		{
			if ( g_Players[iPlayer].iClassMenu == 0 )
			{
				g_Players[iPlayer].iClassMenu = -1;
				g_Players[iPlayer].iMenu = 1;
				ShowMainMenu( pEntity );
				return MRES_SUPERCEDE;
			}
			iCommand = 2;
		}
		if ( ( ( m_ClassManager.Count() - 1 ) > ( g_Players[iPlayer].iClassMenu + 6 ) ) && iCmd == 9  )
		{
			iCommand = 1;
			// More
		}
		if ( iCommand > 0 || ( iCmd != 0 && iCmd != 10 && ( iVal < m_ClassManager.Count() ) ) )
		{
			if ( iCommand == 1 )
			{
				g_Players[iPlayer].iClassMenu += 7;
				ShowClassMenu( pEntity );
			}
			else if ( iCommand == 2 )
			{
				g_Players[iPlayer].iClassMenu -= 7;
				ShowClassMenu( pEntity );
			}
			else
			{
				GETCLASS( iVal );
				SelectZombieClass( pEntity, iPlayer, zClass );
				g_Players[iPlayer].bChoseClass = true;
			}
		}
		else
		{
			g_Players[iPlayer].iClassMenu = -1;
		}
		return MRES_SUPERCEDE;
	}
	return MRES_IGNORED;
}

void ZombiePlugin::SelectZombieClass(edict_t *pEntity, int iPlayer, ZombieClass *zClass, bool bNow, bool bMessages)
{

	if ( !zClass )
	{
		META_LOG( g_PLAPI, "Invalid class for SetZombieClass" );
		return;
	}
	if ( iPlayer == 0 )
	{
		return;
	}
	if ( zClass->iClassId == g_Players[iPlayer].iClass )
	{
		g_Players[iPlayer].iChangeToClass = -1;
	}
	else
	{
		if ( bNow )
		{
			g_Players[iPlayer].iClass = zClass->iClassId;
			g_Players[iPlayer].iChangeToClass = -1;
		}
		else
		{
			g_Players[iPlayer].iChangeToClass = zClass->iClassId;
		}
	}

	IPlayerInfo *iInfo = m_PlayerInfoManager->GetPlayerInfo( pEntity );
	if ( zombie_save_classlist.GetBool() && iInfo )
	{
		kvPlayerClasses->SetInt( iInfo->GetNetworkIDString(), zClass->iClassId );
	}

	//ZombieClasses_t *c;
	//c = &g_ZombieClasses[iClass];
	//GETCLASS( iClass );

	ZombieClass *c = zClass;

	char sFinal[5000] = "";
	char sTmp[255] = "";
	Q_snprintf( sFinal, sizeof( sFinal ), "\n%s: %s\n====================\n", GetLang("classes_selected"), c->GetName() ); //ZombieMod Selected Class
	
	#define AddToString( sString, sVar, sFmt, sLang, sAdd, iPcnt, pEnt ) \
		if ( iPcnt == -1 ) \
		{ \
			Q_snprintf( sVar, sizeof( sVar ), sFmt, sLang, sAdd ); \
		} \
		else \
		{ \
			Q_snprintf( sVar, sizeof( sVar ), sFmt, sLang, sAdd, iPcnt ); \
		} \
		Q_strncat( sString, sVar, sizeof( sString ), -1 );


	if ( zombie_display_class.GetBool() && !g_Players[iPlayer].isBot )
	{
		AddToString( sFinal, sTmp, "%s: %s\n", GetLang("model"), c->GetModelName(), 0, pEntity ); //Model
		
		int iTmp = ( (float)c->iHealth / 100 ) * (float)zombie_health_percent.GetInt();
		AddToString( sFinal, sTmp, "%s: %d (%d%%)\n", GetLang("health"), iTmp, zombie_health_percent.GetInt(), pEntity ); //Health
		
		iTmp = ( (float)c->fSpeed / 100 ) * (float)zombie_speed_percent.GetInt();
		AddToString( sFinal, sTmp, "%s: %d (%d%%)\n", GetLang("speed"), iTmp, zombie_speed_percent.GetInt(), pEntity ); //Speed

		iTmp = ( (float)c->fSpeedDuck / 100 ) * (float)zombie_speed_percent.GetInt();
		AddToString( sFinal, sTmp, "%s: %d (%d%%)\n", GetLang("crouch_speed"), iTmp, zombie_speed_percent.GetInt(), pEntity ); //Speed

		iTmp = ( (float)c->fSpeedRun / 100 ) * (float)zombie_speed_percent.GetInt();
		AddToString( sFinal, sTmp, "%s: %d (%d%%)\n", GetLang("run_speed"), iTmp, zombie_speed_percent.GetInt(), pEntity ); //Speed
		
		iTmp = ( (float)c->iJumpHeight / 100 ) * (float)zombie_jump_height_percent.GetInt();
		AddToString( sFinal, sTmp, "%s: %d (%d%%)\n", GetLang("jump_height"), iTmp, zombie_jump_height_percent.GetInt(), pEntity ); //Jump Height
		
		float fTmp = ( c->fKnockback / 100 ) * zombie_knockback_percent.GetInt();
		AddToString( sFinal, sTmp, "%s: %.2f (%d%%)\n", GetLang("knockback"), fTmp, zombie_knockback_percent.GetInt(), pEntity ); //Knockback

		AddToString( sFinal, sTmp, "%s: %d\n", GetLang("heads_req"), c->iHeadshots, -1, pEntity ); // Headhosts Required

		AddToString( sFinal, sTmp, "%s: %s\n", GetLang("hs_only"), (c->bHeadShotsOnly ? GetLang("yes") : GetLang("no")), -1, pEntity ); // Headhosts only

		AddToString( sFinal, sTmp, "%s: %d\n", GetLang("regen_hp"), c->iRegenHealth, -1, pEntity ); // Regen Health

		AddToString( sFinal, sTmp, "%s: %f\n", GetLang("regen_time"), c->fRegenTimer, -1, pEntity ); // Regen Seconds

		AddToString( sFinal, sTmp, "%s: %f\n", GetLang("gren_multiplier"), c->fGrenadeMultiplier, -1, pEntity ); // Grenade Damage Multiplier

		AddToString( sFinal, sTmp, "%s: %f\n", GetLang("gren_knockback"), c->fGrenadeKnockback, -1, pEntity ); // Grenade Damage Multiplier

		AddToString( sFinal, sTmp, "%s: %d\n", GetLang("health_bonus"), c->iHealthBonus, -1, pEntity ); // Grenade Damage Multiplier

		AddToString( sFinal, sTmp, "%s: %s\n", GetLang("falldamage"), (c->bFallDamage ? GetLang("yes") : GetLang("no")), -1, pEntity ); // Fall Damage
		
		AddToString( sFinal, sTmp, "====================\n", "", "", 0, pEntity );

		if ( Q_strlen( sFinal ) > 0 )
		{
			UTIL_ClientPrintf( pEntity, sFinal );
		}
	}

	if ( bMessages )
	{
		char sPrint[100];
		if ( bNow )
		{
			Q_snprintf( sPrint, sizeof(sPrint), GetLang( "change_class_msg_now" ), c->GetName() );
		}
		else
		{
			Q_snprintf( sPrint, sizeof(sPrint), GetLang( "change_class_msg" ), c->GetName() );
		}
		Hud_Print( pEntity, sPrint );

		c = NULL;
	}

	g_Players[iPlayer].iClassMenu = -1;

	return;
}

const char *ZombiePlugin::GetGameDescription( void )
{
	if ( zombie_game_description.GetBool() )
	{
		if ( !m_sGameDescription )
		{
			m_sGameDescription = new char[1024];
		}
		char sTmp[1024];
		Q_strncpy( m_sGameDescription, zombie_game_description_format.GetString(), 1024 );
		Q_StrSubst( m_sGameDescription, "!d!", g_ZombiePlugin.GetDescription(), sTmp, 1024 );
		Q_StrSubst( sTmp, "!v!", g_ZombiePlugin.GetVersion(), m_sGameDescription, 1024 );
		//g_SMAPI->Format( m_sGameDescription, 1024, "%s - %s", g_ZombiePlugin.GetDescription(), g_ZombiePlugin.GetVersion() );
		RETURN_META_VALUE( MRES_SUPERCEDE, m_sGameDescription );
	}
	RETURN_META_VALUE( MRES_IGNORED, "" );
}

bool ZombiePlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{

	PLUGIN_SAVEVARS(); 

	GET_V_IFACE_ANY(GetServerFactory, m_ServerDll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL); // m_ServerDll
	GET_V_IFACE_CURRENT(GetEngineFactory, m_Engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER); // m_Engine
	GET_V_IFACE_CURRENT(GetServerFactory, m_ServerClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS); // m_ServerClients
	GET_V_IFACE_CURRENT(GetEngineFactory, m_CVar, ICvar, CVAR_INTERFACE_VERSION); // m_CVar
	GET_V_IFACE_CURRENT(GetEngineFactory, m_GameEventManager, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2); // m_GameEventManager
	GET_V_IFACE_CURRENT(GetFileSystemFactory, m_FileSystem, IFileSystem, BASEFILESYSTEM_INTERFACE_VERSION); // m_FileSystem
	GET_V_IFACE_CURRENT(GetEngineFactory, m_EngineSound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION); // m_EngineSound
	GET_V_IFACE_CURRENT(GetEngineFactory, m_Helpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS); // m_Helpers
	GET_V_IFACE_CURRENT(GetEngineFactory, m_NetworkStringTable, INetworkStringTableContainer, INTERFACENAME_NETWORKSTRINGTABLESERVER); // m_NetworkStringTable
	GET_V_IFACE_CURRENT(GetServerFactory, m_PlayerInfoManager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER); // m_PlayerInfoManager
	GET_V_IFACE_CURRENT(GetEngineFactory, m_FileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION); // m_FileSystem
	GET_V_IFACE_CURRENT(GetEngineFactory, m_EngineSound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION); // m_EngineSound
	GET_V_IFACE_CURRENT(GetServerFactory, m_Effects, IEffects, IEFFECTS_INTERFACE_VERSION); // m_Effects
	GET_V_IFACE_CURRENT(GetEngineFactory, m_ModelInfo, IVModelInfo, VMODELINFO_SERVER_INTERFACE_VERSION); // m_ModelInfo
	GET_V_IFACE_CURRENT(GetEngineFactory, m_EngineTrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER); // m_EngineTrace
	GET_V_IFACE_CURRENT(GetEngineFactory, staticpropmgr, IStaticPropMgrServer, INTERFACEVERSION_STATICPROPMGR_SERVER); // m_StaticPropMgr
	GET_V_IFACE_CURRENT(GetEngineFactory, m_VoiceServer, IVoiceServer, INTERFACEVERSION_VOICESERVER); // m_VoiceServer
	GET_V_IFACE_CURRENT(GetServerFactory, m_GameEnts, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS); // m_GameEnts
	GET_V_IFACE_CURRENT(GetEngineFactory, m_SoundEmitterSystem, ISoundEmitterSystemBase, SOUNDEMITTERSYSTEM_INTERFACE_VERSION ); // m_SoundEmitterSystem

	bGameOn = false;
	bFirstEverRound = true;
	bClientConnecting = false;
	int x = 0;
	for ( x = 0; x < WEAPONINFO_COUNT; x++ )
	{
		g_MaxClip[x] = -1;
		g_DefaultClip[x] = -1;
	}
	for ( x = 0; x < CSW_MAX; x++)
	{ 
		g_RestrictT[x] = -1;
		g_RestrictCT[x] = -1;
		g_RestrictW[x] = -1;
	}
	for ( x = 1; x <= MAX_PLAYERS; x++ )
	{
		g_Players[x].isZombie = false;
		g_Players[x].isBot = false;
		g_Players[x].isHooked = false;
		g_Players[x].pPlayer = NULL;
		g_Players[x].iBulletCount = 0;
	}

#ifdef METAMOD_PLAPI_VERSION
	void *laddr = (void *)(g_SMAPI->GetServerFactory(false));
#else
	void *laddr = reinterpret_cast<void *>(g_SMAPI->GetServerFactory(false));
#endif

	InitialiseSigs();

	if ( !LoadSignatures( laddr, error, maxlen ) )
	{
		return false;
	}

	if ( !g_gamerules_addr  )
	{
		g_SMAPI->Format(error, maxlen, "Could not find CGameRules !");
		return false;
	}
	else if ( !g_EntList )
	{
		g_SMAPI->Format(error, maxlen, "Could not find g_EntList !");
		return false;
	}
	else if ( !m_TempEnts )
	{
		g_SMAPI->Format(error, maxlen, "Could not find ITempEntsSystem !");
		return false;
	}

	#if defined ORANGEBOX_BUILD
		/* NOTE! g_pCvar must be set to a valid ICvar instance first. */
		g_pCVar = m_CVar;
		ConVar_Register(0, &g_Accessor);
	#else
		ConCommandBaseMgr::OneTimeInit(&g_Accessor);
	#endif

	g_pSharedChangeInfo = m_Engine->GetSharedEdictChangeInfo();
	InitZMUtils();

	kvPlayerClasses = new KeyValues( "ClientClasses" );
	kvPlayerClasses->LoadFromFile( m_FileSystem, "cfg/zombiemod/player_classes.kv", "MOD" );

	kvPlayerVision = new KeyValues( "VisionSelection" );
	kvPlayerVision->SaveToFile( m_FileSystem, "cfg/zombiemod/player_vision.kv", "MOD" );

	g_Timers = new STimers();

	SH_ADD_HOOK(IVoiceServer, SetClientListening, m_VoiceServer, SH_MEMBER(  this, &ZombiePlugin::SetClientListening ), false);
	SH_ADD_HOOK(IServerGameDLL, LevelInit, m_ServerDll, SH_MEMBER(  this, &ZombiePlugin::LevelInit_Pre ), false);
	SH_ADD_HOOK(IServerGameDLL, LevelInit, m_ServerDll, SH_MEMBER(  this, &ZombiePlugin::LevelInit ), true);
	SH_ADD_HOOK(IServerGameDLL, ServerActivate, m_ServerDll, SH_MEMBER(  this, &ZombiePlugin::ServerActivate ), true);
	SH_ADD_HOOK(IServerGameDLL, GameFrame, m_ServerDll, SH_MEMBER(  this, &ZombiePlugin::GameFrame ), true);
	SH_ADD_HOOK(IServerGameDLL, LevelShutdown, m_ServerDll, SH_MEMBER(  this, &ZombiePlugin::LevelShutdown ), false);
	SH_ADD_HOOK(IServerGameDLL, GetGameDescription, m_ServerDll, SH_MEMBER( this, &ZombiePlugin::GetGameDescription), false );
	SH_ADD_HOOK(IServerGameClients, ClientDisconnect, m_ServerClients, SH_MEMBER(  this, &ZombiePlugin::ClientDisconnect ), false );
	SH_ADD_HOOK(IServerGameClients, ClientPutInServer, m_ServerClients, SH_MEMBER(  this, &ZombiePlugin::ClientPutInServer ), true);
	SH_ADD_HOOK(IServerGameClients, SetCommandClient, m_ServerClients, SH_MEMBER(  this, &ZombiePlugin::SetCommandClient ), true);
	SH_ADD_HOOK(IServerGameClients, ClientCommand, m_ServerClients, SH_MEMBER(  this, &ZombiePlugin::ClientCommand ), false);
	SH_ADD_HOOK(IServerGameClients, ClientActive, m_ServerClients, SH_MEMBER(  this, &ZombiePlugin::ClientActive ), true);
	SH_ADD_HOOK(IGameEventManager2, FireEvent, m_GameEventManager, SH_MEMBER( this, &ZombiePlugin::FireEvent ), false );

#ifdef ENDSOUND
	SH_ADD_HOOK(ISoundEmitterSystemBase, GetWavFileForSound, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::GetWavFileForSoundInt1 ), true );
	SH_ADD_HOOK(ISoundEmitterSystemBase, GetWavFileForSound, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::GetWavFileForSoundInt2 ), true );
	SH_ADD_HOOK(ISoundEmitterSystemBase, AddSound, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::AddSound ), true );
	SH_ADD_HOOK(ISoundEmitterSystemBase, GetParametersForSound, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::GetParametersForSound ), true );
	SH_ADD_HOOK(ISoundEmitterSystemBase, GetWaveName, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::GetWaveName ), false );
	SH_ADD_HOOK(ISoundEmitterSystemBase, AddWaveName, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::AddWaveName ), true );
	SH_ADD_HOOK(ISoundEmitterSystemBase, GetParametersForSoundEx, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::GetParametersForSoundEx ), true );
#endif

	SH_MANUALHOOK_RECONFIGURE( CGameRules_IPointsForKill, cIPointsForKill->iFunctionOffset, 0, 0 );
	SH_MANUALHOOK_RECONFIGURE( Event_Killed_hook, cEventKilled->iFunctionOffset, 0, 0 );
	SH_MANUALHOOK_RECONFIGURE( CommitSuicide_hook, cCommitSuicide->iFunctionOffset, 0, 0 );
	SH_MANUALHOOK_RECONFIGURE( ChangeTeam_hook, cChangeTeam->iFunctionOffset, 0, 0 );
	SH_MANUALHOOK_RECONFIGURE( Weapon_CanUse_hook, cWeaponCanUse->iFunctionOffset, 0, 0 );
	SH_MANUALHOOK_RECONFIGURE( OnTakeDamage_hook, cOnTakeDamage->iFunctionOffset, 0, 0 );
	SH_MANUALHOOK_RECONFIGURE( TraceAttack_hook, cTraceAttack->iFunctionOffset, 0, 0 );
	SH_MANUALHOOK_RECONFIGURE( Weapon_CanSwitchTo_hook, cWeaponCanSwitch->iFunctionOffset, 0, 0 );
	SH_MANUALHOOK_RECONFIGURE( PreThink_hook, cPreThink->iFunctionOffset, 0, 0 );
	SH_MANUALHOOK_RECONFIGURE( Teleport_Hook, cTeleport->iFunctionOffset, 0, 0 );
	SH_MANUALHOOK_RECONFIGURE( GetPlayerMaxSpeed, cGetPlayerMaxSpeed->iFunctionOffset, 0, 0);


	m_GameEventManager->LoadEventsFromFile( "addons/zombiemod/zombieevents.res" );

	m_Engine->ServerCommand( "exec zombiemod/zombiemod.cfg\n" );

	
	if ( late )
	{
		META_LOG( g_PLAPI, "Loaded late so trying to catch up..." );

		AllPluginsLoaded();

		LevelInit( NULL, NULL, NULL, NULL, false, false );
		LevelInit_Pre( NULL, NULL, NULL, NULL, false, false );
		
		for ( x = 1; x <= MAX_PLAYERS; x++ )
		{
			edict_t *pEnt;
			if ( IsValidPlayer ( x,  &pEnt ) )
			{
				ClientPutInServer( pEnt, NULL );
			}
		}
		if ( zombie_startup.GetBool() )
		{
			m_Engine->ServerCommand( "zombie_mode 1\n" );
		}
	}

	return true;
}


bool ZombiePlugin::Unload(char *error, size_t maxlen)
{

	UnloadZMUtils();

	int x;
	for ( x = 1; x <= MAX_PLAYERS; x++ )
	{
		edict_t *pEntity = m_Engine->PEntityOfEntIndex( x );
		if ( pEntity && !pEntity->IsFree() && m_Engine->GetPlayerUserId( pEntity ) != -1 )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)m_GameEnts->EdictToBaseEntity( pEntity );
			if ( pPlayer )
			{
				g_Players[x].pPlayer = pPlayer;
				UnHookPlayer( x );
			}
		}
		g_Players[x].pPlayer = NULL;
		g_Players[x].isHooked = false;
		g_Players[x].isBot = false;
	}

	if ( kvPlayerClasses )
	{
		kvPlayerClasses->deleteThis();
		kvPlayerClasses = NULL;
	}

	if ( kvPlayerVision )
	{
		kvPlayerVision->deleteThis();
		kvPlayerVision = NULL;
	}

	UnLoadZombieEvents();

	FFA_Disable();

	if ( pKillCmd != NULL )
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, pKillCmd, SH_STATIC(cmdKill), false);
	}
	if ( pSayCmd != NULL )
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, pSayCmd, SH_STATIC(cmdSay), false);
	}
	if ( pTeamSayCmd != NULL )
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, pTeamSayCmd, SH_STATIC(cmdTeamSay), false);
	}
	if ( pAutoBuyCmd != NULL )
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, pAutoBuyCmd, SH_STATIC(cmdAutoBuy), false);
	}
	if ( pReBuyCmd != NULL )
	{
		SH_REMOVE_HOOK(ConCommand, Dispatch, pReBuyCmd, SH_STATIC(cmdAutoBuy), false);
	}

	SH_REMOVE_HOOK(IVoiceServer, SetClientListening, m_VoiceServer, SH_MEMBER( this, &ZombiePlugin::SetClientListening ), false);
	SH_REMOVE_HOOK(IServerGameDLL, LevelInit, m_ServerDll, SH_MEMBER( this, &ZombiePlugin::LevelInit_Pre ), false);
	SH_REMOVE_HOOK(IServerGameDLL, LevelInit, m_ServerDll, SH_MEMBER( this, &ZombiePlugin::LevelInit ), true);
	SH_REMOVE_HOOK(IServerGameDLL, ServerActivate, m_ServerDll, SH_MEMBER( this, &ZombiePlugin::ServerActivate ), true);
	SH_REMOVE_HOOK(IServerGameDLL, GameFrame, m_ServerDll, SH_MEMBER( this, &ZombiePlugin::GameFrame ), true);
	SH_REMOVE_HOOK(IServerGameDLL, LevelShutdown, m_ServerDll, SH_MEMBER( this, &ZombiePlugin::LevelShutdown ), false);
	SH_REMOVE_HOOK(IServerGameDLL, GetGameDescription, m_ServerDll, SH_MEMBER( this, &ZombiePlugin::GetGameDescription), false );
	SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, m_ServerClients, SH_MEMBER( this, &ZombiePlugin::ClientDisconnect ), false );
	SH_REMOVE_HOOK(IServerGameClients, ClientPutInServer, m_ServerClients, SH_MEMBER( this, &ZombiePlugin::ClientPutInServer ), true);
	SH_REMOVE_HOOK(IServerGameClients, SetCommandClient, m_ServerClients, SH_MEMBER( this, &ZombiePlugin::SetCommandClient ), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientConnect, m_ServerClients, SH_MEMBER( this, &ZombiePlugin::ClientConnect ), false);
	SH_REMOVE_HOOK(IServerGameClients, ClientActive, m_ServerClients, SH_MEMBER( this, &ZombiePlugin::ClientActive ), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientCommand, m_ServerClients, SH_MEMBER( this, &ZombiePlugin::ClientCommand ), false);
	SH_REMOVE_HOOK(IGameEventManager2, FireEvent, m_GameEventManager, SH_MEMBER( this, &ZombiePlugin::FireEvent ), false );

#ifdef ENDSOUND
	SH_REMOVE_HOOK(ISoundEmitterSystemBase, GetWavFileForSound, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::GetWavFileForSoundInt1 ), true );
	SH_REMOVE_HOOK(ISoundEmitterSystemBase, GetWavFileForSound, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::GetWavFileForSoundInt2 ), true );
	SH_REMOVE_HOOK(ISoundEmitterSystemBase, AddSound, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::AddSound ), true );
	SH_REMOVE_HOOK(ISoundEmitterSystemBase, GetParametersForSound, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::GetParametersForSound ), true );
	SH_REMOVE_HOOK(ISoundEmitterSystemBase, GetWaveName, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::GetWaveName ), false );
	SH_REMOVE_HOOK(ISoundEmitterSystemBase, AddWaveName, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::AddWaveName ), true );
	SH_REMOVE_HOOK(ISoundEmitterSystemBase, GetParametersForSoundEx, m_SoundEmitterSystem, SH_MEMBER( this, &ZombiePlugin::GetParametersForSoundEx ), true );
#endif

	g_TermRoundFunc = NULL;
	g_GiveNamedItemFunc = NULL;
	g_GetNameFunc = NULL;
	g_WeaponDropFunc = NULL;
	g_WantsLag = NULL;
	g_TakeDamage = NULL;
	g_GetFileWeaponInfoFromHandle = NULL;
	g_CCSPlayer_RoundRespawn = NULL;
	staticpropmgr = NULL;
	g_ZombieModels.RemoveAll();
	pKillCmd = NULL;
	pSayCmd = NULL;
	pTeamSayCmd = NULL;
	m_EngineSound = NULL;
	m_ServerDll = NULL;
	m_Engine = NULL;
	m_PlayerInfoManager = NULL;
	m_Effects = NULL;
	m_ModelInfo = NULL;
	m_EngineTrace = NULL;
	m_VoiceServer = NULL;
	m_NetworkStringTable = NULL; 
	m_CVar = NULL;
	m_FileSystem = NULL;
	m_GameEventManager = NULL;
	m_ServerClients = NULL;

	sLangError = NULL;

	if ( m_sGameDescription )
	{
		delete [] m_sGameDescription;
	}
	DestroySigs();

	return true;
}

#if !defined ORANGEBOX_BUILD
	void cmdKill( void )
	{
		if ( zombie_enabled.GetBool() && zombie_suicide.GetBool() )
		{
			edict_t *pEnt = m_Engine->PEntityOfEntIndex( g_ZombiePlugin.iCurrentPlayer );
			if ( pEnt && !pEnt->IsFree() && m_Engine->GetPlayerUserId( pEnt ) != -1 )
			{
				g_ZombiePlugin.Hud_Print( pEnt, "\x03[ZOMBIE]\x01 %s", zombie_suicide_text.GetString() );
			}
			RETURN_META( MRES_SUPERCEDE );
		}
		RETURN_META( MRES_IGNORED );
	}
#else
	void cmdKill( const CCommand &command )
	{
		if ( zombie_enabled.GetBool() && zombie_suicide.GetBool() )
		{
			edict_t *pEnt = m_Engine->PEntityOfEntIndex( g_ZombiePlugin.iCurrentPlayer );
			if ( pEnt && !pEnt->IsFree() && m_Engine->GetPlayerUserId( pEnt ) != -1 )
			{
				g_ZombiePlugin.Hud_Print( pEnt, "\x03[ZOMBIE]\x01 %s", zombie_suicide_text.GetString() );
			}
			RETURN_META( MRES_SUPERCEDE );
		}
		RETURN_META( MRES_IGNORED );
	}

	void cmdSay( const CCommand &command )
	{
		META_RES mReturn;
		mReturn = g_ZombiePlugin.Event_Player_Say( g_ZombiePlugin.iCurrentPlayer, command.Arg( 0 ) );
		RETURN_META( mReturn );
	}

	void cmdTeamSay( const CCommand &command )
	{
		META_RES mReturn;
		mReturn = g_ZombiePlugin.Event_Player_Say( g_ZombiePlugin.iCurrentPlayer, command.Arg( 0 ) );
		RETURN_META( mReturn );
	}

	void cmdAutoBuy( const CCommand &command )
	{
		int iPlayer = g_ZombiePlugin.iCurrentPlayer;
		CBaseEntity *pPlayer = NULL;
		if ( IsValidPlayer( iPlayer, &pPlayer ) )
		{
			g_ZombiePlugin.CheckAutobuy( iPlayer, (CBasePlayer *)pPlayer );
		}
		RETURN_META( MRES_HANDLED );
	}

#endif

void ZombiePlugin::FindCVars()
{
	if ( !mp_spawnprotectiontime )
		mp_spawnprotectiontime = m_CVar->FindVar("mp_spawnprotectiontime");
	
	if ( !mp_tkpunish )
		mp_tkpunish = m_CVar->FindVar("mp_tkpunish");

	if ( !sv_alltalk )
		sv_alltalk = m_CVar->FindVar("sv_alltalk");

	if ( !sv_forcepreload )
		sv_forcepreload = m_CVar->FindVar("sv_forcepreload");

	if ( !ammo_buckshot_max )
		ammo_buckshot_max = m_CVar->FindVar("ammo_buckshot_max");

	if ( !mp_roundtime )
		mp_roundtime = m_CVar->FindVar("mp_roundtime");

	if ( !mp_friendlyfire )
		mp_friendlyfire = m_CVar->FindVar("mp_friendlyfire");
	mp_friendlyfire->InstallChangeCallback( CVar_CallBack );

	if ( !mp_limitteams )
		mp_limitteams = m_CVar->FindVar("mp_limitteams");
	mp_limitteams->InstallChangeCallback( CVar_CallBack );

	if ( !mp_autoteambalance )
		mp_autoteambalance = m_CVar->FindVar("mp_autoteambalance");
	mp_autoteambalance->InstallChangeCallback( CVar_CallBack );

	if ( !mp_restartgame )
		mp_restartgame = m_CVar->FindVar( "mp_restartgame" );
	mp_restartgame->InstallChangeCallback( CVar_CallBack );

	if ( !mp_buytime )
		mp_buytime = m_CVar->FindVar( "mp_buytime" );


	if ( !sv_cheats )
		sv_cheats = m_CVar->FindVar( "sv_cheats" );
		sv_cheats->m_nFlags &= ~FCVAR_NOTIFY;
}

void ZombiePlugin::AllPluginsLoaded()
{

	FindCVars();

	zombie_ddos_protect.InstallChangeCallback( CVar_CallBack );

	ConCommandBase *pCmd = m_CVar->GetCommands();
	while (pCmd)
	{
		if ( pCmd->IsCommand() )
		{
			const char *sName = pCmd->GetName();
			if ( FStrEq( sName, "kill" ) )
			{
				pKillCmd = (ConCommand *)pCmd;
				SH_ADD_HOOK( ConCommand, Dispatch, pKillCmd, SH_STATIC( cmdKill ), false );
				META_CONPRINTF( "[ZOMBIE] Found kill command. %p\n", pKillCmd );
			}
			else if ( FStrEq( sName, "say" ) )
			{
				pSayCmd = (ConCommand *)pCmd;
				SH_ADD_HOOK( ConCommand, Dispatch, pSayCmd, SH_STATIC( cmdSay ), false );
				META_CONPRINTF( "[ZOMBIE] Found say command. %p\n", pSayCmd );
			}
			else if ( FStrEq( sName, "say_team" ) )
			{
				pTeamSayCmd = (ConCommand *)pCmd;
				SH_ADD_HOOK( ConCommand, Dispatch, pTeamSayCmd, SH_STATIC( cmdTeamSay ), false );
				META_CONPRINTF( "[ZOMBIE] Found say_team command. %p\n", pTeamSayCmd );
			}
			else if ( FStrEq( sName, "autobuy" ) )
			{
				pAutoBuyCmd = (ConCommand *)pCmd;
				SH_ADD_HOOK( ConCommand, Dispatch, pAutoBuyCmd, SH_STATIC( cmdAutoBuy ), false );
				META_CONPRINTF( "[ZOMBIE] Found autobuy command. %p\n", pAutoBuyCmd );
			}
			else if ( FStrEq( sName, "rebuy" ) )
			{
				pReBuyCmd = (ConCommand *)pCmd;
				SH_ADD_HOOK( ConCommand, Dispatch, pReBuyCmd, SH_STATIC( cmdAutoBuy ), false );
				META_CONPRINTF( "[ZOMBIE] Found rebuy command. %p\n", pReBuyCmd );
			}
			else if ( FStrEq( sName, "noclip" ) )
			{
				pNoClip = (ConCommand *)pCmd;
				META_CONPRINTF( "[ZOMBIE] Found noclip command. %p\n", pReBuyCmd );
			}

		}
		pCmd = const_cast<ConCommandBase *>(pCmd->GetNext());
	}
	return;
}

void Timed_ConsGreet( void **params )
{
	edict_t *pEnt = (edict_t *)params[0];
	if ( IsValidPlayer( pEnt ) )
	{

		const char *sCons = zombie_welcome_text.GetString();
		char sTmp[1024];
		char sOut[1024];
		static CUtlVector<char*> sOutStrings;
		
		if ( Q_strlen( sCons ) > 0 )
		{
			Q_SplitString( sCons, "\\n", sOutStrings );

			for ( int i = 0; i < sOutStrings.Count(); i++ )
			{
				Q_StrSubst( sOutStrings[i], "[ZOMBIE]", "\x03[ZOMBIE]\x01", sOut, 1024, true );
				Q_StrSubst( sOut, "ZombieMod", "\x04ZombieMod\x01", sTmp, 1024, true );

				g_ZombiePlugin.Hud_Print( pEnt, sTmp );

				delete [] sOutStrings[i];
			}

			sOutStrings.RemoveAll();
		}
	}
}

void Timed_SetBuyZone( void **params )
{
	if ( params && params[0] )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)params[0];
		if ( IsValidPlayer( pPlayer ) )
		{
			if ( !CCSPlayer_IsInBuyZone( pPlayer ) )
			{
				CBasePlayer_SwitchTeam( pPlayer, ( g_ZombiePlugin.GetTeam( m_GameEnts->BaseEntityToEdict( pPlayer ) ) == TERRORISTS ? COUNTERTERRORISTS : TERRORISTS )  );
			}
		}
	}
}

void Timed_SetModel( void **params )
{
	CBasePlayer *pPlayer = (CBasePlayer *)params[0];
	//if ( pPlayer && m_GameEnts->BaseEntityToEdict( pPlayer ) && !m_GameEnts->BaseEntityToEdict( pPlayer )->IsFree() && ( m_Engine->GetPlayerUserId( m_GameEnts->BaseEntityToEdict( pPlayer ) ) != -1 ) )
	int iPlayer;
	if ( IsValidPlayer( pPlayer, &iPlayer ) )
	{
		//int iPlayer = m_Engine->IndexOfEdict( m_GameEnts->BaseEntityToEdict( pPlayer ) );
		if ( g_Players[iPlayer].isZombie )
		{
			if ( CBaseEntity_GetModelIndex( pPlayer ) != g_Players[iPlayer].iModel )
			{
				//UTIL_SetModel( pPlayer,
				CBaseEntity_SetModelIndex( pPlayer, g_Players[iPlayer].iModel );
			}
		}
	}
}

void Timed_JetPack( void **params )
{
}

void Timed_SetFOV( void **params )
{
	return;
	if ( zombie_enabled.GetBool() )
	{
		CBasePlayer *pBase = (CBasePlayer *)params[0];
		edict_t *pEdict = NULL;
		int iPlayer = 0;
		if ( IsValidPlayer( pBase, &iPlayer ) )
		{
			if ( !g_Players[iPlayer].isBot && g_Players[iPlayer].isZombie )
			{
				pEdict = m_GameEnts->BaseEntityToEdict( pBase );
				g_ZombiePlugin.SetFOV( pEdict, zombie_fov.GetInt() );
			}
		}
	}
}

void ZombiePlugin::MakeZombie( CBasePlayer *pPlayer, int iHealth, bool bFirst, int iAttacker )
{
	char sModel[128];
	int iPlayer = m_Engine->IndexOfEdict( m_GameEnts->BaseEntityToEdict( pPlayer ) );

	GETCLASS( g_Players[iPlayer].iClass );

	if ( g_ZombieModels.Count() < 1 )
	{
		Q_strncpy( sModel, "models/zombie/classic.mdl", sizeof( sModel ) );
	} 
	else
	{
		if ( zClass )
		{
			Q_snprintf( sModel, sizeof( sModel ), "%s.mdl", zClass->GetModelName() );
			//Q_strncpy( sModel, g_ZombieClasses[ iClass ].sModel.c_str(), sizeof( sModel ) );
			g_Players[iPlayer].iModel = zClass->iPrecache;
		}
		else
		{
			int iTmp = RandomInt( 0, g_ZombieModels.Count() - 1 );
			Q_strncpy( sModel, g_ZombieModels[ iTmp ].sModel.c_str(), sizeof( sModel ) );
			g_Players[iPlayer].iModel = g_ZombieModels[ iTmp ].iPrecache;
		}
	}
	UTIL_SetModel( pPlayer, sModel );
	g_Players[iPlayer].isZombie = true;
	g_Players[iPlayer].iHeadShots = 0;
	g_Players[iPlayer].bHeadCameOff = false;


	ZombieDropWeapons( pPlayer, true );
	edict_t *pEntity = m_GameEnts->BaseEntityToEdict( pPlayer );


	int iHP = 0;


	if ( zClass )
	{
		iHP = ( (float)zClass->iHealth / 100 ) * (float)zombie_health_percent.GetInt();
		if ( bFirst )
		{
			iHP *= 2;
		}
	}
	else
	{
		iHP = iHealth;
	}

	int iPercent = 0;
	int iDecrease = 0;
	bool bOKtoGO = false;
	if ( zombie_balancer_health_ratio.GetInt() > 0 && zombie_balancer_player_ratio.GetInt() > 0 && iPlayerCount > 0 && iZombieCount > 0 )
	{
		if ( zombie_balancer_type.GetInt() == 1 )
		{
			iPercent = (iZombieCount * 100) / iPlayerCount;
			bOKtoGO = (iPercent >= zombie_balancer_player_ratio.GetInt());
		}
		else
		{
			bOKtoGO = ( iZombieCount >= zombie_balancer_player_ratio.GetInt() );
		}
		if ( bOKtoGO )
		{
			iDecrease = iHP * zombie_balancer_health_ratio.GetInt() / 100;
			iHealthDecrease += iDecrease;
			iHP = iHP - iHealthDecrease;
			if ( iHP <= zombie_balancer_min_health.GetInt() )
			{
				iHP = zombie_balancer_min_health.GetInt();
			}
		}
	}
	
	ZombificationEvent( iPlayer, g_Players[iPlayer].iClass, iAttacker );
	
	g_Players[iPlayer].iZombieHealth = iHP;
	g_Players[iPlayer].iHealth = iHP;
	UTIL_SetProperty( g_Offsets.m_iHealth, pEntity, iHP );

	iZombieCount++;


	bool bOn = false;
	if ( !g_Players[iPlayer].isBot )
	{
		bOn = true;

		DoFade( (CBaseEntity*)pPlayer );

		SetFOV( pEntity, zombie_fov.GetInt(), true );

		if ( g_Players[iPlayer].bJetPack && !zombie_jetpack.GetBool() )
		{
			m_Engine->ClientCommand( pEntity, "-jetpack" );
		}
		if ( zombie_shake.GetBool() )
		{
			UTIL_ScreenShake( pPlayer, 25.0, 150.0, 1.0, 750.0f, SHAKE_START, true );
		}
		if ( g_Players[iPlayer].bShowZombieVision )
		{
			ZombieVision( pEntity, true );
		}
	}
	//Vector vecPlayer = pEntity->GetCollideable()->GetCollisionOrigin();
	ICollideable *cCol = CBaseEntity_GetCollideable( pPlayer );

	Vector vecPlayer = vec3_origin;
	if ( cCol )
	{
		vecPlayer = cCol->GetCollisionOrigin();
	}

	if ( zombie_effect.GetBool() )
	{
		vecPlayer.z += 20;
		ZombieEffect( vecPlayer );
		vecPlayer.z -= 20;
	}
	if ( zombie_teams.GetBool() )
	{
		CBasePlayer_SwitchTeam( pPlayer, TERRORISTS );
	}

	Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("zombification") );
	if ( g_Players[iPlayer].bShowClassMenu && m_ClassManager.Enabled() )
	{
		g_Players[iPlayer].bShowClassMenu = false;
		ShowClassMenu( pEntity );
	}

	if ( zClass )
	{
		if ( zClass->fRegenTimer > 0.0 && zClass->iRegenHealth > 0 )
		{
			void *params[1];
			params[0] = (void *)iPlayer;
			if ( g_Players[iPlayer].iRegenTimer != 0 )
			{
				g_Timers->RemoveTimer( g_Players[iPlayer].iRegenTimer );
				g_Players[iPlayer].iRegenTimer = 0;
			}
			g_Players[iPlayer].iRegenTimer = g_Timers->AddTimer( zClass->fRegenTimer, Timed_Regen, params, 1 );
		}
	}

	if ( bFirst && zombie_first_zombie_tele.GetBool() )
	{
		Vector vecVelocity = vec3_origin;
		QAngle qAngles = vec3_angle;
 		if ( iNextSpawn == 0 )
		{
			//CBaseEntity_Teleport( (CBaseEntity*)pPlayer, &g_Players[iPlayer].vSpawn, &qAngles, &vecVelocity );
			Teleport( (CBaseEntity*)pPlayer, &g_Players[iPlayer].vSpawn, NULL, NULL );
		}
		else
		{
 			//CBaseEntity_Teleport( (CBaseEntity*)pPlayer, &vSpawnVec[iUseSpawn], &qAngles, &vecVelocity );
			Teleport( (CBaseEntity*)pPlayer, &vSpawnVec[iUseSpawn], NULL, NULL );
		}
		if ( iNextSpawn != 0 )
		{
			iUseSpawn = (++iUseSpawn) % iNextSpawn;
		}
	}

	
	vecPlayer = cCol->GetCollisionOrigin();
	Vector vecClosest = ClosestPlayer( iPlayer, vecPlayer );

	Teleport( (CBaseEntity*)pPlayer, &vecClosest, NULL, NULL );
	vecPlayer = cCol->GetCollisionOrigin();
	vecClosest = ClosestPlayer( iPlayer, vecPlayer );
	float fDistance = vecPlayer.DistTo( vecClosest );

	if ( fDistance <= zombie_stuckcheck_radius.GetFloat() )
	{
		/*
		bool bOldValue = false;
		Set_sv_cheats( true, &bOldValue );
		m_Helpers->ClientCommand( pEntity, "noclip" );
		m_Helpers->ClientCommand( pEntity, "noclip" );
		Set_sv_cheats( false, &bOldValue );
		*/
		UnstickPlayer( iPlayer );
	}
	return;
}

void ZombiePlugin::ZombieEffect( Vector vLocation )
{
	CEffectData data;
	data.m_vOrigin = vLocation;
	DispatchEffect( "HelicopterMegaBomb", data );
}

void ZombiePlugin::UTIL_BloodSpray( const Vector &pos, const Vector &dir, int color, int amount, int flags )
{	
	CEffectData	data;

	data.m_vOrigin = pos;
	data.m_vNormal = dir;
	data.m_flScale = (float)amount;
	data.m_fFlags = flags;
	data.m_nColor = (unsigned char)color;

	DispatchEffect( "bloodspray", data );
}

Vector ZombiePlugin::ClosestPlayer( int iPlayer, Vector vecPlayer )
{
	if ( !IsValidPlayer( g_Players[iPlayer].pPlayer ) )
	{
		return Vector( 0,0,0 );
	}
	int x = 1;
	float fClosest = 500;
	Vector vClosest;

	for ( x = 1; x <= MAX_CLIENTS; x++ )
	{
		edict_t *pEnt = NULL;
		if ( x != iPlayer && IsValidPlayer( x, &pEnt ) && pEnt )
		{
			Vector vecLocation;
			//vecLocation = pEnt->GetCollideable()->GetCollisionOrigin();
			
			CBaseEntity *pBase = m_GameEnts->EdictToBaseEntity( pEnt );
			if ( pBase )
			{
				ICollideable *cCol = CBaseEntity_GetCollideable( pBase );

				if ( cCol )
				{
					vecLocation = cCol->GetCollisionOrigin();
				}
				else
				{
					META_LOG( g_PLAPI, "Could not find CollissionOrigin!!" );
				}

				float fTmp = CalcDistance( vecLocation, vecPlayer );
				//if ( fTmp > 0 && fTmp < fClosest )
				if ( fTmp < fClosest )
				{
					if ( fTmp < 0 )
					{
						//inside.... lol...
						META_LOG( g_PLAPI, "Player inside another..." );
					}

					vClosest = vecLocation;
					fClosest = fTmp;
				}
			}

		}
	}
	if ( fClosest != 500 )
	{
		return vClosest;
	}
	else
	{
		return vec3_origin;
	}
}
//typedef void (SourceHook::EmptyClass::*MFP_Teleport)( const Vector *, const QAngle *, const Vector * ); 
void ZombiePlugin::Slap( int iPlayer )
{
	if ( g_Players[iPlayer].pPlayer && IsValidPlayer( g_Players[iPlayer].pPlayer ) )
	{
		Vector vecVelocity;
		IPlayerInfo *iInfo = m_PlayerInfoManager->GetPlayerInfo( m_GameEnts->BaseEntityToEdict( g_Players[iPlayer].pPlayer ) );
		Vector vecLocation;
		if ( iInfo )
		{
			vecLocation = iInfo->GetAbsOrigin();
			QAngle qAngles = iInfo->GetAbsAngles();
			CBaseAnimating_GetVelocity( (CBaseAnimating*)g_Players[iPlayer].pPlayer, &vecVelocity );
			float fX = 1000.0f; //( ( RandomInt( 0, 1 ) == 0 ) ? -1000 : 1000 );
			float fY = 1000.0f;//( ( RandomInt( 0, 1 ) == 0 ) ? -1000 : 1000 );
			float fZ = 200.0f; //RandomFloat( 10, 200 );

			vecVelocity.x += fX;
			vecVelocity.y += fY;
			vecVelocity.z += fZ;
			
			Teleport( g_Players[iPlayer].pPlayer, &vecLocation, &qAngles, &vecVelocity );
			
		}
	}
}

void ZombiePlugin::DispatchEffect( const char *pName, const CEffectData &data )
{
	CPASFilters filter( data.m_vOrigin, MAX_CLIENTS, m_Engine );
	m_TempEnts->DispatchEffect( filter, 0.01, data.m_vOrigin, pName, data );
}

void ZombiePlugin::ZombieDropWeapons( CBasePlayer *pPlayer, bool bHook, bool bRespawn )
{
	int iWeaponSlots[] = { SLOT_NADE, SLOT_NADE, SLOT_NADE, SLOT_NADE, SLOT_SECONDARY, SLOT_PRIMARY, SLOT_KNIFE };
	if ( !zombie_enabled.GetBool() )
	{
		return;
	}
	bool bDeleted = false;
	CBaseCombatWeapon *pWeapon =  NULL;
	CBaseCombatWeapon *pKnife =  NULL;
	for (int x = 6; x >= 0; x--)
	{
		char buf[2];
		Q_snprintf( buf, sizeof(buf), "%d", iWeaponSlots[x] );
		if ( !bRespawn || ( bRespawn && ( Q_strstr( zombie_respawn_dontdrop_indexes.GetString(), buf ) == NULL ) ) )
		{
			pWeapon = CBaseCombatCharacter_Weapon_GetSlot( (CBaseCombatCharacter*) pPlayer, iWeaponSlots[x] );
			edict_t *pWeaponEdict = m_GameEnts->BaseEntityToEdict( pWeapon );
			if ( pWeapon && pWeaponEdict )
			{
				
				if ( FStrEq( pWeaponEdict->GetClassName(), "weapon_knife" ) )
				{
					pKnife = pWeapon;
					/*if ( bHook  )
					{
						CBaseEntity *pBase = (CBaseEntity *)pWeapon;
						int iDX = g_HookedWeapons.Find( pBase );
						if ( iDX != -1 )
						{
							SH_REMOVE_MANUALHOOK_MEMFUNC( Delete_hook, pBase, this, &ZombiePlugin::Delete, true);
							g_HookedWeapons.Remove(x);
						}
						int iPlayer = m_Engine->IndexOfEdict( m_GameEnts->BaseEntityToEdict( pPlayer ) );
						g_Players[iPlayer].pKnife = pWeapon;
						
						SH_ADD_MANUALHOOK_MEMFUNC(Delete_hook, pBase, this, &ZombiePlugin::Delete, true);
						g_HookedWeapons.AddToTail(pBase);
					}*/
				}
				else
				{
					bDeleted = true;
					UTIL_WeaponDrop( (CBasePlayer*)pPlayer, pWeapon, &Vector( 500, 500, 500 ), &Vector( 500, 500, 500 ) );
					if ( bHook && zombie_delete_dropped_weapons.GetBool() ) //Hook only true from MakeZombie
					{
						g_UtilRemoveFunc( (CBaseEntity *)pWeapon );
					}
				}
				//}
			}
		}
	}
	CCSPlayer_Weapon_Switch( (CCSPlayer*)pPlayer, pKnife );
}

void ZombiePlugin::DropWeapon( CBasePlayer *pPlayer, bool bPrimary )
{
	int iWeaponSlots[] = { SLOT_NADE, SLOT_NADE, SLOT_NADE, SLOT_NADE, SLOT_SECONDARY, SLOT_PRIMARY, SLOT_KNIFE };
	if ( !zombie_enabled.GetBool() )
	{
		return;
	}
	CBaseCombatWeapon *pWeapon =  NULL;
	if ( bPrimary ) 
	{
		pWeapon = CBaseCombatCharacter_Weapon_GetSlot( (CBaseCombatCharacter*) pPlayer, iWeaponSlots[5] );
	}
	else
	{
		pWeapon = CBaseCombatCharacter_Weapon_GetSlot( (CBaseCombatCharacter*) pPlayer, iWeaponSlots[4] );
	}

	if ( pWeapon )
	{
		if ( m_GameEnts->BaseEntityToEdict( pWeapon )->GetClassName() )
		{
			UTIL_WeaponDrop( (CBasePlayer*)pPlayer, pWeapon, &Vector( 500, 500, 500 ), &Vector( 500, 500, 500 ) );
		}
	}
}

void ZombiePlugin::HintTextMsg( edict_t *pEntity, const char *sMsg, ... ) 
{
	MRecipientFilter recipient_filter;
	char msg[1024];
	bf_write *buffer;
	va_list argptr;
	int iPlayer;

	if ( IsValidPlayer( pEntity, &iPlayer ) )
	{
		recipient_filter.AddPlayer( iPlayer );
	}
	else
	{
		recipient_filter.AddAllPlayers( MAX_CLIENTS  );
	}

	va_start ( argptr, sMsg );
	Q_vsnprintf ( msg, sizeof( msg ), sMsg, argptr );
	va_end ( argptr  );

	static int iHintText = -1;
	if ( iHintText == -1 ) 
	{
		iHintText = UserMessageIndex("HintText");
	}
	if ( iHintText == -1 )
	{
		return;
	}

	buffer = m_Engine->UserMessageBegin ( &recipient_filter, iHintText );
		buffer->WriteByte(-1);
		buffer->WriteString(msg);
	m_Engine->MessageEnd();

	return ;
}

void ZombiePlugin::Hud_Print( edict_t *pEntity, const char *sMsg, ... )
{
    bf_write				*netmsg;
    MRecipientFilter		recipient_filter;
 	char					szText[1024];
	va_list					argptr;

	va_start ( argptr, sMsg );
	Q_vsnprintf ( szText, sizeof( szText ), sMsg, argptr );
	va_end ( argptr  );

    strcat ( szText, "\n" );
 
	int iPlayer = 0;

	if ( IsValidPlayer( pEntity, &iPlayer ) )
	{
		recipient_filter.AddPlayer( iPlayer );
	}
	else
	{
		recipient_filter.AddAllPlayers( MAX_CLIENTS  );
	}
	static int iSayText = -1;
	if ( iSayText == -1 ) 
	{
		iSayText = UserMessageIndex("SayText");
	}
	if ( iSayText == -1 )
	{
		return;
	}
    netmsg = m_Engine->UserMessageBegin ( &recipient_filter, iSayText );
		netmsg->WriteByte ( 0 );
		netmsg->WriteString ( szText );
		netmsg->WriteByte ( 1 );
    m_Engine->MessageEnd ();
    return;
}

void ZombiePlugin::UTIL_DrawMenu( int player_index, int   time, int range, const char *menu_string, bool final )
{
	unsigned int	keys = 0;
	char			save = 0;
	size_t			len = 0;
	char			*sString = new char[strlen(menu_string) + 1];
	char			*ptr;
	strcpy(sString, menu_string);
	ptr = sString;

	for ( int i = 1; i <= range; i++ )
	{
		keys |= (1 << (i-1));
	}

	MRecipientFilter mrf;
	mrf.AddPlayer( player_index );

	static int iCmd = 0;
	len = Q_strlen( sString );
	while ( true )
	{
		if ( len > 240 )
		{
			save = ptr[240];
			ptr[240] = '\0';
		}
		if ( iCmd == 0 )
		{
			iCmd = UserMessageIndex( "ShowMenu" );
		}
		bf_write *msg_buffer = m_Engine->UserMessageBegin( &mrf, iCmd );
		msg_buffer->WriteShort( keys );
		msg_buffer->WriteChar( time );
		msg_buffer->WriteByte( ( len > 240 ) ? 1 : 0 );

		msg_buffer->WriteString ( ptr );   

		m_Engine->MessageEnd();
		if (len > 240)
		{
			ptr[240] = save;
			ptr = &ptr[240];
			len -= 240;
		}
		else
		{
			break;
		}
	}
	delete [] sString; 
}

void ZombiePlugin::UTIL_ClientPrintf(edict_t *pEdict, const char *szMsg)
{
	char			save = 0;
	size_t			len = 0;
	char			*sString = new char[strlen(szMsg) + 1];
	char			*ptr;
	strcpy(sString, szMsg);
	ptr = sString;

	len = Q_strlen( sString );
	while ( true )
	{
		if ( len > 240 )
		{
			save = ptr[240];
			ptr[240] = '\0';
		}
		m_Engine->ClientPrintf( pEdict, ptr );

		if (len > 240)
		{
			ptr[240] = save;
			ptr = &ptr[240];
			len -= 240;
		}
		else
		{
			break;
		}
	}
	delete [] sString; 
}

int ZombiePlugin::EntIdxOfUserIdx(int useridx)
{
	IPlayerInfo		*pInfo;
	int				i = 1;
	//for ( i = 1; i <= g_SMAPI->GetCGlobals()->maxClients; i++ )
	for ( i = 1; i <= MAX_CLIENTS; i++ )
	{
		edict_t *pEnt;
		if ( IsValidPlayer( i, &pEnt ) )
		{
			pInfo = m_PlayerInfoManager->GetPlayerInfo( pEnt );
			if (pInfo)
			{
				if ( pInfo->GetUserID() == useridx )
				{
					return i;
				}
			}
		}
	}
	return -1;
}

int ZombiePlugin::GetTeam(edict_t *pEntity)
{
	int iPlayer = 0;
	if ( !IsValidPlayer( pEntity, &iPlayer ) )
		return 0;
	int iTeam = 0;
	if ( g_Players[iPlayer].bConnected && UTIL_GetProperty( g_Offsets.m_iTeamNum, pEntity, &iTeam ) )
	{
		if ( iTeam > 0 && iTeam < 4 )
		{
			return iTeam;
		}
	}
	return 0;
}

int m_iLastZombie = -1;
void RandomZombie(void **params)
{
	//META_LOG( g_PLAPI, "RANDOM ZOMBIE." );
	g_ZombiePlugin.bAllowedToJetPack = true;
	g_ZombiePlugin.bZombieDone = true;
	if ( !zombie_enabled.GetBool() || !g_ZombiePlugin.bRoundStarted )
	{
		return;
	}

	int			aliveCount = 0;
	bool		aAlive[MAX_PLAYERS+1] = {false};
	edict_t		*aEdict[MAX_PLAYERS+1] = {NULL};
	
	if ( m_iLastZombie != -1 )
	{
		for ( int x = 1; x <= MAX_CLIENTS; x++ )
		{
			if ( m_iLastZombie != x )
			{
				aEdict[x] = NULL;
				if ( IsValidPlayer( x, &aEdict[x] ) )
				{
					aAlive[x] = false;
					if ( g_ZombiePlugin.IsAlive( aEdict[x] ) && !g_Players[x].isZombie )
					{
						aAlive[x] = true;
						aliveCount++;
					}
				}
			}
		}
	}
	if ( aliveCount == 0 )
	{
		for ( int x = 1; x <= MAX_CLIENTS; x++ )
		{
			aEdict[x] = NULL;
			if ( IsValidPlayer( x, &aEdict[x] ) )
			{
				aAlive[x] = false;
				if ( g_ZombiePlugin.IsAlive( aEdict[x] ) && !g_Players[x].isZombie )
				{
					aAlive[x] = true;
					aliveCount++;
				}
			}
		}
	}


	int iCnt;
	int iMin = zombie_count_min.GetInt();
	int iMax = zombie_count_max.GetInt();
	
	if ( iMin > 0 && iMin < iMax && iMax > 0 )
	{
		iCnt = RandomInt( iMin, iMax );
	}
	else
	{
		iCnt = zombie_count.GetInt();
	}

	if ( aliveCount > 0 )
	{
		if ( iCnt > 0 && aliveCount > 3 )
		{
			int iZombied = 0;


			if ( iCnt >= ( aliveCount - 2 ) )
			{
				iCnt = ( aliveCount - 2 );
			}
			for ( int y = 1; y <= iCnt; y++ )
			{
				srand( time( NULL ) );
				int num = ( rand() % aliveCount ) + 1;

				for ( int x = 1; x <= MAX_CLIENTS; x++ )
				{
					if ( aAlive[x] )
					{
						num--;
					}
					if ( num <= 0 )
					{
						iZombied++;
						aliveCount--;
						aAlive[x] = false;
						META_LOG( g_PLAPI, "Zombie: %d", x );

						//int iHp;
						if ( zombie_count_doublehp.GetInt() == 0 )
						{
							// None get double.
							g_ZombiePlugin.MakeZombie( g_Players[x].pPlayer, zombie_health.GetInt(), true );
						}
						else if ( zombie_count_doublehp.GetInt() == 1 && iZombied != 1 )
						{
							// First gets double.
							g_ZombiePlugin.MakeZombie( g_Players[x].pPlayer, zombie_health.GetInt(), true );
						}
						else
						{
							// All get double.
							g_ZombiePlugin.MakeZombie( g_Players[x].pPlayer, (zombie_health.GetInt() * 2), true );
						}
						m_iLastZombie = x;
						zombie_first_zombie.SetValue( m_Engine->GetPlayerUserId( m_GameEnts->BaseEntityToEdict( g_Players[x].pPlayer ) ) );
						g_ZombiePlugin.g_ZombieRoundOver = false;
						break;
					}
				}
			}
			if ( iZombied == 0 )
			{
				float fRand = RandomFloat(zombie_timer_min.GetFloat(), zombie_timer_max.GetFloat());
				//META_LOG( g_PLAPI, "Random zombie in %f.", fRand );
				g_ZombiePlugin.g_Timers->AddTimer( fRand, RandomZombie, NULL, 0, RANDOM_ZOMBIE_TIMER_ID );
			}
			return;
		}
		else
		{
			srand( time( NULL ) );
			int num = ( rand() % aliveCount ) + 1;

			for ( int x = 1; x <= MAX_CLIENTS; x++ )
			{
				if ( aAlive[x] )
				{
					num--;
				}
				if ( num <= 0 )
				{
					m_iLastZombie = x;
					META_LOG( g_PLAPI, "Zombie: %d", x );
					g_ZombiePlugin.MakeZombie( g_Players[x].pPlayer, ( zombie_count_doublehp.GetInt() == 0 ? zombie_health.GetInt() : (zombie_health.GetInt() * 2) ), true );
					zombie_first_zombie.SetValue( m_Engine->GetPlayerUserId( m_GameEnts->BaseEntityToEdict( g_Players[x].pPlayer ) ) );
					g_ZombiePlugin.g_ZombieRoundOver = false;
					return;
				}
				#ifdef _ICEPICK
					m_Engine->ServerCommand ( "zombiepicked\n" );
				#endif
			}
		}
	}
	else
	{
		float fRand = RandomFloat(zombie_timer_min.GetFloat(), zombie_timer_max.GetFloat());
		//META_LOG( g_PLAPI, "Random zombie in %f.", fRand );
		g_ZombiePlugin.g_Timers->AddTimer( fRand, RandomZombie, NULL, 0, RANDOM_ZOMBIE_TIMER_ID );
	}
}

void ZombiePlugin::ShowZomb()
{
	int x = 1;
	for ( x = 1; x <= MAX_CLIENTS; x++ )
	{
		edict_t *pEdict = m_Engine->PEntityOfEntIndex( x );
		if ( pEdict && !pEdict->IsFree() && m_Engine->GetPlayerUserId( pEdict ) != -1 )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)m_GameEnts->EdictToBaseEntity( pEdict );
			META_LOG( g_PLAPI, "\n==================================" );
			META_LOG( g_PLAPI, "=== Player: %d", x );
			META_LOG( g_PLAPI, "=== %s", ( g_Players[x].isBot ? "BOT" : "Player" )  );
			META_LOG( g_PLAPI, "=== %s", ( g_Players[x].isHooked ? "Hooked" : "--> NOT HOOKED <--" ) );
			META_LOG( g_PLAPI, "=== Knife: %p", g_Players[x].pKnife );
			META_LOG( g_PLAPI, "=== Player: %p // %p", pPlayer, g_Players[x].pPlayer );
			META_LOG( g_PLAPI, "==================================\n" );
		}
	}
}

void Timed_Regen( void **params )
{
	int iPlayer = (int)params[0];
	edict_t *pEntity = NULL;
	if ( IsValidPlayer( iPlayer, &pEntity ) && g_ZombiePlugin.IsAlive(pEntity ) && g_Players[iPlayer].isZombie )
	{
		GETCLASS( g_Players[iPlayer].iClass );
		if ( zClass )
		{
			int iRegHealth = zClass->iRegenHealth;
			if ( iRegHealth > 0 )
			{
				int iHealth = ( g_Players[iPlayer].iHealth + iRegHealth );
				if ( iHealth <= g_Players[iPlayer].iZombieHealth )
				{
					UTIL_SetProperty( g_ZombiePlugin.g_Offsets.m_iHealth, pEntity, iHealth );
					g_Players[iPlayer].iHealth = iHealth;
				}
				else if ( iHealth != g_Players[iPlayer].iZombieHealth )
				{
					UTIL_SetProperty( g_ZombiePlugin.g_Offsets.m_iHealth, pEntity, g_Players[iPlayer].iZombieHealth );
					g_Players[iPlayer].iHealth = g_Players[iPlayer].iZombieHealth;
				}
				if ( zClass->fRegenTimer > 0.0 )
				{
					void *params[1];
					params[0] = (void *)iPlayer;
					g_Players[iPlayer].iRegenTimer = g_ZombiePlugin.g_Timers->AddTimer( zClass->fRegenTimer, Timed_Regen, params, 1 );
				}
			}
		}
	}
}

void ClearRegen()
{
	for ( int x = 0; x <= MAX_CLIENTS; x++ )
	{
		if ( g_Players[x].iRegenTimer != 0 )
		{
			g_ZombiePlugin.g_Timers->RemoveTimer( g_Players[x].iRegenTimer );
			g_Players[x].iRegenTimer = 0;
		}
	}
}

void CheckZombies( void **params )
{
	if ( !zombie_enabled.GetBool() )
	{
		return;
	}
	bool bGiveHealth = false;
	static bool bCheck = false;
	if ( zombie_regen_timer.GetInt() > 0 && !m_ClassManager.Enabled() )
	{
		iNextHealth = iNextHealth + 1;
		if ( ( iNextHealth * 2 ) >= zombie_regen_timer.GetInt() )
		{
			iNextHealth = 0;
			bGiveHealth = true;
		}
	}
	if ( g_ZombiePlugin.bRoundStarted && teamCT && teamT )
	{
		if ( iGlobalVoteTimer > 0 )
		{
			iGlobalVoteTimer--;
		}
		int c = UTIL_GetNumPlayers( teamCT ) + UTIL_GetNumPlayers( teamT );
		if ( ( c > 1 ) && !g_ZombiePlugin.g_ZombieRoundOver )
		{
			int zombies = 0;
			int humans = 0;
			int iJetTime = zombie_jetpack_timer.GetInt() * 2;
			for ( int x = 1; x <= MAX_CLIENTS; x++ )
			{
				edict_t *pEdict = m_Engine->PEntityOfEntIndex( x );
				CBasePlayer *pPlayer = (CBasePlayer *)m_GameEnts->EdictToBaseEntity( pEdict );
				if ( pEdict && ( m_Engine->GetPlayerUserId( pEdict ) != -1 ) && pPlayer )
				{
					if ( !g_Players[x].isBot && ( zombie_jetpack.GetBool() || zombie_humans_jetpack.GetBool() ) )
					{
						if ( g_Players[x].bJetPack )
						{
							g_Players[x].iJetPack++;
							if ( iJetTime != 0 && g_Players[x].iJetPack >= iJetTime )
							{
								g_ZombiePlugin.Hud_Print( pEdict, "\x03[ZOMBIE]\x01 %s", g_ZombiePlugin.GetLang("jetpack_expired") ); //Oh oh, JetPack expired for this round!
								m_Helpers->ClientCommand( pEdict, "-jetpack" );
							}
						}
					}
					if ( g_ZombiePlugin.IsAlive( pEdict ) )
					{
						if ( bGiveHealth && g_Players[x].isZombie )
						{
							int iHealth = ( g_Players[x].iHealth + zombie_regen_health.GetInt() );
							if ( iHealth <= g_Players[x].iZombieHealth )
							{
								UTIL_SetProperty( g_ZombiePlugin.g_Offsets.m_iHealth, pEdict, iHealth );
								g_Players[x].iHealth = iHealth;
							}
							else if ( iHealth != g_Players[x].iZombieHealth )
							{
								UTIL_SetProperty( g_ZombiePlugin.g_Offsets.m_iHealth, pEdict, g_Players[x].iZombieHealth );
								g_Players[x].iHealth = g_Players[x].iZombieHealth;
							}
						}
						if( bCheck )
						{
							if ( g_Players[x].isZombie )
							{
								//m_Engine->StartQueryCvarValue( pEdict, "r_screenoverlay" );
								if ( RandomInt(0, zombie_sound_rand.GetInt()) == 3 )
								{
									char sSound[128];
									MRecipientFilter filter;
									filter.AddAllPlayers( MAX_CLIENTS );
									Q_snprintf( sSound, sizeof(sSound), "npc/zombie/%s.wav", g_sZombieSounds[ RandomInt( 0, NUM_ZOMBIE_SOUNDS - 1) ] );

									//SH_CALL( g_ZombiePlugin.m_EngineSound, &IEngineSound)(filter, x, CHAN_VOICE, sSound, RandomFloat( 0.1, 1.0 ), 0.5, 0, RandomInt( 70, 150 ) );
									g_ZombiePlugin.m_EngineSound->EmitSound(filter, x, CHAN_VOICE, sSound, RandomFloat( 0.1, 1.0 ), 0.5, 0, RandomInt( 70, 150 ) );
								}
								if ( zombie_teams.GetBool() )
								{
									int iTeam = g_ZombiePlugin.GetTeam( pEdict );
									if ( iTeam == COUNTERTERRORISTS )
									{
										CBasePlayer_SwitchTeam( pPlayer, TERRORISTS );
									}
								}
								zombies++;
							}
							else
							{
								if ( zombie_teams.GetBool() )
								{
									int iTeam = g_ZombiePlugin.GetTeam( pEdict );
									if (  iTeam == TERRORISTS )
									{
										CBasePlayer_SwitchTeam( pPlayer, COUNTERTERRORISTS );
									}
								}

								humans++;
							}
						}
					}
				}
			}
			if ( bCheck )
			{
				if ( humans == 0 )
				{
					g_ZombiePlugin.g_ZombieRoundOver = true;
					g_ZombiePlugin.Hud_Print( NULL, "\x03[ZOMBIE]\x01 %s", g_ZombiePlugin.GetLang("zombies_win") ); //Zombies have taken over the world. The human race has been destroyed.
					UTIL_TermRound( 5.0, Terrorists_Win );
					g_ZombiePlugin.AddTeamScore( teamT );
				}
				else if ( zombies == 0 )
				{
					g_ZombiePlugin.Hud_Print( NULL, "\x03[ZOMBIE]\x01 %s", g_ZombiePlugin.GetLang("humans_win") );
					g_ZombiePlugin.g_ZombieRoundOver = true;
					UTIL_TermRound( 5.0, CTs_Win );
					g_ZombiePlugin.AddTeamScore( teamCT );
				}
			}
		}
	}
	bCheck = !bCheck;
	g_ZombiePlugin.g_Timers->AddTimer( 0.5, CheckZombies, NULL, 0, ZOMBIECHECK_TIMER_ID );
}

void Timed_RoundTimer( void **params )
{
	META_LOG( g_PLAPI, "FORCING ROUND END!!!" );
	if ( !g_ZombiePlugin.g_ZombieRoundOver || zombie_force_endround.GetBool() )
	{
		g_ZombiePlugin.Hud_Print( NULL, "\x03[ZOMBIE]\x01 %s", g_ZombiePlugin.GetLang("humans_win") ); //Humans have killed all the zombies.. for now.
		g_ZombiePlugin.g_ZombieRoundOver = true;
		UTIL_TermRound( 5.0, CTs_Win );
		g_ZombiePlugin.AddTeamScore( teamCT );
	}
}

void ZombiePlugin::AddTeamScore( CTeam *cTeam )
{
	edict_t *pEntity = m_GameEnts->BaseEntityToEdict( cTeam );
	int iScore = 0;
	bool bError = false;
	if ( UTIL_GetProperty( g_ZombiePlugin.g_Offsets.m_iScore, pEntity, &iScore ) )
	{
		iScore++;
		if ( !UTIL_SetProperty( g_ZombiePlugin.g_Offsets.m_iScore, pEntity, iScore ) )
		{
			bError = true;
		}
	}
	else
	{
		bError = true;
	}
	if ( bError )
	{
		META_LOG( g_PLAPI, "Error setting team score!" );
	}
}

void ZombiePlugin::ClearBuyMenus( )
{
	g_tBuyMenu.bEquip = false;
	g_tBuyMenu.bMGs = false;
	g_tBuyMenu.bPistols = false;
	g_tBuyMenu.bRifles = false;
	g_tBuyMenu.bSMGs = false;
	for ( int x = 0; x <= 7; x++ )
	{
		g_tBuyMenu.sEquip[x].clear();
		g_tBuyMenu.iEquip[x].clear();

		g_tBuyMenu.sMGs[x].clear();
		g_tBuyMenu.iMGs[x].clear();

		g_tBuyMenu.sPistols[x].clear();
		g_tBuyMenu.iPistols[x].clear();

		g_tBuyMenu.sRifles[x].clear();
		g_tBuyMenu.iRifles[x].clear();

		g_tBuyMenu.sSMGs[x].clear();
		g_tBuyMenu.iSMGs[x].clear();
	}
}

bool ZombiePlugin::LoadBuyMenus( )
{
	char sFilePath[255];
	char sFolder[255];
	char sMap[100];
	char sFile[100];
	bool bLoadFile = false;
	bool bLoaded = false;
	int iCount = 0;

	char sTitle[100];

	bool bLoadedFile[5];

	ClearBuyMenus() ;

	for ( int i = 0; i <= 1; i++ )
	{
		if ( i == 0 )
		{
			Q_snprintf( sMap, sizeof( sMap ), "%s/", g_CurrentMap );
		}
		else
		{
			sMap[0] = '\0';
		}
		Q_snprintf(sFolder, sizeof( sFolder ), "cfg/zombiemod/buymenu/%s", sMap );
		if ( !bLoaded ) // Loaded map specifc, continue.
		{
			#define SETBVAL( sTitle, sValue, iMenu, iIndex  ) \
				switch ( iMenu ) \
				{ \
					case 0: \
						if (iIndex > 0) g_tBuyMenu.bPistols = true; \
						g_tBuyMenu.sPistols[iIndex].assign(sTitle); \
						g_tBuyMenu.iPistols[iIndex].assign(sValue); \
						break; \
					case 1: \
						if (iIndex > 0) g_tBuyMenu.bSMGs = true; \
						g_tBuyMenu.sSMGs[iIndex].assign(sTitle); \
						g_tBuyMenu.iSMGs[iIndex].assign(sValue); \
						break; \
					case 2: \
						if (iIndex > 0) g_tBuyMenu.bRifles = true; \
						g_tBuyMenu.sRifles[iIndex].assign(sTitle); \
						g_tBuyMenu.iRifles[iIndex].assign(sValue); \
						break; \
					case 3: \
						if (iIndex > 0) g_tBuyMenu.bMGs = true; \
						g_tBuyMenu.sMGs[iIndex].assign(sTitle); \
						g_tBuyMenu.iMGs[iIndex].assign(sValue); \
						break; \
					default: \
						if (iIndex > 0) g_tBuyMenu.bEquip = true; \
						g_tBuyMenu.sEquip[iIndex].assign(sTitle); \
						g_tBuyMenu.iEquip[iIndex].assign(sValue); \
						break; \
				}
				

			for ( int x = 0; x <= 4; x++ )
			{
				if ( i == 0 )
				{
					bLoadedFile[x] = false;
				}
				sFile[0] = '\0';
				switch ( x )
				{
					case 0:
						Q_strncpy( sFile, "pistol.cfg", sizeof( sFile ) );
						break;
					case 1:
						Q_strncpy( sFile, "smg.cfg", sizeof( sFile ) );
						break;
					case 2:
						Q_strncpy( sFile, "rifle.cfg", sizeof( sFile ) );
						break;
					case 3:
						Q_strncpy( sFile, "mg.cfg", sizeof( sFile ) );
						break;
					default:
						Q_strncpy( sFile, "equip.cfg", sizeof( sFile ) );
						break;
				}
				sFilePath[0] = '\0';
				Q_snprintf( sFilePath, sizeof( sFilePath ), "%s%s", sFolder, sFile );

				if ( !m_FileSystem->FileExists( sFilePath, "MOD" ) )
				{
					if ( i == 1 && !bLoadedFile[x])
					{
						META_LOG( g_PLAPI, "ERROR: LoadBuyMenus() - Unable to find file '%s' - skipping.", sFile );
					}
					bLoadFile = false;
				}
				else
				{
					bLoadFile = true;
				}

				if ( bLoadFile )
				{
					bLoaded = true;
					KeyValues *kv = new KeyValues( sFilePath );
					KeyValues *t;
					if ( kv )
					{
						if ( kv->LoadFromFile( m_FileSystem, sFilePath, "MOD" ) )
						{
							t = kv;
							KeyValues *p;
							Q_strncpy( sTitle, t->GetName(), sizeof( sTitle ) );
							SETBVAL( sTitle, t->GetName(), x, 0 );
							p = t->GetFirstSubKey();
							iCount = 0;
							while ( p )
							{
								iCount++;
								if ( iCount > 8 ) 
								{
									META_LOG( g_PLAPI, "ERROR: LoadBuyMenus() - Too many weapons in '%s' - clamping. Max weapons per menu is 8.", sFilePath );
								}
								else
								{
									SETBVAL( p->GetString(), p->GetName(), x, iCount );
								}
								p = p->GetNextKey();
							}
							bLoadedFile[x] = true;
							META_LOG( g_PLAPI, "LoadBuyMenus() - Successfully loaded menu file '%s'", sFilePath );
						}
						else
						{
							META_LOG( g_PLAPI, "ERROR: LoadBuyMenus() - Unable to load file '%s'.", sFilePath );
						}
					}
					else
					{
						bLoadedFile[x] = false;
						META_LOG( g_PLAPI, "ERROR: LoadBuyMenus() - File is corrupt or not enough permissiosn '%s'.", sFilePath );
					}
					kv->deleteThis();
					
				}
			}
		}
	}
	for ( int x = 0; x <= 4; x++ )
	{
		if ( bLoadedFile[x] )
		{
			return true;
		}
	}
	return false;
}

bool ZombiePlugin::LoadLanguages( )
{
	char sFilePath[255];
	if ( kLanguage ) 
	{
		kLanguage->deleteThis();
		kLanguage = NULL;
	}
	
	if ( Q_strstr( zombie_language_file.GetString(), ".cfg" ) )
	{
		Q_snprintf( sFilePath, sizeof( sFilePath ), "cfg/zombiemod/language/%s", zombie_language_file.GetString() );
	}
	else
	{
		Q_snprintf( sFilePath, sizeof( sFilePath ), "cfg/zombiemod/language/%s.cfg", zombie_language_file.GetString() );
	}

	if ( !m_FileSystem->FileExists( sFilePath, "MOD" ) )
	{
		META_LOG( g_PLAPI, "ERROR: LoadLanguages()  Unable to find file '%s' - Reverting to english.cfg", sFilePath );
		Q_strncpy( sFilePath, "cfg/zombiemod/language/english.cfg", 255 );
		if ( !m_FileSystem->FileExists( sFilePath, "MOD" ) )
		{
			META_LOG( g_PLAPI, "ERROR: LoadLanguages()  Unable to find default file '%s' - disabling languages.", sFilePath );
			return false;
		}
	}

	KeyValues *kv = new KeyValues( sFilePath );
	KeyValues *t;
	if ( kv )
	{
		if ( kv->LoadFromFile( m_FileSystem, sFilePath, "MOD" ) )
		{
			t = kv;
			KeyValues *p;
			p = t->GetFirstSubKey();
			if ( p )
			{
				kLanguage = t;
				t = NULL;
				p = NULL;
				kv = NULL;
				bLanguagesLoaded = true;
				META_LOG( g_PLAPI, "Successfully loaded language file '%s'", sFilePath );
			}
		}
		else
		{
			META_LOG( g_PLAPI, "ERROR: LoadLanguages() - Unable to load file '%s'", sFilePath );
			return false;
		}
	}
	return true;
}

bool ZombiePlugin::LoadZombieModelList( const char *filename, const char *downloadfile )
{
	m_Engine->PrecacheModel( "models/zombie/classic.mdl" );
	FileHandle_t file = m_FileSystem->Open(filename, "r");
	if (!file)
	{
		return false;
	}
	int lineno = 0;
	g_ZombieModels.Purge();
	while (!m_FileSystem->EndOfFile(file)) 
	{
		char line[1024];
		char File[128];
		lineno++;
		if (!m_FileSystem->ReadLine(line, sizeof(line), file))
		{
			break;
		}
		int l = strlen(line);
		int c = 0;
		int x = 0;
		
		GET_CFG_ENDPARAM(File, MAX_SERVERCMD_LENGTH);

		#define TRIMWHITE( sString ) \
			int iL = Q_strlen( sString ) - 1; \
			if ( iL > 0 ) \
			{ \
				while ( sString[iL] == ' ' ) \
				{ \
					sString[iL] = '\0'; \
					iL--; \
				} \
			}

		TRIMWHITE( File );
		FixSlashes(File);
		const char *sExts[] = { ".dx80.vtx", ".dx90.vtx", ".phy", ".sw.vtx", ".vvd", ".mdl" };
		for (int y = 0; y <= 5; y++ )
		{
			char buf[128];
			Q_snprintf( buf, sizeof(buf), "%s%s", File, sExts[y] );
			if ( !m_FileSystem->FileExists( buf ) )
			{
				META_LOG( g_PLAPI, "ERROR: Zombie model file %s does not exist !", buf );
			}
			else
			{
				AddDownload( buf );
				if ( y == 5 )
				{
					META_CONPRINTF( "[ZOMBIE] Adding Model:%s\n", buf );
					//META_LOG( g_PLAPI, "Adding Model:%s", buf );
					int id = g_ZombieModels.AddToTail();
					g_ZombieModels[id].iPrecache = m_Engine->PrecacheModel( buf );
					g_ZombieModels[id].sModel.assign( buf );
					g_ZombieModels[id].bHasHead = LoadZombieHeads( id, buf );
				}
			}
		}
	}
	m_FileSystem->Close(file);

	file = m_FileSystem->Open(downloadfile, "r");
	if (!file)
	{
		return false;
	}
	lineno = 0;
	while (!m_FileSystem->EndOfFile(file)) 
	{
		char line[1024];
		char File[128];
		lineno++;
		if (!m_FileSystem->ReadLine(line, sizeof(line), file))
		{
			break;
		}
		int l = strlen(line);
		int c = 0;
		int x = 0;
		
		GET_CFG_ENDPARAM( File, MAX_SERVERCMD_LENGTH );

		TRIMWHITE( File );

		FixSlashes( File );
        if ( ! m_FileSystem->FileExists( File ) )
		{
			META_LOG( g_PLAPI, "ERROR: Zombie download '%s' does not exist !", File );
        }
		else
		{
			AddDownload( File );
        }
	}
	m_FileSystem->Close( file );
	return true;
}

bool ZombiePlugin::LoadZombieHeads( int iIndex, char *sModel )
{
	const char *sExts[] = { ".dx80.vtx", ".dx90.vtx", ".phy", ".sw.vtx", ".vvd", ".mdl" };
	char sModelName[200];
	char sModelHead[200];
	char sModelHeadless[200];
	Q_StripExtension( sModel, sModelName, sizeof( sModelName ) );
	Q_snprintf( sModelHeadless, sizeof(sModelHeadless), "%s_hs", sModelName );
	Q_snprintf( sModelHead, sizeof(sModelHead), "%s_head", sModelName );
	int x = 0, y = 0;
	for ( y = 0; y <= 1; y++ )
	{
		for ( x = 0; x <= 5; x++ )
		{
			char buf[128];
			if ( y == 0 )
			{
				Q_snprintf( buf, sizeof(buf), "%s%s", sModelHeadless, sExts[x] );
			}
			else
			{
				Q_snprintf( buf, sizeof(buf), "%s%s", sModelHead, sExts[x] );
			}
			if ( !m_FileSystem->FileExists( buf ) )
			{
				if ( y == 0 )
				{
					META_LOG( g_PLAPI, "ERROR: Zombie headless model %s does not exist !", buf );
					return false;
				}
				else
				{
					META_LOG( g_PLAPI, "ERROR: Zombie head model %s does not exist !", buf );
					return false;
				}
			}
			else
			{
				AddDownload( buf );
				if ( x == 5 )
				{
					if ( y == 0 )
					{
						META_CONPRINTF( "[ZOMBIE] Adding HeadLess Model:%s\n", buf );
						m_Engine->PrecacheModel( buf );
						g_ZombieModels[iIndex].sHeadLess.assign( buf );
					}
					else
					{
						META_CONPRINTF( "[ZOMBIE] Adding Head Model:%s\n", buf );
						g_ZombieModels[iIndex].iHeadPrecache = m_Engine->PrecacheModel( buf );
						g_ZombieModels[iIndex].sHead.assign( buf );
					}
				}
			}
		}
	}
	return true;
}

void ZombiePlugin::ZombieOff() 
{
	if ( !zombie_enabled.GetBool() )
	{
		return;
	}
	bUsingCVAR = true;
	zombie_enabled.SetValue(0);
	g_ZombiePlugin.DoAmmo( false );
	bUsingCVAR = false;
	if ( zombie_dark.GetInt() > 0 )
	{
		m_Engine->LightStyle(0, "m");
	}

	mp_spawnprotectiontime->SetValue( g_oldSpawnProtectionTime );
	mp_tkpunish->SetValue( g_oldTKPunish );
	//sv_alltalk->SetValue( g_oldAlltalkValue );
	
	for (int x = 0; x < CSW_MAX; x++)
	{
		g_RestrictT[x] = -1;
		g_RestrictCT[x] = -1;
	}
	FFA_Disable();
	if ( zombie_dark.GetInt() > 0 )
	{
		m_Engine->ChangeLevel( g_CurrentMap, NULL );
	}
}

void ZombiePlugin::ZombieOn()
{
	if ( zombie_enabled.GetBool() )
	{
		return;
	}
	

	g_oldFFValue = mp_friendlyfire->GetInt();
	g_oldSpawnProtectionTime = mp_spawnprotectiontime->GetInt();
	g_oldTKPunish = mp_tkpunish->GetInt();
	//g_oldAlltalkValue = sv_alltalk->GetInt();
	g_oldAmmoBuckshotValue = ammo_buckshot_max->GetInt();
	
	bUsingCVAR = true;
	zombie_enabled.SetValue(1);
	g_ZombiePlugin.DoAmmo( zombie_unlimited_ammo.GetBool() );
	bUsingCVAR = false;
	
	for ( int x = 1; x <= MAX_CLIENTS; x++ )
	{
		edict_t *pEntity = NULL;
		if ( IsValidPlayer( x, &pEntity ) )
		{
			m_Engine->ClientCommand( pEntity, "cl_minmodels 0" );
		}
		g_Players[x].isZombie = false;
	}
	if ( zombie_dark.GetInt() > 0 )
	{
	    //*g_pGameOver = true;
	    m_Engine->ChangeLevel( g_CurrentMap, NULL );
	}
	else
	{
	    g_ZombieRoundOver = true;
	    ZombieLevelInit( NULL );
		g_Timers->RemoveTimer( RANDOM_ZOMBIE_TIMER_ID );
		float fRandom = RandomFloat(zombie_timer_min.GetFloat(), zombie_timer_max.GetFloat());
		//META_LOG( g_PLAPI, "Random zombie in %f.", fRandom );
		g_Timers->AddTimer( fRandom, RandomZombie, NULL, 0, RANDOM_ZOMBIE_TIMER_ID );
	    g_Timers->AddTimer( 0.5, CheckZombies, NULL, 0, ZOMBIECHECK_TIMER_ID );
	}
}
void ZombieLevelInit( void **params )
{
	if ( zombie_enabled.GetBool() )
	{
		if ( zombie_dark.GetInt() == 1 )
		{
			m_Engine->LightStyle( 0, "a" );
		}
		else if ( zombie_dark.GetInt() == 2 )
		{
			m_Engine->LightStyle( 0, "b" );
		}
		g_ZombiePlugin.DoAmmo( zombie_unlimited_ammo.GetBool() );
		
		if ( zombie_ffa_enabled.GetBool() )
		{
			FFA_Enable();
		}

		META_LOG( g_PLAPI, "g_EntList = [%p]", g_EntList );

		mp_friendlyfire->SetValue( 0 );
		m_Engine->ServerCommand( "mp_flashlight 1\n" );
		m_Engine->ServerCommand( "mp_limitteams 0\n" );
		mp_spawnprotectiontime->SetValue(0);
		mp_tkpunish->SetValue(0);
		sv_forcepreload->SetValue(1);
		//sv_alltalk->SetValue(1);
		ammo_buckshot_max->SetValue(128);
		//mp_friendlyfire->SetValue(0);
		g_ZombiePlugin.RestrictWeapons();
	}
}

/*
bool ZombiePlugin::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex)
{
	RETURN_META_VALUE(MRES_IGNORED, true);
	if ( zombie_enabled.GetBool() )
	{
		edict_t *pEdict = m_GameEnts->BaseEntityToEdict( pWeapon );
		CBasePlayer *pPlayer = META_IFACEPTR( CBasePlayer );
		edict_t *pEdPlayer = m_GameEnts->BaseEntityToEdict( pPlayer );
		if ( pWeapon )
		{
			int x;
			for ( x = 1; x <= MAX_CLIENTS; x++ )
			{
				if ( g_Players[x].pKnife == pWeapon ) 
				{
					if ( g_Players[x].fOriginalKnifeSpeed < 0 )
					{
						UTIL_GetProperty( g_Offsets.m_flLaggedMovementValue, pEdPlayer, &g_Players[x].fOriginalKnifeSpeed);
					}
					if ( g_Players[x].isZombie )
					{
						if ( g_bZombieClasses && zombie_classes.GetBool() && g_Players[x].iClass > -1 && g_Players[x].iClass <= g_iZombieClasses )
						{
							if ( g_Players[x].fOriginalKnifeSpeed <= 0 )
							{
								g_Players[x].fOriginalKnifeSpeed = 1;
							}
							//float fSpeed = ( (float)g_ZombieClasses[ g_Players[x].iClass ].iSpeed / 100 ) * (float)zombie_speed_percent.GetInt();
							float fSpeed = ( (float)g_ZombieClasses[ g_Players[x].iClass ].iSpeed / 100 ) * (float)zombie_speed_percent.GetInt() / (g_Players[x].fOriginalKnifeSpeed * 100);

							//char sTmp[50];
							//Q_snprintf( sTmp, sizeof(sTmp), "%d", iSpeed );
							//CBaseEntity_KeyValue( pPlayer, "speed", sTmp );
							UTIL_SetProperty( g_Offsets.m_flLaggedMovementValue, pEdPlayer, fSpeed);
							RETURN_META_VALUE( MRES_HANDLED, true );
						}
						else
						{
							float fSpeed = zombie_speed.GetFloat();
							UTIL_SetProperty( g_Offsets.m_flLaggedMovementValue, pEdPlayer, fSpeed);
							RETURN_META_VALUE( MRES_HANDLED, true );
						}
					}
					else
					{
						float fCurrentSpeed;
						UTIL_GetProperty( g_Offsets.m_flLaggedMovementValue, pEdPlayer, &fCurrentSpeed);
						float fDifference = ( g_Players[x].fOriginalKnifeSpeed - fCurrentSpeed );
						if (!( fDifference > -0.01 && fDifference < 0.01 ))
						{
							UTIL_SetProperty( g_Offsets.localdata + g_Offsets.m_flLaggedMovementValue, pEdPlayer, &g_Players[x].fOriginalKnifeSpeed);
							RETURN_META_VALUE( MRES_HANDLED, true );
						}
					}
				}
			}
		}
		RETURN_META_VALUE(MRES_IGNORED, true);
	}
	else
	{
		RETURN_META_VALUE(MRES_IGNORED, true);
	}
	RETURN_META_VALUE(MRES_IGNORED, true);
}*/

void ZombiePlugin::RestrictWeapons()
{
	for (int x = 0; x < CSW_MAX; x++)
	{ 
		g_RestrictT[x] = -1;
		g_RestrictCT[x] = -1;
		g_RestrictW[x] = -1;
	}
	if ( strlen( zombie_restrictions.GetString() ) )
	{
		char buf[128];
		char tmp[100];
		int x = 0;
		const char *str = zombie_restrictions.GetString();
		int len = strlen(str);
		sRestrictedWeapons.clear();
		while (x < len) 
		{
			for (int q = 0; q < 20; q++)
			{
				tmp[x] = 0;
			}
			int c = 0;
			while (x < len && str[x] != ' ')
			{
				tmp[c] = str[x];
				c++;
				x++;
				if (c >= 19)
				{
					break;
				}
			}
			tmp[c] = 0;
			if ( strlen( tmp ) )
			{
				Q_snprintf( buf, sizeof(buf), "zombie_restrict a %s", tmp );
				RestrictWeapon( "a", tmp, "", buf );
				//m_Engine->ServerCommand( buf );
			}
			x++;
		}
	}
	return;
}

bool ZombiePlugin::CanUseWeapon( CBaseEntity* pPlayer, CBaseEntity *pEnt, bool bDrop )
{
	if ( !pEnt || !pPlayer )
	{
		return true;
	}
	edict_t *pwEdict = m_GameEnts->BaseEntityToEdict( pEnt );
	int iPlayer = 0;
	if ( !pwEdict || pwEdict->IsFree() || !pwEdict->GetClassName() || !IsValidPlayer( pPlayer, &iPlayer ) )
		return true;

	const char *szWeapon = pwEdict->GetClassName();
	if ( Q_strncmp( szWeapon, "weapon_", 7 ) )
	{
		return true;
	}

	if ( iPlayer <= MAX_PLAYERS )
	{
		if ( Q_stristr( szWeapon, "c4" ) || g_Players[iPlayer].isZombie && !FStrEq( szWeapon, "weapon_knife" )  )
		{
			return false;
		}
	}
	
	if ( !zombie_allow_picked_up_weapons.GetBool() )
	{
		CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pEnt;
		if ( pWeapon )
		{
			int nCount;
			int iID = LookupBuyID( szWeapon );
			if ( !AllowWeapon( (CBasePlayer*)pPlayer, iID, nCount ) )
			{
				if ( bDrop )
				{
					UTIL_WeaponDrop( (CBasePlayer*)pPlayer, pWeapon, NULL, NULL );
				}
				return false;
			}
		}
	}
	return true;
}


bool ZombiePlugin::AllowWeapon( CBasePlayer *pPlayer, int WeaponID, int &nCount )
{
	int iPlayer;
	if ( !IsValidPlayer( pPlayer, &iPlayer ) || WeaponID == CSW_NONE )
		return true;
	if ( WeaponID == CSW_C4 )
	{
		return false;
	}
	if ( iPlayer < 1 || iPlayer > MAX_PLAYERS )
	{
		return false;
	}
	/*if ( !g_Players[iPlayer].isZombie && WeaponID == CSW_NVGS )
	{
		return false;
	}*/
	int teamid = GetTeam( m_GameEnts->BaseEntityToEdict( pPlayer ) );
	if ( (teamid != TERRORISTS) && (teamid != COUNTERTERRORISTS))
	{
		return false;
	}
	bool bAllow = true;
 	if (teamid == TERRORISTS)
	{
		if ( g_RestrictT[WeaponID] == 0 )
		{
			bAllow = false;
		}
	}
	else
	{
		if (g_RestrictCT[WeaponID] == 0)
		{
			bAllow = false;
		}
	}
	return bAllow;
}

void ZombiePlugin::CheckAutobuy( int nIndex, CBasePlayer* pPlayer )
{
	if (nIndex < 1 || nIndex > MAX_CLIENTS )
	{
		return;
	}
	char *autobuy = (char *)m_Engine->GetClientConVarValue(nIndex, "cl_autobuy");
	int len = strlen(autobuy);
	char word[32];
	memset(word, 0, sizeof(word));
	int x = 0;
	int c = 0;
	while (x < len)
	{
		bool added = false;
		if (autobuy[x] != ' ' && autobuy[x] != '\t') 
		{
			word[c] = autobuy[x];
			c++;
			added = true;
		}
		if (!added || ((x+1) >= len) || (c >= 31)) {
			word[c] = 0;
			if (c > 0)
			{
				int WeaponID = LookupBuyID( word );
				if ( WeaponID )
				{
					int nCount = 0;
					if ( ! AllowWeapon(pPlayer, WeaponID, nCount)) 
					{
						int start = x - c;
						for (int z = 0; z < c; z++) {
							autobuy[start + z] = ' ';
						}
					}
				}
			}
			c = 0;
			memset(word, 0, sizeof(word));
		}
		x++;
	}
	autobuy = (char *)m_Engine->GetClientConVarValue(nIndex, "cl_rebuy");
	len = strlen(autobuy);
	memset(word, 0, sizeof(word));
	x = 0;
	c = 0;
	while (x < len) {
		bool added = false;
		if (autobuy[x] != ' ' && autobuy[x] != '\t') {
			word[c] = autobuy[x];
			c++;
			added = true;
		}
		if (!added || ((x+1) >= len) || (c >= 31)) {
			word[c] = 0;
			if (c > 0) {
				int WeaponID = LookupRebuyID(word);
				if (WeaponID) {
					int nCount = 0;
					if (!AllowWeapon(pPlayer, WeaponID, nCount)) {
						int start = x - c;
						for (int z = 0; z < c; z++) {
							autobuy[start + z] = ' ';
						}
					}
				}
			}
			c = 0;
			memset(word, 0, sizeof(word));
		}
		x++;
	}		
}

/*CBaseCombatWeapon* UTIL_WeaponSlot(CBaseCombatCharacter *pCombat, int slot)
{

	CBaseCombatWeapon *r = NULL;
	#ifdef WIN32
		__asm {
			push ecx;
			mov ecx, pCombat;
			push slot;
			call g_WeaponSlotFunc;
			mov r, eax;
			pop ecx;
		};
	#else
		r = (g_WeaponSlotFunc)(pCombat, slot);
	#endif

	return r;

}*/

char* UTIL_GetName(CBaseCombatWeapon *pWeapon)
{

	char *r = NULL;
	#ifdef WIN32
		__asm {
			push ecx;
			mov ecx, pWeapon;
			call g_GetNameFunc;
			mov r, eax;
			pop ecx;
		};
	#else
		r = (g_GetNameFunc)(pWeapon);
	#endif

	return r;

}
void UTIL_WeaponDrop( CBasePlayer *pBasePlayer, CBaseCombatWeapon *pWeapon, const Vector *target, const Vector *velocity )
{
	#ifdef WIN32
		__asm {
			push ecx;
			mov ecx, pBasePlayer;
			push velocity;
			push target;
			push pWeapon;
			call g_WeaponDropFunc;
			pop ecx;
		};
	#else
		(g_WeaponDropFunc)(pBasePlayer, pWeapon, target, velocity);
	#endif
}

CBaseEntity	*UTIL_GiveNamedItem( CBasePlayer *pPlayer, const char *pszName, int iSubType )
{
	#ifdef WIN32
		__asm
		{
			push ecx;
			mov ecx, pPlayer;
			push iSubType;
			push pszName;
			call g_GiveNamedItemFunc;
			pop ecx;
		};
	#else
		( g_GiveNamedItemFunc )( pPlayer, pszName, iSubType );
	#endif
}

//----------------------------------------------------------
//									FindEntityByClassName
//----------------------------------------------------------
CBaseEntity *UTIL_FindEntityByClassName( CGlobalEntityList *EntList, CBaseEntity *pStartEntity, const char *szName )
{
	CBaseEntity *pTmp;
	#ifdef WIN32
		__asm
		{
			push ecx;
			mov ecx, EntList;
			push szName;
			push pStartEntity;
			call g_FindEnt;
			mov pTmp, eax;
			pop ecx;
		};
	#else
		pTmp = (g_FindEnt)(EntList, pStartEntity, szName );
	#endif
	return pTmp;
}

/*
union
{
	bool	(vEmptyClass::*mfpnew)( CBaseEntity *, const char * );
	void	*addr;
} u_FindEntity;	

CBaseEntity *FindEntityByClassName( CGlobalEntityList *EntList, CBaseEntity *pStartEntity, const char *szName )
{
	void* funcptr = v_FindEntity;
	void* thisptr = EntList;
	u_FindEntity.addr = funcptr;
	return (CBaseEntity *)(reinterpret_cast<vEmptyClass*>(thisptr)->*u_FindEntity.mfpnew)( pStartEntity, szName );
}
*/
//----------------------------------------------------------
//									KeyValue
//----------------------------------------------------------
//union
//{
//	bool	(vEmptyClass::*mfpnew)( const char *, const char * );
//	void	*addr;
//} u_KeyValue;	
//
//bool CBaseEntity_KeyValues( CBaseEntity *pBaseEntity, const char *szKeyName, const char *szValue )
//{
//	void* funcptr = v_KeyValues;
//	void* thisptr = pBaseEntity;
//	u_KeyValue.addr = funcptr;
//	return (bool)(reinterpret_cast<vEmptyClass*>(thisptr)->*u_KeyValue.mfpnew)( szKeyName, szValue );
//}


CCollisionProperty *UTIL_GetCollision( edict_t *pEntity )
{
	CCollisionProperty *cCollission = NULL;
	if ( UTIL_GetProperty( g_ZombiePlugin.g_Offsets.m_Collision, pEntity, &cCollission ) )
	{
		return cCollission;
	}
	else
	{
		return NULL;
	}

}

//----------------------------------------------------------
//									IsInBuyZone
//----------------------------------------------------------
union
{
	bool	(vEmptyClass::*mfpnew)( );
	void	*addr;
} u_IsInBuyZone;	

bool CCSPlayer_IsInBuyZone( CBasePlayer *pPlayer )
{
	bool bInBuyZone = false;
	edict_t *pEntity = m_GameEnts->BaseEntityToEdict( (CBaseEntity *)pPlayer );
	if ( UTIL_GetProperty( g_ZombiePlugin.g_Offsets.m_bInBuyZone, pEntity, &bInBuyZone ) )
	{
		return bInBuyZone;
	}
	else
	{
		return false;
	}

	/*
	void* funcptr = v_IsInBuyZone;
	void* thisptr = pPlayer;
	u_IsInBuyZone.addr = funcptr;
	return (bool)(reinterpret_cast<vEmptyClass*>(thisptr)->*u_IsInBuyZone.mfpnew)( );
	*/
}

void CBaseEntity_Teleport( CBaseEntity *pThisPtr, Vector *newPosition, QAngle *newAngles, Vector *newVelocity )
{
	Teleport( (CBasePlayer*)pThisPtr, newPosition, newAngles, newVelocity );
}

//----------------------------------------------------------
//									ApplyAbsVelocityImpulse
//----------------------------------------------------------
union
{
	void	(vEmptyClass::*mfpnew)( const Vector & );
	void	*addr;
} u_ApplyAbsVelocity;	

void CBasePlayer_ApplyAbsVelocityImpulse( CBasePlayer *pPlayer, Vector &vecImpulse )
{
	/*
	edict_t *pEdict = m_GameEnts->BaseEntityToEdict( pPlayer );
	if ( pEdict )
	{

		//UTIL_SetProperty( g_ZombiePlugin.g_Offsets.localdata + g_ZombiePlugin.g_Offsets.m_vecBaseVelocity, pEdict, vecImpulse );
		ICollideable *iCol = CBaseEntity_GetCollideable( pPlayer );
		Vector vPos = iCol->GetCollisionOrigin();
		QAngle vAng = iCol->GetCollisionAngles();

		//CBaseEntity_Teleport( pPlayer, &vPos, &vAng, &vecImpulse );
		//g_Offsets.m_vecBaseVelocity
		Teleport( pPlayer, &vPos, &vAng, &vecImpulse );
	}
	*/

	void* funcptr = v_ApplyAbsVelocity;
	void* thisptr = pPlayer;
	u_ApplyAbsVelocity.addr = funcptr;
	(reinterpret_cast<vEmptyClass*>(thisptr)->*u_ApplyAbsVelocity.mfpnew)( vecImpulse );
}

//----------------------------------------------------------
//									SwitchTeam
//----------------------------------------------------------
union
{
	void	(vEmptyClass::*mfpnew)( int );
	void	*addr;
} u_SwitchTeam;

void CBasePlayer_SwitchTeam( CBasePlayer *pPlayer, int iTeam, bool bIgnorePlayerCount )
{
	if ( !IsBadPtr( mp_limitteams ) && mp_limitteams->GetInt() != 0 )
	{
		mp_limitteams->SetValue( 0 );
	}
	if ( !IsBadPtr( mp_autoteambalance ) && mp_autoteambalance->GetInt() != 0 )
	{
		mp_autoteambalance->SetValue( 0 );
	}
	int iPlayer;
	if ( !IsValidPlayer( pPlayer, &iPlayer ) )
		return;
	
	CTeam *pTeam = ( iTeam == COUNTERTERRORISTS ? teamT : teamCT );
	if ( !bIgnorePlayerCount && pTeam && UTIL_GetNumPlayers( pTeam ) <= 1 )
		//if ( pTeam && UTIL_GetNumPlayers( pTeam ) <= 1 )
		{
			return;
		}
	void* funcptr = v_SwitchTeam;
	void* thisptr = pPlayer;
	u_SwitchTeam.addr = funcptr;
	g_Players[iPlayer].isChangingTeams = true;
	(reinterpret_cast<vEmptyClass*>(thisptr)->*u_SwitchTeam.mfpnew)( iTeam );
}
//----------------------------------------------------------
//									SetFOV
//----------------------------------------------------------
union
{
	bool	(vEmptyClass::*mfpnew)( CBaseEntity *, int, float );
	void *addr;
} u_SetFOV;												//CBasePlayer::SetFOV

bool CBasePlayer_SetFOV( CBasePlayer *pPlayer, CBaseEntity *pRequester, int FOV, float zoomRate )
{
	int iPlayer;
	if ( !IsValidPlayer( pPlayer, &iPlayer ) )
		return false;
	if ( !g_Players[iPlayer].isBot )
	{
		void* funcptr = v_SetFOV;
		void* thisptr = pPlayer;
		u_SetFOV.addr = funcptr;
		return (bool)(reinterpret_cast<vEmptyClass*>(thisptr)->*u_SetFOV.mfpnew)( pRequester, FOV, zoomRate );
	}
	return false;
}

void UTIL_RestartRound( CBasePlayer *pPlayer )
{
	#ifdef WIN32
		__asm
		{
			push ecx;
			mov ecx, pPlayer;
			call g_CCSPlayer_RoundRespawn;
			pop ecx;
		};
	#else
		( g_CCSPlayer_RoundRespawn )( pPlayer );
	#endif
}

void UTIL_TermRound(float delay, int reason)
{
	if ( bRoundTerminating )
	{
		return;
	}
	void *rules = NULL;  
	//memcpy(&rules, reinterpret_cast<void*>(m_pGameRules),sizeof(char*));  //0x0B9023C0 from this scan (Dec1 update)
	memcpy(&rules, reinterpret_cast<void*>(g_gamerules_addr),sizeof(char*));  //0x0B9023C0 from this scan (Dec1 update)
	#ifdef WIN32
		__asm
		{
			push ecx;
			mov ecx, rules;
			push reason;
			push delay;
			call g_TermRoundFunc;
			pop ecx;
		};
	#else
		(g_TermRoundFunc)(rules, delay, reason);
	#endif
}

int UTIL_GetNumPlayers( CTeam *pTeam )
{
	if ( TeamCount )
	{
		return TeamCount( pTeam, 0 );
	}
	return 0;
}
#ifndef WIN32
void UTIL_SetCollisionBounds( CBaseEntity *pBase, const Vector& mins, const Vector &maxs )
{
	
}
#endif

void UTIL_SetModel( CBaseEntity *pEntity, const char *pModelName )
{
	//(g_SetModel)( pEntity, pModelName );

	CBasePlayer_SetModel( (CBasePlayer *)pEntity, pModelName );	
#ifdef WIN32
	//META_CONPRINTF( "-16 -16 0: %f %f %f",  CGameRules_GetViewVectors( NULL )->m_vHullMin.x,  CGameRules_GetViewVectors( NULL )->m_vHullMin.y,  CGameRules_GetViewVectors( NULL )->m_vHullMin.z );
	(g_SetMinMaxSize)( pEntity, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );
#else
	(g_SetCollisionBounds)( pEntity, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );
#endif
}

int GetUnknownInt(edict_t *pEdict, int offset)
{
	if (offset < 1)
		return 0;

	return *((int*)pEdict->GetUnknown() + offset);
}

CBaseEntity *UTIL_GetPlayerRagDoll( edict_t *pEntity )
{
	if ( !pEntity || pEntity->IsFree() )
	{
		return NULL;
	}
	CBaseHandle pHandle;
	if ( UTIL_GetProperty( g_ZombiePlugin.g_Offsets.m_hRagdoll, pEntity, &pHandle ) )
	{
		if ( pHandle.GetEntryIndex() > 0 )
		{
			edict_t *pEnt;
			pEnt = m_Engine->PEntityOfEntIndex( pHandle.GetEntryIndex() );
			if ( pEnt && !pEnt->IsFree() )
			{
				CBaseEntity *pRagDoll = m_GameEnts->EdictToBaseEntity( pEnt ); //->GetUnknown()->GetBaseEntity();
				if ( pRagDoll )
				{
					return pRagDoll;
				}
				/*
				else
				{
					META_LOG( g_PLAPI, "ERROR: Couldnt get BaseEntity for ragdoll." );
				}*/
			}
		}
		/*
		else
		{
			META_LOG( g_PLAPI, "ERROR: Couldnt get EntryIndex for ragdoll." );
		}
		*/
	}
	/*
	else
	{
		META_LOG( g_PLAPI, "ERROR: Couldnt get pHandle !" );
	}
	*/
	return NULL;
}

CBaseEntity *UTIL_GetPlayerWeapon( edict_t *pEntity )
{
	if ( !pEntity || pEntity->IsFree() )
	{
		return NULL;
	}
	CBaseHandle pHandle;
	if ( UTIL_GetProperty( g_ZombiePlugin.g_Offsets.m_hActiveWeapon, pEntity, &pHandle ) )
	{
		if ( pHandle.GetEntryIndex() > 0 )
		{
			edict_t *pEnt;
			pEnt = m_Engine->PEntityOfEntIndex( pHandle.GetEntryIndex() );
			if ( pEnt && !pEnt->IsFree() )
			{
				CBaseEntity *pWeapon = m_GameEnts->EdictToBaseEntity( pEnt ); //->GetUnknown()->GetBaseEntity();
				if ( pWeapon )
				{
					return pWeapon;
				}
				else
				{
					META_LOG( g_PLAPI, "ERROR: Couldnt get BaseEntity for weapon." );
				}
			}
		}
		else
		{
			META_LOG( g_PLAPI, "ERROR: Couldnt get EntryIndex for weapon." );
		}
	}
	else
	{
		META_LOG( g_PLAPI, "ERROR: Couldnt get pHandle for weapon!" );
	}
	return NULL;
}

int LookupBuyID(const char *name) {
	if (Q_stristr(name, "228")) {
		return CSW_P228;
	} else if (Q_stristr(name, "glock") || Q_stristr(name, "9x19")) {
		return CSW_GLOCK;
	} else if (Q_stristr(name, "scout")) {
		return CSW_SCOUT;
	} else if (Q_stristr(name, "hegren")) {
		return CSW_HEGRENADE;
	} else if (Q_stristr(name, "xm1") || Q_stristr(name, "autoshot")) {
		return CSW_XM1014;
	} else if (Q_stristr(name, "c4")) {
		return CSW_C4;
	} else if (Q_stristr(name, "mac")) {
		return CSW_MAC10;
	} else if (Q_stristr(name, "aug") || Q_stristr(name, "bull")) {
		return CSW_AUG;
	} else if (Q_stristr(name, "smoke")) {
		return CSW_SMOKEGRENADE;
	} else if (Q_stristr(name, "elite")) {
		return CSW_ELITE;
	} else if (Q_stristr(name, "fiveseven") || Q_stristr(name, "fn57")) {
		return CSW_FIVESEVEN;
	} else if (Q_stristr(name, "ump")) {
		return CSW_UMP45;
	} else if (Q_stristr(name, "550")) {
		return CSW_SG550;
	} else if (Q_stristr(name, "galil") || Q_stristr(name, "defend")) {
		return CSW_GALIL;
	} else if (Q_stristr(name, "famas") || Q_stristr(name, "clarion")) {
		return CSW_FAMAS;
	} else if (Q_stristr(name, "usp") || Q_stristr(name, "km45")) {
		return CSW_USP;
	} else if (Q_stristr(name, "awp") || Q_stristr(name, "magn")) {
		return CSW_AWP;
	} else if (Q_stristr(name, "mp5")) {
		return CSW_MP5;
	} else if (Q_stristr(name, "m249")) {
		return CSW_M249;
	} else if (Q_stristr(name, "m3") || Q_stristr(name, "12gauge")) {
		return CSW_M3;
	}
	else if (Q_stristr(name, "m4a"))
	{
		return CSW_M4A1;
	} else if (Q_stristr(name, "tmp")) {
		return CSW_TMP;
	} else if (Q_stristr(name, "g3sg") || Q_stristr(name, "d3au")) {
		return CSW_G3SG1;
	} else if (Q_stristr(name, "flash")) {
		return CSW_FLASHBANG;
	} else if (Q_stristr(name, "deag") || Q_stristr(name, "hawk")) {
		return CSW_DEAGLE;
	} else if (Q_stristr(name, "552")) {
		return CSW_SG552;
	} else if (Q_stristr(name, "47")) {
		return CSW_AK47;
	} else if (Q_stristr(name, "knife")) {
		return CSW_KNIFE;
	} else if (Q_stristr(name, "p90") || Q_stristr(name, "c90")) {
		return CSW_P90;
	} else if (Q_stristr(name, "primammo")) {
		return CSW_PRIMAMMO;
	} else if (Q_stristr(name, "secammo")) {
		return CSW_SECAMMO;
	} else if (Q_stristr(name, "nvg")) {
		return CSW_NVGS;
	} else if (Q_stristr(name, "helm")) {
		return CSW_VESTHELM;
	} else if (Q_stristr(name, "vest")) {
		return CSW_VEST;
	} else if (Q_stristr(name, "defuse") || Q_stristr(name, "kit")) {
		return CSW_DEFUSEKIT; 
	}
	return CSW_NONE;
}

int LookupRebuyID(const char *name) {
	if (Q_stristr(name, "SmokeGrenade"))
	{
		return CSW_SMOKEGRENADE;
	}
	else if (Q_stristr(name, "HEGrenade"))
	{
		return CSW_HEGRENADE;
	}
	else if (Q_stristr(name, "Flashbang"))
	{
		return CSW_FLASHBANG;
	}
	else if (Q_stristr(name, "PrimaryAmmo")) 
	{
		return CSW_PRIMAMMO;
	}
	else if (Q_stristr(name, "SecondaryAmmo")) 
	{
		return CSW_SECAMMO;
	}
	else if (Q_stristr(name, "NightVision")) 
	{
		return CSW_NVGS;
	}
	else if (Q_stristr(name, "Armor"))
	{
		return CSW_VEST;
	}
	else if (Q_stristr(name, "Defuser"))
	{
		return CSW_DEFUSEKIT; 
	}
	return CSW_NONE;
}

const char *LookupWeaponName(int id) {
	switch (id)
	{
        case CSW_P228:
	        return "228 Compact";
        case CSW_GLOCK:
	        return "9x19mm Sidearm";
        case CSW_SCOUT:
	        return "Schmidt Scout";
        case CSW_HEGRENADE:
	        return "HE Grenade";
        case CSW_XM1014:
	        return "Leone YG1265 Auto Shotgun";
        case CSW_C4:
	        return "C4 Bomb";
        case CSW_MAC10:
	        return "Ingram MAC-10";
        case CSW_AUG:
	        return "Bullpup";
        case CSW_SMOKEGRENADE:
	        return "Smoke Grenade";
        case CSW_ELITE:
	        return ".40 Dual Elites";
        case CSW_FIVESEVEN:
	        return "ES Five//Seven";
        case CSW_UMP45:
	        return "KM UMP45";
        case CSW_SG550:
	        return "Krieg 550 Commando";
        case CSW_GALIL:
	        return "IDF Defender";
        case CSW_FAMAS:
	        return "Clarion 5.56";
        case CSW_USP:
	        return "KM .45 Tactical";
        case CSW_AWP:
	        return "Magnum Sniper Rifle";
        case CSW_MP5:
	        return "KM Sub-Machine Gun";
        case CSW_M249:
	        return "M249";
        case CSW_M3:
	        return "Leone 12 Gauge Super";
        case CSW_M4A1:
	        return "Maverick M4A1 Carbine";
        case CSW_TMP:
	        return "Schmidt Machine Pistol";
        case CSW_G3SG1:
	        return "D3/AU1";
        case CSW_FLASHBANG:
	        return "Flashbang";
        case CSW_DEAGLE:
	        return "Night Hawk .50C";
        case CSW_SG552:
	        return "Krieg 552";
        case CSW_AK47:
	        return "CV-47";
        case CSW_KNIFE:
	        return "Knife";
        case CSW_P90:
	     	return "ES C90";
		case CSW_PRIMAMMO:
			return "Primary Ammo";
		case CSW_SECAMMO:
			return "Secondary Ammo";
		case CSW_NVGS:
			return "Nightvision Goggles";
		case CSW_VEST:
			return "Vest";
		case CSW_VESTHELM:
			return "Vest & Helmet";
		case CSW_DEFUSEKIT:
			return "Defusal Kit";
	}
	return "Unknown";
}

float DamageForce(const Vector &size, float damage, float fKnockback)
{
	float force = damage * ((32 * 32 * 72.0) / (size.x * size.y * size.z)) * fKnockback;
	if (force > 1000.0)
	{
			force = 1000.0;
	}
	return force;
}

void FixSlashes( char *str )
{
	if (!str)
	{
		return;
	}
	int len = strlen(str);
	for (int x = 0; x < len; x++)
	{
		if (str[x] == '\\')
		{
			str[x] = '/';
		}
	}
}

size_t DM_StringToBytes(const char *str, unsigned char buffer[], size_t maxlength)
{
	size_t real_bytes = 0;
	size_t length = strlen(str);

	for (size_t i=0; i<length; i++)
	{
		if (real_bytes >= maxlength)
		{
			break;
		}
		buffer[real_bytes++] = (unsigned char)str[i];
		if (str[i] == '\\'
			&& str[i+1] == 'x')
		{
			if (i + 3 >= length)
			{
				continue;
			}
			/* Get the hex part */
			char s_byte[3];
			int r_byte;
			s_byte[0] = str[i+2];
			s_byte[1] = str[i+3];
			s_byte[2] = '\n';
			/* Read it as an integer */
			sscanf(s_byte, "%x", &r_byte);
			/* Save the value */
			buffer[real_bytes-1] = (unsigned char)r_byte;
			/* Adjust index */
			i += 3;
		}
	}

	return real_bytes;
}

void FFA_Disable()
{
	if (!g_ffa)
	{
		return;
	}
	DM_ApplyPatch(g_WantsLagPatch, cWantsLagCompPatch->iPatchOffset, &g_lagcomp_restore, NULL);
	DM_ApplyPatch(g_TakeDamagePatch, cOnTakeDamage->iPatchOffset1, &g_takedmg_restore[0], NULL);
	DM_ApplyPatch(g_TakeDamagePatch, cOnTakeDamage->iPatchOffset2, &g_takedmg_restore[1], NULL);
	DM_ApplyPatch(g_domrev_addr, cCalcDominationAndRevenge->iPatchOffset, &g_domrev_restore, NULL);

	SH_REMOVE_MANUALHOOK_STATICFUNC(CGameRules_IPointsForKill, *g_gamerules_addr, OnIPointsForKill, false);

	META_CONPRINTF( "[ZOMBIE] Free For All has been disabled.\n" );
    g_ffa = false;
}

void DM_ProtectMemory(void *addr, int length, int prot)
{
#ifndef WIN32
	void *addr2 = (void *)ALIGN(addr);
	mprotect(addr2, sysconf(_SC_PAGESIZE), prot);
#else
	DWORD old_prot;
	VirtualProtect(addr, length, prot, &old_prot);
#endif
}

void DM_ApplyPatch(void *address, int offset, const dmpatch_t *patch, dmpatch_t *restore)
{
	unsigned char *addr = (unsigned char *)address + offset;

	DM_ProtectMemory(addr, 20, PAGE_EXECUTE_READWRITE);

	if (restore)
	{
		for (size_t i=0; i<patch->bytes; i++)
		{
			restore->patch[i] = addr[i];
		}
		restore->bytes = patch->bytes;
	}

	for (size_t i=0; i<patch->bytes; i++)
	{
		addr[i] = patch->patch[i];
	}
}

void FFA_Enable()
{
	//return;
    if (g_ffa)
	{
		return;
	}

//#ifdef WIN32
//	const char _hackbuf[] = "\x90\x90";
//	const char _takedmg1[] = "\xEB";
//	const char _takedmg2[] = "\x90\xE9";
//	const char _calcdom[] = "\x90\x90\x90\x90\x90\x90";
//#else
//	const char _hackbuf[] = "\x90\x90\x90\x90\x90\x90";
//	const char _takedmg1[] = "\x90\x90\x90\x90\x90\x90";
//	const char _takedmg2[] = "\x90\x90\x90\x90\x90\x90";
//	const char _calcdom[] = "\x90\x90\x90\x90\x90\x90";
//#endif

	if ( *g_gamerules_addr == NULL )
	{
		META_LOG( g_PLAPI, "Could not find CGameRules!!!" );
		return;
	}
	
	g_takedmg_patch[0].bytes = DM_StringToBytes(cOnTakeDamage->sPatch1, g_takedmg_patch[0].patch, sizeof(g_takedmg_patch[0].patch));
	g_takedmg_patch[1].bytes = DM_StringToBytes(cOnTakeDamage->sPatch2, g_takedmg_patch[1].patch, sizeof(g_takedmg_patch[1].patch));
	g_lagcomp_patch.bytes = DM_StringToBytes(cWantsLagComp->sPatch1, g_lagcomp_patch.patch, sizeof(g_lagcomp_patch.patch));
	g_domrev_patch.bytes = DM_StringToBytes(cCalcDominationAndRevenge->sPatch1, g_domrev_patch.patch, sizeof(g_domrev_patch.patch));

	DM_ApplyPatch(g_WantsLagPatch, cWantsLagComp->iPatchOffset, &g_lagcomp_patch, &g_lagcomp_restore);
	DM_ApplyPatch(g_TakeDamagePatch, cOnTakeDamage->iPatchOffset1, &g_takedmg_patch[0], &g_takedmg_restore[0]);
	DM_ApplyPatch(g_TakeDamagePatch, cOnTakeDamage->iPatchOffset2, &g_takedmg_patch[1], &g_takedmg_restore[1]);
	DM_ApplyPatch(g_domrev_addr, cCalcDominationAndRevenge->iPatchOffset, &g_domrev_patch, &g_domrev_restore);

	SH_ADD_MANUALHOOK_STATICFUNC(CGameRules_IPointsForKill, *g_gamerules_addr, OnIPointsForKill, false);

	META_CONPRINTF( "[ZOMBIE] Free For All has been enabled.\n" );
    g_ffa = true;
}

void CheckRespawn( void **params ) 
{
	bool bZombie = false;
	if ( ( !zombie_respawn.GetBool() && !zombie_respawn_onconnect.GetBool() ) || !g_ZombiePlugin.bRoundStarted )
	{
		return;
	}
	edict_t *pEntity = (edict_t*)params[0];
	int iPlayer;
	if ( !IsValidPlayer( pEntity, &iPlayer ) )
		return;

	for ( int x = 0; x < g_RespawnTimers.Count(); x++ )
	{
		if ( g_RespawnTimers[x].iPlayer == iPlayer )
		{
			g_ZombiePlugin.g_Timers->RemoveTimer( g_RespawnTimers[x].iTimer );
			//g_RespawnTimers[iId].iPlayer = iVic;
			g_RespawnTimers.Remove( x );
			break;
		}
	}

	int team = g_ZombiePlugin.GetTeam( pEntity );
	if ( team == COUNTERTERRORISTS || team == TERRORISTS )
	{
		if ( g_Players[iPlayer].bHasSpawned && !zombie_respawn.GetBool() )
		{
			return;
		}
		if ( zombie_respawn_onconnect_as_zombie.GetBool() && !g_Players[iPlayer].bHasSpawned )
		{
			bZombie = true;
		}
		g_Players[iPlayer].bHasSpawned = true;
		//}

		CBasePlayer_SwitchTeam( g_Players[iPlayer].pPlayer, COUNTERTERRORISTS );
		
		g_Players[iPlayer].bAsZombie = ( bZombie || zombie_respawn_as_zombie.GetBool() );
		
		UTIL_RestartRound( g_Players[iPlayer].pPlayer );
		if ( g_Players[iPlayer].bAsZombie )
		{
			g_ZombiePlugin.MakeZombie( g_Players[iPlayer].pPlayer, zombie_health.GetInt() );
		}
		if ( zombie_respawn_protection.GetFloat() > 0.0f )
		{
			g_ZombiePlugin.SetProtection( iPlayer, true );
		}
		else
		{
			g_ZombiePlugin.GiveWeapons( iPlayer, pEntity );
		}
	}
}

void ZombiePlugin::GiveWeapons( int iPlayer, edict_t *cEntity )
{

	CBaseEntity *pPlayer = NULL;
	if ( !g_Players[iPlayer].isZombie && IsAlive( cEntity ) && IsValidPlayer( iPlayer, &pPlayer ) )
	{

		int iIndex = 0;
		if ( zombie_respawn_dropweapons.GetBool() )
		{
			ZombieDropWeapons( (CBasePlayer *)pPlayer, false, true );
		}

		if ( Q_strlen( zombie_respawn_primary.GetString() ) != 0 )
		{
			UTIL_GiveNamedItem( g_Players[iPlayer].pPlayer, zombie_respawn_primary.GetString(), 0.0 );
			iIndex = GetAmmoIndex( zombie_respawn_primary.GetString() );
			if ( iIndex != 0 )
			{
				//#define PrimAm() CBaseCombatCharacter_GiveAmmo( (CBaseCombatCharacter *)g_Players[iPlayer].pPlayer, 200, GetAmmoIndex( zombie_respawn_primary.GetString() ) ); //GiveNamedItem_Test( g_Players[iPlayer].pPlayer, "primammo", 0.0 );
				//PrimAm();
				//PrimAm();
				GiveAmmo( (CBaseCombatCharacter *)g_Players[iPlayer].pPlayer, 200, zombie_respawn_primary.GetString() );
				GiveAmmo( (CBaseCombatCharacter *)g_Players[iPlayer].pPlayer, 200, zombie_respawn_primary.GetString() );
			}
			else
			{
				META_LOG( g_PLAPI, "%s: %s'%s'.", GetLang("error"), GetLang("prim_index"), zombie_respawn_primary.GetString() ); //No index for primary weapon ammo for weapon
			}
		}
		if ( Q_strlen( zombie_respawn_secondary.GetString() ) != 0 )
		{
			//GiveNamedItem_Test( g_Players[iPlayer].pPlayer, zombie_respawn_secondary.GetString(), 0.0 );
			UTIL_GiveNamedItem( g_Players[iPlayer].pPlayer, zombie_respawn_secondary.GetString(), 0.0 );
			iIndex = GetAmmoIndex( zombie_respawn_secondary.GetString() );
			if ( iIndex != 0 )
			{
				//#define SecAm() CBaseCombatCharacter_GiveAmmo( (CBaseCombatCharacter *)g_Players[iPlayer].pPlayer, 200, GetAmmoIndex( zombie_respawn_secondary.GetString() ) ); //GiveNamedItem_Test( g_Players[iPlayer].pPlayer, "secammo", 0.0 );
				//SecAm();
				//SecAm();
				GiveAmmo( (CBaseCombatCharacter *)g_Players[iPlayer].pPlayer, 200, zombie_respawn_secondary.GetString() );
				GiveAmmo( (CBaseCombatCharacter *)g_Players[iPlayer].pPlayer, 200, zombie_respawn_secondary.GetString() );
			}
			else
			{
				META_LOG( g_PLAPI, "%s: %s '%s'.", GetLang("error"), GetLang("sec_index"), zombie_respawn_secondary.GetString() ); //No index for secondary weapon ammo for weapon
			}
		}
		if ( zombie_respawn_grenades.GetBool() )
		{
			//GiveNamedItem_Test( g_Players[iPlayer].pPlayer, "weapon_hegrenade", 0.0 );
			UTIL_GiveNamedItem( g_Players[iPlayer].pPlayer, "weapon_hegrenade", 0.0 );
		}
		//GiveNamedItem_Test( g_Players[iPlayer].pPlayer, "weapon_knife", 0.0 );
	}
}

void ZombiePlugin::GiveAmmo( CBaseCombatCharacter *pPlayer, int iCount, const char *sWeapon )
{
	CBaseCombatCharacter_GiveAmmo( pPlayer, iCount, GetAmmoIndex( sWeapon ) );
}

int GetAmmoIndex( const char *sWeapon )
{
	int iIndex;
   if ( FStrEq( sWeapon, "weapon_glock" ) )
   {
	  iIndex = 6;

   }
   else if ( FStrEq( sWeapon, "weapon_usp" ) )
   {
	  iIndex = 8;

   }
   else if ( FStrEq( sWeapon, "weapon_p228" ) )
   {
	  iIndex = 9;

   }
   else if ( FStrEq( sWeapon, "weapon_deagle" ) )
   {
	  iIndex = 1;

   }
   else if ( FStrEq( sWeapon, "weapon_elite" ) )
   {
	  iIndex = 6;

   }
   else if ( FStrEq( sWeapon, "weapon_fiveseven" ) )
   {
	  iIndex = 10;

   }
   else if ( FStrEq( sWeapon, "weapon_m3" ) )
   {
	  iIndex = 7;

   }
   else if ( FStrEq( sWeapon, "weapon_xm1014" ) )
   {
	  iIndex = 7;

   }
   else if ( FStrEq( sWeapon, "weapon_scout" ) )
   {
	  iIndex = 2;

   }
   else if ( FStrEq( sWeapon, "weapon_aug" ) )
   {
	  iIndex = 2;

   }
   else if ( FStrEq( sWeapon, "weapon_mac10" ) )
   {
	  iIndex = 8;

   }
   else if ( FStrEq( sWeapon, "weapon_tmp" ) )
   {
	  iIndex = 6;

   }
   else if ( FStrEq( sWeapon, "weapon_mp5navy" ) )
   {
	  iIndex = 6;

   }
   else if ( FStrEq( sWeapon, "weapon_ump45" ) )
   {
	  iIndex = 8;

   }
   else if ( FStrEq( sWeapon, "weapon_p90" ) )
   {
	  iIndex = 10;

   }
   else if ( FStrEq( sWeapon, "weapon_galil" ) )
   {
	  iIndex = 3;

   }
   else if ( FStrEq( sWeapon, "weapon_famas" ) )
   {
	  iIndex = 3;

   }
   else if ( FStrEq( sWeapon, "weapon_ak47" ) )
   {
	  iIndex = 2;

   }
   else if ( FStrEq( sWeapon, "weapon_sg552" ) )
   {
	  iIndex = 3;

   }
   else if ( FStrEq( sWeapon, "weapon_sg550" ) )
   {
	  iIndex = 3;

   }
   else if ( FStrEq( sWeapon, "weapon_g3sg1" ) )
   {
	  iIndex = 2;

   }
   else if ( FStrEq( sWeapon, "weapon_m249" ) )
   {
	  iIndex = 4;

   }
   else if ( FStrEq( sWeapon, "weapon_m4a1" ) )
   {
	  iIndex = 3;

   }
   else if ( FStrEq( sWeapon, "weapon_awp" ) )
   {
	  iIndex = 5;

   }
   else
   {
	   iIndex = 0;
   }
	return iIndex;
}

void ZombiePlugin::ShowMOTD(int index, char *title, char *msg, int type, char *cmd)
{
	bf_write *buffer;
	MRecipientFilter filter;

	//filter.AddRecipient(index);
	filter.AddPlayer( index );

	static int iVGUIMenu = -1;
	if ( iVGUIMenu == -1 ) 
	{
		iVGUIMenu = UserMessageIndex("VGUIMenu");
	}
	if ( iVGUIMenu == -1 )
	{
		return;
	}
	buffer = m_Engine->UserMessageBegin( &filter, iVGUIMenu );
	buffer->WriteString("info");
	buffer->WriteByte(1);

	if(cmd != NULL)
		buffer->WriteByte(4);
	else
		buffer->WriteByte(3);

	buffer->WriteString("title");
	buffer->WriteString(title);

	buffer->WriteString("type");
	switch( type )
	{
		case TYPE_TEXT:
			buffer->WriteString("0"); //TYPE_TEXT = 0, just display this plain text
			break;
		case TYPE_INDEX:
			buffer->WriteString("1"); //TYPE_INDEX, lookup text & title in stringtable
			break;
		case TYPE_URL:
			buffer->WriteString("2"); //TYPE_URL, show this URL
			break;
		case TYPE_FILE:
			buffer->WriteString("3"); //TYPE_FILE, show this local file
			break;
	}

	buffer->WriteString("msg");
	buffer->WriteString(msg); // msg must not be greater than 192 characters

	if(cmd != NULL) {
		buffer->WriteString("cmd");
		buffer->WriteString(cmd); // exec this command if panel closed
	}

	m_Engine->MessageEnd();
	return ;
}

int ZombiePlugin::UserMessageIndex(const char *messageName)
{
	char name[128] = "";
	int sizereturn = 0;
	bool boolrtn = false;
	for( int x=1; x < 27; x++ )
	{
		boolrtn =  m_ServerDll->GetUserMessageInfo( x, name, 128, sizereturn ); 
		if( name && FStrEq( messageName, name ) )
		{
			return x;     
		}
	}
	return -1;
} 

const float MAX_SHAKE_AMPLITUDE = 16.0f;
void ZombiePlugin::UTIL_ScreenShake( CBasePlayer *pPlayer, float amplitude, float frequency, float duration, float radius, ShakeCommand_t eCommand, bool bAirShake )
{

	if ( amplitude > MAX_SHAKE_AMPLITUDE )
	{
		amplitude = MAX_SHAKE_AMPLITUDE;
	}
	if ( !pPlayer )
	{
		return;
	}
	if (amplitude < 0)
		return;
	TransmitShakeEvent( (CBasePlayer *)pPlayer, amplitude, frequency, duration, eCommand );
}

float ZombiePlugin::ComputeShakeAmplitude( const Vector &center, const Vector &shakePt, float amplitude, float radius ) 
{
	if ( radius <= 0 )
		return amplitude;

	float localAmplitude = -1;
	Vector delta = center - shakePt;
	float distance = delta.Length();

	if ( distance <= radius )
	{
		// Make the amplitude fall off over distance
		float flPerc = 1.0 - (distance / radius);
		localAmplitude = amplitude * flPerc;
	}

	return localAmplitude;
}

void ZombiePlugin::TransmitShakeEvent( CBasePlayer *pPlayer, float localAmplitude, float frequency, float duration, ShakeCommand_t eCommand )
{
	if ( !pPlayer || !m_GameEnts->BaseEntityToEdict( pPlayer ) || m_GameEnts->BaseEntityToEdict( pPlayer )->IsFree() )
		return;
	if ( ( localAmplitude > 0 ) || ( eCommand == SHAKE_STOP ) )
	{
		if ( eCommand == SHAKE_STOP )
			localAmplitude = 0;
		//CSingleUserRecipientFilter user( pPlayer );
		MRecipientFilter user;
		user.AddPlayer( m_Engine->IndexOfEdict( m_GameEnts->BaseEntityToEdict( pPlayer ) ) );
		static int iShake = -1;
		if ( iShake == -1 )
		{
			iShake = UserMessageIndex( "Shake" );
		}
		if ( iShake == -1 )
		{
			return;
		}
		bf_write* bWrite = m_Engine->UserMessageBegin( &user, iShake );
			bWrite->WriteByte( eCommand );
			bWrite->WriteFloat( localAmplitude );
			bWrite->WriteFloat( frequency );
			bWrite->WriteFloat( duration );
		m_Engine->MessageEnd();
	}
}

void ZombiePlugin::RestrictWeapon( const char *sWhoFor, const char *sWeapon, const char *sCount, const char *sWholeString )
{
	if ( !zombie_enabled.GetBool() )
	{
		return;
	}
	CUtlVector<int> matches;
	if ( !Q_stricmp(sWeapon, "all"))
	{
		matches.AddToTail(CSW_P228);
		matches.AddToTail(CSW_GLOCK);
		matches.AddToTail(CSW_ELITE);
		matches.AddToTail(CSW_FIVESEVEN);
		matches.AddToTail(CSW_USP);
		matches.AddToTail(CSW_DEAGLE);
		matches.AddToTail(CSW_M3);
		matches.AddToTail(CSW_XM1014);
		matches.AddToTail(CSW_MAC10);
		matches.AddToTail(CSW_UMP45);
		matches.AddToTail(CSW_MP5);
		matches.AddToTail(CSW_TMP);
		matches.AddToTail(CSW_P90);
		matches.AddToTail(CSW_SCOUT);
		matches.AddToTail(CSW_AUG);
		matches.AddToTail(CSW_SG550);
		matches.AddToTail(CSW_GALIL);
		matches.AddToTail(CSW_FAMAS);
		matches.AddToTail(CSW_AWP);
		matches.AddToTail(CSW_M4A1);
		matches.AddToTail(CSW_G3SG1);
		matches.AddToTail(CSW_SG552);
		matches.AddToTail(CSW_AK47);
		matches.AddToTail(CSW_M249);
		matches.AddToTail(CSW_SMOKEGRENADE);
		matches.AddToTail(CSW_HEGRENADE);
		matches.AddToTail(CSW_FLASHBANG);
		matches.AddToTail(CSW_PRIMAMMO);
		matches.AddToTail(CSW_SECAMMO);
		matches.AddToTail(CSW_VEST);
		matches.AddToTail(CSW_VESTHELM);
		matches.AddToTail(CSW_NVGS);
		matches.AddToTail(CSW_DEFUSEKIT);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted all weapons for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else if (!Q_strnicmp(sWeapon, "equip", 5)) {
		matches.AddToTail(CSW_SMOKEGRENADE);
		matches.AddToTail(CSW_HEGRENADE);
		matches.AddToTail(CSW_FLASHBANG);
		matches.AddToTail(CSW_PRIMAMMO);
		matches.AddToTail(CSW_SECAMMO);
		matches.AddToTail(CSW_VEST);
		matches.AddToTail(CSW_VESTHELM);
		matches.AddToTail(CSW_NVGS);
		matches.AddToTail(CSW_DEFUSEKIT);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted equipment for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else if (!Q_strnicmp(sWeapon, "pistol", 6)) {
		matches.AddToTail(CSW_P228);
		matches.AddToTail(CSW_GLOCK);
		matches.AddToTail(CSW_ELITE);
		matches.AddToTail(CSW_FIVESEVEN);
		matches.AddToTail(CSW_USP);
		matches.AddToTail(CSW_DEAGLE);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted pistols for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else if (!Q_strnicmp(sWeapon, "shotgun", 7)) {
		matches.AddToTail(CSW_M3);
		matches.AddToTail(CSW_XM1014);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted shotguns for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else if (!Q_strnicmp(sWeapon, "smg", 3)) {
		matches.AddToTail(CSW_MAC10);
		matches.AddToTail(CSW_UMP45);
		matches.AddToTail(CSW_MP5);
		matches.AddToTail(CSW_TMP);
		matches.AddToTail(CSW_P90);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted smgs for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else if (!Q_strnicmp(sWeapon, "rifle", 5)) {
		matches.AddToTail(CSW_SCOUT);
		matches.AddToTail(CSW_AUG);
		matches.AddToTail(CSW_SG550);
		matches.AddToTail(CSW_GALIL);
		matches.AddToTail(CSW_FAMAS);
		matches.AddToTail(CSW_AWP);
		matches.AddToTail(CSW_M4A1);
		matches.AddToTail(CSW_G3SG1);
		matches.AddToTail(CSW_SG552);
		matches.AddToTail(CSW_AK47);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted rifles for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else if (!Q_strnicmp(sWeapon, "sniper", 5)) {
		matches.AddToTail(CSW_SCOUT);
		matches.AddToTail(CSW_SG550);
		matches.AddToTail(CSW_AWP);
		matches.AddToTail(CSW_G3SG1);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted sniper rifles for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else {
		int id = LookupBuyID(sWeapon);
		if (id == CSW_NONE)
		{
			//print(pEdict, "%s: Unknown weapon specified\n", engine->Cmd_Argv(0));
			META_LOG( g_PLAPI, "%s: %s\n", sWholeString, GetLang("uknown_weapon") ); //Unknown weapon specified
			return;
		}
		//logact(pEdict, engine->Cmd_Argv(0), "restricted %s for %s", LookupWeaponName(id),
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
		matches.AddToTail(id);
	}
	int num = 0;
	if ( strlen(sCount) != 0 )
	{
		num = abs(atoi(sCount));
	}
	//sRestrictedWeapons.clear();
	char sWeapI[10];
	for (int x = 0; x < matches.Count(); x++) {
		int id = matches[x];
		Q_snprintf( sWeapI, sizeof( sWeapI ), "%d", id );
		sRestrictedWeapons.append( sWeapI );
		sRestrictedWeapons.append('|');
		char sPrint[200];
		if (*sWhoFor == 't') {
			g_RestrictT[id] = num;
			Q_snprintf( sPrint, sizeof( sPrint ), GetLang("restricted_t"), LookupWeaponName(id) );
			META_CONPRINTF( "[ZOMBIE] %s\n", sPrint ); //The %s has been restricted for the Terrorists
		} else if (*sWhoFor == 'c') {
			g_RestrictCT[id] = num;
			Q_snprintf( sPrint, sizeof( sPrint ), GetLang("restricted_ct"), LookupWeaponName(id) );
			META_CONPRINTF( "[ZOMBIE] %s\n", sPrint ); //The %s has been restricted for the Counter-Terrorists
		} else if (*sWhoFor == 'w') {
			g_RestrictW[id] = num;
			Q_snprintf( sPrint, sizeof( sPrint ), GetLang("winning_rest"), LookupWeaponName(id) );
			META_CONPRINTF( "[ZOMBIE] %s\n", sPrint); //The %s has been restricted for the winning team
		} else {
			g_RestrictT[id] = num;
			g_RestrictCT[id] = num; 
			Q_snprintf( sPrint, sizeof( sPrint ), GetLang("restricted"), LookupWeaponName(id) );
			META_CONPRINTF( "[ZOMBIE] %s\n", sPrint ); //The %s has been restricted
		}
		// x < matches.Count() )
		//{
		//}
	}
	/*for (int i = 1; i <= g_SMAPI->GetCGlobals()->maxClients; i++) 
	{
		edict_t *pEdict = m_Engine->PEntityOfEntIndex(i);
		if ( !IsValidPlayer( i, &pEdict ) )
		{
			continue;
		}
		//g_EmptyServerPlugin.ClientSettingsChanged(pEdict);
		CBasePlayer *pP = (CBasePlayer*)pEdict->GetUnknown()->GetBaseEntity();
		if ( g_ZombiePlugin.IsAlive( pEdict ) )
		{
			g_ZombiePlugin.ZombieDropWeapons( pP, false );
		}
	}*/
}

void UTIL_StringToVector( float *pVector, const char *pString )
{
	UTIL_StringToFloatArray( pVector, 3, pString );
}

void UTIL_StringToFloatArray( float *pVector, int count, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int	j;

	Q_strncpy( tempString, pString, sizeof(tempString) );
	pstr = pfront = tempString;

	for ( j = 0; j < count; j++ )			// lifted from pr_edict.c
	{
		pVector[j] = atof( pfront );

		// skip any leading whitespace
		while ( *pstr && *pstr <= ' ' )
			pstr++;

		// skip to next whitespace
		while ( *pstr && *pstr > ' ' )
			pstr++;

		if (!*pstr)
			break;

		pstr++;
		pfront = pstr;
	}
	for ( j++; j < count; j++ )
	{
		pVector[j] = 0;
	}
}

Vector UTIL_MidPoint(Vector p1,Vector p2)
{
   Vector p;

   p.x = (p1.x + p2.x) / 2;
   p.y = (p1.y + p2.y) / 2;
   p.z = (p1.z + p2.z) / 2;

   return p;
}

/*
void ClientPrint_hook( CBasePlayer *player, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	if ( msg_dest == HUD_PRINTCONSOLE && ( FStrEq( msg_name, "noclip OFF\n" ) || FStrEq( msg_name, "noclip ON\n" ) ) )
	{
#ifdef WIN32
		cPrintDetour.Ret( false );
#endif
	}
	return;
}
*/

void UTIL_MemProtect( void *addr, int length, int prot )
{
#ifdef __linux__
        void *addr2 = (void *)ALIGN(addr);
        int ret = mprotect(addr2, sysconf(_SC_PAGESIZE), prot);
#else
        DWORD old_prot;
		VirtualProtect(addr, length, prot, &old_prot);
#endif
}

void ZombiePlugin::DoFade( CBaseEntity* cBase, bool bEnable, int iPlayer )
{
	if ( iPlayer == -1 )
	{
		if ( !IsValidPlayer( cBase, &iPlayer ) )
		{
			return;
		}
	}
	color32 green = { zombie_vision_r.GetInt(), zombie_vision_g.GetInt(), zombie_vision_b.GetInt() , zombie_vision_a.GetInt()};
	g_Players[iPlayer].bFaded = bEnable;
	if ( bEnable )
	{
		UTIL_ScreenFade( cBase, green, 5.0f, 0, ( FFADE_OUT | FFADE_PURGE | FFADE_STAYOUT ) );
	}
	else
	{
		UTIL_ScreenFade( cBase, green, 0.0f, 0.0, ( FFADE_IN | FFADE_PURGE ) );
	}
}

void ZombiePlugin::SetFOV( edict_t* cEntity, int iFOV, bool bChangeCheats )
{
	int iPlayer = 0;
	if ( IsValidPlayer( cEntity, &iPlayer ) )
	{
		int iCurrentFOV = 0;
		if ( UTIL_GetProperty( g_Offsets.m_iFOV, cEntity, &iCurrentFOV ) )
		{
			if ( iCurrentFOV == iFOV )
			{
				return;
			}
		}
		UTIL_SetProperty( g_Offsets.m_iFOV, cEntity, iFOV );
	}
}

void ForceScreenFades( void **params )
{
	CBaseEntity *cBase = (CBaseEntity*)params[0];
	if ( IsValidPlayer( cBase ) )
	{
		color32 green = {0,160,0,128};
		UTIL_ScreenFade( cBase, green, 0.0f, 0.0, FFADE_STAYOUT );
	}
}

int ZombiePlugin::SetStringTable( const char *sTable, const char *sString )
{
	INetworkStringTable *g_pStringTableVguiScreen = m_NetworkStringTable->FindTable(sTable);
	if ( g_pStringTableVguiScreen )
	{
#if defined ORANGEBOX_BUILD
			return g_pStringTableVguiScreen->AddString( ADDSTRING_ISSERVER, sString );	
#else
		return g_pStringTableVguiScreen->AddString( sString );	
#endif
	}
	return -1;
}

int ZombiePlugin::GetStringTable( const char *sTable, const char *sMaterial )
{
	INetworkStringTable *g_pStringTableMaterials = m_NetworkStringTable->FindTable(sTable);
	//g_pStringTableMaterials = networkstringtable->CreateStringTable( "Materials", MAX_MATERIAL_STRINGS );
	if (sMaterial)
	{
		int nIndex = g_pStringTableMaterials->FindStringIndex( sMaterial );
		
		if (nIndex != INVALID_STRING_INDEX )
		{
			return nIndex;
		}
		else
		{
			META_LOG( g_PLAPI, "Warning! GetMaterialIndex: couldn't find material %s", sMaterial ); 
			return 0;
		}
	}

	// This is the invalid string index
	return 0;

}

/*
void ZombiePlugin::OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
	RETURN_META( MRES_HANDLED );
	int iPlayer;
	if ( IsValidPlayer( pPlayerEntity, &iPlayer ) )
	{
		//if ( FStrEq( pCvarName, "zombie_class" ) )
		//{
		//	if ( Q_strlen( pCvarValue ) != 0 )
		//	{
				//char sTmp[255];
				//const char sReplace[] = {0x0D, 0x0A};
				//V_StrSubst( zombie_clrestrict_string.GetString(), "<br>", sReplace, sTmp, 254 );
				//ShowMOTD( iPlayer, "Welcome to ZombieMod", sTmp, (zombie_clrestrict_motd.GetInt() - 1), "toggle cl_restrict_server_commands 0" );
		//	}
		//}
	}
	RETURN_META( MRES_HANDLED );
}
*/

void ZombiePlugin::AddDownload( const char *file )
{
	INetworkStringTable *pDownloadablesTable = m_NetworkStringTable->FindTable("downloadables");
	if ( pDownloadablesTable )
	{
		if ( !m_FileSystem->FileExists( file, "MOD" ) )
		{
			META_LOG( g_PLAPI, "File does not exist: \"%s\"", file );
		}
		else
		{
			if ( pDownloadablesTable->FindStringIndex( file ) < 0 )
			{
				META_LOG( g_PLAPI, "The following file already exits in the download tables: \"%s\"", file );
				return;
			}
			bool save = m_Engine->LockNetworkStringTables( false );
#if defined ORANGEBOX_BUILD
			pDownloadablesTable->AddString( ADDSTRING_ISSERVER, file, strlen( file ) + 1 );
#else
			pDownloadablesTable->AddString( file, strlen( file ) + 1 );
#endif

			m_Engine->LockNetworkStringTables( save );
		}
	}
} 

void ZombiePlugin::SetProtection( int iPlayer, bool bProtect, bool bReport, bool bRoundEnd )
{
	RenderMode_t	tRenderMode;
	edict_t			*cEnt;
	const char *sMessage = ( bProtect ? GetLang("on") : GetLang("off") );

	if ( !zombie_enabled.GetBool() || RoundFloatToInt( zombie_respawn_protection.GetFloat() ) == 0 )
	{
		return;
	}

	CBaseEntity *pBase = NULL;
	if ( IsValidPlayer( iPlayer, &cEnt, &pBase ) && IsAlive( cEnt ) && !(g_Players[iPlayer].isZombie && bProtect) )
	{
		g_Players[iPlayer].pPlayer = (CBasePlayer*)pBase;
		color32 cColour;
		if ( bProtect )
		{
			tRenderMode = kRenderNone;
			cColour.a = 0;
			cColour.b = 0;
			cColour.g = 0;
			cColour.r = 0;
		}
		else
		{
			tRenderMode = kRenderNormal;
			cColour.a = 255;
			cColour.b = 255;
			cColour.g = 255;
			cColour.r = 255;
		}
		g_Players[iPlayer].bProtected = bProtect;
		if ( g_Players[iPlayer].iProtectTimer != 99 )
		{
			g_ZombiePlugin.g_Timers->RemoveTimer( g_Players[iPlayer].iProtectTimer );
			g_Players[iPlayer].iProtectTimer = 99;
		}
		if ( g_Players[iPlayer].isZombie && bProtect )
		{
			g_Players[iPlayer].bProtected = false;
			return;
		}
		if ( bProtect )
		{
			void *params[1];
			params[0] = (void *)iPlayer;
			g_Players[iPlayer].iProtectTimer = g_ZombiePlugin.g_Timers->AddTimer( zombie_respawn_protection.GetFloat(), TimerProtection, params, 1 );
		}
		//RenderMode_t tRender = GetRenderMode( g_Players[iPlayer].pPlayer );
		if ( !SetRenderMode( g_Players[iPlayer].pPlayer, tRenderMode ) || !SetClrRender( g_Players[iPlayer].pPlayer, cColour) )
		{
			Hud_Print( cEnt, "\x03[ZOMBIE]\x01 %s", GetLang("protection_render") ); //Problem with set render!
		}
		else
		{
			SetWeaponProtection( g_Players[iPlayer].pPlayer, bProtect, tRenderMode, cColour );
		}
		if ( bReport )
		{
			char buf[100];
			Q_snprintf( buf, sizeof( buf ), GetLang( "protection_indicator" ), sMessage );
			Hud_Print( cEnt, "\x03[ZOMBIE]\x01 %s", buf ); //Your Zombie Protection is now %s.
		}
	}
	/*else if ( g_Players[iPlayer].bConnected && !bRoundEnd )
	{
		int iTeam = GetTeam( m_GameEnts->BaseEntityToEdict( g_Players[iPlayer].pPlayer ) );
		if ( iTeam == COUNTERTERRORISTS || iTeam == TERRORISTS )
		{
			META_LOG( g_PLAPI, "Invalid player for SetProtection %d, %s!", iPlayer, g_Players[iPlayer].sUserName.c_str() );
		}
	}*/
	return;
}

void ZombiePlugin::SetWeaponProtection( CBasePlayer *pPlayer, bool bProtect, RenderMode_t tRenderMode, color32 cColour )
{
	int iWeaponSlots[] = { SLOT_NADE, SLOT_NADE, SLOT_NADE, SLOT_NADE, SLOT_SECONDARY, SLOT_PRIMARY, SLOT_KNIFE };
	
	CBaseEntity *pWeapon =  NULL;
	edict_t *pWeaponEdict = NULL;
	for (int x = 6; x >= 0; x--)
	{
		pWeapon = CBaseCombatCharacter_Weapon_GetSlot( (CBaseCombatCharacter*) pPlayer, iWeaponSlots[x] );
		pWeaponEdict = m_GameEnts->BaseEntityToEdict( pWeapon );
		int iWeapon = m_Engine->IndexOfEdict( pWeaponEdict );
		if ( pWeapon )
		{
			SetRenderMode( pWeapon, tRenderMode );
			SetClrRender( pWeapon, cColour);
			SetEntProp( iWeapon, Prop_Data, "m_nRenderMode", tRenderMode );
			SetEntPropClr( iWeapon, Prop_Data, "m_clrRender", cColour );
			
		}
	}
}

void TimerShowVictims( void **params )
{
	edict_t *pEntity = (edict_t *)params[0];
	edict_t *pVictim = (edict_t *)params[1];
	//_ZombiePlugin.
	g_ZombiePlugin.ShowDamageMenu( pEntity, pVictim );
}

void TimerProtection(void **params)
{
	int iPlayer = (int)params[0];
	edict_t *pEntity = NULL;
	if ( IsValidPlayer( iPlayer, &pEntity ) )
	{
		if ( g_Players[iPlayer].bProtected )
		{
			g_ZombiePlugin.SetProtection( iPlayer, false );
			g_Players[iPlayer].iProtectTimer = 99;
		}
		g_ZombiePlugin.GiveWeapons( iPlayer, pEntity );
	}
	return;
}

void ZombiePlugin::ShowDamageMenu( edict_t *pEntity, edict_t *pKiller )
{
	myString sMenu;
	int x;
	int i = 1;
	int iPlayer = -1;
	int iKiller = -1;
	bool bKiller = false;
	bool bDisplay = false;

	if ( IsValidPlayer( pEntity, &iPlayer ) )
	{
		if ( g_Players[iPlayer].iClassMenu != -1 || g_Players[iPlayer].iBuyMenu != -1 || g_Players[iPlayer].iMenu > 0 )
		{
			return;
		}
		if ( !IsValidPlayer( pKiller, &iKiller ) )
		{
			iKiller = -1;
		}
		//zombie_damage
		sMenu.assign( GetLang( "zombie_damage" ) ); //Zombie Damage
		sMenu.append( "\n==============\n" );
		for ( x = 0; x <= MAX_PLAYERS; x++ )
		{
			if ( g_Players[iPlayer].iVictimList[x] > 0 )
			{
				if ( ! bDisplay )
				{
					bDisplay = true;
				}
				char sTmp[100];
				if ( iKiller == x )
				{
					bKiller = true;
					int iHealth = 0;
					UTIL_GetProperty( g_Offsets.m_iHealth, pKiller, &iHealth );
					Q_snprintf( sTmp, sizeof(sTmp), "->%d. %s - %d (%d%s)\n", i, g_Players[x].sUserName.c_str(), g_Players[iPlayer].iVictimList[x], iHealth, GetLang("hp_left") ); //hp left
				}
				else
				{
					Q_snprintf( sTmp, sizeof(sTmp), "->%d. %s - %d\n", i, g_Players[x].sUserName.c_str(), g_Players[iPlayer].iVictimList[x] );
				}
				g_Players[iPlayer].iVictimList[x] = 0;
				sMenu.append( sTmp );
				i++;
			}
		}
		if ( !bKiller && (iKiller > -1) )
		{
			if ( ! bDisplay )
			{
				bDisplay = true;
			}
			char sTmp[100];
			char sTmp2[100];
			int iHealth = 0;
			UTIL_GetProperty( g_Offsets.m_iHealth, pKiller, &iHealth );
			//damage_infector
			Q_snprintf( sTmp2, sizeof(sTmp2), GetLang("damage_infector"), g_Players[iKiller].sUserName.c_str(), iHealth );
			Q_snprintf( sTmp, sizeof(sTmp), " \n%s", sTmp2 ); //Infector: %s has %d left.

			sMenu.append( sTmp );
		}
		if ( bDisplay )
		{
			UTIL_DrawMenu( iPlayer, zombie_damagelist_time.GetInt(), 10, sMenu.c_str() );
		}
	}
}

void ZombiePlugin::RoundEndOverlay( bool bEnable, int iWinners )
{
	int x = 0;

	if ( g_bOverlayEnabled && Q_strlen( zombie_end_round_overlay.GetString() ) != 0 )
	{
		char sOverlay[100];
		if ( bEnable )
		{
			if ( iWinners == 1 )
			{
				Q_snprintf( sOverlay, sizeof(sOverlay), "r_screenoverlay vgui/hud/zombiemod/%s_zombies", zombie_end_round_overlay.GetString() );
			}
			else
			{
				Q_snprintf( sOverlay, sizeof(sOverlay), "r_screenoverlay vgui/hud/zombiemod/%s_humans", zombie_end_round_overlay.GetString() );
			}
		}
		else
		{
			Q_strncpy( sOverlay, "r_screenoverlay off", sizeof( sOverlay ) );
		}
		for ( x = 0; x <= MAX_PLAYERS; x++ )
		{
			edict_t *pEntity = NULL;
			if ( IsValidPlayer( x, &pEntity ) )
			{
				m_Engine->ClientCommand( pEntity, sOverlay );
			}
		}
	}
}

void ZombiePlugin::ZombieVision( edict_t *pEntity, bool bEnable )
{
	int iPlayer;
	if ( Q_strlen( zombie_vision_material.GetString() ) == 0 && bEnable )
	{
		return;
	}
	if ( IsValidPlayer( pEntity, &iPlayer ) )
	{
		char sTmp[100];
		if ( bEnable && g_Players[iPlayer].isZombie )
		{
			Q_snprintf( sTmp, sizeof( sTmp ), "r_screenoverlay vgui/hud/zombiemod/%s", zombie_vision_material.GetString() );
		}
		else
		{
			Q_strncpy( sTmp, "r_screenoverlay off", sizeof(sTmp) );
		}
		m_Engine->ClientCommand( pEntity, sTmp );
		
		g_Timers->RemoveTimer( g_Players[iPlayer].iVisionTimer );
		if ( zombie_vision_timer.GetInt() > 0 && bEnable && IsAlive( pEntity ) )
		{
			void *params[1];
			params[0] = pEntity;
			g_Players[iPlayer].iVisionTimer = g_Timers->AddTimer( zombie_vision_timer.GetFloat(), TimerVision, params, 1 );
		}
	}
}

void TimerVision(void **params)
{
	edict_t *pEntity = (edict_t *)params[0];
	int iPlayer;
	if ( IsValidPlayer( pEntity, &iPlayer ) )
	{
		g_ZombiePlugin.ZombieVision( pEntity, g_Players[iPlayer].bFaded );
	}
	return;
}

void ZombiePlugin::Set_sv_cheats( bool bEnable, bool *bOldValue )
{
	if ( bEnable )
	{
		/*if ( bOldValue )
		{
			*bOldValue = sv_cheats->GetBool();
		}*/
		sv_cheats->SetValue( 1 );
	}
	else
	{
		if ( bOldValue )
		{
			//sv_cheats->SetValue( ( *bOldValue ? 1 : 0 ) );
			sv_cheats->SetValue( 0 );
		}
		else
		{
			sv_cheats->SetValue( 0 );
		}
		//sv_cheats->SetValue( ( bEnable ? 1 : 0 ) );
	}
}



bool ZombiePlugin::LoadZombieClasses()
{
	bool bGotDefault = false;
	//g_iZombieClasses = -1;
	int x = 0;

	/*#define CLEARCLASSES() \
		for( x = 0; x < 100; x++ ) \
		{ \
			g_ZombieClasses[x].bInUse = false; \
			g_ZombieClasses[x].fKnockback = 0.0; \
			g_ZombieClasses[x].iHealth = 0; \
			g_ZombieClasses[x].iJumpHeight = 0; \
			g_ZombieClasses[x].iPrecache = 0; \
			g_ZombieClasses[x].fSpeed = 0; \
			g_ZombieClasses[x].fSpeedRun = 0; \
			g_ZombieClasses[x].fSpeedDuck = 0; \
			g_ZombieClasses[x].sClassname.clear(); \
			g_ZombieClasses[x].sModel.clear(); \
			g_ZombieClasses[x].bHeadShotsOnly = false; \
			g_ZombieClasses[x].iHeadshots = -1; \
			g_ZombieClasses[x].iRegenHealth = 0; \
			g_ZombieClasses[x].fRegenTimer = 0; \
			g_ZombieClasses[x].fGrenadeMultiplier = 0; \
			g_ZombieClasses[x].fGrenadeKnockback = 0; \
		} \
		g_iZombieClasses = -1;

	CLEARCLASSES();*/

	m_ClassManager.Clear();

	//#define SizeOf( iReturn, aArray ) \
	//	iReturn = sizeof(aArray) / sizeof(aArray[0]);
	
	//int size = 0;
	//SizeOf(size, g_ZombieClasses);

	if ( m_FileSystem->FileExists( g_SettingsFile, "MOD" ) )
	{
		KeyValues *kv = new KeyValues( g_SettingsFile );
		KeyValues *t;
		if ( kv )
		{
			if ( kv->LoadFromFile( m_FileSystem, g_SettingsFile, "MOD" ) )
			{
				t = kv;
				if ( t->GetFirstSubKey() )
				{
					char const *keyname = t->GetName();
					if ( FStrEq( keyname, "zm_classes" ) )
					{
						KeyValues *p;
						int iTmp;
						float fTmp;
						const char *sTmp;

						p = t->GetFirstSubKey();
						while ( p )
						{
							/*
							#define LOADSTRING( Name, Var, Def ) \
								sTmp = p->GetString( Name, Def ); \
								if ( sTmp ) \
								{ \
									Var.assign( sTmp ); \
								}
							*/
							#define LOADINT( Name, Var, Def ) \
								iTmp = p->GetInt( Name, Def ); \
								if ( iTmp > -1 ) \
								{ \
									Var = iTmp; \
								}
							#define LOADFLOAT( Name, Var, Def ) \
								fTmp = p->GetFloat( Name, Def ); \
								if ( fTmp > -1.0 ) \
								{ \
									Var = fTmp; \
								}

							//g_iZombieClasses++;

							ZombieClass *zClass = new ZombieClass;
							sTmp = p->GetString( "classname", "Default" );
							zClass->SetName( sTmp );
							zClass->SetModelName( p->GetString( "model", "" ) );

							
							//g_ZombieClasses[g_iZombieClasses].bInUse = true;
							//LOADSTRING( "classname", g_ZombieClasses[g_iZombieClasses].sClassname, "Default" );
							//LOADSTRING( "model", g_ZombieClasses[g_iZombieClasses].sModel, "" );

							bool bFound = false;
							char sModel[MAX_PATH];
							Q_snprintf( sModel, sizeof( sModel ), "%s.mdl", strlwr( (char *)zClass->GetModelName() ) );

							for ( int z = 0; z <= g_ZombieModels.Count() - 1; z++ )
							{
								const char *sLwr = strlwr( (char *)g_ZombieModels[z].sModel.c_str() );
								if ( FStrEq( sLwr, sModel ) )
								{
									bFound = true;
									zClass->iPrecache = g_ZombieModels[z].iPrecache;
									break;
								}
							}

							if ( bFound ) 
							{
								int iHsOnly = 0;
								LOADINT( "headshots", zClass->iHeadshots, zombie_headshot_count.GetInt() );
								LOADINT( "health", zClass->iHealth, zombie_health.GetInt() );
								LOADFLOAT( "speed", zClass->fSpeed, zombie_speed.GetFloat() );
								LOADFLOAT( "crouch_speed", zClass->fSpeedDuck, zombie_speed_crouch.GetFloat() );
								LOADFLOAT( "run_speed", zClass->fSpeedRun, zombie_speed_run.GetFloat() );
								LOADINT( "jump_height", zClass->iJumpHeight, zombie_jump_height.GetInt() );
								LOADFLOAT( "knockback", zClass->fKnockback, zombie_knockback.GetFloat() );
								LOADINT( "hs_only", iHsOnly, 0 );
								zClass->bHeadShotsOnly = (iHsOnly == 1);
								LOADINT( "regen", zClass->iRegenHealth, zombie_regen_health.GetInt() );
								LOADFLOAT( "regen_time", zClass->fRegenTimer, zombie_regen_timer.GetFloat() );
								LOADFLOAT( "gren_multiplier", zClass->fGrenadeMultiplier, zombie_grenade_damage_multiplier.GetFloat() );
								LOADFLOAT( "gren_knockback", zClass->fGrenadeKnockback, zombie_grenade_knockback_multiplier.GetFloat() );
								LOADINT( "health_bonus", zClass->iHealthBonus, zombie_health_bonus.GetInt() );
								LOADINT( "falldamage", iHsOnly, 0 ); //re using var
								zClass->bFallDamage = (iHsOnly != 0);
							}
							else
							{
								zClass->SetModelName( "" );
							}

							//zClass->iClassId = m_ClassManager.AddClass( zClass );
							int iNewId = m_ClassManager.AddClass( zClass );
							zClass = m_ClassManager.GetClass( iNewId, true );
							zClass->iClassId = iNewId;
							
							if ( FStrEq( p->GetName(), "zombie_classic" ) )
							{
								g_iDefaultClass = zClass->iClassId;
								bGotDefault = true;
							}

							p = p->GetNextKey();
						}
					}
					else
					{
						META_LOG( g_PLAPI, "ERROR: LoadZombieClasses() could not find classes section." );
					}
				}
			}
			else
			{
				META_LOG( g_PLAPI, "ERROR: LoadZombieClasses() Unable to load file '%s'", g_SettingsFile );
			}
		}
	}
	if ( !bGotDefault )
	{
		META_LOG( g_PLAPI, "Failed to load class file." ); 
		m_ClassManager.Clear();
	}
	else
	{
		META_LOG( g_PLAPI, "Successfully loaded %d classes from file.", ( m_ClassManager.Count() ) );
	}
	return bGotDefault;
}


bool ZombiePlugin::LoadSignatures( void* laddr, char *error, size_t maxlen )
{
	int x = 0;
	bool bSuccess = false;
	int z = 0;
	void *gamerules = NULL;
	KeyValues *kSigs = NULL;

	if ( m_FileSystem->FileExists( g_SigFile, "MOD" ) )
	{
		kSigs = new KeyValues( g_SigFile );
		if ( !kSigs->LoadFromFile( m_FileSystem, g_SigFile, "MOD" ) || !kSigs )
		{
			META_LOG( g_PLAPI, "ERROR: LoadSignatures() Unable to load file '%s'", g_SigFile );
		}
		else
		{

			if ( kSigs->GetFirstSubKey() )
			{
				char const *keyname = kSigs->GetName();
				if ( FStrEq( keyname, "zm_sigs" ) )
				{
					
#ifdef WIN32
					const char *sSystem = "Windows";
					const char *sOffset = "Windows_Offset";
					const char *sOffset1 = "Windows_Offset1";
					const char *sOffset2 = "Windows_Offset2";
					const char *sPatch = "Windows_Patch";
					const char *sPatch1 = "Windows_Patch1";
					const char *sPatch2 = "Windows_Patch2";
#else
					const char *sSystem = "Linux";
					const char *sOffset = "Linux_Offset";
					const char *sOffset1 = "Linux_Offset1";
					const char *sOffset2 = "Linux_Offset2";
					const char *sPatch = "Linux_Patch";
					const char *sPatch1 = "Linux_Patch1";
					const char *sPatch2 = "Linux_Patch2";
#endif

					KeyValues *kTmp = NULL;
					char sSignature[1024];

					#define GETSIGN( pVar ) \
						kTmp = kSigs->FindKey( pVar->sSectionName ); \
						if ( kTmp ) \
						{ \
							if ( pVar->bSignature ) \
							{ \
								Q_strncpy( sSignature, kTmp->GetString( sSystem, ""), 1024 ); \
								META_LOG( g_PLAPI, "	%s - Signature: [%s]", pVar->sSectionName, pVar->sPatch2 ); \
							} \
							if ( pVar->bPatch1 ) \
							{ \
								Q_strncpy( pVar->sPatch1, kTmp->GetString( sPatch1, ""), 1024 ); \
								META_LOG( g_PLAPI, "	%s - Patch 1: [%s]", pVar->sSectionName, pVar->sPatch2 ); \
								if ( Q_strlen( pVar->sPatch1 ) == 0 ) \
								{ \
									g_SMAPI->Format( error, maxlen, "Could not locate patch #1 for '%s' in signature file.", pVar->sSectionName ); \
									return false; \
								} \
							} \
							if ( pVar->bPatch2 ) \
							{ \
								Q_strncpy( pVar->sPatch2, kTmp->GetString( sPatch2, ""), 1024 ); \
								META_LOG( g_PLAPI, "	%s - Patch 2: [%s]", pVar->sSectionName, pVar->sPatch2 ); \
								if ( Q_strlen( pVar->sPatch2 ) == 0 ) \
								{ \
									g_SMAPI->Format( error, maxlen, "Could not locate patch #2 for '%s' in signature file.", pVar->sSectionName ); \
									return false; \
								} \
							} \
							if ( pVar->bPatchOffset ) \
							{ \
								pVar->iPatchOffset = kTmp->GetInt( sOffset, -1 ); \
								META_LOG( g_PLAPI, "	%s - Patch Offset: %d", pVar->sSectionName, pVar->iPatchOffset ); \
								if ( pVar->iPatchOffset == -1 ) \
								{ \
									g_SMAPI->Format( error, maxlen, "Could not locate patch offset for '%s' in signature file.", pVar->sSectionName ); \
									return false; \
								} \
							} \
							if ( pVar->bPatchOffset1 ) \
							{ \
								pVar->iPatchOffset1 = kTmp->GetInt( sOffset1, -1 ); \
								META_LOG( g_PLAPI, "	%s - Patch Offset 1: %d", pVar->sSectionName, pVar->iPatchOffset1 ); \
								if ( pVar->iPatchOffset1 == -1 ) \
								{ \
									g_SMAPI->Format( error, maxlen, "Could not locate patch offset #1 for '%s' in signature file.", pVar->sSectionName ); \
									return false; \
								} \
							} \
							if ( pVar->bPatchOffset2 ) \
							{ \
								pVar->iPatchOffset2 = kTmp->GetInt( sOffset2, -1 ); \
								META_LOG( g_PLAPI, "	%s - Patch Offset 2: %d", pVar->sSectionName, pVar->iPatchOffset2 ); \
								if ( pVar->iPatchOffset2 == -1 ) \
								{ \
									g_SMAPI->Format( error, maxlen, "Could not locate patch offset #2 for '%s' in signature file.", pVar->sSectionName ); \
									return false; \
								} \
							} \
							if ( pVar->bFunctionOffset ) \
							{ \
								pVar->iFunctionOffset = kTmp->GetInt( sOffset, -1 ); \
								META_LOG( g_PLAPI, "	%s - Function Offset: %d", pVar->sSectionName, pVar->iFunctionOffset ); \
								if ( pVar->iFunctionOffset == -1 ) \
								{ \
									g_SMAPI->Format( error, maxlen, "Could not locate function offset for '%s' in signature file.", pVar->sSectionName ); \
									return false; \
								} \
							} \
						} \
						else \
						{ \
							g_SMAPI->Format( error, maxlen, "Could not locate the signature for '%s' in signature file.", pVar->sSectionName ); \
							return false; \
						};

					#define SIGFIND( var, type, sig ) \
						GETSIGN( sig ); \
						if ( !( var == laddr ) ) \
						{ \
							var = NULL; \
							var = (type)LoadSignature( laddr, sSignature ); \
							if ( var ) META_LOG( g_PLAPI, "	%s - Function Pointer: [%p]", sig->sSectionName, var); \
							if (!var) \
							{ \
								g_SMAPI->Format(error, maxlen, "Could not find all sigs!\n%s:%d - [%s]\n", __FILE__, __LINE__, sig->sSectionName); \
								return false; \
							} \
						};


					#define PVFN2( classptr , offset ) ((*(DWORD*) classptr ) + offset)

					#define VFN2( classptr , offset ) *(DWORD*)PVFN2( classptr , offset )

					SIGFIND( gamerules,						void *,							cCGameRules					);

					#ifndef WIN32
						void *tempents = NULL;

						SIGFIND( g_SetCollisionBounds,			SetCollisionBounds_Func,		cSetCollisionBounds		);
						SIGFIND( tempents,						void *,							cTempEnts				);
						SIGFIND( g_EntList,						void *,							cEntList				);

						g_gamerules_addr = reinterpret_cast<void **>(gamerules);

						m_TempEnts = *(ITempEntsSystem **) tempents;
					#else
						SIGFIND( g_LevelShutdown,				void *,							cLevelShutDown			);
						SIGFIND( g_SetMinMaxSize,				SetMinMaxSize,					cSetMinMaxSize			);

						g_EntList = (void **)*(void **)((unsigned char *)g_LevelShutdown + cLevelShutDown->iPatchOffset  );
						m_TempEnts = **(ITempEntsSystem***)(VFN2(m_Effects, 12) + (111));
						g_gamerules_addr = (void **)*(void **)((unsigned char *)gamerules + cCGameRules->iPatchOffset );
					#endif

					void *pNull = NULL;

					//cCreateEntityByName
					SIGFIND( g_UtilRemoveFunc,				RemoveFunction,					cUtilRemove					);
					SIGFIND( v_ApplyAbsVelocity,			void *,							cApplyAbsVelocity			);
					SIGFIND( v_SwitchTeam,					void *,							cSwitchTeam					);
					SIGFIND( g_WantsLag,					WantsLagFunction,				cWantsLagComp				);
					SIGFIND( g_WantsLagPatch,				void *,							cWantsLagCompPatch			);
					SIGFIND( g_TakeDamage,					OnTakeDamageFunction,			cOnTakeDamage				);
					SIGFIND( g_TakeDamagePatch,				void *,							cOnTakeDamage				);
					SIGFIND( g_GetFileWeaponInfoFromHandle, GetFileWeaponInfoFromHandleFunc,cGetFileWeaponInfoFromHandle);
					SIGFIND( g_CCSPlayer_RoundRespawn,		CCSPlayer_RoundRespawnFunc,		cRoundRespawn				);
					SIGFIND( g_WeaponDropFunc,				WeaponDropFunction,				cWeaponDrop					);
					SIGFIND( g_TermRoundFunc,				TermRoundFunction,				cTermRound					);
					SIGFIND( g_GiveNamedItemFunc,			GiveNamedItem_Func,				cGiveNamedItem				);
					SIGFIND( g_domrev_addr,					void *,							cCalcDominationAndRevenge	);
					SIGFIND( g_FindEnt,						FindEntityByClassname_Func,		cFindEntityByClassname		);
					pNull = laddr;
					SIGFIND( pNull,							void *,							cGetPlayerMaxSpeed			);
					SIGFIND( pNull,							void *,							cWeaponCanSwitch			);
					SIGFIND( pNull,							void *,							cWeaponCanUse				);
					SIGFIND( pNull,							void *,							cTraceAttack				);
					SIGFIND( pNull,							void *,							cChangeTeam					);
					SIGFIND( pNull,							void *,							cCommitSuicide				);
					SIGFIND( pNull,							void *,							cEventKill					);
					SIGFIND( pNull,							void *,							cTeleport					);
					SIGFIND( pNull,							void *,							cPreThink					);
					SIGFIND( pNull,							void *,							cIPointsForKill				);
					SIGFIND( pNull,							void *,							cGetCollidable				);
					SIGFIND( pNull,							void *,							cGetModelIndex				);
					SIGFIND( pNull,							void *,							cSetModelIndex				);
					SIGFIND( pNull,							void *,							cGetDataDescMap				);
					SIGFIND( pNull,							void *,							cSetModel					);
					SIGFIND( pNull,							void *,							cKeyValue					);
					SIGFIND( pNull,							void *,							cKeyValueFloat				);
					SIGFIND( pNull,							void *,							cKeyValueVector				);
					SIGFIND( pNull,							void *,							cGetKeyValue				);
					SIGFIND( pNull,							void *,							cSetParent					);
					SIGFIND( pNull,							void *,							cAcceptInput				);
					SIGFIND( pNull,							void *,							cEventKilled				);
					SIGFIND( pNull,							void *,							cChangeTeam					);
					SIGFIND( pNull,							void *,							cGetVelocity				);
					SIGFIND( pNull,							void *,							cWorldSpaceCenter			);
					SIGFIND( pNull,							void *,							cIgnite						);
					SIGFIND( pNull,							void *,							cGiveAmmo					);
					SIGFIND( pNull,							void *,							cWeaponSwitch				);
					SIGFIND( pNull,							void *,							cEventDying					);
					SIGFIND( pNull,							void *,							cGetSlot					);
					SIGFIND( pNull,							void *,							cIsBot						);
					SIGFIND( pNull,							void *,							cGetViewVectors				);
					//SIGFIND( g_NoClipOff,					NoClipFunction,					cNoClipOff					);

					bSuccess = true;
				}
				else
				{
					META_LOG( g_PLAPI, "ERROR: LoadSignatures() could not find signature section - malformed file." );
				}
			}
			else
			{
				META_LOG( g_PLAPI, "ERROR: LoadSignatures() could not find signature section - malformed file." );
			}
		}
	}
	else
	{
		META_LOG( g_PLAPI, "ERROR: LoadSignatures() Unable to find signature file '%s'", g_SigFile );
	}

	if ( kSigs )
	{
		kSigs->deleteThis();
		kSigs = NULL;
	}
	return bSuccess;
}

void ZombiePlugin::ShowMainMenu( edict_t * pEntity )
{
	myString sMenu;
	int iPlayer;
	char sTmp[255];

	// 1 Class
	// 2 Buymenu
	// 3 Spawn
	// 4 Tele
	// 5 Stuck

	if ( IsValidPlayer( pEntity, &iPlayer ) && !g_Players[iPlayer].isBot  )
	{
		g_Players[iPlayer].iMenu = 1;
		g_Players[iPlayer].iBuyMenu = -1;
		g_Players[iPlayer].iClassMenu = -1;
		sMenu.assign( GetLang("zombie_menu") ); //Zombie Classes
		sMenu.append( "\n============\n" );
		

		if ( m_ClassManager.Enabled() )
		{
			Q_snprintf( sTmp, sizeof(sTmp), "->1. %s\n", GetLang("menu_classes") ); //Exit
		}
		else
		{
			Q_snprintf( sTmp, sizeof(sTmp), "1. %s\n", GetLang("menu_classes") ); //Exit
		}
		sMenu.append( sTmp );

		if ( zombie_buymenu.GetBool() && g_bBuyMenu )
		{
			Q_snprintf( sTmp, sizeof(sTmp), "->2. %s\n", GetLang("menu_buymenu") ); //Exit
		}
		else
		{
			Q_snprintf( sTmp, sizeof(sTmp), "2. %s\n", GetLang("menu_buymenu") ); //Exit
		}
		sMenu.append( sTmp );

		Q_snprintf( sTmp, sizeof(sTmp), "->3. %s\n", GetLang("menu_spawn") ); //Exit
		sMenu.append( sTmp );

		Q_snprintf( sTmp, sizeof(sTmp), "->4. %s\n", GetLang("menu_tele") ); //Exit
		sMenu.append( sTmp );

		Q_snprintf( sTmp, sizeof(sTmp), "->5. %s\n", GetLang("menu_stuck") ); //Exit
		sMenu.append( sTmp );

		Q_snprintf( sTmp, sizeof(sTmp), " \n0. %s", GetLang("exit") ); //Exit
		sMenu.append( sTmp );

		UTIL_DrawMenu( iPlayer, -1, 10, sMenu.c_str() );
	}
}

void ZombiePlugin::GiveSameWeapons( edict_t *pEntity, int iPlayer )
{
	char sWeapon[100] = "";
	char sTitle[100] = "";
	#define GETDETAILSSAME( Index, GunType ) \
		switch ( GunType ) \
		{ \
			sWeapon[0] = '\0'; \
			sTitle[0] = '\0'; \
			case 1: \
				Q_strncpy( sTitle, g_tBuyMenu.sPistols[Index].c_str(), sizeof( sTitle ) ); \
				Q_strncpy( sWeapon, g_tBuyMenu.iPistols[Index].c_str(), sizeof( sWeapon ) ); \
				break; \
			case 2: \
				Q_strncpy( sTitle, g_tBuyMenu.sSMGs[Index].c_str(), sizeof( sTitle ) ); \
				Q_strncpy( sWeapon, g_tBuyMenu.iSMGs[Index].c_str(), sizeof( sWeapon ) ); \
				break; \
			case 3: \
				Q_strncpy( sTitle, g_tBuyMenu.sRifles[Index].c_str(), sizeof( sTitle ) ); \
				Q_strncpy( sWeapon, g_tBuyMenu.iRifles[Index].c_str(), sizeof( sWeapon ) ); \
				break; \
			case 4: \
				Q_strncpy( sTitle, g_tBuyMenu.sMGs[Index].c_str(), sizeof( sTitle ) ); \
				Q_strncpy( sWeapon, g_tBuyMenu.iMGs[Index].c_str(), sizeof( sWeapon ) ); \
				break; \
			case 5: \
				Q_strncpy( sTitle, g_tBuyMenu.sEquip[Index].c_str(), sizeof( sTitle ) ); \
				Q_strncpy( sWeapon, g_tBuyMenu.iEquip[Index].c_str(), sizeof( sWeapon ) ); \
				break; \
			default: \
				break; \
		}
	for ( int x = 1; x <= 6; x++ )
	{
		if ( g_Players[iPlayer].iBuyWeapons[x-1] > 0 )
		{
			GETDETAILSSAME( g_Players[iPlayer].iBuyWeapons[x-1], x ); 
			if ( x == 1 ) 
			{
				DropWeapon(g_Players[iPlayer].pPlayer, false );
			}
			else if ( x != 5 ) 
			{
				DropWeapon(g_Players[iPlayer].pPlayer, true );
			}
			if ( Q_strlen( sWeapon ) > 0 )
			{
				UTIL_GiveNamedItem( g_Players[iPlayer].pPlayer, sWeapon, 0 );
				GiveAmmo( g_Players[iPlayer].pPlayer, 200, sWeapon);
				GiveAmmo( g_Players[iPlayer].pPlayer, 200, sWeapon);
			}
		}
	}
}

void ZombiePlugin::ShowBuyMenu( edict_t * pEntity, int iMenu )
{
	char sWeapon[100];
	char sTitle[100];
	#define GETDETAILS( Index ) \
		sWeapon[0] = '\0'; \
		sTitle[0] = '\0'; \
		if ( Index < 8 ) \
		{ \
			switch ( iBuyMenu ) \
			{ \
				case 1: \
					Q_strncpy( sTitle, g_tBuyMenu.sPistols[Index].c_str(), sizeof( sTitle ) ); \
					Q_strncpy( sWeapon, g_tBuyMenu.iPistols[Index].c_str(), sizeof( sWeapon ) ); \
					break; \
				case 2: \
					Q_strncpy( sTitle, g_tBuyMenu.sSMGs[Index].c_str(), sizeof( sTitle ) ); \
					Q_strncpy( sWeapon, g_tBuyMenu.iSMGs[Index].c_str(), sizeof( sWeapon ) ); \
					break; \
				case 3: \
					Q_strncpy( sTitle, g_tBuyMenu.sRifles[Index].c_str(), sizeof( sTitle ) ); \
					Q_strncpy( sWeapon, g_tBuyMenu.iRifles[Index].c_str(), sizeof( sWeapon ) ); \
					break; \
				case 4: \
					Q_strncpy( sTitle, g_tBuyMenu.sMGs[Index].c_str(), sizeof( sTitle ) ); \
					Q_strncpy( sWeapon, g_tBuyMenu.iMGs[Index].c_str(), sizeof( sWeapon ) ); \
					break; \
				case 5: \
					Q_strncpy( sTitle, g_tBuyMenu.sEquip[Index].c_str(), sizeof( sTitle ) ); \
					Q_strncpy( sWeapon, g_tBuyMenu.iEquip[Index].c_str(), sizeof( sWeapon ) ); \
					break; \
				default: \
					Q_strncpy( sTitle, "ERROR", sizeof( sTitle ) ); \
					Q_strncpy( sWeapon, "ERROR", sizeof( sWeapon ) ); \
					break; \
			} \
		}

	int iPlayer = 0;
	if ( IsValidPlayer( pEntity, &iPlayer ) && !g_Players[iPlayer].isBot  )
	{
		int iBuyMenu = g_Players[iPlayer].iBuyMenu;
		if ( iBuyMenu > 0 )
		{
			if ( iMenu == 9 )
			{
				g_Players[iPlayer].iBuyMenu = 0;
				DisplayBuyMenu( pEntity, iPlayer, -1 );
			}
			else
			{
				bool bTimeLimitExceeded = (BuyTimeExceeded() && zombie_buymenu_timelimit.GetBool());
				if ( bTimeLimitExceeded )
				{
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("buy_limit") );
					g_Players[iPlayer].iBuyMenu = -1;
					ShowBuyMenu( pEntity, -1 );
					return;
				}

				// Select weapon on submenu here.
				if ( iMenu == -1 )
				{
					DisplayBuyMenu( pEntity, iPlayer, iMenu );
				}
				else
				{
					GETDETAILS( iMenu );
					if ( Q_strlen( sWeapon ) )
					{
						if ( g_Players[iPlayer].isZombie )
						{
							Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("buy_zombie") );
						}
						else if ( !IsAlive( pEntity ) )
						{
							Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("buy_dead") );
						}
						else
						{
							iBuyMenu--;
							g_Players[iPlayer].iBuyWeapons[iBuyMenu] = iMenu;
							if ( iBuyMenu == 2 )
							{
								g_Players[iPlayer].iBuyWeapons[1] = 0;
								g_Players[iPlayer].iBuyWeapons[3] = 0;
							}
							else if ( iBuyMenu == 2 )
							{
								g_Players[iPlayer].iBuyWeapons[1] = 0;
								g_Players[iPlayer].iBuyWeapons[3] = 0;
							}
							else if ( iBuyMenu == 3 )
							{
								g_Players[iPlayer].iBuyWeapons[1] = 0;
								g_Players[iPlayer].iBuyWeapons[2] = 0;
							}
							if (( zombie_buymenu_zone_only.GetBool() && CCSPlayer_IsInBuyZone( g_Players[iPlayer].pPlayer ) || !zombie_buymenu_zone_only.GetBool() ))
							{
								if ( iBuyMenu == 0 ) 
								{
									DropWeapon(g_Players[iPlayer].pPlayer, false );
								}
								else if ( iBuyMenu != 4 ) 
								{
									DropWeapon(g_Players[iPlayer].pPlayer, true );
								}
								UTIL_GiveNamedItem( g_Players[iPlayer].pPlayer, sWeapon, 0 );
								GiveAmmo( g_Players[iPlayer].pPlayer, 200, sWeapon);
								GiveAmmo( g_Players[iPlayer].pPlayer, 200, sWeapon);
							}
							else
							{
								Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("buyzone") );
							}
						}
					}
				}
				g_Players[iPlayer].iBuyMenu = -1;
				ShowBuyMenu( pEntity, -1 );
			}
		}
		else
		{
			if ( iMenu == -1 )
			{
				DisplayBuyMenu( pEntity, iPlayer, iMenu );
			}
			else
			{
				// Display sub menu pressed.
				if ( iMenu == 7 )
				{
					g_Players[iPlayer].iBuyMenu = -1;
					if (( zombie_buymenu_zone_only.GetBool() && CCSPlayer_IsInBuyZone( g_Players[iPlayer].pPlayer ) || !zombie_buymenu_zone_only.GetBool() ))
					{
						GiveSameWeapons( pEntity, iPlayer );
					}
					else
					{
						Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("buyzone") );
					}
					// Same again.
				}
				else if ( iMenu == 8 )
				{
					g_Players[iPlayer].iBuyMenu = 0;
					g_Players[iPlayer].bSameWeapons = !g_Players[iPlayer].bSameWeapons;
					DisplayBuyMenu( pEntity, iPlayer, -1 );
					// Always same.
				}
				else if ( iMenu == 9 )
				{
					g_Players[iPlayer].iBuyMenu = -1;
					ShowMainMenu( pEntity );
					// Back
				}
				else if ( iMenu >= 1 && iMenu <= 5 )
				{
					g_Players[iPlayer].iBuyMenu = iMenu;
					// Display sub menu pressed.	
					DisplayBuyMenu( pEntity, iPlayer, iMenu );
				}
				else if ( iMenu == 0 || iMenu == 10 )
				{
					g_Players[iPlayer].iBuyMenu = -1;
					//Hud_Print( pEntity, "Disabled buy menu." );
				}
			}
		}
	}
}

void ZombiePlugin::DisplayBuyMenu( edict_t * pEntity, int iPlayer, int iMenu )
{
	myString sMenu;
	char sTmp[500];
	char sWeapon[100];
	char sTitle[100];
	bool bDrawMenu = false;
	if ( iMenu == -1 )
	{
		// Display Base Buy Menu.

		sMenu.assign( GetLang("buy_menu") ); //Zombie Classes
		sMenu.append( "\n============\n" );
		
		#define ADDMENUSTRING( bBool, sCaption, Index ) \
			if ( bBool && sCaption.size() > 0 ) \
			{ \
				Q_snprintf( sTmp, sizeof(sTmp), "->%d. %s\n", Index, sCaption.c_str() );  \
			} \
			else \
			{ \
				if ( sCaption.size() > 0 ) \
				{ \
					Q_snprintf( sTmp, sizeof(sTmp), "%d. %s\n", Index, sCaption.c_str() ); \
				} \
				else \
				{ \
					Q_snprintf( sTmp, sizeof(sTmp), "%d. \n", Index ); \
				} \
			} \
			sMenu.append( sTmp );

		ADDMENUSTRING( g_tBuyMenu.bPistols, g_tBuyMenu.sPistols[0], 1 );
		ADDMENUSTRING( g_tBuyMenu.bSMGs, g_tBuyMenu.sSMGs[0], 2 );
		ADDMENUSTRING( g_tBuyMenu.bRifles, g_tBuyMenu.sRifles[0], 3 );
		ADDMENUSTRING( g_tBuyMenu.bMGs, g_tBuyMenu.sMGs[0], 4 );
		ADDMENUSTRING( g_tBuyMenu.bEquip, g_tBuyMenu.sEquip[0], 5 );

		Q_snprintf( sTmp, sizeof(sTmp), " \n->7. %s\n", GetLang("use_last") ); // Use last.
		sMenu.append( sTmp );

		Q_snprintf( sTmp, sizeof(sTmp), "%s8. %s\n\n", (g_Players[iPlayer].bSameWeapons ? "" : "->" ),GetLang("always_last") ); // Use last.
		sMenu.append( sTmp );

		Q_snprintf( sTmp, sizeof(sTmp), "9. %s\n", GetLang("back") ); // Always last.
		sMenu.append( sTmp );

		Q_snprintf( sTmp, sizeof(sTmp), "0. %s", GetLang("exit") ); // Exit
		sMenu.append( sTmp );

		g_Players[iPlayer].iBuyMenu = 0;

		UTIL_DrawMenu( iPlayer, -1, 10, sMenu.c_str() );
	}
	else
	{

		switch ( iMenu )
		{
			case 1:
				bDrawMenu = g_tBuyMenu.bPistols;
				break;
			case 2:
				bDrawMenu = g_tBuyMenu.bSMGs;
				break;
			case 3:
				bDrawMenu = g_tBuyMenu.bRifles;
				break;
			case 4:
				bDrawMenu = g_tBuyMenu.bMGs;
				break;
			case 5:
				bDrawMenu = g_tBuyMenu.bEquip;
				break;
			default:
				break;
		}
		#define GETTITLE( Index ) \
			switch ( iMenu ) \
			{ \
				sWeapon[0] = '\0'; \
				sTitle[0] = '\0'; \
				case 1: \
					Q_strncpy( sTitle, g_tBuyMenu.sPistols[Index].c_str(), sizeof( sTitle ) ); \
					Q_strncpy( sWeapon, g_tBuyMenu.iPistols[Index].c_str(), sizeof( sWeapon ) ); \
					break; \
				case 2: \
					Q_strncpy( sTitle, g_tBuyMenu.sSMGs[Index].c_str(), sizeof( sTitle ) ); \
					Q_strncpy( sWeapon, g_tBuyMenu.iSMGs[Index].c_str(), sizeof( sWeapon ) ); \
					break; \
				case 3: \
					Q_strncpy( sTitle, g_tBuyMenu.sRifles[Index].c_str(), sizeof( sTitle ) ); \
					Q_strncpy( sWeapon, g_tBuyMenu.iRifles[Index].c_str(), sizeof( sWeapon ) ); \
					break; \
				case 4: \
					Q_strncpy( sTitle, g_tBuyMenu.sMGs[Index].c_str(), sizeof( sTitle ) ); \
					Q_strncpy( sWeapon, g_tBuyMenu.iMGs[Index].c_str(), sizeof( sWeapon ) ); \
					break; \
				case 5: \
					Q_strncpy( sTitle, g_tBuyMenu.sEquip[Index].c_str(), sizeof( sTitle ) ); \
					Q_strncpy( sWeapon, g_tBuyMenu.iEquip[Index].c_str(), sizeof( sWeapon ) ); \
					break; \
				default: \
					break; \
			}
		if ( bDrawMenu )
		{
			GETTITLE( 0 );
			sMenu.assign( sTitle ); //Title
			sMenu.append( "\n============\n" );

			for ( int x = 1; x <= 7; x++ )
			{
				GETTITLE( x );
				if ( Q_strlen( sWeapon ) > 0 && Q_strlen( sTitle ) > 0 && ( GetAmmoIndex( sWeapon ) > 0 || iMenu == 5 ) )
				{
					Q_snprintf( sTmp, sizeof( sTmp ), "->%d. %s\n", x, sTitle );
				}
				else if ( Q_strlen( sTitle ) > 0 )
				{
					Q_snprintf( sTmp, sizeof( sTmp ), "%d. %s\n", x, sTitle );
				}
				else
				{
					Q_strncpy( sTmp, " \n", sizeof( sTmp ) );
				}
				
				sMenu.append( sTmp );			
			}
			Q_snprintf( sTmp, sizeof( sTmp ), " \n\n9. %s\n0. %s", GetLang("back"), GetLang("exit") );
			sMenu.append( sTmp );
			g_Players[iPlayer].iBuyMenu = iMenu;
			UTIL_DrawMenu( iPlayer, -1, 10, sMenu.c_str() );
			// Display sub menu.
		}
		else
		{
			g_Players[iPlayer].iBuyMenu = -1;
		}
	}
}

const char *GetPlayerClassName( edict_t *pEntity )
{
	return pEntity->GetClassName();
}

void ZombiePlugin::ShowClassMenu( edict_t * pEntity )
{
	myString sMenu;
	int x;
	int iPlayer;
	char sTmp[255];
	int iCountTo;
	bool bMore = false;
	bool bBack = false;

	if ( IsValidPlayer( pEntity, &iPlayer ) && !g_Players[iPlayer].isBot  )
	{
		if ( g_Players[iPlayer].iClassMenu == -1 )
		{
			g_Players[iPlayer].iClassMenu = 0;
		}
		//sMenu.assign( GetLang("zombie_classes") ); //Zombie Classes
		int iCurrentMenu = ( g_Players[iPlayer].iClassMenu == 0 ? 1 : ( ( g_Players[iPlayer].iClassMenu + 1 ) / 7 ) + 1 );
		Q_snprintf( sTmp, sizeof( sTmp ), "%s (%d)", GetLang("zombie_classes"), iCurrentMenu );
		sMenu.assign( sTmp );
		sMenu.append( "\n============\n" );
		if ( g_Players[iPlayer].iClassMenu > -1 )
		{
			bBack = true;
		}
		if ( ( (m_ClassManager.Count()-1) - g_Players[iPlayer].iClassMenu) > 6 )
		{
			bMore = true;
		}
		iCountTo = g_Players[iPlayer].iClassMenu + 6;
	
		if ( (m_ClassManager.Count() - 1) < iCountTo )
		{
			iCountTo = (m_ClassManager.Count() - 1);
		}
		int iNum = 0;
		for ( x = g_Players[iPlayer].iClassMenu; x < m_ClassManager.Count(); x++ )
		{
			GETCLASS( x );
			Q_snprintf( sTmp, sizeof(sTmp), "%s%d. %s\n", ( ( g_Players[iPlayer].iClass == (g_Players[iPlayer].iClassMenu + x) ) ? "" : "->" ), ( ++iNum ), zClass->GetName() );
			sMenu.append( sTmp );
		}
		sTmp[0] = '\0';
		if ( bBack )
		{
			if ( iNum < 7 )
			{
				for ( x = 7; x > iNum; x-- )
				{
					Q_strncat( sTmp, " \n", sizeof( sTmp ) );
				}
			}
			Q_snprintf( sTmp, sizeof(sTmp), "%s8. %s\n", sTmp, GetLang("back") ); //Back
			sMenu.append( sTmp );
		}
		sTmp[0] = '\0';
		if ( bMore )
		{
			if ( !bBack && iNum < 8 )
			{
				for ( x = 8; x > iNum; x-- )
				{
					Q_strncat( sTmp, " \n", sizeof( sTmp ) );
				}
			}
			Q_snprintf( sTmp, sizeof(sTmp), "%s9. %s\n", sTmp, GetLang("more") ); //More
			sMenu.append( sTmp );
		}
		else
		{
			Q_strncat( sTmp, " \n", sizeof( sTmp ) );
			sMenu.append( sTmp );
		}
		sTmp[0] = '\0';
		if ( !bBack && !bMore && iNum < 9 )
		{
			for ( x = 8; x > iNum; x-- )
			{
				Q_strncat( sTmp, " \n", sizeof( sTmp ) );
			}
		}
		Q_snprintf( sTmp, sizeof(sTmp), "%s\n0. %s", sTmp, GetLang("exit") ); //Exit
		sMenu.append( sTmp );

		if ( g_Players[iPlayer].iChangeToClass > -1 && g_Players[iPlayer].iChangeToClass < m_ClassManager.Count() )
		{
			GETCLASS( g_Players[iPlayer].iChangeToClass );
			Q_snprintf( sTmp, sizeof(sTmp), " \n \n%s", zClass->GetName(), GetLang("change_class_dead") ); //Change to %s when you die.
			sMenu.append( sTmp );
		}

		if ( sMenu.size() > 0 )
		{
			UTIL_DrawMenu( iPlayer, -1, 10, sMenu.c_str(), true );
		}
	}
}

void ZombiePlugin::HudMessage( int EdictId, int channel, float x, float y, byte r1, byte g1, byte b1, byte a1, byte r2, byte g2, byte b2, byte a2, int effect, float fadeinTime, float fadeoutTime, float holdTime, float fxTime, const char *pMessage)
{
	hudtextparms_t tp;
	tp.channel = channel;
	tp.x = x; tp.y = y;
	tp.r1 = r1; tp.g1 = g1; tp.b1 = b1; tp.a1 = a1;
	tp.r2 = r2; tp.g2 = g2; tp.b2 = b2; tp.a1 = a2;
	tp.effect = effect;
	tp.fadeinTime = fadeinTime;
	tp.fadeoutTime = fadeoutTime;
	tp.holdTime = holdTime;
	tp.fxTime = fxTime;

	HudMessage(EdictId, tp, pMessage);
}

// show a HudMsg (colored text with a specified location)
// !!! works only on clients with a fixed ClientScheme.res !!!
void ZombiePlugin::HudMessage( int EdictId, const hudtextparms_t &textparms, const char *pMessage )
{
	static int MsgIdx = -1;
	
	if (MsgIdx==-1)
	{
		MsgIdx = UserMessageIndex("HudMsg");
		if (MsgIdx==-1)
		{
			return;
		}
	}

	//MRecipientFilter mrf(m_Engine);
	MRecipientFilter mrf;
	//filter.AddAllPlayers( MAX_CLIENTS );

	if ( EdictId == 0 )
	{
		mrf.AddAllPlayers( MAX_CLIENTS );
	}
	else
	{
		mrf.AddPlayer( EdictId );
	}

	bf_write *bf;
	bf = m_Engine->UserMessageBegin( &mrf, MsgIdx );
		bf->WriteByte ( textparms.channel & 0xFF );
		bf->WriteFloat( textparms.x );
		bf->WriteFloat( textparms.y );
		bf->WriteByte( textparms.r1 );
		bf->WriteByte( textparms.g1 );
		bf->WriteByte( textparms.b1 );
		bf->WriteByte( textparms.a1 );
		bf->WriteByte( textparms.r2 );
		bf->WriteByte( textparms.g2 );
		bf->WriteByte( textparms.b2 );
		bf->WriteByte( textparms.a2 );
		bf->WriteByte( textparms.effect );
		bf->WriteFloat( textparms.fadeinTime );
		bf->WriteFloat( textparms.fadeoutTime );
		bf->WriteFloat( textparms.holdTime );
		bf->WriteFloat( textparms.fxTime );
		bf->WriteString( pMessage );
	m_Engine->MessageEnd();
}

void ZombiePlugin::RemoveAllSpawnTimers()
{
	for ( int x = 0; x < g_RespawnTimers.Count(); x++ )
	{
		g_Timers->RemoveTimer( g_RespawnTimers[x].iTimer );
	}
	g_RespawnTimers.Purge();
	return;
}

void KickStartEnd( void **params ) 
{
	for ( int x=0; x< MAX_PLAYERS; x++ )
	{
		bKickStart[x] = false;
	}
	g_ZombiePlugin.Hud_Print( NULL, "\x03[ZOMBIE]\x01 %s", g_ZombiePlugin.GetLang("kickstart_failed") ); //Kick start finished unsuccessfully!
	iGlobalVoteTimer = zombie_vote_interval.GetInt() * 2;
	return;
}

const char* ZombiePlugin::GetLang( const char *sKeyName )
{
	if ( !bLanguagesLoaded || !kLanguage )
	{
		return "*** There was an error loading the language file ****.";
	}
	else if ( kLanguage->FindKey( sKeyName ) )
	{
		return kLanguage->GetString( sKeyName );
	}
	else
	{
		char sRet[255];
		Q_snprintf( sRet, sizeof( sRet), zombie_language_error.GetString(), sKeyName );
		sLangError.assign( sRet );
		return sLangError.c_str();
	}
}

void ZombiePlugin::GiveMoney( edict_t *pEntity, int iMoneyToGive )
{
	if ( IsValidPlayer( pEntity ) )
	{
		int iMoney;
		if ( UTIL_GetProperty( g_Offsets.m_iAccount, pEntity, &iMoney ) )
		{
			iMoney += iMoneyToGive;
			UTIL_SetProperty( g_Offsets.m_iAccount, pEntity, iMoney );
		}
	}
}

void Timed_Dissolve( void **params )
{
	edict_t *pEdict = (edict_t *)params[0];
	g_ZombiePlugin.Dissolve( pEdict );
}

void ZombiePlugin::Dissolve( edict_t *pEntity )
{
	int				iPlayer = 0;
	CBaseEntity		*pPlayer = NULL;
	CBaseEntity		*pRagdoll = NULL;
	edict_t			*pEdDisolver = NULL;

	iPlayer = m_Engine->IndexOfEdict( pEntity );
	if ( IsValidPlayer( iPlayer, NULL, &pPlayer ) && !IsAlive( pEntity ) )
	{
		CBaseEntity *pDisolve = UTIL_GiveNamedItem( (CBasePlayer*)pPlayer, "env_entity_dissolver", 0 );
		if ( pDisolve )
		{

			pRagdoll = UTIL_GetPlayerRagDoll( pEntity );
			if ( !pRagdoll )
			{
				return;
			}

			char sTargetName[64];
			g_SMAPI->Format( sTargetName, sizeof(sTargetName), "zm_dissolve_%d", pRagdoll );
			CBaseEntity_KeyValue( pRagdoll, "targetname", sTargetName );

			CBaseEntity_KeyValue( pDisolve, "target", sTargetName ); //"cs_ragdoll" );
			CBaseEntity_KeyValue( pDisolve, "magnitude", "1" );
			CBaseEntity_KeyValue( pDisolve, "dissolvetype", "3" ); // (can be disove type 2,1 etc)
			variant_t emptyVariant;
			CBaseEntity_AcceptInput( pDisolve, "Dissolve", pPlayer, pPlayer, emptyVariant, 0 );

			pEdDisolver = m_GameEnts->BaseEntityToEdict( pDisolve );
			if ( pDisolve && pEdDisolver && m_Engine->IndexOfEdict( pEdDisolver ) > 0 )
			{
				m_Engine->RemoveEdict( pEdDisolver );
			}
			//m_Engine->RemoveEdict( );

			//void *params[1];
			//params[0] = pDisolve;
			//g_Timers->AddTimer( 1.0, Timed_Remove, params, 1 );
		}
		/*
		else
		{
			META_LOG( g_PLAPI, "Could not create env_entity_dissolver" );
		}
		*/
	}
}

void Timed_Remove( void **params )
{
	CBaseEntity *pBase = (CBaseEntity *)params[0];
	edict_t *pEdict = m_GameEnts->BaseEntityToEdict( pBase );
	if ( pBase && pEdict && m_Engine->IndexOfEdict( pEdict ) > 0 )
	{
		g_UtilRemoveFunc( pBase  );
	}
}
bool m_bRemoving = false;
void Timed_Bot( void **params )
{
	if (!bBotAdded )
	{
		bBotAdded = true;
	}
	float flTime = 0;
	if ( m_bRemoving )
	{
		m_Engine->ServerCommand( "bot_quota 31\n" );
		flTime = (float)RandomInt( 3, 15 );
		META_LOG( g_PLAPI, "Adding another bot in %f seconds.", flTime );
		if ( !g_ZombiePlugin.g_Timers->IsTimer(ibTimer) )
		{
			ibTimer = g_ZombiePlugin.g_Timers->AddTimer( flTime, Timed_Bot );
		}
	}
	else
	{
		m_Engine->ServerCommand( "bot_quota 32\n" );
		flTime = (float)RandomInt( 15, 60 );
		META_LOG( g_PLAPI, "Removing another bot in %f seconds.", flTime );
		if ( !g_ZombiePlugin.g_Timers->IsTimer(ibTimer) )
		{
			ibTimer = g_ZombiePlugin.g_Timers->AddTimer( flTime, Timed_Bot );
		}
	}
	m_bRemoving = !m_bRemoving;
}


int OnIPointsForKill(CBasePlayer *pl1, CBasePlayer *pl2)
{
	/* If we're hooked, FFA is always on. */
	RETURN_META_VALUE(MRES_SUPERCEDE, 1);
}



// + DDOS

/*
void AddDoSCount(int ip1, int ip2, int ip3, int ip4)
{
	SourceHook::List<DoSCount *>::iterator iter;
	DoSCount *pInfo;
	for(iter=m_pDoSAddr.begin(); iter!=m_pDoSAddr.end(); iter++)
	{
		pInfo = (*iter);
		if(ip1 == pInfo->ip[0] && ip2 == pInfo->ip[1] && ip3 == pInfo->ip[2] && ip4 == pInfo->ip[3])
		{
			pInfo->count++;
			return;
		}
	}
	DoSCount *d = new DoSCount;
	d->ip[0] = ip1;
	d->ip[1] = ip2;
	d->ip[2] = ip3;
	d->ip[3] = ip4;
	d->count = 1;
	m_pDoSAddr.push_back(d);
}

void UnHookRecvFrom()
{
	if(g_recvfrom_hooked)
	{
		g_pVCR->Hook_recvfrom = g_real_recvfrom_ptr;
		g_recvfrom_hooked = false;
		META_CONPRINT("[-] DoS Attack Protect - Disable\n");
	}
}

int MyRecvFromHook(int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
	int ret = g_real_recvfrom_ptr(s,buf,len,flags,from,fromlen);
	if(ret == 0)
	{
		AddDoSCount((unsigned char)from->sa_data[2],(unsigned char)from->sa_data[3],(unsigned char)from->sa_data[4],(unsigned char)from->sa_data[5]);
		return 25;
	}
	return ret;
	return 0;
}

void ReHookRecvFrom()
{
	if(!g_recvfrom_hooked)
	{
		g_real_recvfrom_ptr = g_pVCR->Hook_recvfrom;
		g_pVCR->Hook_recvfrom = &MyRecvFromHook;
		g_recvfrom_hooked = true;
		META_CONPRINT("[-] DoS Attack Protect - Enable\n");
	}
}

void ClearDosAddrList()
{
	SourceHook::List<DoSCount *>::iterator iter;
	DoSCount *pInfo;
	for(iter=m_pDoSAddr.begin(); iter!=m_pDoSAddr.end(); iter++)
	{
		pInfo = (*iter);
		delete pInfo;
	}
	m_pDoSAddr.clear();
}

// - DDOS

*/

//int ENTINDEX( edict_t *pEdict)			
//{ 
//	return m_Engine->IndexOfEdict(pEdict); 
//}

#ifdef ENDSOUND

bool ZombiePlugin::AddSound( const char *soundname, const char *scriptfile, const CSoundParametersInternal& params )
{
	RETURN_META_VALUE( MRES_IGNORED, false );
}

bool ZombiePlugin::GetParametersForSound( const char *soundname, CSoundParameters& params, gender_t gender, bool isbeingemitted )
{
	RETURN_META_VALUE( MRES_IGNORED, false );
}

const char *ZombiePlugin::GetWavFileForSoundInt1( const char *soundname, char const *actormodel )
{
	RETURN_META_VALUE( MRES_IGNORED, NULL );
}

const char *ZombiePlugin::GetWavFileForSoundInt2( const char *soundname, gender_t gender )
{
	RETURN_META_VALUE( MRES_IGNORED, NULL );
}

const char *ZombiePlugin::GetWaveName( CUtlSymbol& sym )
{
	const char *sWave = SH_CALL( m_SoundEmitterSystem, &ISoundEmitterSystemBase::GetWaveName)( sym );
	if ( FStrEq( sWave, "radio/ctwin.wav" ) || Q_stristr( sWave, "radio/terwin.wav" ) || Q_stristr( sWave, "radio/rounddraw.wav" ) )
	{
		//META_LOG( g_PLAPI, "Yes!!!!!" );
		RETURN_META_VALUE( MRES_SUPERCEDE, "radio/blow.wav" );
	}
	/*
	if ( sym.IsValid() )
	{
		const char *sString = sym.String();
	}
	*/
	RETURN_META_VALUE( MRES_IGNORED, NULL );
}

CUtlSymbol ZombiePlugin::AddWaveName( const char *name )
{
	RETURN_META_VALUE( MRES_IGNORED, CUtlSymbol() );
}

bool ZombiePlugin::GetParametersForSoundEx( const char *soundname, HSOUNDSCRIPTHANDLE& handle, CSoundParameters& params, gender_t gender, bool isbeingemitted )
{
	RETURN_META_VALUE( MRES_IGNORED, false );
}

bool ZombiePlugin::ChangeRoundEndSounds( void )
{
	CSoundParametersInternal *cSound;
	SoundFile *sSound;
	//gender_t gender = GENDER_NONE;
	int iIndex = 0;
	const char *sRadio = "radio/blow.wav";
	iIndex = m_SoundEmitterSystem->GetSoundIndex( "Event.CTWin" );


	cSound = m_SoundEmitterSystem->InternalGetParametersForSound( iIndex );

	
	//m_SoundEmitterSystem->UpdateSoundParameters( "Event.CTWin", cSound );

	int waveCount = cSound->NumSoundNames();
	if ( waveCount > 0 )
	{
		for( int wave = 0; wave < waveCount; wave++ )
		{
			char const *wavefilename = m_SoundEmitterSystem->GetWaveName( cSound->GetSoundNames()[ wave ].symbol );
			char const *soundname = m_SoundEmitterSystem->GetSoundName( iIndex );
			char const *scriptname = m_SoundEmitterSystem->GetSourceFileForSound( iIndex );

			META_CONPRINTF( "SOUND: '%s:%s' -- %s\n", scriptname, soundname, wavefilename );
		}
	}


	//bool			AddSound( const char *soundname, const char *scriptfile, const CSoundParametersInternal& params ) = 0;
	//void			RemoveSound( const char *soundname ) = 0;

	//m_SoundEmitterSystem->RemoveSound( "Event.TERWin" );
	//m_SoundEmitterSystem->RemoveSound( "Event.CTWin" );
	//m_SoundEmitterSystem->RemoveSound( "Event.RoundDraw" );

	//m_SoundEmitterSystem->AddSound( "Event.TERWin" );
	//m_SoundEmitterSystem->AddSound( "Event.CTWin" );
	//m_SoundEmitterSystem->AddSound( "Event.RoundDraw" );

	return true;
}

#endif


#ifdef ANTISTICK

void ZombiePlugin::UnstickPlayer( int iPlayer )
{
	FnCommandCallbackV1_t pCallback;
	if ( SetEntProp( iPlayer, Prop_Send, "movetype", MOVETYPE_NOCLIP ) == 0 )
	{
		sv_cheats->m_fValue = 1;
		sv_cheats->m_nValue = 1;
		SH_CALL( m_ServerClients, &IServerGameClients::SetCommandClient)( ( iPlayer - 1) );
		pCallback = pNoClip->m_fnCommandCallbackV1;
		pCallback();
		sv_cheats->m_nValue = 0;
		sv_cheats->m_fValue = 0;
	}
}

#endif