/* ======== ZombieMod_mm ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): David "BAILOPAN" Anderson
* ============================
*/

#include <oslink.h>
#include "ZombiePlugin.h"
#include "cvars.h"
#include "myutils.h"
#include "eiface.h"
#include "KeyValues.h"
#include "VFunc.h"

#include "EventQueue.h"

#ifdef WIN32
//#include "CDetour.h"
//CDetour						cPrintDetour;
#endif

myString					sRestrictedWeapons;

CUtlVector<TimerInfo_t>	g_RespawnTimers;

ZombiePlugin				g_ZombiePlugin;
PlayerInfo_t				g_Players[65];
ZombieClasses_t				g_ZombieClasses[100];
int							g_iZombieClasses = -1;
int							g_RestrictT[CSW_MAX];
int							g_RestrictCT[CSW_MAX];
int							g_RestrictW[CSW_MAX];

IGameEventManager2			*m_GameEventManager;	
IEngineTrace				*m_EngineTrace;
IServerGameDLL				*m_ServerDll;
IVEngineServer				*m_Engine;
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
CreateEntityByNameFunction	g_CreateEntityByName = NULL;
TermRoundFunction			g_TermRoundFunc = NULL;
GiveNamedItem_Func			g_GiveNamedItemFunc = NULL;
GetNameFunction				g_GetNameFunc = NULL;
WeaponDropFunction			g_WeaponDropFunc = NULL;
WantsLagFunction			g_WantsLag = NULL;
OnTakeDamageFunction		g_TakeDamage = NULL;
GetFileWeaponInfoFromHandleFunc g_GetFileWeaponInfoFromHandle = NULL;
CCSPlayer_RoundRespawnFunc  g_CCSPlayer_RoundRespawn = NULL;
IStaticPropMgrServer		*staticpropmgr = NULL;

void						*v_GiveNamedItem = NULL;
void						*v_SetFOV = NULL;
void						*v_SwitchTeam = NULL;
void						*v_ApplyAbsVelocity = NULL;
void						*v_IsInBuyZone = NULL;
void						*v_KeyValues = NULL;

ClientPrint_Func			v_ClientPrint = NULL;
CreateEntityByNameFunction	v_CreateEntityByName = NULL;

CGameRules					*g_pGameRules = NULL;
CBaseEntityList				*g_pEntityList = NULL;
//CGlobalEntityList			*g_EntList = NULL;
CUtlVector<ModelInfo_t>		g_ZombieModels;
CUtlVector<CBaseEntity *>	g_HookedWeapons;
ConCommand					*pKillCmd = NULL;
ConCommand					*pGiveCmd = NULL;
//ConCommand					*pEntFireCmd = NULL;
ConCommand					*pRadarCmd = NULL;
bool						bUsingCVAR = false;
bool						bChangeCheats = false;

bool						bRoundTerminating = false;
void						*vUClient;
char						g_lagbuf[WantsLagComp_OffsetBytes];
char						g_hackbuf[OnTakeDamage_OffsetBytes];
char						g_hackbuf1[OnTakeDamage_OffsetBytes];
bool						g_ffa = false;
int							iBeamLaser = 0;
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

bool bBotAdded		= false;
bool						bKickStart[MAX_PLAYERS];
unsigned int				iKickTimer = -1;

unsigned int				iGlobalVoteTimer = 0;

bool						bLanguagesLoaded = false;

KeyValues					*kLanguage = NULL;
myString					sLangError;

unsigned int ibTimer = -1;


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
			snprintf(error, maxlen, "Could not find interface %s", name); \
		return false; \
	}

inline bool IsBadPtr( void *p ) { return ( (p==NULL)||(p==(void*)(-1)) ); }

#define	Debug() return zombie_debug.GetBool()


ConVar *mp_friendlyfire = NULL;
ConVar *mp_limitteams = NULL;
ConVar *mp_autoteambalance = NULL;
ConVar *mp_spawnprotectiontime = NULL;
ConVar *mp_tkpunish = NULL;
ConVar *sv_alltalk = NULL;
ConVar *sv_cheats = NULL;
ConVar *rcon_password = NULL;
ConVar *mp_roundtime = NULL;
ConVar *mp_restartgame = NULL;
ConVar *ammo_buckshot_max = NULL;

#define WEAPONINFO_COUNT 29
static int g_MaxClip[WEAPONINFO_COUNT];
static int g_DefaultClip[WEAPONINFO_COUNT];

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

void CVar_CallBack( ConVar *var, char const *pOldString )
{
	if ( FStrEq( var->GetName(), zombie_unlimited_ammo.GetName() ) )
	{
		g_ZombiePlugin.DoAmmo( zombie_unlimited_ammo.GetBool() );
	}
	else if ( FStrEq( var->GetName(), zombie_restrictions.GetName() ) )
	{
		g_ZombiePlugin.RestrictWeapons();
	}
	else if ( FStrEq( var->GetName(), zombie_enabled.GetName() ) )
	{
		if ( !bUsingCVAR )
		{
			bUsingCVAR = true;
			zombie_enabled.SetValue( pOldString );
			bUsingCVAR = false;
			META_LOG( g_PLAPI, "ERROR: ZombieMod must be enabled/disabled via the 'zombie_mode' command." );
		}
	}
	else if ( zombie_enabled.GetBool() && FStrEq( mp_restartgame->GetName(), var->GetName() ) )
	{
		//FireGameEvent( IGameEvent *event )
		IGameEvent *event = m_GameEventManager->CreateEvent( "round_end", true );
		g_ZombiePlugin.FireGameEvent( event ); 
	}
	else if ( zombie_enabled.GetBool() && ( FStrEq( var->GetName(), mp_friendlyfire->GetName() ) || ( zombie_teams.GetBool() &&  ( FStrEq( var->GetName(), mp_limitteams->GetName() ) || FStrEq( var->GetName(), mp_autoteambalance->GetName() ) ) ) ) )
	{
		if ( var->GetInt() != 0 )
		{
			var->SetValue( 0 );
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
		//g_GetFileWeaponInfoFromHandle(x);
		//CCSWeaponInfo *info = NULL;
		//void *iInfo = g_GetFileWeaponInfoFromHandle(x);
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

bool ZombiePlugin::FireEvent( IGameEvent *event, bool bDontBroadcast )
{
	if ( zombie_enabled.GetBool() && event && event->GetName() )
	{
		if ( zombie_teams.GetBool() && FStrEq( event->GetName(), "player_team" ) )
		{
			RETURN_META_VALUE( MRES_SUPERCEDE, true );
		}
		else if ( FStrEq( event->GetName(), "player_say" ) )
		{
			int iPlayer = EntIdxOfUserIdx( event->GetInt( "userid" ) );
			const char *sText = event->GetString( "text" );
			//Event_Player_Say( iPlayer, sText );
			RETURN_META_VALUE( Event_Player_Say( iPlayer, sText ), true );
		}
	}
	
	RETURN_META_VALUE( MRES_IGNORED, true );
}

void ZombiePlugin::FireGameEvent( IGameEvent *event )
{
	if ( !event || !event->GetName() )
	{
		return;
	}
	const char *name = event->GetName();
	if ( zombie_enabled.GetBool() )
	{
		if ( FStrEq( name, "round_freeze_end") )
		{
			Event_Round_Freeze_End();
			return;
		}
		/*else if ( FStrEq( name, "player_say" ) )
		{
			int iPlayer = EntIdxOfUserIdx( event->GetInt( "userid" ) );
			const char *sText = event->GetString( "text" );
			Event_Player_Say( iPlayer, sText );
			return;
		}*/
		else if ( FStrEq( name, "player_spawn" ) )
		{
			int iPlayer = EntIdxOfUserIdx( event->GetInt( "userid" ) );
			Event_Player_Spawn( iPlayer );
			return;
		}

		else if ( FStrEq( name, "player_death" ) )
		{
			int iVic = EntIdxOfUserIdx( event->GetInt( "userid" ) );
			int iAtt = EntIdxOfUserIdx( event->GetInt( "attacker" ) );
			if ( iVic == -1 )
				return;
			if ( zombie_notices.GetBool() )
			{
				const char *sWeapon = event->GetString( "weapon" );
				if ( sWeapon && FStrEq( sWeapon, zombie_weapon.GetString() ) )
				{
					return;
				}
			}
			Event_Player_Death( iVic, iAtt );
			return;
		}
		else if ( FStrEq( name, "round_end" ) )
		{
			int iWinner = event->GetInt( "winner" );
			Event_Round_End( iWinner );
			for ( int x = 0; x < g_HookedWeapons.Count(); x++ )
			{
				SH_REMOVE_MANUALHOOK_MEMFUNC( GetMaxSpeed_hook, g_HookedWeapons[x], &g_ZombiePlugin, &ZombiePlugin::GetMaxSpeed, true);
				SH_REMOVE_MANUALHOOK_MEMFUNC( Delete_hook, g_HookedWeapons[x], &g_ZombiePlugin, &ZombiePlugin::Delete, true);
				g_HookedWeapons.Remove(x);
			}
			return;
		}
		else if ( FStrEq( name, "hostage_follows" ) )
		{
			int iHosti = event->GetInt( "hostage" );
			Event_Hostage_Follows( iHosti );
			return;
		}
		else if ( FStrEq( name, "player_jump" ) )
		{
			int iUserID = event->GetInt( "userid" );
			int iPlayer = EntIdxOfUserIdx( iUserID );
			CBaseEntity *cEntity;
			if ( IsValidPlayer( iPlayer, &cEntity ) )
			{
				Event_Player_Jump( iPlayer, cEntity );
			}
		}
	}
	if ( FStrEq( name, "round_start" ) )
	{
		Event_Round_Start( );
	}
	return;
}

bool ZombiePlugin::LevelInit_Pre(const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background)
{	
	if ( zombie_enabled.GetBool() )
	{
		m_Engine->ServerCommand( "exec zombiemod/zombiemod.cfg\n" );
		if ( zombie_fog.GetBool() && zombie_enabled.GetBool() && pMapName != NULL )
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
	Q_snprintf( sTmp, sizeof(sTmp), "cfg/zombiemod/%s.cfg", g_CurrentMap );
	if ( m_FileSystem->FileExists( sTmp ) )
	{
		Q_snprintf( sTmp, sizeof(sTmp), "exec zombiemod/%s.cfg\n", g_CurrentMap );
		m_Engine->ServerCommand( sTmp );
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
	g_Offsets.m_lifeState = UTIL_FindOffset( "CBasePlayer", "m_lifeState" );
	g_Offsets.m_Local = UTIL_FindOffsetTable( "CBasePlayer", "localdata", "m_Local" );
	g_Offsets.m_iFOV = UTIL_FindOffsetTable( "CBasePlayer", "localdata", "m_Local", "m_iFOV" );
	g_Offsets.m_bDrawViewmodel = UTIL_FindOffsetTable( "CBasePlayer", "localdata", "m_Local", "m_bDrawViewmodel" );
	g_Offsets.m_iHideHUD = UTIL_FindOffsetTable( "CBasePlayer", "localdata", "m_Local", "m_iHideHUD" );
	g_Offsets.m_bInBuyZone = UTIL_FindOffset( "CCSPlayer", "m_bInBuyZone" );
	g_Offsets.m_vecOrigin = UTIL_FindOffset( "CBaseEntity", "m_vecOrigin" );
	//g_Offsets.m_MoveType = UTIL_FindOffset( "CBasePlayer", "m_MoveType" );
	g_Offsets.m_MoveType = UTIL_FindOffset( "CBaseEntity", "movetype" );
	g_Offsets.m_Collision = UTIL_FindOffset( "CBaseEntity", "m_Collision" );
	g_Offsets.m_clrRender = UTIL_FindOffset( "CBaseEntity", "m_clrRender" );
	g_Offsets.m_nRenderMode = UTIL_FindOffset( "CBaseEntity", "m_nRenderMode" );
	g_Offsets.m_hRagdoll = UTIL_FindOffset( "CCSPlayer", "m_hRagdoll" );

	for ( x = 0; x < m_Engine->GetEntityCount(); x++ )
	{
		edict_t *pTmp = m_Engine->PEntityOfEntIndex( x );
		if ( pTmp && !pTmp->IsFree() /*&& pTmp->GetClassName()*/ )
		{
			if ( FStrEq( pTmp->GetClassName(), "cs_team_manager" ) )
			{
				CBaseEntity *pBase = pTmp->GetUnknown()->GetBaseEntity();
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

	m_GameEventManager->AddListener( this, "player_spawn", true );
	m_GameEventManager->AddListener( this, "round_start", true );
	m_GameEventManager->AddListener( this, "round_end", true );
	m_GameEventManager->AddListener( this, "player_death", true );
	m_GameEventManager->AddListener( this, "player_say", true );
	m_GameEventManager->AddListener( this, "round_freeze_end", true );
	m_GameEventManager->AddListener( this, "hostage_follows", true );
	m_GameEventManager->AddListener( this, "player_jump", true );


	ibTimer = -1;
	bRoundStarted = false;
	if ( pMapName != NULL )
	{
		mp_friendlyfire = m_CVar->FindVar("mp_friendlyfire");
		mp_spawnprotectiontime = m_CVar->FindVar("mp_spawnprotectiontime");
		mp_tkpunish = m_CVar->FindVar("mp_tkpunish");
		sv_alltalk = m_CVar->FindVar("sv_alltalk");
		ammo_buckshot_max = m_CVar->FindVar("ammo_buckshot_max");
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

	

	//g_bZombieClasses  =  ( zombie_classes.GetBool() &&  LoadZombieClasses() );

	m_Engine->ServerCommand( "sv_maxspeed 1000\n" );

	INetworkStringTable *pInfoPanel = m_NetworkStringTable->FindTable("InfoPanel");
	if (pInfoPanel)
	{
		bool save = m_Engine->LockNetworkStringTables(false);
		DisplayHelp( pInfoPanel );
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
		/*Q_strcpy( sTmp, "\"info_player\"\n" );
		Q_strcat( sTmp, Sub_Str( sEntString, iStart, iEnd - iStart + 1 ).c_str() );*/

		Q_snprintf( sTmp, sizeof(sTmp), "\"%s\"\n%s", sEntity, Sub_Str( sEntString, iStart, iEnd - iStart + 1 ).c_str() );

		KeyValues *kv = new KeyValues( "" );

		kv->LoadFromBuffer( sEntity, sTmp );
		return myString( kv->GetString( "origin" ) );
	}
	return myString("");
}

const char *ZombiePlugin::ParseAndFilter( const char *map, const char *ents )
{
	/*
	const char *sSound = "{\n" \
							"\"origin\" \"%s\"\n" \
							"\"message\" \"ambient/zombiemod/zombie_ambient.mp3\"\n" \
							"\"health\" \"6\"\n" \
							"\"pitch\" \"100\"\n" \
							"\"pitchstart\" \"100\"\n" \
							"\"spawnflags\" \"1\"\n" \
							"\"radius\" \"1250\"\n" \
							"\"volstart\" \"6\"\n" \
							"\"classname\" \"ambient_generic\"\n" \
						"}\n";
	*/

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

	/*
	
	myString sSoundLocations[3];
	Vector vSounds[4];

	sSoundLocations[0].assign( ExtractLocationFromEnt( ents, iLen, "info_player_terrorist" ).c_str() );
	sSoundLocations[1].assign( ExtractLocationFromEnt( ents, iLen, "info_player_counterterrorist" ).c_str() );
	sSoundLocations[2].assign( ExtractLocationFromEnt( ents, iLen, "hostage_entity" ).c_str() );
	if ( sSoundLocations[2].size() == 0 )
	{
		sSoundLocations[2].assign( ExtractLocationFromEnt( ents, iLen, "decals/siteb" ).c_str() );
		if ( sSoundLocations[2].size() == 0 )
		{
			vSounds[2].Init();
		}
		else
		{
			UTIL_StringToVector( vSounds[2].Base(), sSoundLocations[2].c_str() );
		}
	}

	UTIL_StringToVector( vSounds[0].Base(), sSoundLocations[0].c_str() );
	UTIL_StringToVector( vSounds[1].Base(), sSoundLocations[1].c_str() );
	vSounds[3] = UTIL_MidPoint( vSounds[0], vSounds[1] );
	*/

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
	
	//DumpMapEnts();
	/*
	char sTmps[1024] = "";
	int x = 0;
	for ( x = 0; x < 3; x++ )
	{
		Q_snprintf( sTmps, 1024, sSound, sSoundLocations[x].c_str() );
	 	g_MapEntities.append( sTmps );
	}
 	*/
	return g_MapEntities.c_str();
}

void ZombiePlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
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
	iBeamLaser = m_Engine->PrecacheModel( "sprites/bluelaser1.vmt" );
	m_EngineSound->PrecacheSound("npc/fast_zombie/fz_scream1.wav", true);

	if ( Q_strlen( zombie_vision_material.GetString() ) != 0 )
	{
		Q_snprintf( buf, sizeof(buf), "vgui/hud/zombiemod/%s_dx6", zombie_vision_material.GetString() );
		//m_Engine->PrecacheDecal( "vgui/hud/zombiemod/zombie_vision_dx6", true );
		m_Engine->PrecacheDecal( buf, true );
		Q_snprintf( buf, sizeof(buf), "vgui/hud/zombiemod/%s", zombie_vision_material.GetString() );
		m_Engine->PrecacheDecal( buf, true );
		//m_Engine->PrecacheDecal( "vgui/hud/zombiemod/zombie_vision", true );
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

void ZombiePlugin::LevelShutdown( void )
{
	int x;
	for ( x = 0; x < g_HookedWeapons.Count(); x++ )
	{
		SH_REMOVE_MANUALHOOK_MEMFUNC( GetMaxSpeed_hook, g_HookedWeapons[x], &g_ZombiePlugin, &ZombiePlugin::GetMaxSpeed, true);
		SH_REMOVE_MANUALHOOK_MEMFUNC( Delete_hook, g_HookedWeapons[x], &g_ZombiePlugin, &ZombiePlugin::Delete, true);
		g_HookedWeapons.Remove(x);
	}
	for ( x = 1; x <= MAX_CLIENTS; x++ )
	{
		edict_t *pEntity = m_Engine->PEntityOfEntIndex( x );
		if ( pEntity && !pEntity->IsFree() && m_Engine->GetPlayerUserId( pEntity ) != -1 )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)GetContainingEntity( pEntity );
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
		if ( kvPlayerClasses->GetFirstSubKey() )
		{
			kvPlayerClasses->SaveToFile( m_FileSystem, "cfg/zombiemod/player_classes.kv", "MOD" );
		}
	}

	g_MapEntities.clear();
	g_Timers->Reset();
	m_GameEventManager->RemoveListener( this );
	RETURN_META(MRES_IGNORED);
}

int ZombiePlugin::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !zombie_enabled.GetBool() )
	{
		RETURN_META_VALUE( MRES_IGNORED, 0 );
	}
	CBasePlayer *pAttacker = NULL;
	CBaseEntity *pBase = NULL;
	CBaseEntity *pInflictor = NULL;
	bool bIsGrenade = false;

	CBasePlayer *pVictim =  META_IFACEPTR( CBasePlayer );
	edict_t *pEdict = m_GameEnts->BaseEntityToEdict( pVictim );
	edict_t *edict = NULL;
	edict_t *pAEdict = NULL;
	if ( pVictim && pEdict && !pEdict->IsFree() )
	{
		int nIndex = m_Engine->IndexOfEdict( pEdict );
		int AIndex = -1;

		if ( info.m_hAttacker.GetEntryIndex() > 0 && info.m_hAttacker.GetEntryIndex() <= MAX_CLIENTS ) 
		{
			pAEdict = m_Engine->PEntityOfEntIndex( info.m_hAttacker.GetEntryIndex() );
			if ( pAEdict )
			{
				pAttacker = (CBasePlayer*)pAEdict->GetUnknown()->GetBaseEntity();
				//pAttacker = g_Players[info.m_hAttacker.GetEntryIndex()].pPlayer;
				AIndex = m_Engine->IndexOfEdict( m_GameEnts->BaseEntityToEdict( pAttacker ) );
			}
		}
		if ( info.m_hInflictor.GetEntryIndex() > 0 ) 
		{
			edict = m_Engine->PEntityOfEntIndex(info.m_hInflictor.GetEntryIndex() );
			if ( edict ) 
			{
				pInflictor = edict->GetUnknown()->GetBaseEntity();
			}
		}

		bool isZombie = g_Players[nIndex].isZombie;
		int iHealth = 0;
		float fDamage = 0.0f;
		int iBonus = 0;

		if ( pAttacker )
		{
			bool attackerZombie = g_Players[AIndex].isZombie;
			if ( attackerZombie && !isZombie )
			{
				if ( g_Players[nIndex].bProtected || ( pInflictor && pInflictor->GetClassname() && Q_stristr( pInflictor->GetClassname(), "nade" ) ) )
				{
					RETURN_META_VALUE( MRES_SUPERCEDE, 0 );
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
					IGameEvent *event = m_GameEventManager->CreateEvent( "player_death", true );
					if ( event )
					{
						event->SetString( "weapon", zombie_weapon.GetString() );
						event->SetBool( "headshot", false );
						event->SetInt( "userid", m_Engine->GetPlayerUserId( pEdict ) );
						event->SetInt( "attacker", m_Engine->GetPlayerUserId( pAEdict ) );
						m_GameEventManager->FireEvent( event );
					}

					event = m_GameEventManager->CreateEvent( "zombification", true );
					if ( event )
					{
						event->SetInt( "userid", m_Engine->GetPlayerUserId( pEdict ) );
						event->SetInt( "attacker", m_Engine->GetPlayerUserId( pAEdict ) );
						event->SetInt( "class", g_Players[AIndex].iClass );
						m_GameEventManager->FireEvent( event );
					}
				}

				if ( zombie_zombie_money.GetInt() > 0 )
				{
					GiveMoney( pAEdict, zombie_zombie_money.GetInt() );
				}

				int iBonus = -919;
				if ( g_bZombieClasses && zombie_classes.GetBool() && g_Players[nIndex].iClass > -1 && g_Players[nIndex].iClass <= g_iZombieClasses )
				{
					iBonus = g_ZombieClasses[g_Players[nIndex].iClass].iHealthBonus;
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

				MakeZombie( pVictim, zombie_health.GetInt() );


				//TODO-
				MRecipientFilter filter;
				filter.AddAllPlayers( MAX_CLIENTS );
				m_EngineSound->EmitSound( filter, nIndex, CHAN_VOICE, "npc/fast_zombie/fz_scream1.wav", RandomFloat( 0.6, 1.0 ), 0.5, 0, RandomInt( 70, 150 ) );

				if ( zombie_damagelist.GetBool() )
				{
					void *params[1];
					params[0] = (void *)pEdict;
					params[1] = (void *)pAEdict;
					g_Timers->AddTimer( 1.0, TimerShowVictims, params, 2 );
				}

				RETURN_META_VALUE( MRES_SUPERCEDE, 0 );
			}
			else if ( ( !isZombie && !attackerZombie ) || ( isZombie && attackerZombie ) )
			{
				RETURN_META_VALUE( MRES_SUPERCEDE, 0 );
			}
			else if ( isZombie )
			{

				float fMultiplier;
				fDamage = info.m_flDamage;

				UTIL_GetProperty( g_Offsets.m_iHealth, pEdict, &iHealth );

				if ( pInflictor && pInflictor->GetClassname() && Q_stristr( pInflictor->GetClassname(), "nade" ) )
				{ // Damage multipler for grenades.
					bIsGrenade = true;
					fMultiplier = zombie_grenade_damage_multiplier.GetFloat();

					if ( g_bZombieClasses && zombie_classes.GetBool() && g_Players[nIndex].iClass > -1 && g_Players[nIndex].iClass <= g_iZombieClasses )
					{
						fMultiplier = g_ZombieClasses[g_Players[nIndex].iClass].fGrenadeMultiplier;
					}
					
					fDamage = fDamage * fMultiplier;

					
					iHealth -= fDamage;
					UTIL_SetProperty( g_Offsets.m_iHealth, pEdict, iHealth );
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

				MRecipientFilter filter;
				filter.AddAllPlayers( MAX_CLIENTS );//g_SMAPI->pGlobals()->maxClients );
				
				Q_snprintf(sound, sizeof(sound), "npc/zombie/zombie_pain%d.wav", RandomInt(1,6));
				m_EngineSound->EmitSound( filter, nIndex, CHAN_VOICE, sound, RandomFloat(0.6, 1.0), 0.5, 0, RandomInt(70, 150) );

				float fKnockback = 0.0f;
				float fKnockbackMult = 0.0f;
				fKnockback = zombie_knockback.GetFloat();
				fKnockbackMult = zombie_grenade_knockback_multiplier.GetFloat();

				if ( g_bZombieClasses && zombie_classes.GetBool() && g_Players[nIndex].iClass > -1 && g_Players[nIndex].iClass <= g_iZombieClasses )
				{
					float fTmp = ( g_ZombieClasses[g_Players[nIndex].iClass].fKnockback / 100 ) * zombie_knockback_percent.GetInt();
					fKnockback = fTmp;

					fKnockbackMult = g_ZombieClasses[g_Players[nIndex].iClass].fGrenadeKnockback;
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

					Vector vecDir = pInflictor->WorldSpaceCenter() - Vector (0, 0, 10) - pVictim->WorldSpaceCenter();
					VectorNormalize( vecDir );
					Vector force = vecDir * -DamageForce( pInflictor->WorldAlignSize(), info.GetBaseDamage(), fKnockback );
					if ( force.z > 250.0f )
					{
							force.z = 250.0f;
					}
					CBasePlayer_ApplyAbsVelocityImpulse( pVictim, force );
				}
			}
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
		//RETURN_META_VALUE_NEWPARAMS( MRES_SUPERCEDE, true, &IVoiceServer::SetClientListening, ( iReceiver, iSender, true ) );
		return_value = SH_CALL( m_VoiceServer_CC, &IVoiceServer::SetClientListening )( iReceiver, iSender, return_value );
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
	/*if ( !pEntity || pEntity->IsFree() || !pEntity->GetUnknown() || !pEntity->GetUnknown()->GetBaseEntity() || m_Engine->GetPlayerUserId( pEntity ) == -1 )
	{
		return false;
	}
	return pEntity->GetUnknown()->GetBaseEntity()->IsAlive();
	IPlayerInfo *iInfo;
	iInfo = m_PlayerInfoManager->GetPlayerInfo( pEntity );*/
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
				//if ( pBase && m_GameEnts->BaseEntityToEdict( pBase ) )
				//{
					if ( !IsValidPlayer( edict, &iAttacker ) )
					{
						RETURN_META( MRES_IGNORED );
					}
					else
					{
						pBase = edict->GetUnknown()->GetBaseEntity();
					}
					//iAttacker = m_Engine->IndexOfEdict( m_GameEnts->BaseEntityToEdict( pBase ) );
				//}
			}
		}
		if ( pBase && m_GameEnts->BaseEntityToEdict( pBase ) && m_Engine->GetPlayerUserId( m_GameEnts->BaseEntityToEdict( pPlayer ) ) != -1 )
		{
			bool bAttacker = g_Players[iAttacker].isZombie;
			bool bVictim = g_Players[iPlayer].isZombie;
			bool bHeadShotsOnly = false;
			if ( g_bZombieClasses && zombie_classes.GetBool() && g_Players[iPlayer].iClass >= 0 && g_Players[iPlayer].iClass <= g_iZombieClasses )
			{
				bHeadShotsOnly = g_ZombieClasses[ g_Players[iPlayer].iClass ].bHeadShotsOnly;
			}

			if ( g_Players[iPlayer].bProtected || ( bAttacker && bVictim ) || ( !bAttacker && !bVictim ) || 
					( ( zombie_headshots_only.GetBool() || bHeadShotsOnly ) && ptr->hitbox == HITGROUP_HEAD ) )
			{
				RETURN_META( MRES_SUPERCEDE );
			}

			g_Players[iPlayer].isHeadshot = ( ptr && ptr->hitgroup == 1 && iAttacker > 0 && iAttacker <= MAX_CLIENTS );
			g_Players[iPlayer].vLastHit = ptr->endpos;
			g_Players[iPlayer].vDirection = vecDir;
			if ( g_Players[iPlayer].isHeadshot )
			{
				int iShots = 0;
				if ( g_bZombieClasses && zombie_classes.GetBool() && g_Players[iPlayer].iClass >= 0 && g_Players[iPlayer].iClass <= g_iZombieClasses )
				{
					iShots = g_ZombieClasses[ g_Players[iPlayer].iClass ].iHeadshots;
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
				META_LOG( g_PLAPI, "Random zombie now." );
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

void ZombiePlugin::Delete()
{
	CBaseEntity *pBase = META_IFACEPTR(CBaseEntity);
	int HWidx = g_HookedWeapons.Find( pBase );
	if ( HWidx != -1 )
	{
		SH_REMOVE_MANUALHOOK_MEMFUNC(GetMaxSpeed_hook, pBase, &g_ZombiePlugin, &ZombiePlugin::GetMaxSpeed, true);
		SH_REMOVE_MANUALHOOK_MEMFUNC(Delete_hook, pBase, &g_ZombiePlugin, &ZombiePlugin::Delete, true);
		g_HookedWeapons.Remove(HWidx);
	}
	RETURN_META(MRES_IGNORED);
}

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
						int iSpeed = ( (float)g_ZombieClasses[ g_Players[x].iClass ].iSpeed / 100 ) * (float)zombie_speed_percent.GetInt();
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

	/*
	if ( msg_type == 26 )
	{
		bf_write *bfNew = NULL;
		RETURN_META_VALUE_NEWPARAMS( MRES_HANDLED, bfNew, &IVEngineServer::UserMessageBegin, ( filter, 2 ) );
	}
	*/
	RETURN_META_VALUE( MRES_IGNORED, NULL );
}

void ZombiePlugin::CommitSuicide()
{
	CBasePlayer *pPlayer = META_IFACEPTR( CBasePlayer );
	//if ( IsBadPtr( pPlayer ) || IsBadPtr( m_GameEnts->BaseEntityToEdict( pPlayer ) ) )
	if ( !IsValidPlayer( pPlayer ) )
	{
		RETURN_META( MRES_IGNORED );
	}
	if ( !zombie_enabled.GetBool() || (!zombie_changeteam_block.GetBool() && !zombie_suicide.GetBool() ))
	{
		RETURN_META( MRES_IGNORED );
	}
	void *params[1];
	params[0] = (void *)pPlayer;
	g_Timers->AddTimer( 0.1, Timed_SetModel, params, 1 );
	RETURN_META( MRES_SUPERCEDE );
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

void ZombiePlugin::HookPlayer( int iPlayer )
{
	if ( iPlayer > 0 && iPlayer <= MAX_PLAYERS && !g_Players[iPlayer].isHooked && !IsBadPtr( g_Players[iPlayer].pPlayer ) )
	{
		//META_LOG( g_PLAPI, "Hooking %p", g_Players[iPlayer].pPlayer );

		g_Players[iPlayer].pCallClass = SH_GET_MCALLCLASS( g_Players[iPlayer].pPlayer, sizeof(void*) );

		SH_ADD_MANUALHOOK_MEMFUNC( Event_Killed_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Event_Killed, false );
		SH_ADD_MANUALHOOK_MEMFUNC( CommitSuicide_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::CommitSuicide, false );
		SH_ADD_MANUALHOOK_MEMFUNC( ChangeTeam_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::ChangeTeam, false );
		//SH_ADD_MANUALHOOK_MEMFUNC( Touch_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Touch, false );
		SH_ADD_MANUALHOOK_MEMFUNC( Weapon_CanUse_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Weapon_CanUse, false );
		SH_ADD_MANUALHOOK_MEMFUNC( OnTakeDamage_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::OnTakeDamage, false );
		//SH_ADD_MANUALHOOK_MEMFUNC( PlayStepSound_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::PlayStepSound, false );
		SH_ADD_MANUALHOOK_MEMFUNC( TraceAttack_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::TraceAttack, false );
		SH_ADD_MANUALHOOK_MEMFUNC( Weapon_CanSwitchTo_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Weapon_CanSwitchTo, false );
		//SH_ADD_MANUALHOOK_MEMFUNC( DoMuzzleFlash_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::DoMuzzleFlash, false );
	}
	g_Players[iPlayer].isHooked = true;
}

void ZombiePlugin::UnHookPlayer( int iPlayer )
{
	if ( iPlayer > 0 && iPlayer <= MAX_PLAYERS && g_Players[iPlayer].isHooked && !IsBadPtr( g_Players[iPlayer].pPlayer ) )
	{
		//META_LOG( g_PLAPI, "UnHooking %p", g_Players[iPlayer].pPlayer );

		SH_REMOVE_MANUALHOOK_MEMFUNC( Event_Killed_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Event_Killed, false );
		SH_REMOVE_MANUALHOOK_MEMFUNC( CommitSuicide_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::CommitSuicide, false );
		SH_REMOVE_MANUALHOOK_MEMFUNC( ChangeTeam_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::ChangeTeam, false );
		//SH_REMOVE_MANUALHOOK_MEMFUNC( Touch_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Touch, false );
		SH_REMOVE_MANUALHOOK_MEMFUNC( Weapon_CanUse_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Weapon_CanUse, false );
		SH_REMOVE_MANUALHOOK_MEMFUNC( OnTakeDamage_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::OnTakeDamage, false );
		SH_REMOVE_MANUALHOOK_MEMFUNC( TraceAttack_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::TraceAttack, false );
		//SH_REMOVE_MANUALHOOK_MEMFUNC( PlayStepSound_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::PlayStepSound, false );
		SH_REMOVE_MANUALHOOK_MEMFUNC( Weapon_CanSwitchTo_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Weapon_CanSwitchTo, false );
		//SH_REMOVE_MANUALHOOK_MEMFUNC( DoMuzzleFlash_hook, g_Players[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::DoMuzzleFlash, false );
		
		SH_RELEASE_CALLCLASS( g_Players[iPlayer].pCallClass );
		g_Players[iPlayer].pCallClass = NULL;
	}
	g_Players[iPlayer].isHooked = false;
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
		const char *sSteamID = m_Engine->GetPlayerNetworkIDString( pEntity );
		g_Players[iPlayer].isBot = ( sSteamID ? ( Q_stricmp( sSteamID, "BOT" ) == 0 ) : true );
		CBasePlayer *pBase = (CBasePlayer *)GetContainingEntity( pEntity );
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
				//m_Engine->ClientCommand( pEntity, "toggle cl_restrict_server_commands 0" );
				/*
				m_Helpers->ClientCommand( pEntity, "toggle cl_restrict_server_commands 0" );
				m_Engine->ClientCommand( pEntity, "cl_minmodels 0" );
				*/
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
			g_Players[iPlayer].iClass = g_iDefaultClass;
			g_Players[iPlayer].iChangeToClass = -1;
			g_Players[iPlayer].bShowClassMenu = false;
			g_Players[iPlayer].bChoseClass = false;
			IPlayerInfo *iInfo = m_PlayerInfoManager->GetPlayerInfo( pEntity );
			if ( iInfo && zombie_classes.GetBool() )
			{
				if ( zombie_classes_random.GetInt() > 0 )
				{
					int iChangeTo = 0;
					iChangeTo = RandomInt( 0, g_iZombieClasses );
					SelectZombieClass( pEntity, iPlayer, iChangeTo, true, false );
				}
				else
				{
					int iClass = kvPlayerClasses->GetInt( iInfo->GetNetworkIDString(), -1 );
					if ( iClass > -1 && iClass <= g_iZombieClasses )
					{
						SelectZombieClass( pEntity, iPlayer, iClass, true, false );
					}
					else
					{
						g_Players[iPlayer].bShowClassMenu = true;
						SelectZombieClass( pEntity, iPlayer, g_iDefaultClass, true, false );
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

bool ZombiePlugin::ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	RETURN_META_VALUE(MRES_IGNORED, true);
}
*/
void ZombiePlugin::ClientCommand(edict_t *pEntity)
{
	const char *sCmd = m_Engine->Cmd_Argv(0);
	if ( !pEntity || !pEntity->GetUnknown() || !pEntity->GetUnknown()->GetBaseEntity() || !sCmd )
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
			iFOV = Q_atoi( m_Engine->Cmd_Argv(1) );
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
		CBasePlayer *pPlayer = (CBasePlayer *)GetContainingEntity( pEntity );
		if ( !pPlayer )
		{
			RETURN_META( MRES_IGNORED );
		}
		CheckAutobuy( iPlayer, pPlayer );
		if ( FStrEq( sCmd, "takethem" ) )
		{
			//ZombieDropWeapons( pPlayer, false );
			GiveWeapons( iPlayer, pEntity );
			RETURN_META( MRES_SUPERCEDE );
		}
		else if ( FStrEq( sCmd, "givethem" ) )
		{
			GiveWeapons( iPlayer, pEntity );
			RETURN_META( MRES_SUPERCEDE );
		}
		if ( ( m_Engine->Cmd_Argc() > 1 ) && FStrEq( sCmd, "buy") )
		{
			int WeaponID = LookupBuyID(m_Engine->Cmd_Argv(1));
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
					UTIL_GiveNamedItem( pPlayer, m_Engine->Cmd_Argv(1), 0 );
				}*/
			//}
		}
		else if ( ( ( zombie_jetpack.GetBool() && g_Players[iPlayer].isZombie ) || ( humans_jetpack.GetBool() && !g_Players[iPlayer].isZombie ) ) && FStrEq( sCmd, "+jetpack" ) )
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
				Vector pos = pEntity->GetCollideable()->GetCollisionOrigin(); 
				MRecipientFilter mrf;
				mrf.AddAllPlayers( MAX_CLIENTS );//g_SMAPI->pGlobals()->maxClients );
				m_EngineSound->EmitSound((IRecipientFilter &)mrf, m_Engine->IndexOfEdict(pEntity), CHAN_AUTO, JetpackSound, 0.7, ATTN_NORM, 0, PITCH_NORM, &pos, 0, 0, true, 0, m_Engine->IndexOfEdict(pEntity));
			}
			RETURN_META(MRES_SUPERCEDE);
		}
		else if ( ( zombie_jetpack.GetBool() || humans_jetpack.GetBool() ) && g_Players[iPlayer].bJetPack && FStrEq( sCmd, "-jetpack" ) )
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
		else if ( FStrEq( sCmd, "zombie_vision" ) )
		{
			if ( zombie_allow_disable_nv.GetBool() && g_Players[iPlayer].isZombie )
			{
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
			if ( g_Players[iPlayer].iClassMenu != -1 )
			{
				int iCmd = atoi( m_Engine->Cmd_Argv(1) );
				int iVal = -1;
				int iCommand = 0;
				
				iVal = g_Players[iPlayer].iClassMenu + iCmd -1;
				if ( iCmd == 8 )
				{
					if ( g_Players[iPlayer].iClassMenu == 0 )
					{
						g_Players[iPlayer].iClassMenu = -1;
						RETURN_META( MRES_SUPERCEDE );
					}
					iCommand = 2;
				}
				if ( ( g_iZombieClasses > ( g_Players[iPlayer].iClassMenu + 6 ) ) && iCmd == 9  )
				{
					iCommand = 1;
					// More
				}
				if ( iCommand > 0 || ( iCmd != 0 && iCmd != 10 && ( iVal <= g_iZombieClasses ) ) )
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
						SelectZombieClass( pEntity, iPlayer, iVal );
						g_Players[iPlayer].bChoseClass = true;
						//ZombieClasses_t *c;
						//c = &g_ZombieClasses[iVal];

						//char sFinal[255] = "";
						//char sTmp[255] = "";
						//Q_snprintf( sFinal, sizeof( sFinal ), "\n%s: %s\n====================\n", GetLang("classes_selected"), c->sClassname.c_str() ); //ZombieMod Selected Class
						//
						//#define AddToString( sString, sVar, sFmt, sLang, sAdd, iPcnt, pEnt ) \
						//	Q_snprintf( sVar, sizeof( sVar ), sFmt, sLang, sAdd, iPcnt ); \
						//	if ( Q_strlen( sString ) + Q_strlen( sVar ) > 255 ) \
						//	{ \
						//		m_Engine->ClientPrintf( pEntity, sString ); \
						//		sString[0] = '\0'; \
						//	} \
						//	Q_strncat( sString, sVar, sizeof( sString ), -1 );

						//AddToString( sFinal, sTmp, "%s: %s\n", GetLang("model"), c->sModel.c_str(), 0, pEntity ); //Model
						//
						//int iTmp = ( (float)c->iHealth / 100 ) * (float)zombie_health_percent.GetInt();
						//AddToString( sFinal, sTmp, "%s: %d (%d%%)\n", GetLang("health"), iTmp, zombie_health_percent.GetInt(), pEntity ); //Health
						//
						//iTmp = ( (float)c->iSpeed / 100 ) * (float)zombie_speed_percent.GetInt();
						//AddToString( sFinal, sTmp, "%s: %d (%d%%)\n", GetLang("speed"), iTmp, zombie_speed_percent.GetInt(), pEntity ); //Speed
						//
						//iTmp = ( (float)c->iJumpHeight / 100 ) * (float)zombie_jump_height_percent.GetInt();
						//AddToString( sFinal, sTmp, "%s: %d (%d%%)\n", GetLang("jump_height"), iTmp, zombie_jump_height_percent.GetInt(), pEntity ); //Jump Height
						//
						//float fTmp = ( c->fKnockback / 100 ) * zombie_knockback_percent.GetInt();
						//AddToString( sFinal, sTmp, "%s: %.2f (%d%%)\n", GetLang("knockback"), fTmp, zombie_knockback_percent.GetInt(), pEntity ); //Knockback

						//AddToString( sFinal, sTmp, "%s: %d\n", GetLang("heads_req"), c->iHeadshots, c->iHeadshots, pEntity ); // Headhosts Required


						//AddToString( sFinal, sTmp, "====================\n", "", "", 0, pEntity );

						//if ( Q_strlen( sFinal ) > 0 )
						//{
						//	m_Engine->ClientPrintf( pEntity, sFinal );
						//}

						////HudMessage( iPlayer, 59, 0.4, 0.1, 188, 112, 0, 128, 188, 112, 0, 128, 0, 0.0, 0.0, 100.0, 0.0, sFinal );

						//g_Players[iPlayer].iClassMenu = -1;

						//char sPrint[100];
						//Q_snprintf( sPrint, sizeof(sPrint), GetLang( "change_class_msg" ), c->sClassname.c_str() );
						//Hud_Print( pEntity, sPrint );

						//c = NULL;

					}
				}
				else
				{
					g_Players[iPlayer].iClassMenu = -1;
				}
				RETURN_META( MRES_SUPERCEDE );
			}
			RETURN_META( MRES_IGNORED );
		}
		else if ( FStrEq( sCmd, "zombie_class_menu" ) )
		{
			if ( !g_bZombieClasses || !zombie_classes.GetBool() || g_iZombieClasses == -1 )
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
			filter.AddAllPlayers( MAX_CLIENTS );//g_SMAPI->pGlobals()->maxClients );
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

void ZombiePlugin::SelectZombieClass(edict_t *pEntity, int iPlayer, int iClass, bool bNow, bool bMessages)
{

	if ( iClass == g_Players[iPlayer].iClass )
	{
		g_Players[iPlayer].iChangeToClass = -1;
	}
	else
	{
		if ( bNow )
		{
			g_Players[iPlayer].iClass = iClass;
			g_Players[iPlayer].iChangeToClass = -1;
		}
		else
		{
			g_Players[iPlayer].iChangeToClass = iClass;
		}
	}

	IPlayerInfo *iInfo = m_PlayerInfoManager->GetPlayerInfo( pEntity );
	if ( iInfo )
	{
		kvPlayerClasses->SetInt( iInfo->GetNetworkIDString(), iClass );
	}

	ZombieClasses_t *c;
	c = &g_ZombieClasses[iClass];

	char sFinal[5000] = "";
	char sTmp[255] = "";
	Q_snprintf( sFinal, sizeof( sFinal ), "\n%s: %s\n====================\n", GetLang("classes_selected"), c->sClassname.c_str() ); //ZombieMod Selected Class
	
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

	/*
	#define AddToString( sString, sVar, sFmt, sLang, sAdd, iPcnt, pEnt ) \
		if ( iPcnt == -1 ) \
		{ \
			Q_snprintf( sVar, sizeof( sVar ), sFmt, sLang, sAdd ); \
		} \
		else \
		{ \
			Q_snprintf( sVar, sizeof( sVar ), sFmt, sLang, sAdd, iPcnt ); \
		} \
		if ( ( Q_strlen( sString ) + Q_strlen( sVar ) ) > 255 ) \
		{ \
			m_Engine->ClientPrintf( pEntity, sString ); \
			sString[0] = '\0'; \
		} \
		Q_strncat( sString, sVar, sizeof( sString ), -1 );
	*/

	if ( !g_Players[iPlayer].isBot )
	{
		AddToString( sFinal, sTmp, "%s: %s\n", GetLang("model"), c->sModel.c_str(), 0, pEntity ); //Model
		
		int iTmp = ( (float)c->iHealth / 100 ) * (float)zombie_health_percent.GetInt();
		AddToString( sFinal, sTmp, "%s: %d (%d%%)\n", GetLang("health"), iTmp, zombie_health_percent.GetInt(), pEntity ); //Health
		
		iTmp = ( (float)c->iSpeed / 100 ) * (float)zombie_speed_percent.GetInt();
		AddToString( sFinal, sTmp, "%s: %d (%d%%)\n", GetLang("speed"), iTmp, zombie_speed_percent.GetInt(), pEntity ); //Speed
		
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
		
		AddToString( sFinal, sTmp, "====================\n", "", "", 0, pEntity );

		if ( Q_strlen( sFinal ) > 0 )
		{
			//m_Engine->ClientPrintf( pEntity, sFinal );
			UTIL_ClientPrintf( pEntity, sFinal );
		}
	}

	if ( bMessages )
	{
		char sPrint[100];
		if ( bNow )
		{
			Q_snprintf( sPrint, sizeof(sPrint), GetLang( "change_class_msg_now" ), c->sClassname.c_str() );
		}
		else
		{
			Q_snprintf( sPrint, sizeof(sPrint), GetLang( "change_class_msg" ), c->sClassname.c_str() );
		}
		Hud_Print( pEntity, sPrint );

		c = NULL;
	}

	g_Players[iPlayer].iClassMenu = -1;

	return;
}

bool ZombiePlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	char iface_buffer[255];
	int num = 0;

	strcpy(iface_buffer, INTERFACEVERSION_SERVERGAMEDLL);
	FIND_IFACE(serverFactory, m_ServerDll, num, iface_buffer, IServerGameDLL *)
	strcpy(iface_buffer, INTERFACEVERSION_VENGINESERVER);
	FIND_IFACE(engineFactory, m_Engine, num, iface_buffer, IVEngineServer *)
	strcpy(iface_buffer, INTERFACEVERSION_SERVERGAMECLIENTS);
	FIND_IFACE(serverFactory, m_ServerClients, num, iface_buffer, IServerGameClients *)
	strcpy(iface_buffer, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	FIND_IFACE(engineFactory, m_GameEventManager, num, iface_buffer, IGameEventManager2 *);
	strcpy(iface_buffer, INTERFACEVERSION_PLAYERINFOMANAGER); // m_PlayerInfoManager
	FIND_IFACE(serverFactory, m_PlayerInfoManager, num, iface_buffer, IPlayerInfoManager *);
	strcpy(iface_buffer, INTERFACENAME_NETWORKSTRINGTABLESERVER); // m_NetworkStringTable
	FIND_IFACE(engineFactory, m_NetworkStringTable, num, iface_buffer, INetworkStringTableContainer *);
	strcpy(iface_buffer, FILESYSTEM_INTERFACE_VERSION); // m_FileSystem
	FIND_IFACE(engineFactory, m_FileSystem, num, iface_buffer, IFileSystem *);
	strcpy(iface_buffer, VENGINE_CVAR_INTERFACE_VERSION); // m_CVar
	FIND_IFACE(engineFactory, m_CVar, num, iface_buffer, ICvar *);
	strcpy(iface_buffer, IENGINESOUND_SERVER_INTERFACE_VERSION); // m_EngineSound
	FIND_IFACE(engineFactory, m_EngineSound, num, iface_buffer, IEngineSound *);
	strcpy(iface_buffer, IEFFECTS_INTERFACE_VERSION); // m_Effects
	FIND_IFACE(serverFactory, m_Effects, num, iface_buffer, IEffects *);
	strcpy(iface_buffer, VMODELINFO_SERVER_INTERFACE_VERSION); // m_ModelInfo
	FIND_IFACE(engineFactory, m_ModelInfo, num, iface_buffer, IVModelInfo *);
	strcpy(iface_buffer, INTERFACEVERSION_ENGINETRACE_SERVER); // m_EngineTrace
	FIND_IFACE(engineFactory, m_EngineTrace, num, iface_buffer, IEngineTrace *);
	strcpy(iface_buffer, INTERFACEVERSION_STATICPROPMGR_SERVER); // staticpropmgr
	FIND_IFACE(engineFactory, staticpropmgr, num, iface_buffer, IStaticPropMgrServer *);
	strcpy(iface_buffer, INTERFACEVERSION_VOICESERVER); // m_VoiceServer
	FIND_IFACE(engineFactory, m_VoiceServer, num, iface_buffer, IVoiceServer *);
	strcpy(iface_buffer, INTERFACEVERSION_ISERVERPLUGINHELPERS); // m_Helpers
	FIND_IFACE(engineFactory, m_Helpers, num, iface_buffer, IServerPluginHelpers *);
	strcpy(iface_buffer, INTERFACEVERSION_SERVERGAMEENTS); // m_GameEnts
	FIND_IFACE(serverFactory, m_GameEnts, num, iface_buffer, IServerGameEnts *);
	
	bFirstEverRound = true;
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
	void *laddr = reinterpret_cast<void *>(g_SMAPI->GetServerFactory(false));
#else
	void *laddr = reinterpret_cast<void *>(g_SMAPI->serverFactory(false));
#endif

	//void *vEntList;
	void *vDisptachEffect;

	//SIGFIND(g_GetNameFunc, GetNameFunction, CBaseCombatWeapon_GetName_Sig, CBaseCombatWeapon_GetName_SigBytes);
#ifdef WIN32
	#define SIGFIND( var, type, sig, sigbytes ) \
	var = (type)g_SigMngr.ResolveSig(laddr, sig, sigbytes); \
	META_CONPRINTF("[ZOMBIE] %p\n", var); \
	if (!var) { \
		snprintf(error, maxlen, "Could not find all sigs!\n%s:%d\n", __FILE__, __LINE__); \
		return false; \
	}


	//SIGFIND( vEntList, void *, CEntList_Pattern, CEntList_PatternBytes);

	SIGFIND( vUClient, void *, CPlayer_UpdateClient_Sig, CPlayer_UpdateClient_SigBytes);

	SIGFIND( g_GiveNamedItemFunc,			GiveNamedItem_Func,				 CCSPlayer_GiveNamedItem_Sig,			CCSPlayer_GiveNamedItem_SigBytes			);
	SIGFIND( g_WeaponDropFunc,				WeaponDropFunction,				 CBasePlayer_WeaponDrop_Sig,			CBasePlayer_WeaponDrop_SigBytes				);
	SIGFIND( g_TermRoundFunc,				TermRoundFunction,				 CCSGame_TermRound_Sig,					CCSGame_TermRound_SigBytes					);
	
	
	SIGFIND( g_WantsLag,					WantsLagFunction,				 WantsLagComp_Sig,						WantsLagComp_SigBytes						);

	SIGFIND( g_CCSPlayer_RoundRespawn,		CCSPlayer_RoundRespawnFunc,		 CCSPlayer_RoundRespawn_Sig,			CCSPlayer_RoundRespawn_SigBytes				);
	
	SIGFIND( g_GetFileWeaponInfoFromHandle, GetFileWeaponInfoFromHandleFunc, GetFileWeaponInfoFromHandle_Sig,		GetFileWeaponInfoFromHandle_SigBytes		);

	SIGFIND( v_GiveNamedItem,				void *,							 CCSPlayer_GiveNamedItem_Sig,			CCSPlayer_GiveNamedItem_SigBytes			);
	SIGFIND( v_SetFOV,						void *,							 SetFOV_Sig,							SetFOV_SigBytes								);
	SIGFIND( v_SwitchTeam,					void *,							 CCSPlayer_SwitchTeam_Sig,				CCSPlayer_SwitchTeam_SigBytes				);
	SIGFIND( v_ApplyAbsVelocity,			void *,							 CBaseEntity_ApplyAbsVelocity_Sig,		CBaseEntity_ApplyAbsVelocity_SigBytes		);
	SIGFIND( g_TakeDamage,					OnTakeDamageFunction,			 OnTakeDamage_Sig,						OnTakeDamage_SigBytes						);
	SIGFIND( v_IsInBuyZone,					void *,							 CCSPlayer_IsInBuyZone_Sig,				CCSPlayer_IsInBuyZone_SigBytes				);
	SIGFIND( g_CreateEntityByName,			CreateEntityByNameFunction,		 CreateEntityByName_Sig,				CreateEntityByName_Sigbytes					);
	SIGFIND( v_KeyValues,					void *,							 CBaseEntity_KeyValues_Sig,				CBaseEntity_KeyValues_SigBytes				);
	SIGFIND( g_SetMinMaxSize,				SetMinMaxSize,					 SetMinMaxSize_Sig,						SetMinMaxSize_SigBytes						);
	SIGFIND( vDisptachEffect,				void *,							 DispatchEffect_Sig,					DispatchEffect_SigBytes						);

	SIGFIND( v_ClientPrint,					ClientPrint_Func,				 ClientPrint_Sig,						ClientPrint_SigBytes						);

	
	SIGFIND( g_CreateEntityByName,			CreateEntityByNameFunction,		 CreateEntityByName_Sig,				CreateEntityByName_Sigbytes 				);
	SIGFIND( g_UtilRemoveFunc,				RemoveFunction,					 UtilRemove_Sig,						UtilRemove_SigBytes							);	

	memcpy(&m_TempEnts, ((char*)vDisptachEffect + DispatchEffect_Offset), sizeof(char*));
	int *ptr = *( (int **)(m_TempEnts) );
	m_TempEnts = (ITempEntsSystem *)ptr;
	if ( !m_TempEnts )
	{
		snprintf(error, maxlen, "Could not find all sigs!\n%s:%d\n", __FILE__, __LINE__);
		return false;
	}
	//m_TempEnts = **(ITempEntsSystem***)( VFN( m_Effects, 0x0C ) + ( 107 ) );
#else
	void    *handle;
	void	*var_address;

	#define SIGFIND(var,name) \
	var = dlsym( handle, name );\
	META_CONPRINTF("[ZOMBIE] %p\n", var); \
	if (!var) { \
		snprintf(error, maxlen, "Could not find all sigs!"); \
		return false; \
	}

	char sGameDir[200];

	m_Engine->GetGameDir( sGameDir, 200 );
	strcat( sGameDir, "/bin/server_i486.so" );
    handle = dlopen( sGameDir, RTLD_NOW );

    if ( handle == NULL )
    {
            META_CONPRINTF( "[ZOMBIE] Failed to open server image, error [%s]\n", dlerror() );
			return false;
    }
    else
    {
            META_CONPRINTF( "[ZOMBIE] Server Dll Start at [%p]\n", handle );

			SIGFIND( var_address,					"te"									);	
			SIGFIND( vUClient,						CPlayer_UpdateClient_Sig				);
			SIGFIND( g_TermRoundFunc,				CCSGame_TermRound_Sig					);
			SIGFIND( g_GiveNamedItemFunc,			CCSPlayer_GiveNamedItem_Sig				);
			SIGFIND( g_WeaponDropFunc,				CBasePlayer_WeaponDrop_Sig				);
			SIGFIND( g_WantsLag,					WantsLagComp_Sig						);
			SIGFIND( g_TakeDamage,					OnTakeDamage_Sig						);
			SIGFIND( g_GetFileWeaponInfoFromHandle, GetFileWeaponInfoFromHandle_Sig			);
			SIGFIND( g_CCSPlayer_RoundRespawn,		CCSPlayer_RoundRespawn_Sig				);
			SIGFIND( v_GiveNamedItem,				CCSPlayer_GiveNamedItem_Sig				);
			SIGFIND( v_SetFOV,						SetFOV_Sig								);
			SIGFIND( v_SwitchTeam,					CCSPlayer_SwitchTeam_Sig				);
			SIGFIND( v_ApplyAbsVelocity,			CBaseEntity_ApplyAbsVelocity_Sig		);
			SIGFIND( v_IsInBuyZone,					CCSPlayer_IsInBuyZone_Sig				);
			SIGFIND( g_CreateEntityByName,			CreateEntityByName_Sig					);
			SIGFIND( g_SetCollisionBounds,			CBaseEntity_SetCollisionBounds_Sig		);
			SIGFIND( v_KeyValues,					CBaseEntity_KeyValues_Sig				);
			SIGFIND( g_UtilRemoveFunc,				UtilRemove_Sig							);	
			
			SIGFIND( v_ClientPrint,					ClientPrint_Sig							);

			var_address = dlsym( handle, "te" );
			m_TempEnts = *(ITempEntsSystem **) var_address; 

            dlclose( handle );
    }
#endif
	if ( !m_TempEnts )
	{
		snprintf(error, maxlen, "Could not find ITempEntsSystem !");
		return false;
	}
	if ( vUClient )
	{
		FindGameRules( (char *)vUClient );
	}
	else
	{
		snprintf(error, maxlen, "Could not find CGameRules !");
		return false;	
	}

	ConCommandBaseMgr::OneTimeInit(&g_Accessor);

	m_VoiceServer_CC = SH_GET_CALLCLASS( m_VoiceServer );
	m_ServerDll_CC = SH_GET_CALLCLASS( m_ServerDll );

	SH_ADD_HOOK_MEMFUNC(IVoiceServer, SetClientListening, m_VoiceServer, &g_ZombiePlugin, &ZombiePlugin::SetClientListening, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::LevelInit_Pre, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::LevelInit, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::ServerActivate, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::GameFrame, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::LevelShutdown, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientDisconnect, false );
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientPutInServer, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::SetCommandClient, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientCommand, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientActive, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientActive, true);
	
	SH_ADD_HOOK_MEMFUNC( IGameEventManager2, FireEvent, m_GameEventManager, &g_ZombiePlugin, &ZombiePlugin::FireEvent, false );

	SH_ADD_HOOK_MEMFUNC( IServerGameDLL, OnQueryCvarValueFinished, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::OnQueryCvarValueFinished, false );
	
	//SH_ADD_HOOK_MEMFUNC( IVEngineServer, UserMessageBegin, m_Engine, &g_ZombiePlugin, &ZombiePlugin::UserMessageBegin, false );
/*
#ifdef WIN32
	cPrintDetour.Detour( (BYTE*)v_ClientPrint, (BYTE*)ClientPrint_hook, true, true, true );
	cPrintDetour.Apply();
#endif
*/

	kvPlayerClasses = new KeyValues( "ClientClasses" );
	kvPlayerClasses->LoadFromFile( m_FileSystem, "cfg/zombiemod/player_classes.kv", "MOD" );

	m_Engine->ServerCommand( "exec zombiemod/zombiemod.cfg\n" );

	g_Timers = new STimers();
/*
	bLoadedLate = late;
	if ( late )
	{
*/
	bLoadedLate = false;
	if ( false )
	{
		LevelInit( NULL, NULL, NULL, NULL, false, false );
		LevelInit_Pre( NULL, NULL, NULL, NULL, false, false );
		mp_friendlyfire = m_CVar->FindVar("mp_friendlyfire");
		mp_spawnprotectiontime = m_CVar->FindVar("mp_spawnprotectiontime");
		mp_tkpunish = m_CVar->FindVar("mp_tkpunish");
		sv_alltalk = m_CVar->FindVar("sv_alltalk");
		ammo_buckshot_max = m_CVar->FindVar("ammo_buckshot_max");
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
	int x;
	for ( x = 0; x < g_HookedWeapons.Count(); x++ )
	{
		SH_REMOVE_MANUALHOOK_MEMFUNC( GetMaxSpeed_hook, g_HookedWeapons[x], &g_ZombiePlugin, &ZombiePlugin::GetMaxSpeed, true);
		SH_REMOVE_MANUALHOOK_MEMFUNC( Delete_hook, g_HookedWeapons[x], &g_ZombiePlugin, &ZombiePlugin::Delete, true);
		g_HookedWeapons.Remove(x);
	}
	for ( x = 1; x <= MAX_PLAYERS; x++ )
	{
		edict_t *pEntity = m_Engine->PEntityOfEntIndex( x );
		if ( pEntity && !pEntity->IsFree() && m_Engine->GetPlayerUserId( pEntity ) != -1 )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)GetContainingEntity( pEntity );
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
	kvPlayerClasses->deleteThis();
	kvPlayerClasses = NULL;
	m_GameEventManager->RemoveListener( this );
/*
#ifdef WIN32
	cPrintDetour.Remove();
#endif
*/

	FFA_Disable();

	if ( pKillCmd != NULL )
		SH_ADD_HOOK_STATICFUNC( ConCommand, Dispatch, pKillCmd, cmdKill, false );
	
	SH_REMOVE_HOOK_MEMFUNC(IVoiceServer, SetClientListening, m_VoiceServer, &g_ZombiePlugin, &ZombiePlugin::SetClientListening, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::LevelInit_Pre, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::LevelInit, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::ServerActivate, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::GameFrame, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::LevelShutdown, false);
	SH_REMOVE_HOOK_MEMFUNC( IServerGameClients, ClientDisconnect, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientDisconnect, false );
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientPutInServer, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::SetCommandClient, true);
	//SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientSettingsChanged, true);
	//SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientConnect, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientConnect, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientActive, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientActive, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientCommand, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientCommand, false);
	SH_REMOVE_HOOK_MEMFUNC(IGameEventManager2, FireEvent, m_GameEventManager, &g_ZombiePlugin, &ZombiePlugin::FireEvent, false);

//	SH_REMOVE_HOOK_MEMFUNC( IVEngineServer, UserMessageBegin, m_Engine, &g_ZombiePlugin, &ZombiePlugin::UserMessageBegin, false );

	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, OnQueryCvarValueFinished, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::OnQueryCvarValueFinished, false );

	SH_RELEASE_CALLCLASS( m_VoiceServer_CC );
	SH_RELEASE_CALLCLASS( m_ServerDll_CC );

	g_TermRoundFunc = NULL;
	g_GiveNamedItemFunc = NULL;
	g_GetNameFunc = NULL;
	g_WeaponDropFunc = NULL;
	g_WantsLag = NULL;
	g_TakeDamage = NULL;
	g_GetFileWeaponInfoFromHandle = NULL;
	g_CCSPlayer_RoundRespawn = NULL;
	g_pGameRules = NULL;
	staticpropmgr = NULL;
	g_ZombieModels.RemoveAll();
	g_HookedWeapons.RemoveAll();
	pKillCmd = NULL;
	vUClient = NULL;
	m_EngineSound = NULL;
	m_ServerDll = NULL;
	m_Engine = NULL;
	m_PlayerInfoManager = NULL;
	m_Effects = NULL;
	m_ModelInfo = NULL;
	m_EngineTrace = NULL;
	m_VoiceServer_CC = NULL;
	m_VoiceServer = NULL;
	m_NetworkStringTable = NULL; 
	m_CVar = NULL;
	m_FileSystem = NULL;
	m_GameEventManager = NULL;
	m_ServerClients = NULL;

	sLangError = NULL;
	return true;
}

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

void ZombiePlugin::AllPluginsLoaded()
{
	mp_roundtime = m_CVar->FindVar("mp_roundtime");
	mp_friendlyfire = m_CVar->FindVar("mp_friendlyfire");
	mp_friendlyfire->InstallChangeCallback( CVar_CallBack );
	mp_limitteams = m_CVar->FindVar("mp_limitteams");
	mp_limitteams->InstallChangeCallback( CVar_CallBack );
	mp_autoteambalance = m_CVar->FindVar("mp_autoteambalance");
	mp_autoteambalance->InstallChangeCallback( CVar_CallBack );
	
	mp_restartgame = m_CVar->FindVar( "mp_restartgame" );
	mp_restartgame->InstallChangeCallback( CVar_CallBack );
	
	sv_cheats = m_CVar->FindVar( "sv_cheats" );
	sv_cheats->m_nFlags &= ~FCVAR_NOTIFY;
	rcon_password = m_CVar->FindVar( "rcon_password" );

	ConCommandBase *pCmd = m_CVar->GetCommands();
	while (pCmd)
	{
		if ( pCmd->IsCommand() && (FStrEq(pCmd->GetName(), "kill")) )
		{
			pKillCmd = (ConCommand *)pCmd;
			SH_ADD_HOOK_STATICFUNC( ConCommand, Dispatch, pKillCmd, cmdKill, false );
			META_CONPRINTF( "[ZOMBIE] Found kill command. %p\n", pKillCmd );
		}
		else if ( pCmd->IsCommand() && (FStrEq(pCmd->GetName(), "give")) )
		{
			pGiveCmd = (ConCommand *)pCmd;
			//pGiveCmd->m_fnCommandCallback = NULL;
			pGiveCmd->m_nFlags &= ~FCVAR_CHEAT;
			META_CONPRINTF( "[ZOMBIE] Found give command. %p // %p\n", pGiveCmd, pGiveCmd->m_fnCommandCallback );
		}
		//else if ( pCmd->IsCommand() && (FStrEq(pCmd->GetName(), "ent_fire")) )
		//{
		//	pEntFireCmd = (ConCommand *)pCmd;
		//	pEntFireCmd->m_nFlags &= ~FCVAR_CHEAT;
		//	META_CONPRINTF( "[ZOMBIE] Found ent_fire command. %p // %p\n", pEntFireCmd, pEntFireCmd->m_fnCommandCallback );
		//}
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
		const char sReplace[] = {0x0D, 0x0A};
		Q_strcpy( sTmp, sCons );
		Q_StrSubst( sTmp, "[ZOMBIE]", "\x03[ZOMBIE]\x01", sOut, 1024, true );
		Q_StrSubst( sOut, "ZombieMod", "\x04ZombieMod\x01", sTmp, 1024, true );
		Q_StrSubst( sTmp, "<br>", sReplace, sOut, 1024 );
		strncpy( sTmp, sOut, sizeof( sTmp ) );
		Q_StrSubst( sTmp, "\n", sReplace, sOut, 1024 );
		g_ZombiePlugin.Hud_Print( pEnt, sOut );
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
		int iPlayer = 0;
		if ( IsValidPlayer( pBase, &iPlayer ) )
		{
			if ( !g_Players[iPlayer].isBot && g_Players[iPlayer].isZombie )
			{
				int iFOV;
				if ( UTIL_GetProperty( g_ZombiePlugin.g_Offsets.m_iFOV + g_ZombiePlugin.g_Offsets.m_Local, m_GameEnts->BaseEntityToEdict( pBase ), &iFOV ) )
				{
					if ( iFOV == zombie_fov.GetInt() )
					{
						return;
					}
				}
				CBasePlayer_SetFOV( pBase, pBase, zombie_fov.GetInt() );
			}
		}
	}
}

void ZombiePlugin::MakeZombie( CBasePlayer *pPlayer, int iHealth, bool bFirst )
{
	char sModel[128];
	int iPlayer = m_Engine->IndexOfEdict( m_GameEnts->BaseEntityToEdict( pPlayer ) );
	int iClass = -1;
	if ( g_ZombieModels.Count() < 1 )
	{
		Q_strncpy( sModel, "models/zombie/classic.mdl", sizeof( sModel ) );
	} 
	else
	{
		if ( g_bZombieClasses && zombie_classes.GetBool() && g_Players[iPlayer].iClass > -1 && g_Players[iPlayer].iClass <= g_iZombieClasses )
		{
			iClass = g_Players[iPlayer].iClass;
			Q_snprintf( sModel, sizeof( sModel ), "%s.mdl", g_ZombieClasses[ iClass ].sModel.c_str() );
			//Q_strncpy( sModel, g_ZombieClasses[ iClass ].sModel.c_str(), sizeof( sModel ) );
			g_Players[iPlayer].iModel = g_ZombieClasses[ iClass ].iPrecache;
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


	if ( iClass != -1 )
	{
		iHP = ( (float)g_ZombieClasses[ iClass ].iHealth / 100 ) * (float)zombie_health_percent.GetInt();
		if ( bFirst )
		{
			iHP *= 2;
		}
	}
	else
	{
		iHP = iHealth;
	}
	//iHP = //( bFirst ? iHP * 2 : iHP );

	//int iHP = ( ( iClass != -1 ) ? ( bFirst ? g_ZombieClasses[ iClass ].iHealth * 2 : g_ZombieClasses[ iClass ].iHealth ) : iHealth );
	//int iHP = iHealth;
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
	
	
	g_Players[iPlayer].iZombieHealth = iHP;
	g_Players[iPlayer].iHealth = iHP;
	UTIL_SetProperty( g_Offsets.m_iHealth, pEntity, iHP );

	iZombieCount++;


	bool bOn = false;
	int iInt = 0;
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

		//UTIL_SetProperty( g_Offsets.m_bHasNightVision, pEntity, bOn );
		//UTIL_SetProperty( g_Offsets.m_bNightVisionOn, pEntity, bOn );

		ZombieVision( pEntity, true );
		//g_Players[iPlayer].sScreenOverlay.assign( "r_screenoverlay vgui/hud/zombiemod/zombie_vision" );
		//m_Engine->ClientCommand( pEntity, "zombie_vision" );
	}
	Vector vecPlayer = pEntity->GetCollideable()->GetCollisionOrigin();
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

	Vector vecClosest = ClosestPlayer( iPlayer, vecPlayer );
	if ( vecPlayer.DistTo( vecClosest ) <= zombie_stuckcheck_radius.GetFloat() )
	{
		//m_Engine->ServerCommand( "sv_cheats 1\n" );
		bool bOldValue = false;
		Set_sv_cheats( true, &bOldValue );
		m_Helpers->ClientCommand( pEntity, "noclip" );
		m_Helpers->ClientCommand( pEntity, "noclip" );
		Set_sv_cheats( false, &bOldValue );
		//m_Engine->ServerCommand("sv_cheats 0\n");
	}

	//sv_cheats->m_nValue = 0.0;


	Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("zombification") );
	if ( g_Players[iPlayer].bShowClassMenu && g_bZombieClasses && zombie_classes.GetBool() && g_iZombieClasses > -1 )
	{
		g_Players[iPlayer].bShowClassMenu = false;
		ShowClassMenu( pEntity );
	}

	if ( g_bZombieClasses && zombie_classes.GetBool() && g_Players[iPlayer].iClass >= 0 && g_Players[iPlayer].iClass <= g_iZombieClasses )
	{
		if ( g_ZombieClasses[ g_Players[iPlayer].iClass ].fRegenTimer > 0.0 && g_ZombieClasses[ g_Players[iPlayer].iClass ].iRegenHealth > 0 )
		{
			void *params[1];
			params[0] = (void *)iPlayer;
			if ( g_Players[iPlayer].iRegenTimer != 0 )
			{
				g_Timers->RemoveTimer( g_Players[iPlayer].iRegenTimer );
				g_Players[iPlayer].iRegenTimer = 0;
			}
			g_Players[iPlayer].iRegenTimer = g_Timers->AddTimer( g_ZombieClasses[ g_Players[iPlayer].iClass ].fRegenTimer, Timed_Regen, params, 1 );
		}
	}

	if ( bFirst && zombie_first_zombie_tele.GetBool() )
	{
		Vector vecVelocity = vec3_origin;
		QAngle qAngles = vec3_angle;
 		if ( iNextSpawn == 0 )
		{
			CBaseAnimating_Teleport( pPlayer, &g_Players[iPlayer].vSpawn, &qAngles, &vecVelocity );
		}
		else
		{
			CBaseAnimating_Teleport( pPlayer, &vSpawnVec[iUseSpawn], &qAngles, &vecVelocity );
		}
		if ( iNextSpawn != 0 )
		{
			iUseSpawn = (++iUseSpawn) % iNextSpawn;
		}
		//CBaseAnimating_Teleport( pPlayer, &g_Players[iPlayer].vSpawn, &g_Players[iPlayer].qSpawn, &vecVelocity );
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
	/*
	IPlayerInfo *iPInfo = m_PlayerInfoManager->GetPlayerInfo( g_Players[iPlayer].pPlayer->edict() );
	if ( !iPInfo )
	{
		return Vector( 0, 0, 0);
	}
	Vector vecPlayer = iPInfo->GetAbsOrigin();
	*/
	for ( x = 1; x <= MAX_CLIENTS; x++ )
	{
		edict_t *pEnt = NULL;
		if ( x != iPlayer && IsValidPlayer( x, &pEnt ) && pEnt && pEnt->GetCollideable() )
		{
			Vector vecLocation;
			//IPlayerInfo *iInfo = m_PlayerInfoManager->GetPlayerInfo( pEnt );
			//if ( iInfo )
			//{
				vecLocation = pEnt->GetCollideable()->GetCollisionOrigin();
				//vecLocation = iInfo->GetAbsOrigin();
				float fTmp = CalcDistance( vecLocation, vecPlayer );
				if ( fTmp > 0 && fTmp < fClosest )
				{
					vClosest = vecLocation;
					fClosest = fTmp;
				}
			//}
		}
	}
	if ( fClosest != 500 )
	{
		return vClosest;
	}
	else
	{
		return Vector( 0, 0, 0);
	}
}
typedef void (SourceHook::EmptyClass::*MFP_Teleport)( const Vector *, const QAngle *, const Vector * ); 
void ZombiePlugin::Slap( int iPlayer )
{
	int iBackup = iPlayer;
	if ( g_Players[iPlayer].pPlayer && IsValidPlayer( g_Players[iPlayer].pPlayer ) )
	{
		Vector vecVelocity;
		IPlayerInfo *iInfo = m_PlayerInfoManager->GetPlayerInfo( m_GameEnts->BaseEntityToEdict( g_Players[iPlayer].pPlayer ) );
		Vector vecLocation;
		if ( iInfo )
		{
			vecLocation = iInfo->GetAbsOrigin();
			QAngle qAngles = iInfo->GetAbsAngles();
			CBaseEntity_GetVelocity( g_Players[iPlayer].pPlayer, &vecVelocity );
			float fX = 1000.0f; //( ( RandomInt( 0, 1 ) == 0 ) ? -1000 : 1000 );
			float fY = 1000.0f;//( ( RandomInt( 0, 1 ) == 0 ) ? -1000 : 1000 );
			float fZ = 200.0f; //RandomFloat( 10, 200 );

			vecVelocity.x += fX;
			vecVelocity.y += fY;
			vecVelocity.z += fZ;
			//Teleport( g_Players[iPlayer].pPlayer, &vecLocation, &qAngles, &vecVelocity );

			//SourceHook::ManualCallClass *mcc = SH_GET_MCALLCLASS( g_Players[iPlayer].pPlayer, sizeof(void*) );
			
			//SH_MCALL2( g_Players[iPlayer].pCallClass, MFP_Teleport(), CBASEANIMATING_TELEPORT, 0, 0)( &vecLocation, &qAngles, &vecVelocity ); 
			//META_CONPRINTF( "CallClass: %p", g_Players[iBackup].pCallClass );
			//SH_MCALL( g_Players[iBackup].pCallClass, Teleport_Hook )( &vecLocation, &qAngles, &vecVelocity );
			
			//SH_RELEASE_CALLCLASS( mcc );
			
			Teleport( g_Players[iPlayer].pCallClass, &vecLocation, &qAngles, &vecVelocity );
			
		}
	}
}

void ZombiePlugin::DispatchEffect( const char *pName, const CEffectData &data )
{
	CPASFilters filter( data.m_vOrigin, MAX_CLIENTS, m_Engine );//g_SMAPI->pGlobals()->maxClients, m_Engine );
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
			pWeapon = CBaseCombatCharacter_WeaponSlot( (CBaseCombatCharacter*) pPlayer, iWeaponSlots[x] );
			if ( pWeapon )
			{
				if ( m_GameEnts->BaseEntityToEdict( pWeapon )->GetClassName() )
				{
					if ( FStrEq( m_GameEnts->BaseEntityToEdict( pWeapon )->GetClassName(), "weapon_knife" ) )
					{
						pKnife = pWeapon;
						if ( bHook  )
						{
							CBaseEntity *pBase = (CBaseEntity *)pWeapon;
							int iDX = g_HookedWeapons.Find( pBase );
							if ( iDX != -1 )
							{
								SH_REMOVE_MANUALHOOK_MEMFUNC( GetMaxSpeed_hook, pBase, &g_ZombiePlugin, &ZombiePlugin::GetMaxSpeed, true);
								SH_REMOVE_MANUALHOOK_MEMFUNC( Delete_hook, pBase, &g_ZombiePlugin, &ZombiePlugin::Delete, true);
								g_HookedWeapons.Remove(x);
							}
							int iPlayer = m_Engine->IndexOfEdict( m_GameEnts->BaseEntityToEdict( pPlayer ) );
							g_Players[iPlayer].pKnife = pWeapon;
							
							SH_ADD_MANUALHOOK_MEMFUNC(GetMaxSpeed_hook, pBase, &g_ZombiePlugin, &ZombiePlugin::GetMaxSpeed, true);
							SH_ADD_MANUALHOOK_MEMFUNC(Delete_hook, pBase, &g_ZombiePlugin, &ZombiePlugin::Delete, true);
							g_HookedWeapons.AddToTail(pBase);
						}
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
				}
			}
		}
	}
	CBaseCombatCharacter_Weapon_Switch( (CBaseCombatCharacter*)pPlayer, pKnife );
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
	//for ( i = 1; i <= g_SMAPI->pGlobals()->maxClients; i++ )
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
	META_LOG( g_PLAPI, "RANDOM ZOMBIE." );
	g_ZombiePlugin.bAllowedToJetPack = true;
	g_ZombiePlugin.bZombieDone = true;
	if ( !zombie_enabled.GetBool() || !g_ZombiePlugin.bRoundStarted )
	{
		return;
	}
	int aliveCount = 0;
	bool		aAlive[MAX_PLAYERS+1] = {false};
	edict_t		*aEdict[MAX_PLAYERS+1] = {NULL};
	
	if ( m_iLastZombie != -1 )
	{
		for ( int x = 1; x <= MAX_CLIENTS; x++ )
		{
			if ( m_iLastZombie != x )
			{
				edict_t *pEdict = NULL;
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
			edict_t *pEdict = NULL;
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


	if ( aliveCount > 0 )
	{
		if ( zombie_count.GetInt() != 0 && aliveCount > 3 )
		{
			int iCnt;
			int iZombied = 0;
			iCnt = zombie_count.GetInt();
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
					//edict_t *pEdict = aEdict[x];
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
				META_LOG( g_PLAPI, "Random zombie in %f.", fRand );
				g_ZombiePlugin.g_Timers->AddTimer( fRand, RandomZombie, NULL, 0, 8672396 );
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
		META_LOG( g_PLAPI, "Random zombie in %f.", fRand );
		g_ZombiePlugin.g_Timers->AddTimer( fRand, RandomZombie, NULL, 0, 8672396 );
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
			CBasePlayer *pPlayer = (CBasePlayer *)GetContainingEntity( pEdict );
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
		if ( g_bZombieClasses && zombie_classes.GetBool() && g_Players[iPlayer].iClass >= 0 && g_Players[iPlayer].iClass <= g_iZombieClasses )
		{
			int iRegHealth = g_ZombieClasses[ g_Players[iPlayer].iClass ].iRegenHealth;
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
				if ( g_ZombieClasses[ g_Players[iPlayer].iClass ].fRegenTimer > 0.0 )
				{
					void *params[1];
					params[0] = (void *)iPlayer;
					g_Players[iPlayer].iRegenTimer = g_ZombiePlugin.g_Timers->AddTimer( g_ZombieClasses[ g_Players[iPlayer].iClass ].fRegenTimer, Timed_Regen, params, 1 );
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
	if ( zombie_regen_timer.GetInt() > 0 && (!g_ZombieClasses || !zombie_classes.GetBool()) )
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
				CBasePlayer *pPlayer = (CBasePlayer *)GetContainingEntity( pEdict );
				if ( pEdict && ( m_Engine->GetPlayerUserId( pEdict ) != -1 ) && pPlayer )
				{
					if ( !g_Players[x].isBot && ( zombie_jetpack.GetBool() || humans_jetpack.GetBool() ) )
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
					UTIL_TermRound( 5.0
						, Terrorists_Win );
				}
				else if ( zombies == 0 )
				{
					g_ZombiePlugin.Hud_Print( NULL, "\x03[ZOMBIE]\x01 %s", g_ZombiePlugin.GetLang("humans_win") );
					g_ZombiePlugin.g_ZombieRoundOver = true;
					UTIL_TermRound( 5.0, CTs_PreventEscape );
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
		UTIL_TermRound( 5.0, CTs_PreventEscape );
	}
}

bool ZombiePlugin::LoadLanguages( )
{
	char sFilePath[255];
	if ( kLanguage ) 
	{
		//kLanguage->RemoveEverything();
		//return true;
		//kLanguage->Clear();
		//delete kLanguage;
		kLanguage->deleteThis();
		kLanguage = NULL;
	}
	
	//bLanguagesLoaded = LoadLanguages
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
		bool quotes = false;
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
		bool quotes = false;
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
		g_Timers->RemoveTimer( 8672396 );
		float fRandom = RandomFloat(zombie_timer_min.GetFloat(), zombie_timer_max.GetFloat());
		META_LOG( g_PLAPI, "Random zombie in %f.", fRandom );
		g_Timers->AddTimer( fRandom, RandomZombie, NULL, 0, 8672396 );
	    g_Timers->AddTimer( 0.5, CheckZombies, NULL, 0, ZOMBIECHECK_TIMER_ID );
	}
}
void ZombieLevelInit( void **params )
{
	/*void *params[1];
	params[0] = (void *)pPlayer;*/
	//Timed_FogController( NULL );
	if ( zombie_dark.GetInt() == 1 )
	{
		m_Engine->LightStyle( 0, "a" );
	}
	else if ( zombie_dark.GetInt() == 2 )
	{
		m_Engine->LightStyle( 0, "b" );
	}
	g_ZombiePlugin.DoAmmo( zombie_unlimited_ammo.GetBool() );
	FFA_Enable();
	mp_friendlyfire->SetValue( 0 );
	m_Engine->ServerCommand( "mp_flashlight 1\n" );
	m_Engine->ServerCommand( "mp_limitteams 0\n" );
	mp_spawnprotectiontime->SetValue(0);
	mp_tkpunish->SetValue(0);
	//sv_alltalk->SetValue(1);
	ammo_buckshot_max->SetValue(128);
	//mp_friendlyfire->SetValue(0);
	g_ZombiePlugin.RestrictWeapons();
}

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

	if ( g_Players[iPlayer].isZombie && !FStrEq( szWeapon, "weapon_knife" ) )
		return false;
	
	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pEnt;
	if ( pWeapon )
	{
		int nCount;
		int iID = LookupBuyID( szWeapon );
		if ( !AllowWeapon( (CBasePlayer*)pPlayer, iID, nCount ) )
		{
			if ( bDrop == true )
			{
				UTIL_WeaponDrop( (CBasePlayer*)pPlayer, pWeapon, NULL, NULL );
			}
			return false;
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
/*
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
		pTmp = (g_FindEnt)(EntList, delay, reason);
	#endif
	return pTmp;
}
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
union
{
	bool	(vEmptyClass::*mfpnew)( const char *, const char * );
	void	*addr;
} u_KeyValue;	

bool CBaseEntity_KeyValues( CBaseEntity *pBaseEntity, const char *szKeyName, const char *szValue )
{
	void* funcptr = v_KeyValues;
	void* thisptr = pBaseEntity;
	u_KeyValue.addr = funcptr;
	return (bool)(reinterpret_cast<vEmptyClass*>(thisptr)->*u_KeyValue.mfpnew)( szKeyName, szValue );
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
	void* funcptr = v_IsInBuyZone;
	void* thisptr = pPlayer;
	u_IsInBuyZone.addr = funcptr;
	return (bool)(reinterpret_cast<vEmptyClass*>(thisptr)->*u_IsInBuyZone.mfpnew)( );
}
//----------------------------------------------------------
//									ApplyAbsVelocityImpulse
//----------------------------------------------------------
union
{
	void	(vEmptyClass::*mfpnew)( const Vector & );
	void	*addr;
} u_ApplyAbsVelocity;	

void CBasePlayer_ApplyAbsVelocityImpulse( CBasePlayer *pPlayer, const Vector &vecImpulse )
{
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
//----------------------------------------------------------
//									GiveNamedItem
//----------------------------------------------------------
union
{
	CBaseEntity	*(vEmptyClass::*mfpnew)( const char *, int );
	void *addr;
} u_GiveNamedItem;												//UTIL_GiveNamedItem

CBaseEntity	*GiveNamedItem_Test( CBasePlayer *pPlayer, const char *pszName, int iSubType )
{
	if ( !IsValidPlayer( pPlayer ) )
		return false;
	void* funcptr = v_GiveNamedItem;
	void* thisptr = pPlayer;
	u_GiveNamedItem.addr = funcptr;
	return (CBaseEntity	*)(reinterpret_cast<vEmptyClass*>(thisptr)->*u_GiveNamedItem.mfpnew)( pszName, iSubType );
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
	memcpy(&rules, reinterpret_cast<void*>(g_pGameRules),sizeof(char*));  //0x0B9023C0 from this scan (Dec1 update)
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

	CBaseEntity_SetModel( (CBasePlayer *)pEntity, pModelName );	
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
				CBaseEntity *pRagDoll = pEnt->GetUnknown()->GetBaseEntity();
				if ( pRagDoll )
				{
					return pRagDoll;
				}
				else
				{
					META_LOG( g_PLAPI, "ERROR: Couldnt get BaseEntity for ragdoll." );
				}
			}
		}
		else
		{
			META_LOG( g_PLAPI, "ERROR: Couldnt get EntryIndex for ragdoll." );
		}
	}
	else
	{
		META_LOG( g_PLAPI, "ERROR: Couldnt get pHandle !" );
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

/*void FindEntListFuncs( char *addr )
{
	char *new_addr = NULL;
	memcpy( &g_EntList, (addr + CEntList_gEntList), sizeof(char *) );
	//memcpy( &new_addr, (addr + CEntList_FindEntity - sizeof(char *)), sizeof(char *) );
	//new_addr += (unsigned long)addr + CEntList_FindEntity;
	//memcpy( &g_Core->g_FindEntity, &new_addr, sizeof(char *) );
	g_pEntityList = g_EntList;
}*/

void FindGameRules(char *addr)
{
	memcpy(&g_pGameRules, (addr + CPlayer_g_pGameRules), sizeof(char*));
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

void FFA_Disable()
{
        if (!g_ffa)
                return;
#ifndef WIN32
		char *hack = (char *)g_TakeDamage + OnTakeDamage_Offset1;
        UTIL_MemProtect(hack, 20, PAGE_EXECUTE_READWRITE);
        memcpy(hack, g_hackbuf, OnTakeDamage_OffsetBytes);
		hack = (char *)g_TakeDamage + OnTakeDamage_Offset2;
        UTIL_MemProtect(hack, 20, PAGE_EXECUTE_READWRITE);
        memcpy(hack, g_hackbuf1, OnTakeDamage_OffsetBytes);
#else
		char *hack = (char *)g_TakeDamage + OnTakeDamage_Offset;
        UTIL_MemProtect(hack, 20, PAGE_EXECUTE_READWRITE);
        memcpy(hack, g_hackbuf, OnTakeDamage_OffsetBytes);
#endif
        hack = (char *)g_WantsLag + WantsLagComp_Offset;
        UTIL_MemProtect(hack, 20, PAGE_EXECUTE_READWRITE);
        memcpy(hack, g_lagbuf, WantsLagComp_OffsetBytes);
		META_CONPRINTF( "[ZOMBIE] Free For All has been disabled.\n" );
        g_ffa = false;
}

void FFA_Enable()
{
        if (g_ffa)
                return;
#ifndef WIN32
        char *hack = (char *)g_TakeDamage + OnTakeDamage_Offset1;
        UTIL_MemProtect(hack, 20, PAGE_EXECUTE_READWRITE);
        const char _hackbuf[] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
		memcpy(g_hackbuf, hack, OnTakeDamage_OffsetBytes);
		memcpy(hack, _hackbuf, OnTakeDamage_OffsetBytes);
		hack = (char *)g_TakeDamage + OnTakeDamage_Offset2;
		memcpy(g_hackbuf1, hack, OnTakeDamage_OffsetBytes);
        UTIL_MemProtect(hack, 20, PAGE_EXECUTE_READWRITE);
		memcpy(hack, _hackbuf, OnTakeDamage_OffsetBytes);
#else
		char *hack = (char *)g_TakeDamage + OnTakeDamage_Offset;
        UTIL_MemProtect(hack, 20, PAGE_EXECUTE_READWRITE);
        const char _hackbuf[] = {0xEB};
		memcpy(g_hackbuf, hack, OnTakeDamage_OffsetBytes);
        memcpy(hack, _hackbuf, OnTakeDamage_OffsetBytes);
#endif

        //now patch lag compensation
        hack = (char *)g_WantsLag + WantsLagComp_Offset;
        const char _lagbuf[] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90};

        UTIL_MemProtect(hack, 20, PAGE_EXECUTE_READWRITE);
        memcpy(g_lagbuf, hack, WantsLagComp_OffsetBytes);
        memcpy(hack, _lagbuf, WantsLagComp_OffsetBytes);
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
				#define PrimAm() CBaseCombatCharacter_GiveAmmo( (CBaseCombatCharacter *)g_Players[iPlayer].pPlayer, 200, GetAmmoIndex( zombie_respawn_primary.GetString() ) ); //GiveNamedItem_Test( g_Players[iPlayer].pPlayer, "primammo", 0.0 );
				PrimAm();
				PrimAm();
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
				#define SecAm() CBaseCombatCharacter_GiveAmmo( (CBaseCombatCharacter *)g_Players[iPlayer].pPlayer, 200, GetAmmoIndex( zombie_respawn_secondary.GetString() ) ); //GiveNamedItem_Test( g_Players[iPlayer].pPlayer, "secammo", 0.0 );
				SecAm();
				SecAm();
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
   else if ( FStrEq( sWeapon, "weapon_p288 " ) )
   {
	  iIndex = 9;

   }
   else if ( FStrEq( sWeapon, "weapon_deagle" ) )
   {
	  iIndex = 1;

   }
   else if ( FStrEq( sWeapon, "weapon_elite " ) )
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
	/*for (int i = 1; i <= g_SMAPI->pGlobals()->maxClients; i++) 
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
	bool bOldValue;
	if ( bChangeCheats )
	{
		Set_sv_cheats( true, &bOldValue );
	}
	int iPlayer = 0;
	if ( IsValidPlayer( cEntity, &iPlayer ) )
	{
		//CBasePlayer_SetFOV( g_Players[iPlayer].pPlayer, g_Players[iPlayer].pPlayer, iFOV );
		char sTmp[50];
		Q_snprintf( sTmp, sizeof(sTmp), "fov %d", iFOV );
		m_Helpers->ClientCommand( cEntity, sTmp );
	}
	//char sFOV[100];
	//Q_snprintf( sFOV, sizeof(sFOV), "fov %d", iFOV );
	//m_Helpers->ClientCommand( cEntity, sFOV );
	if ( bChangeCheats )
	{
		Set_sv_cheats( false, &bOldValue );
		//sv_cheats->SetValue( 0 );
		//sv_cheats->m_nValue = 0.0;
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
		return g_pStringTableVguiScreen->AddString( sString );	
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
			pDownloadablesTable->AddString( file, strlen( file ) + 1 );
			m_Engine->LockNetworkStringTables( save );
		}
	}
} 

void ZombiePlugin::SetProtection( int iPlayer, bool bProtect, bool bReport, bool bRoundEnd )
{
	RenderMode_t	tRenderMode;
	edict_t			*cEnt;
	const char *sMessage = ( bProtect ? GetLang("on") : GetLang("off") );

	if ( !zombie_enabled.GetBool() )
	{
		return;
	}
	if ( IsValidPlayer( iPlayer, &cEnt ) && IsValidPlayer( g_Players[iPlayer].pPlayer ) && IsAlive( cEnt ) && !(g_Players[iPlayer].isZombie && bProtect) )
	{
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
			//META_LOG( g_PLAPI, "Protecting Player %d for %f!", iPlayer, zombie_respawn_protection.GetFloat() );
			void *params[1];
			params[0] = (void *)iPlayer;
			g_Players[iPlayer].iProtectTimer = g_ZombiePlugin.g_Timers->AddTimer( zombie_respawn_protection.GetFloat(), TimerProtection, params, 1 );
		}
		if ( !SetRenderMode( g_Players[iPlayer].pPlayer, tRenderMode ) || !SetClrRender( g_Players[iPlayer].pPlayer, cColour) )
		{
			Hud_Print( cEnt, "\x03[ZOMBIE]\x01 %s", GetLang("protection_render") ); //Problem with set render!
		}
		else
		{
			SetWeaponProtection( g_Players[iPlayer].pPlayer, bProtect, kRenderNone, cColour );
		}
		if ( bReport )
		{
			char buf[100];
			Q_snprintf( buf, sizeof( buf ), GetLang( "protection_indicator" ), sMessage );
			Hud_Print( cEnt, "\x03[ZOMBIE]\x01 %s", buf ); //Your Zombie Protection is now %s.
		}
	}
	else if ( g_Players[iPlayer].bConnected && !bRoundEnd )
	{
		int iTeam = GetTeam( m_GameEnts->BaseEntityToEdict( g_Players[iPlayer].pPlayer ) );
		if ( iTeam == COUNTERTERRORISTS || iTeam == TERRORISTS )
		{
			META_LOG( g_PLAPI, "\x03[ZOMBIE]\x01 Invalid player for SetProtection %d, %s!", iPlayer, g_Players[iPlayer].sUserName.c_str() );
		}
	}
	return;
}

void ZombiePlugin::SetWeaponProtection( CBasePlayer *pPlayer, bool bProtect, RenderMode_t tRenderMode, color32 cColour )
{
	int iWeaponSlots[] = { SLOT_NADE, SLOT_NADE, SLOT_NADE, SLOT_NADE, SLOT_SECONDARY, SLOT_PRIMARY, SLOT_KNIFE };
	
	CBaseCombatWeapon *pWeapon =  NULL;
	for (int x = 6; x >= 0; x--)
	{
		pWeapon = CBaseCombatCharacter_WeaponSlot( (CBaseCombatCharacter*) pPlayer, iWeaponSlots[x] );
		if ( pWeapon )
		{
			SetRenderMode( pWeapon, tRenderMode );
			SetClrRender( pWeapon, cColour);
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
		g_ZombiePlugin.SetProtection( iPlayer, false );
		g_Players[iPlayer].iProtectTimer = 99;
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
		if ( g_Players[iPlayer].iClassMenu != -1 )
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
	g_iZombieClasses = -1;
	int x = 0;

	#define CLEARCLASSES() \
		for( x = 0; x < 100; x++ ) \
		{ \
			g_ZombieClasses[x].bInUse = false; \
			g_ZombieClasses[x].fKnockback = 0.0; \
			g_ZombieClasses[x].iHealth = 0; \
			g_ZombieClasses[x].iJumpHeight = 0; \
			g_ZombieClasses[x].iPrecache = 0; \
			g_ZombieClasses[x].iSpeed = 0; \
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

	CLEARCLASSES();

	#define SizeOf( iReturn, aArray ) \
		iReturn = sizeof(aArray) / sizeof(aArray[0]);
	
	int size = 0;
	SizeOf(size, g_ZombieClasses);

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
							#define LOADSTRING( Name, Var ) \
								sTmp = p->GetString( Name, "" ); \
								if ( sTmp ) \
								{ \
									Var.assign( sTmp ); \
								}
							#define LOADINT( Name, Var ) \
								iTmp = p->GetInt( Name, -1 ); \
								if ( iTmp > -1 ) \
								{ \
									Var = iTmp; \
								}
							#define LOADFLOAT( Name, Var ) \
								fTmp = p->GetFloat( Name, -1.0 ); \
								if ( fTmp > -1.0 ) \
								{ \
									Var = fTmp; \
								}

							g_iZombieClasses++;

							if ( FStrEq( p->GetName(), "zombie_classic" ) )
							{
								g_iDefaultClass = g_iZombieClasses;
								bGotDefault = true;
							}
							g_ZombieClasses[g_iZombieClasses].bInUse = true;
							LOADSTRING( "classname", g_ZombieClasses[g_iZombieClasses].sClassname );
							LOADSTRING( "model", g_ZombieClasses[g_iZombieClasses].sModel );

							bool bFound = false;
							char sModel[255];
							Q_snprintf( sModel, sizeof( sModel ), "%s.mdl", strlwr( (char *)g_ZombieClasses[g_iZombieClasses].sModel.c_str() ) );

							for ( int z = 0; z <= g_ZombieModels.Count() - 1; z++ )
							{
								const char *sLwr = strlwr( (char *)g_ZombieModels[z].sModel.c_str() );
								if ( FStrEq( sLwr, sModel ) )
								{
									bFound = true;
									g_ZombieClasses[g_iZombieClasses].iPrecache = g_ZombieModels[z].iPrecache;
									break;
								}
							}

							if ( bFound ) 
							{
								LOADINT( "headshots", g_ZombieClasses[g_iZombieClasses].iHeadshots );

								if ( g_ZombieClasses[g_iZombieClasses].iHeadshots == -1 )
								{
									g_ZombieClasses[g_iZombieClasses].iHeadshots = zombie_headshot_count.GetInt();
								}

								LOADINT( "health", g_ZombieClasses[g_iZombieClasses].iHealth );
								LOADINT( "speed", g_ZombieClasses[g_iZombieClasses].iSpeed );
								LOADINT( "jump_height", g_ZombieClasses[g_iZombieClasses].iJumpHeight );
								LOADFLOAT( "knockback", g_ZombieClasses[g_iZombieClasses].fKnockback );
								int iHsOnly = 0;
								LOADINT( "hs_only", iHsOnly );
								g_ZombieClasses[g_iZombieClasses].bHeadShotsOnly = (iHsOnly == 1);
								LOADINT( "regen", g_ZombieClasses[g_iZombieClasses].iRegenHealth );
								LOADFLOAT( "regen_time", g_ZombieClasses[g_iZombieClasses].fRegenTimer );
								LOADFLOAT( "gren_multiplier", g_ZombieClasses[g_iZombieClasses].fGrenadeMultiplier );
								LOADFLOAT( "gren_knockback", g_ZombieClasses[g_iZombieClasses].fGrenadeKnockback );
								LOADINT( "health_bonus", g_ZombieClasses[g_iZombieClasses].iHealthBonus );
							}
							else
							{
								g_ZombieClasses[g_iZombieClasses].sModel.clear();
								g_iZombieClasses--;
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
		CLEARCLASSES();
	}
	else
	{
		META_LOG( g_PLAPI, "Successfully loaded %d classes from file.", ( g_iZombieClasses + 1 ) );
	}
	return bGotDefault;
}

void ZombiePlugin::ShowClassMenu( edict_t * pEntity )
{
	myString sMenu;
	int x;
	int i = 1;
	int iPlayer;
	char sTmp[255];
	int iCountTo;
	bool bMore = false;
	bool bBack = false;

	int iSize = 0;
	/*
	#define ADDTOSTRING( ) \
		iSize = sMenu.size() + Q_strlen( sTmp ); \
		if ( ( iSize ) >= 255 ) \
		{ \
			UTIL_DrawMenu( iPlayer, 10, 10, sMenu.c_str(), false ); \
			sMenu.assign( sTmp ); \
		} \
		else \
		{ \
			sMenu.append( sTmp ); \
		}
	*/

	if ( IsValidPlayer( pEntity, &iPlayer ) && !g_Players[iPlayer].isBot  )
	{
		if ( g_Players[iPlayer].iClassMenu == -1 )
		{
			g_Players[iPlayer].iClassMenu = 0;
		}
		sMenu.assign( GetLang("zombie_classes") ); //Zombie Classes
		sMenu.append( "\n============\n" );
		if ( g_Players[iPlayer].iClassMenu > 0 )
		{
			bBack = true;
		}
		if ( (g_iZombieClasses - g_Players[iPlayer].iClassMenu) > 6 )
		{
			bMore = true;
		}
		iCountTo = g_Players[iPlayer].iClassMenu + 6;
	
		if ( g_iZombieClasses < iCountTo )
		{
			iCountTo = g_iZombieClasses;
		}
		int iNum = 0;
		for ( x = g_Players[iPlayer].iClassMenu; x <= iCountTo; x++ )
		{
			Q_snprintf( sTmp, sizeof(sTmp), "%s%d. %s\n", ( ( g_Players[iPlayer].iClass == (g_Players[iPlayer].iClassMenu + x) ) ? "" : "->" ), ( ++iNum ), g_ZombieClasses[x].sClassname.c_str() );
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

		if ( g_Players[iPlayer].iChangeToClass > -1 && g_Players[iPlayer].iChangeToClass <= g_iZombieClasses )
		{
			Q_snprintf( sTmp, sizeof(sTmp), " \n \n%s", g_ZombieClasses[g_Players[iPlayer].iChangeToClass].sClassname.c_str(), GetLang("change_class_dead") ); //Change to %s when you die.
			sMenu.append( sTmp );
		}

		if ( sMenu.size() > 0 )
		{
			UTIL_DrawMenu( iPlayer, zombie_damagelist_time.GetInt(), 10, sMenu.c_str(), true );
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
	g_ZombiePlugin.Dissolve( pEdict  );
}

void ZombiePlugin::Dissolve( edict_t *pEntity )
{
	int				iPlayer = 0;
	CBaseEntity		*pPlayer = NULL;

	iPlayer = m_Engine->IndexOfEdict( pEntity );
	if ( pEntity )
	{
		pPlayer = m_GameEnts->EdictToBaseEntity( pEntity );
		if ( IsValidPlayer( pPlayer ) && !IsAlive( pEntity ) )
		{
			CBaseEntity *pDisolve = UTIL_GiveNamedItem( (CBasePlayer*)pPlayer, "env_entity_dissolver", 0 );
			if ( pDisolve )
			{
				CBaseEntity_KeyValues( pDisolve, "target", "cs_ragdoll" );
				CBaseEntity_KeyValues( pDisolve, "magnitude", "1" );
				CBaseEntity_KeyValues( pDisolve, "dissolvetype", "3" ); // (can be disove type 2,1 etc)
				variant_t emptyVariant;
				CBaseEntity_AcceptInput( pDisolve, "Dissolve", pPlayer, pPlayer, emptyVariant, 0 );

				void *params[1];
				params[0] = pDisolve;
				g_Timers->AddTimer( 1.0, Timed_Remove, params, 1 );
			}
			else
			{
				META_LOG( g_PLAPI, "Could not create env_entity_dissolver" );
			}
		}
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