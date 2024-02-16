/* ======== props_mm ========
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
#include "VFunc.h"

ZombiePlugin				g_ZombiePlugin;
MyListener					g_Listener;
SetModelFunction			g_SetModelFunc = NULL;
TermRoundFunction			g_TermRoundFunc = NULL;
GiveNamedItem_Func			g_GiveNamedItemFunc = NULL;
GetNameFunction				g_GetNameFunc = NULL;
WeaponSlotFunction			g_WeaponSlotFunc = NULL;
WeaponDropFunction			g_WeaponDropFunc = NULL;
WeaponDeleteFunction		g_WeaponDeleteFunc = NULL;
WantsLagFunction			g_WantsLag = NULL;
OnTakeDamageFunction		g_TakeDamage = NULL;
GetFileWeaponInfoFromHandleFunc g_GetFileWeaponInfoFromHandle = NULL;
CCSPlayer_RoundRespawnFunc  g_CCSPlayer_RoundRespawn = NULL;
IStaticPropMgrServer		*staticpropmgr = NULL;
void						*v_GiveNamedItem = NULL;
void						*v_SetFOV = NULL;
void						*v_SwitchTeam = NULL;
void						*v_ApplyAbsVelocity = NULL;
//edict_t						*pFOV_Player = NULL;
PlayerInfo_t				g_ZombiePlayers[65];

CGameRules					*g_pGameRules = NULL;
CBaseEntityList				*g_pEntityList = NULL;
//extern CGameRules			*g_pGameRules;
//extern IStaticPropMgrServer *staticpropmgr;

CUtlVector<myString>		g_ZombieModels;
CUtlVector<CBaseEntity *>	g_HookedWeapons;
ConCommand					*pKillCmd = NULL;

bool						bUsingCVAR = false;
bool						bChangeCheats = false;
bool						bZombieDone = false;
bool						bRoundStarted;
bool						bRoundTerminating = false;
void						*vUClient;
char						g_lagbuf[WantsLagComp_OffsetBytes];
char						g_hackbuf[OnTakeDamage_Offset];
bool						g_ffa = false;
int							iBeamLaser = 0;

bool						bFirstEverRound = false;

int							iZombieCount;
int							iHumanCount;

#define MAX_SERVERCMD_LENGTH     128

static int g_oldFFValue;
static int g_oldSpawnProtectionTime;
static int g_oldTKPunish;
static int g_oldAlltalkValue;
static int g_oldAmmoBuckshotValue;

CTeam *teamCT = NULL;
CTeam *teamT = NULL;
ArrayLengthSendProxyFn TeamCount;

#define NUM_ZOMBIE_SOUNDS	14
#define MAX_MAP_NAME		32	
static char g_sZombieSounds[][20] = {"zombie_voice_idle1", "zombie_voice_idle2", "zombie_voice_idle3", "zombie_voice_idle4", "zombie_voice_idle5", "zombie_voice_idle6", "zombie_voice_idle7", "zombie_voice_idle8", "zombie_voice_idle9", "zombie_voice_idle10", "zombie_voice_idle11", "zombie_voice_idle12", "zombie_voice_idle13", "zombie_voice_idle14"};
static char g_CurrentMap[MAX_MAP_NAME];
PLUGIN_EXPOSE( ZombiePlugin, g_ZombiePlugin );

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

ConVar zombie_model_file("zombie_model_file", "cfg/zombiemod/models.cfg", FCVAR_PLUGIN, "File with list of models to use for zombie mod", true, 0, true, 1);
ConVar zombie_download_file("zombie_download_file", "cfg/zombiemod/downloads.cfg", FCVAR_PLUGIN, "File with list of materials etc to download.", true, 0, true, 1);
ConVar zombie_sound_rand("zombie_sound_rand", "50", FCVAR_PLUGIN, "How often zombies play zombie sounds", true, 1, false, 0);
ConVar zombie_mode("zombie_mode", "0", FCVAR_REPLICATED | FCVAR_SPONLY | FCVAR_NOTIFY, "If non-zero, zombie mode is enabled (DO NOT SET THIS DIRECTLY!)", true, 0, true, 1, CVar_CallBack );
ConVar zombie_health("zombie_health", "2500", FCVAR_PLUGIN, "Health that zombies get");
ConVar zombie_speed("zombie_speed", "190.0", FCVAR_PLUGIN, "The speed that zombies travel at.");
ConVar zombie_fov("zombie_fov", "125", FCVAR_PLUGIN, "Field of vision of the zombies. Normal human FOV is 90.", true, 40, true, 160);
ConVar zombie_knockback("zombie_knockback", "3", FCVAR_PLUGIN, "The knockback multiplier of zombies. Set to 0 to disable knockback", true, 0, false, 0);
ConVar zombie_restrictions("zombie_restrictions", "rifles m249", FCVAR_PLUGIN, "Space separated list of guns that are restricted during zombie mode", CVar_CallBack);
ConVar zombie_dark("zombie_dark", "1", FCVAR_PLUGIN, "Makes maps very dark if enabled. If disabled, the map doesn't have to be reloaded to start zombiemode", true, 0, true, 1);
ConVar zombie_bonus("zombie_bonus", "1", FCVAR_PLUGIN, "Add frags for zombifying other players." );
ConVar zombie_teams("zombie_teams", "0", FCVAR_PLUGIN, "Sets teams Humans = CT; Zombies = T" );
ConVar zombie_respawn("zombie_respawn", "0", FCVAR_PLUGIN, "If enabled, respawn after a set amount of time." );
ConVar zombie_respawn_delay("zombie_respawn_delay", "1.0", FCVAR_PLUGIN, "Time before players are respawned in autorespawn mode", true, 0, false, 0);
ConVar zombie_max_ammo( "zombie_max_ammo", "1.0", FCVAR_PLUGIN, "If enabled, ammo is unlimited.", true, 0, false, 0, CVar_CallBack );
ConVar zombie_talk( "zombie_talk", "1.0", FCVAR_PLUGIN, "If enabled, zombies can only voice with zombies and humans with humans.", true, 0, false, 0 );
ConVar zombie_tracer( "zombie_tracer", "0.0", FCVAR_PLUGIN, "Enable tracers.", true, 0, false, 0 );
ConVar zombie_tracer_count( "zombie_tracer_count", "4", FCVAR_PLUGIN, "Tracers every nth bullet.", true, 0, false, 0 );
ConVar zombie_tracer_all( "zombie_tracer_all", "0", FCVAR_PLUGIN, "If enabled, all players see each others tracers.", true, 0, false, 0 );
ConVar zombie_suicide( "zombie_suicide", "1", FCVAR_PLUGIN, "When 1, disables players from suiciding.", true, 0, false, 0 );
ConVar zombie_suicide_text( "zombie_suicide_text", "Dont be an asshole.", FCVAR_PLUGIN, "Text for suiciding assholes.", true, 0, false, 0 );
ConVar zombie_startup( "zombie_startup", "0", FCVAR_PLUGIN, "If this is in your config.cfg ZombieMod attempts to auto load itself.", true, 0, false, 0 );
ConVar zombie_timer_max( "zombie_timer_max", "12.0", FCVAR_PLUGIN, "Maximum amount of seconds after round_freeze_end for first random Zombification.", true, 4, true, 100 );
ConVar zombie_timer_min( "zombie_timer_min", "4.0", FCVAR_PLUGIN, "Minimum amount of seconds after round_freeze_end for first random Zombification", true, 3, true, 100 );


ConVar *mp_friendlyfire = NULL;
ConVar *mp_limitteams = NULL;
ConVar *mp_autoteambalance = NULL;
ConCommand *cmd_fov = NULL;
ConVar *mp_spawnprotectiontime = NULL;
ConVar *mp_tkpunish = NULL;
ConVar *sv_alltalk = NULL;
ConVar *ammo_buckshot_max = NULL;

#define WEAPONINFO_COUNT 29
static int g_MaxClip[29];
static int g_DefaultClip[29];

void CVar_CallBack( ConVar *var, char const *pOldString )
{
	if ( FStrEq( var->GetName(), zombie_max_ammo.GetName() ) )
	{
		//META_LOG( g_PLAPI, "Changed ammo.");
		g_ZombiePlugin.DoAmmo( zombie_max_ammo.GetBool() );
	}
	else if ( FStrEq( var->GetName(), zombie_restrictions.GetName() ) )
	{
		g_ZombiePlugin.RestrictWeapons();
	}
	else if ( FStrEq( var->GetName(), zombie_mode.GetName() ) )
	{
		if ( !bUsingCVAR )
		{
			bUsingCVAR = true;
			zombie_mode.SetValue( pOldString );
			bUsingCVAR = false;
			META_LOG( g_PLAPI, "ERROR: ZombieMod must be enabled/disabled via the 'zombies' command." );
		}
	}
	return;
}

void ZombiePlugin::DoAmmo( bool bEnable )
{
	if ( !zombie_mode.GetBool() )
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
			if ( info->iMaxClip1 == 1000000 )
			{
				info->iDefaultClip1 = g_DefaultClip[x];
				info->iMaxClip1 = g_MaxClip[x];
				disable = true;
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

void ZombiePlugin::AddDownload( const char *file )
{
	//META_LOG( g_PLAPI, "Adding Download: '%s'", file );
	INetworkStringTable *pDownloadablesTable = m_NetworkStringTable->FindTable("downloadables");
	if ( pDownloadablesTable )
	{
		if ( pDownloadablesTable->FindStringIndex( file ) < 0 )
		{
			return;
		}
		bool save = m_Engine->LockNetworkStringTables( false );
		pDownloadablesTable->AddString( file, strlen( file ) + 1 );
		m_Engine->LockNetworkStringTables( save );
	}
} 

void ZombiePlugin::FireGameEvent( IGameEvent *event )
{
	if ( !event || !event->GetName() )
	{
		return;
	}
	const char *name = event->GetName();
	//META_LOG( g_PLAPI, "FireEvent( %s )", name );
	if ( FStrEq( name, "round_freeze_end") && zombie_mode.GetBool() )
	{
		bZombieDone = false;
		bRoundStarted = true;
		if ( zombie_mode.GetBool() )
		{
			iCountZombies = 0;
			iCountPlayers = 0;
			Hud_Print( NULL, "[ZOMBIE] Zombie mode is on. The game is HUMANS vs. ZOMBIES. You cannot buy rifles." );
			g_ZombieRoundOver = true;
			g_Timers->RemoveTimer( 8672396 );
			g_Timers->AddTimer( RandomFloat(zombie_timer_min.GetFloat(), zombie_timer_max.GetFloat()), RandomZombie, NULL, 0, 8672396 );
		}
	}
	else if ( FStrEq( name, "player_spawn" ) && zombie_mode.GetBool()  )
	{
		int iPlayer = EntIdxOfUserIdx( event->GetInt( "userid" ) );
		edict_t *pEntity = m_Engine->PEntityOfEntIndex( iPlayer );
		CBaseEntity *pBase = pEntity->GetUnknown()->GetBaseEntity();
		if ( zombie_mode.GetBool() && pEntity && !pEntity->IsFree() && pBase ) 
		{
			//pBase->SetMaxHealth( 100 );
			//pBase->SetHealth( 100 );
			//UTIL_SetPropertyInt( "CBasePlayer", "m_iHealth", pEntity, 100 );
			bool bOn = true;
			UTIL_SetProperty( g_Offsets.m_bInBuyZone, pEntity, bOn );
			UTIL_SetProperty( g_Offsets.m_iHealth, pEntity, (int)100 );
			//UTIL_SetPropertyInt( "CCSPlayer", "m_iAccount", pEntity, 16000 );
			UTIL_SetProperty( g_Offsets.m_iAccount, pEntity, 16000 );
			CBasePlayer *pPlayer = (CBasePlayer *)GetContainingEntity( pEntity );
			if ( g_ZombiePlayers[iPlayer].isZombie )
			{
				g_ZombiePlayers[iPlayer].isZombie = false;
				if ( !( m_Engine->GetPlayerNetworkIDString( pEntity ) ? (Q_stricmp( m_Engine->GetPlayerNetworkIDString( pEntity ), "BOT" ) == 0) : true ) )
				{
					//UTIL_SetProperty( g_Offsets.m_bDrawViewmodel + g_Offsets.m_Local, pEntity, bOn );
					//UTIL_SetProperty( g_Offsets.m_iHideHUD + g_Offsets.m_Local, pEntity, (int)0 );
					bOn = false;
					UTIL_SetProperty( g_Offsets.m_bHasNightVision, pEntity, bOn );
					UTIL_SetProperty( g_Offsets.m_bNightVisionOn, pEntity, bOn );
					//UTIL_SetProperty( g_Offsets.m_iFOV + g_Offsets.m_Local, pEntity, (int)0 );
					CBasePlayer_SetFOV( pPlayer, pPlayer, 0 );
				}
				//pFOVr_Player = pEntity;
			}
			if ( zombie_teams.GetBool() )
			{
				//UTIL_SetPropertyInt( "CBaseEntity", "m_iTeamNum", pEntity, COUNTERTERRORISTS );
				//UTIL_SetProperty( g_Offsets.m_iTeamNum, pEntity, COUNTERTERRORISTS );
				if ( GetTeam( pEntity ) == TERRORISTS )
				{
					CBasePlayer_SwitchTeam( pPlayer, COUNTERTERRORISTS );
				}
			}
		}
	}
	else if ( FStrEq( name, "player_death" ) )
	{
		int iVic = EntIdxOfUserIdx( event->GetInt( "userid" ) );
		int iAtt = EntIdxOfUserIdx( event->GetInt( "attacker" ) );
		//g_ZombiePlayers[iVic].bC4Frag = false;
		edict_t *pPlayer = m_Engine->PEntityOfEntIndex ( iVic );
		edict_t *pAttacker = m_Engine->PEntityOfEntIndex ( iAtt );
		if ( zombie_mode.GetBool() )
		{
			if ( pAttacker && !pAttacker->IsFree() && pPlayer && !pPlayer->IsFree() && GetTeam( pAttacker ) == GetTeam( pPlayer ) && ( pAttacker != pPlayer ) )
			{
				CBasePlayer *pBase = (CBasePlayer *)pAttacker->GetUnknown()->GetBaseEntity();
				pBase->IncrementFragCount( 2 );
				//int iMoney = UTIL_GetPropertyInt( "CCSPlayer", "m_iAccount", pAttacker );
				int iMoney;
				if ( UTIL_GetProperty( g_Offsets.m_iAccount, pAttacker, &iMoney ) )
				{
					iMoney += 3000;
					//UTIL_SetPropertyInt( "CCSPlayer", "m_iAccount", pAttacker, iMoney );
					UTIL_SetProperty( g_Offsets.m_iAccount, pAttacker, iMoney );
				}
				
				int *iKills = (int *)(pAttacker->GetUnknown() + KILLSOFFSET);
				if ( iKills )
				{
					*iKills += 2;
				}
				
				//CCSPlayer_AddAccount_(pAttacker, 3000, false);
				//UTIL_AngleDiff//
			}
			if ( g_ZombiePlayers[iVic].isZombie )
			{
				g_ZombiePlayers[iVic].isZombie = false;
        		MRecipientFilter filter;
				filter.AddAllPlayers( g_SMAPI->pGlobals()->maxClients, m_Engine, m_PlayerInfoManager );
				char sound[512];
				Q_snprintf( sound, 512, "npc/zombie/zombie_die%d.wav", RandomInt( 1, 3 ) );
				m_EngineSound->EmitSound( filter, iVic, CHAN_VOICE, sound, RandomFloat( 0.6, 1.0 ), 0.5, 0, RandomInt( 70, 150 ) );
				//m_Engine->ClientCommand( pPlayer, "fov 90\n" );
				if ( !g_ZombiePlayers[iVic].isBot )
				{
					//pFOVr_Player = pPlayer;
					//UTIL_SetProperty( g_Offsets.m_iFOV + g_Offsets.m_Local, pPlayer, (int)0 );
					
					CBasePlayer *pBase = (CBasePlayer *)GetContainingEntity( pPlayer );
					CBasePlayer_SetFOV( pBase, pBase, 0 );
					bool bOn = true;
					//UTIL_SetProperty( g_Offsets.m_bDrawViewmodel + g_Offsets.m_Local, pPlayer, bOn );
					//UTIL_SetProperty( g_Offsets.m_iHideHUD + g_Offsets.m_Local, pPlayer, (int)0 );
					bOn = false;
					UTIL_SetProperty( g_Offsets.m_bHasNightVision, pPlayer, bOn );
					UTIL_SetProperty( g_Offsets.m_bNightVisionOn, pPlayer, bOn );
				}
			}
		}
		if ( zombie_respawn.GetBool() ) 
		{
			//CCSPlayer_AddAccount_(pPlayer, 16000, false);
			//UTIL_SetPropertyInt( "CCSPlayer", "m_iAccount", pPlayer, 16000 );
			UTIL_SetProperty( g_Offsets.m_iAccount, pPlayer, 16000 );
			void *params[1];
			params[0] = pPlayer;
			g_Timers->AddTimer( zombie_respawn_delay.GetFloat(), CheckRespawn, params, 1 );
		}
	}
	else if ( FStrEq( name, "round_start" ) )
	{
		if ( bFirstEverRound )
		{
			if ( zombie_startup.GetBool() )
			{
				m_Engine->ServerCommand( "zombies\n" );
			}
			bFirstEverRound = false;
			return;
		}
		if ( zombie_mode.GetBool() )
		{
			bRoundTerminating = false;
			bRoundStarted = false;
			int x = 0;
			iZombieCount = 0;
			for( x = 1; x < m_Engine->GetEntityCount(); x++ )
			{
				edict_t *pEnt = m_Engine->PEntityOfEntIndex( x );
				if ( pEnt && !pEnt->IsFree() )
				{
					if ( x < MAX_PLAYERS && m_Engine->GetPlayerUserId( pEnt ) != -1)
					{
						/*if ( zombie_teams.GetBool() )
						{
							//int iTeam = UTIL_GetPropertyInt( "CBaseEntity", "m_iTeamNum", pEnt );
							int iTeam;
							if ( UTIL_GetProperty( g_Offsets.m_iTeamNum, pEnt, &iTeam ) )
							{
								if ( iTeam == TERRORISTS || iTeam == COUNTERTERRORISTS )
								{
									//UTIL_SetPropertyInt( "CBaseEntity", "m_iTeamNum", pEnt, COUNTERTERRORISTS );
									//UTIL_SetProperty( g_Offsets.m_iTeamNum, pEnt, (int)COUNTERTERRORISTS );
									CBasePlayer_SwitchTeam( g_ZombiePlayers[x].pPlayer, COUNTERTERRORISTS );
								}
							}
						}*/
						//m_Engine->ClientCommand( pEnt, "fov 90\n" );
						//const char *sNetID = m_Engine->GetPlayerNetworkIDString( pEnt );
						//if ( sNetID && (Q_stricmp( sNetID, "bot" ) != 0) )
						//{
							//UTIL_SetProperty( g_Offsets.m_iFOV + g_Offsets.m_Local, pEnt, (int)90 );
						//}pEnt && !pEnt->IsFree()
					}
					else
					{
						if ( pEnt->GetClassName() && FStrEq( pEnt->GetClassName(), "hostage_entity" ) )
						{
							CBaseEntity *pBase = pEnt->GetUnknown()->GetBaseEntity();
							if ( pBase )
							{
								CBaseEntity_Event_Killed( pBase, CTakeDamageInfo(GetContainingEntity(m_Engine->PEntityOfEntIndex(0)), GetContainingEntity(m_Engine->PEntityOfEntIndex(0)), 0, DMG_NEVERGIB) );
								CBasePlayer_Event_Dying( (CBasePlayer *)pBase );
							}
						}
					}
				}
			}
		}
	}
	else if ( FStrEq( name, "round_end" ) && zombie_mode.GetBool()  )
	{
		int x = 0;
		if ( zombie_mode.GetBool() )
		{
			bRoundStarted = false;
			bRoundTerminating = true;
		}
		for ( x = 0; x < g_HookedWeapons.Count(); x++ )
		{
			SH_REMOVE_MANUALHOOK_MEMFUNC( GetMaxSpeed_hook, g_HookedWeapons[x], &g_ZombiePlugin, &ZombiePlugin::GetMaxSpeed, true);
			SH_REMOVE_MANUALHOOK_MEMFUNC( Delete_hook, g_HookedWeapons[x], &g_ZombiePlugin, &ZombiePlugin::Delete, true);
			g_HookedWeapons.Remove(x);
		}
	}
}
bool ZombiePlugin::LevelInit(const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background)
{

	META_CONPRINTF( "---------------- >LevelInit..." );
	Q_strncpy( g_CurrentMap, pMapName, sizeof(g_CurrentMap) );
	int x;
	for ( x = 0; x < CSW_MAX; x++)
	{
		g_RestrictT[x] = -1;
		g_RestrictCT[x] = -1;
		g_RestrictW[x] = -1;
	}
	for ( x = 1; x <= MAX_PLAYERS; x++ )
	{
		g_ZombiePlayers[x].isZombie = false;
		g_ZombiePlayers[x].isBot = false;
		g_ZombiePlayers[x].isHooked = false;
		g_ZombiePlayers[x].pPlayer = NULL;
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


	for ( x = 0; x <= m_Engine->GetEntityCount(); x++ )
	{
		edict_t *pTmp = m_Engine->PEntityOfEntIndex( x );
		if ( pTmp && !pTmp->IsFree() )
		{
			if ( pTmp->GetClassName() && FStrEq( pTmp->GetClassName(), "cs_team_manager" ) )
			{
				CBaseEntity *pBase = pTmp->GetUnknown()->GetBaseEntity();
				CTeam *tmpTeam = (CTeam *)pBase;
				//int	*team_number = (int *) &(tmpTeam->m_iTeamNum) + CSS_TEAM_OFFSET;
				//int team_number = UTIL_GetPropertyInt( "CTeam", "m_iTeamNum", pTmp );
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

	m_GameEventManager->AddListener( this, "item_pickup", true );
	m_GameEventManager->AddListener( this, "player_spawn", true );
	m_GameEventManager->AddListener( this, "round_start", true );
	m_GameEventManager->AddListener( this, "round_end", true );
	m_GameEventManager->AddListener( this, "player_death", true );
	m_GameEventManager->AddListener( this, "round_freeze_end", true );

	bRoundStarted = false;
	mp_spawnprotectiontime = m_CVar->FindVar("mp_spawnprotectiontime");
	mp_tkpunish = m_CVar->FindVar("mp_tkpunish");
	sv_alltalk = m_CVar->FindVar("sv_alltalk");
	ammo_buckshot_max = m_CVar->FindVar("ammo_buckshot_max");
	if ( g_Timers )
	{
		g_Timers->Reset();
		if ( zombie_mode.GetBool() )
		{
			g_Timers->AddTimer(1.0, ZombieLevelInit);
			g_Timers->AddTimer(0.5, CheckZombies, NULL, 0, ZOMBIECHECK_TIMER_ID);
			g_ZombieRoundOver = true;

		}
	}
	char buf[128];
	if ( strlen( zombie_model_file.GetString() ) && !LoadZombieModelList( zombie_model_file.GetString(), zombie_download_file.GetString() ) )
	{
		META_LOG( g_PLAPI, "Unable to load zombie model list." );
	}
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
	iBeamLaser = m_Engine->PrecacheModel( "sprites/bluelaser1.vmt" );
	m_EngineSound->PrecacheSound("npc/fast_zombie/fz_scream1.wav", true);
	RETURN_META_VALUE( MRES_IGNORED, true );
}

void ZombiePlugin::OnLevelShutdown()
{
}

void ZombiePlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	RETURN_META(MRES_IGNORED);
}

void ZombiePlugin::GameFrame(bool simulating)
{
	/*if ( bRemoveWriteHook )
	{
		if ( bfHook )
		{
			SH_REMOVE_HOOK_MEMFUNC( bf_write, WriteString, bfHook, &g_ZombiePlugin, ZombiePlugin::WriteString, false );
		}
		bRemoveWriteHook = false;
	}*/
	if ( zombie_mode.GetBool() )
	{
		/*if ( pFOV_Player != NULL )
		{
			void *params[2];
			params[0] = (void*)pFOV_Player;
			params[1] = (void*)zombie_fov.GetInt();
			Timed_SetFOV( params );
			pFOV_Player = NULL;
		}*/
		if ( g_Timers )
		{
			g_Timers->CheckTimers();
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
	for ( x = 1; x <= MAX_PLAYERS; x++ )
	{
		edict_t *pEntity = NULL;
		pEntity = m_Engine->PEntityOfEntIndex( x );
		CBasePlayer *pPlayer = (CBasePlayer *)GetContainingEntity( pEntity );
		g_ZombiePlayers[x].pPlayer = pPlayer;
		UnHookPlayer( x );
		g_ZombiePlayers[x].pPlayer = NULL;
		g_ZombiePlayers[x].isHooked = false;
		g_ZombiePlayers[x].isBot = false;
	}
	m_GameEventManager->RemoveListener( this );
	RETURN_META(MRES_IGNORED);
}

int ZombiePlugin::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !zombie_mode.GetBool() )
	{
		RETURN_META_VALUE( MRES_IGNORED, 0 );
	}
	CBasePlayer *pAttacker = NULL;
	CBaseEntity *pBase = NULL;
	CBaseEntity *pInflictor = NULL;

	CBasePlayer *pVictim =  META_IFACEPTR( CBasePlayer );
	if ( pVictim && pVictim->edict() && !pVictim->edict()->IsFree() )
	{
		int nIndex = m_Engine->IndexOfEdict( pVictim->edict() );
		int AIndex = -1;

		if ( info.m_hAttacker.GetEntryIndex() > 0 && info.m_hAttacker.GetEntryIndex() <= g_SMAPI->pGlobals()->maxClients ) 
		{
			edict_t *edict = m_Engine->PEntityOfEntIndex(info.m_hAttacker.GetEntryIndex() );
			if ( edict )
			{
				pAttacker = (CBasePlayer*)edict->GetUnknown()->GetBaseEntity();
				AIndex = m_Engine->IndexOfEdict( pAttacker->edict() );
			}
		}
		if ( info.m_hInflictor.GetEntryIndex() > 0 ) 
		{
			edict_t *edict = m_Engine->PEntityOfEntIndex(info.m_hInflictor.GetEntryIndex() );
			if ( edict ) 
			{
				pInflictor = edict->GetUnknown()->GetBaseEntity();
			}
		}

		if ( pAttacker )
		{
			bool attackerZombie = g_ZombiePlayers[AIndex].isZombie;
			bool isZombie = g_ZombiePlayers[nIndex].isZombie;
			if ( attackerZombie && !isZombie )
			{
				if ( pInflictor && pInflictor->GetClassname() && Q_stristr( pInflictor->GetClassname(), "nade" ) )
				{
					RETURN_META_VALUE( MRES_IGNORED, 0 );
				}
				int *iKills = (int *)(pAttacker->edict()->GetUnknown() + KILLSOFFSET);
				if ( iKills )
				{
					*iKills += 1;
				}
				iKills = (int *)(pVictim->edict()->GetUnknown() + KILLSOFFSET);
				if ( iKills )
				{
					*iKills -= 1;
				}
				MakeZombie( pVictim );
				MRecipientFilter filter;
				filter.AddAllPlayers( g_SMAPI->pGlobals()->maxClients, m_Engine, m_PlayerInfoManager );
				m_EngineSound->EmitSound( filter, nIndex, CHAN_VOICE, "npc/fast_zombie/fz_scream1.wav", RandomFloat( 0.6, 1.0 ), 0.5, 0, RandomInt( 70, 150 ) );
				RETURN_META_VALUE( MRES_IGNORED, 0 );
			}
			else if ( ( !isZombie && !attackerZombie ) || ( isZombie && attackerZombie ) )
			{
				RETURN_META_VALUE( MRES_SUPERCEDE, 0 );
			}
			else if ( isZombie )
			{
				MRecipientFilter filter;
				filter.AddAllPlayers( g_SMAPI->pGlobals()->maxClients, m_Engine, m_PlayerInfoManager );
				char sound[512];
				Q_snprintf(sound, 512, "npc/zombie/zombie_pain%d.wav", RandomInt(1,6));
				m_EngineSound->EmitSound( filter, nIndex, CHAN_VOICE, sound, RandomFloat(0.6, 1.0), 0.5, 0, RandomInt(70, 150) );
				if ( ( zombie_knockback.GetFloat() > 0) && ( pInflictor && pVictim->GetMoveType() == MOVETYPE_WALK ) && ( !pAttacker->IsSolidFlagSet( FSOLID_TRIGGER ) ) )
				{
					Vector vecDir = pInflictor->WorldSpaceCenter() - Vector (0, 0, 10) - pVictim->WorldSpaceCenter();
					VectorNormalize( vecDir );
					Vector force = vecDir * -DamageForce( pInflictor->WorldAlignSize(), info.GetBaseDamage() );
					if ( force.z > 250.0f )
					{
							force.z = 250.0f;
					}
					//pVictim->ApplyAbsVelocityImpulse( force );
					CBasePlayer_ApplyAbsVelocityImpulse( pVictim, force );
				}
			}
		}
	}
	RETURN_META_VALUE( MRES_IGNORED, 0 );
}

void ZombiePlugin::EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, float flVolume, float flAttenuation, int iFlags, int iPitch, const Vector *pOrigin, const Vector *pDirection, CUtlVector< Vector >* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity )
{
	if ( pSample )
	{
		META_LOG( g_PLAPI, "Sound: %s", pSample );
	}
	RETURN_META( MRES_IGNORED );
}

void ZombiePlugin::PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force )
{
	//META_LOG( g_PLAPI, "PlayStepSound" );
	if ( !zombie_mode.GetBool() )
	{
		return;
	}
	CBasePlayer *pPlayer =  META_IFACEPTR( CBasePlayer );
	if ( pPlayer && pPlayer->edict() && !pPlayer->edict()->IsFree() && m_Engine->GetPlayerUserId( pPlayer->edict() ) != -1 )
	{
		int iPlayer = m_Engine->IndexOfEdict( pPlayer->edict() );
		if ( g_ZombiePlayers[iPlayer].isZombie )
		{
		//
			/*int iFOV = 0;
			if ( UTIL_GetProperty( g_Offsets.m_iFOV + g_Offsets.m_Local, pPlayer->edict(), &iFOV ) )
			{
				if ( iFOV != zombie_fov.GetInt() )
				{
					iFOV = zombie_fov.GetInt();
					UTIL_SetProperty( g_Offsets.m_iFOV + g_Offsets.m_Local, pPlayer->edict(), iFOV );
				}
			}*/
		//
			RETURN_META( MRES_SUPERCEDE );
		}
	}
	RETURN_META( MRES_IGNORED );
}
void ZombiePlugin::UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity )
{
	if ( !zombie_mode.GetBool() )
	{
		return;
	}
	CBasePlayer *pPlayer =  META_IFACEPTR( CBasePlayer );
	if ( pPlayer && pPlayer->edict() && !pPlayer->edict()->IsFree() )
	{
		int iPlayer = m_Engine->IndexOfEdict( pPlayer->edict() );
		if ( zombie_mode.GetBool() && g_ZombiePlayers[iPlayer].isZombie )
		{
			RETURN_META( MRES_SUPERCEDE );
		}
	}
	RETURN_META( MRES_IGNORED );
}

bool ZombiePlugin::SetClientListening(int iReceiver, int iSender, bool bListen)
{
	if ( zombie_talk.GetBool() )
	{
		bool return_value;
		if ( g_ZombiePlayers[iReceiver].isZombie == g_ZombiePlayers[iSender].isZombie )
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

void ZombiePlugin::DoMuzzleFlash()
{
	CBasePlayer *pPlayer = META_IFACEPTR( CBasePlayer );
	//Hud_Print( NULL, "DoMuzzleFlash.." );
	//IPlayerInfo *info = m_PlayerInfoManager->GetPlayerInfo( pPlayer->edict() );
	if ( zombie_tracer.GetBool() && pPlayer && pPlayer->edict() && !pPlayer->edict()->IsFree() && !(m_Engine->GetPlayerUserId( pPlayer->edict() ) == -1) )
	{
		int iPlayer = m_Engine->IndexOfEdict( pPlayer->edict() );
		g_ZombiePlayers[iPlayer].iBulletCount++;
		if ( g_ZombiePlayers[iPlayer].iBulletCount >= zombie_tracer_count.GetInt() )
		{
			g_ZombiePlayers[iPlayer].iBulletCount = 0;
			IPlayerInfo *info = m_PlayerInfoManager->GetPlayerInfo( pPlayer->edict() );
			if ( info )
			{
				Vector forward, right, up;
				AngleVectors( info->GetAbsAngles(), &forward, &right, &up );
				//int iFlags = UTIL_GetPropertyInt( "CBasePlayer", "m_fFlags", pPlayer->edict() );
				int iFlags;
				if ( !UTIL_GetProperty( g_Offsets.m_fFlags, pPlayer->edict(), &iFlags ) )
				{
					RETURN_META( MRES_IGNORED );
				}
				Vector m_Hack = info->GetAbsOrigin();
				m_Hack.z +=  ( (iFlags & FL_DUCKING) ? 26 : 63 );
				//Vector vecSrc = info->GetAbsOrigin() + forward * m_HackedGunPos.y + right * m_HackedGunPos.x + up * m_HackedGunPos.z;
				Vector vecSrc = m_Hack;
				Vector vecEnd = vecSrc + forward * MAX_TRACE_LENGTH;

				trace_t tr;
				CTraceFilterSkipTwoEntities traceFilter( pPlayer, NULL, COLLISION_GROUP_NONE );
				UTIL_TraceLines( vecSrc, vecEnd, MASK_SHOT, &traceFilter, &tr, true );
				
				MRecipientFilter Recpt;
				if ( zombie_tracer_all.GetBool() )
				{
					Recpt.AddAllPlayers( g_SMAPI->pGlobals()->maxClients, m_Engine, m_PlayerInfoManager );
				}
				else
				{
					Recpt.AddPlayer( m_Engine->IndexOfEdict( pPlayer->edict() ), m_Engine, m_PlayerInfoManager );
				}
				//m_TempEnts->BeamPoints( Recpt, 0, &vecSrc, &tr.endpos, iBeamLaser, 0, 10, 2.0f, 0.1f, 2, 2, 0, 0, 255, 255, 255, 50, 1000 );
			}
		}
	}
	
	RETURN_META( MRES_IGNORED );
}

void ZombiePlugin::FireBullets( const FireBulletsInfo_t &info )
{
	Hud_Print( NULL, "FireBullets.." );
	static int iCount;
	iCount++;
	CBaseEntity *pPlayer = META_IFACEPTR( CBaseEntity );
	if ( pPlayer && pPlayer->edict() && !pPlayer->edict()->IsFree() && !(m_Engine->GetPlayerUserId( pPlayer->edict() ) == -1) )
	{
		if ( iCount >= 3 )
		{
			iCount = 0;
			Vector vecEnd = info.m_vecSrc + info.m_vecDirShooting * info.m_flDistance;

			trace_t tr;
			CTraceFilterSkipTwoEntities traceFilter( pPlayer, info.m_pAdditionalIgnoreEnt, COLLISION_GROUP_NONE );
			UTIL_TraceLines( info.m_vecSrc, vecEnd, MASK_SHOT, &traceFilter, &tr, true );

			//info.m_pAttacker
			CEffectData data;
			data.m_vStart = info.m_vecSrc;
			data.m_vOrigin = vecEnd;
			data.m_nEntIndex = m_Engine->IndexOfEdict( pPlayer->edict() );
			data.m_flScale = 0.0f;

			/*// Flags
			if ( bWhiz )
			{
				data.m_fFlags |= TRACER_FLAG_WHIZ;
			}
			if ( iAttachment != TRACER_DONT_USE_ATTACHMENT )
			{
				data.m_fFlags |= TRACER_FLAG_USEATTACHMENT;
				// Stomp the start, since it's not going to be used anyway
				data.m_nAttachmentIndex = 1;
			}*/

			//DispatchEffect( "Tracer", data );
			MRecipientFilter Recpt;
			Recpt.AddAllPlayers( g_SMAPI->pGlobals()->maxClients, m_Engine, m_PlayerInfoManager );
			//m_TempEnts->DispatchEffect( Recpt, 0.0, vecEnd, "Tracer", data );
		}
	}
	RETURN_META( MRES_IGNORED );
}

bool ZombiePlugin::Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon )
{
	//RETURN_META_VALUE( MRES_IGNORED, true );
	CBasePlayer *pPlayer =  META_IFACEPTR( CBasePlayer );
	if ( !IsAlive( pPlayer->edict() ) || !bRoundStarted )
	{
		RETURN_META_VALUE( MRES_IGNORED, true );
	}
	if ( !CanUseWeapon( pPlayer, pWeapon, false ) )
	{
		RETURN_META_VALUE( MRES_OVERRIDE, false );
	}
	RETURN_META_VALUE( MRES_IGNORED, true );
}

bool ZombiePlugin::Weapon_CanUse( CBaseCombatWeapon *pWeapon )
{
	/*if ( !zombie_mode.GetBool() )
	{
		RETURN_META_VALUE( MRES_IGNORED, true );
	}*/
	//RETURN_META_VALUE( MRES_IGNORED, true );
	CBasePlayer *pPlayer =  META_IFACEPTR( CBasePlayer );
	if ( !IsAlive( pPlayer->edict() ) || !bRoundStarted )
	{
		RETURN_META_VALUE( MRES_IGNORED, true );
	}
	if ( !CanUseWeapon( pPlayer, pWeapon, false ) )
	{
		RETURN_META_VALUE( MRES_OVERRIDE, false );
	}
	RETURN_META_VALUE( MRES_IGNORED, true );
}

bool ZombiePlugin::IsAlive( edict_t *pEntity )
{
	if ( !pEntity || pEntity->IsFree() || m_Engine->GetPlayerUserId( pEntity ) == -1 )
	{
		return false;
	}
	IPlayerInfo *info = m_PlayerInfoManager->GetPlayerInfo( pEntity );
	if ( info && !info->IsDead() )
	{
		return true;
	}
	return false;
}

void ZombiePlugin::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( !zombie_mode.GetBool() )
	{
		RETURN_META( MRES_IGNORED );
	}
	CBaseEntity *pPlayer = META_IFACEPTR( CBaseEntity );
	if ( pPlayer && pPlayer->edict() && !pPlayer->edict()->IsFree() )
	{
		int iAttacker = -1;
		int iPlayer = m_Engine->IndexOfEdict( pPlayer->edict() );
		CBaseEntity *pBase =  NULL;
		if ( info.m_hAttacker.GetEntryIndex() > 0 ) 
		{
			edict_t *edict = m_Engine->PEntityOfEntIndex( info.m_hAttacker.GetEntryIndex() );
			if ( edict ) 
			{
				pBase = edict->GetUnknown()->GetBaseEntity();
				if ( pBase && pBase->edict() )
				{
					iAttacker = m_Engine->IndexOfEdict( pBase->edict() );
				}
			}
		}
		if ( pBase && pBase->edict() && m_Engine->GetPlayerUserId( pPlayer->edict() ) != -1 )
		{
			bool bAttacker = g_ZombiePlayers[iAttacker].isZombie;
			bool bVictim = g_ZombiePlayers[iPlayer].isZombie;
			if ( ( bAttacker && bVictim ) || ( !bAttacker && !bVictim ) )
			{
				RETURN_META( MRES_SUPERCEDE );
			}
		}
	}
	RETURN_META( MRES_IGNORED );
}

void ZombiePlugin::ClientActive( edict_t *pEntity, bool bLoadGame )
{
	RETURN_META( MRES_IGNORED );
}

void ZombiePlugin::ClientDisconnect(edict_t *pEntity)
{
	if ( pEntity && !pEntity->IsFree() )
	{
		int iPlayer = m_Engine->IndexOfEdict( pEntity );
		CBasePlayer *pPlayer = (CBasePlayer *)GetContainingEntity( pEntity );
		g_ZombiePlayers[iPlayer].pPlayer = pPlayer;
		META_LOG( g_PLAPI, "ClientDisconnect(%d) = %p // %p", iPlayer, g_ZombiePlayers[iPlayer].pPlayer, pPlayer );
		UnHookPlayer( iPlayer );
		g_ZombiePlayers[iPlayer].pPlayer = NULL;
		g_ZombiePlayers[iPlayer].isHooked = false;
		g_ZombiePlayers[iPlayer].isBot = false;
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
	if ( zombie_mode.GetBool() )
	{
		CBaseCombatWeapon *pWeapon = META_IFACEPTR( CBaseCombatWeapon );
		if ( pWeapon )
		{
			int x;
			for ( x = 1; x <= g_SMAPI->pGlobals()->maxClients; x++ )
			{
				if ( g_ZombiePlayers[x].isZombie && g_ZombiePlayers[x].pKnife == pWeapon ) 
				{
					RETURN_META_VALUE( MRES_SUPERCEDE, zombie_speed.GetFloat() );
				}
			}
		}
	}
	RETURN_META_VALUE( MRES_IGNORED, 0.0 );
}

void ZombiePlugin::Touch( CBaseEntity *pOther )
{
	CBaseEntity *pPlayer = META_IFACEPTR( CBaseEntity );
	if (!CanUseWeapon( pPlayer, pOther, false ) )
	{
		RETURN_META( MRES_SUPERCEDE );
	}
	RETURN_META( MRES_IGNORED );
}

void ZombiePlugin::HookPlayer( int iPlayer )
{
	if ( iPlayer > 0 && iPlayer <= MAX_PLAYERS && !g_ZombiePlayers[iPlayer].isHooked && !IsBadPtr( g_ZombiePlayers[iPlayer].pPlayer ) )
	{
		META_LOG( g_PLAPI, "Hooking %p", g_ZombiePlayers[iPlayer].pPlayer );
		SH_ADD_MANUALHOOK_MEMFUNC( Touch_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Touch, false );
		SH_ADD_MANUALHOOK_MEMFUNC( Weapon_CanUse_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Weapon_CanUse, false );
		SH_ADD_MANUALHOOK_MEMFUNC( OnTakeDamage_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::OnTakeDamage, false );
		SH_ADD_MANUALHOOK_MEMFUNC( PlayStepSound_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::PlayStepSound, false );
		SH_ADD_MANUALHOOK_MEMFUNC( TraceAttack_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::TraceAttack, false );
		SH_ADD_MANUALHOOK_MEMFUNC( Weapon_CanSwitchTo_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Weapon_CanSwitchTo, false );
		SH_ADD_MANUALHOOK_MEMFUNC( DoMuzzleFlash_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::DoMuzzleFlash, false );
	}
	g_ZombiePlayers[iPlayer].isHooked = true;
}

void ZombiePlugin::UnHookPlayer( int iPlayer )
{
	if ( iPlayer > 0 && iPlayer <= MAX_PLAYERS && g_ZombiePlayers[iPlayer].isHooked && !IsBadPtr( g_ZombiePlayers[iPlayer].pPlayer ) )
	{
		META_LOG( g_PLAPI, "UnHooking %p", g_ZombiePlayers[iPlayer].pPlayer );
		SH_REMOVE_MANUALHOOK_MEMFUNC( Touch_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Touch, false );
		SH_REMOVE_MANUALHOOK_MEMFUNC( Weapon_CanUse_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Weapon_CanUse, false );
		SH_REMOVE_MANUALHOOK_MEMFUNC( OnTakeDamage_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::OnTakeDamage, false );
		SH_REMOVE_MANUALHOOK_MEMFUNC( TraceAttack_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::TraceAttack, false );
		SH_REMOVE_MANUALHOOK_MEMFUNC( PlayStepSound_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::PlayStepSound, false );
		SH_REMOVE_MANUALHOOK_MEMFUNC( Weapon_CanSwitchTo_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::Weapon_CanSwitchTo, false );
		SH_REMOVE_MANUALHOOK_MEMFUNC( DoMuzzleFlash_hook, g_ZombiePlayers[iPlayer].pPlayer, &g_ZombiePlugin, &ZombiePlugin::DoMuzzleFlash, false );
	}
	g_ZombiePlayers[iPlayer].isHooked = false;
}

void ZombiePlugin::ClientPutInServer(edict_t *pEntity, char const *playername)
{
	//if ( playername && FStrEq( playername, "--DOIT--" )
	{
		if ( pEntity && !pEntity->IsFree() )
		{
			int iPlayer = m_Engine->IndexOfEdict( pEntity );
			const char *sSteamID = m_Engine->GetPlayerNetworkIDString( pEntity );
			g_ZombiePlayers[iPlayer].isBot = ( sSteamID ? (Q_strcmp( sSteamID, "BOT" ) == 0) : true );
			CBasePlayer *pBase = (CBasePlayer *)pEntity->GetUnknown()->GetBaseEntity();
			if ( pBase  )
			{
				if ( g_Offsets.m_hZoomOwner == 0 )
				{
					g_Offsets.m_hZoomOwner = UTIL_FindOffsetMap( pBase, "CBasePlayer", "m_hZoomOwner" );
				}
				g_ZombiePlayers[iPlayer].pPlayer = pBase;
				UnHookPlayer( iPlayer );
				HookPlayer( iPlayer );
				g_ZombiePlayers[iPlayer].isZombie = false;
				if ( !g_ZombiePlayers[iPlayer].isBot )
				{
					if ( zombie_mode.GetBool() )
					{
						m_Engine->ClientCommand( pEntity, "r_radiosity 0\n" );
					}
					else
					{
						m_Engine->ClientCommand( pEntity, "r_radiosity 4\n" );	
					}
					m_Engine->ClientCommand( pEntity, "cl_predictweapons 1\n" );
				}
			}
		}
	}
	RETURN_META( MRES_IGNORED );
}

void ZombiePlugin::SetCommandClient(int index)
{
	iCurrentPlayer = index + 1;
	RETURN_META(MRES_IGNORED);
}

void ZombiePlugin::ClientSettingsChanged(edict_t *pEdict)
{
	RETURN_META(MRES_IGNORED);
}

bool ZombiePlugin::ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	RETURN_META_VALUE(MRES_IGNORED, true);
}

void ZombiePlugin::ClientCommand(edict_t *pEntity)
{
	const char *sCmd = m_Engine->Cmd_Argv(0);
	if ( !pEntity || !pEntity->GetUnknown() || !pEntity->GetUnknown()->GetBaseEntity() || !sCmd )
	{
		RETURN_META( MRES_IGNORED );
	}
	int iPlayer = m_Engine->IndexOfEdict( pEntity );

	if ( FStrEq( sCmd, "resme" ) )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pEntity->GetUnknown()->GetBaseEntity();
		if ( pPlayer )
		{
			UTIL_RestartRound( pPlayer );
			//GiveNamedItem_Test( pPlayer, "weapon_m4a1", 0 );
		}
		RETURN_META( MRES_SUPERCEDE );
	}
	else if ( FStrEq( sCmd, "doup" ) )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pEntity->GetUnknown()->GetBaseEntity();
		if ( pPlayer )
		{
			CBasePlayer_ApplyAbsVelocityImpulse( pPlayer, Vector( 0, 0, 500 ) );
		}
		RETURN_META( MRES_SUPERCEDE );
	}
	else if ( FStrEq( sCmd, "dohealth" ) )
	{
		int iHealth;
		if ( UTIL_GetProperty( g_Offsets.m_iHealth, pEntity, &iHealth ) )
		{
			UTIL_SetProperty( g_Offsets.m_iHealth, pEntity, zombie_health.GetInt() );
			Hud_Print( pEntity, "%d %d %d", g_Offsets.m_iHealth, iHealth, zombie_health.GetInt() );
		}
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
	else if ( zombie_mode.GetBool() && FStrEq( sCmd, "nightvision" ) )
	{
		RETURN_META( MRES_SUPERCEDE );
	}
	else if ( zombie_mode.GetBool() && FStrEq( sCmd, "scream" ) )
	{
		IPlayerInfo *info = m_PlayerInfoManager->GetPlayerInfo( pEntity );
		if ( info && ( !g_ZombiePlayers[iPlayer].isZombie || info->IsDead()  ) )
		{
			Hud_Print( pEntity, "[ZOMBIE] Cannot scream when dead." );
			RETURN_META( MRES_SUPERCEDE );
		}
		MRecipientFilter filter;
		filter.AddAllPlayers( g_SMAPI->pGlobals()->maxClients, m_Engine, m_PlayerInfoManager );
		char sSound[128];
		Q_snprintf( sSound, sizeof(sSound), "npc/zombie/%s.wav", g_sZombieSounds[RandomInt(0, NUM_ZOMBIE_SOUNDS-1)] );
		m_EngineSound->EmitSound(filter, iPlayer, CHAN_VOICE, sSound, RandomFloat(0.1, 1.0), 0.5, 0, RandomInt(70, 150)/*PITCH_NORM*/);
	}
	CBasePlayer *pPlayer = (CBasePlayer *)pEntity->GetUnknown()->GetBaseEntity();
	CheckAutobuy( iPlayer, pPlayer );
	if ( FStrEq( sCmd, "buy") && ( m_Engine->Cmd_Argc() > 1 ) )
	{
		int WeaponID = LookupBuyID(m_Engine->Cmd_Argv(1));
		int nCount = 0;
		if (!AllowWeapon(pPlayer, WeaponID, nCount))
		{
			if (nCount == 0)
			{
				Hud_Print( pEntity, "* Sorry, the %s is restricted.", LookupWeaponName( WeaponID ) );
			}
			RETURN_META( MRES_SUPERCEDE );
		}
	}
	else if ( FStrEq( sCmd, "buyammo1" ) )
	{
		int WeaponID = LookupBuyID( "primammo" );
		int nCount = 0;
		if ( !AllowWeapon( pPlayer, WeaponID, nCount ) )
		{
			if (nCount == 0)
			{
				Hud_Print( pEntity, "* Sorry, the %s is restricted.", LookupWeaponName( WeaponID ) );
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
				Hud_Print( pEntity, "* Sorry, the %s is restricted.", LookupWeaponName( WeaponID ) );
			}
			RETURN_META( MRES_SUPERCEDE );
		}
	}

	RETURN_META( MRES_IGNORED );
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
	strcpy(iface_buffer, IENGINESOUND_SERVER_INTERFACE_VERSION); // m_CVar
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

	bFirstEverRound = true;
	for (int x = 0; x < CSW_MAX; x++)
	{ 
		g_RestrictT[x] = -1;
		g_RestrictCT[x] = -1;
		g_RestrictW[x] = -1;
	}
	for ( x = 1; x <= MAX_PLAYERS; x++ )
	{
		g_ZombiePlayers[x].isZombie = 0;
		g_ZombiePlayers[x].isBot = 0;
		g_ZombiePlayers[x].isHooked = 0;
		g_ZombiePlayers[x].pPlayer = NULL;
		g_ZombiePlayers[x].iBulletCount = 0;
	}
	void *laddr = reinterpret_cast<void *>(g_SMAPI->serverFactory(false));
	void *vEntList;

	//SIGFIND(g_GetNameFunc, GetNameFunction, CBaseCombatWeapon_GetName_Sig, CBaseCombatWeapon_GetName_SigBytes);
#ifdef WIN32
	#define SIGFIND( var, type, sig, sigbytes ) \
	var = (type)g_SigMngr.ResolveSig(laddr, sig, sigbytes); \
	META_CONPRINTF("[ZOMBIE] %p\n", var); \
	if (!var) { \
		snprintf(error, maxlen, "Could not find all sigs!"); \
		return false; \
	}

	//vEntList = g_SigMngr.ResolveSig( laddr, CEntList_Pattern, CEntList_PatternBytes);
	//vUClient = g_SigMngr.ResolveSig( laddr, CPlayer_UpdateClient_Sig, CPlayer_UpdateClient_SigBytes );

	SIGFIND( vEntList, void *, CEntList_Pattern, CEntList_PatternBytes);
	SIGFIND( vUClient, void *, CPlayer_UpdateClient_Sig, CPlayer_UpdateClient_SigBytes);

	SIGFIND( g_SetModelFunc, SetModelFunction, CBaseFlex_SetModel_Sig, CBaseFlex_SetModel_SigBytes);
	SIGFIND( g_GiveNamedItemFunc, GiveNamedItem_Func, CCSPlayer_GiveNamedItem_Sig, CCSPlayer_GiveNamedItem_SigBytes);
	SIGFIND( g_WeaponSlotFunc, WeaponSlotFunction, CBaseCombatPlayer_WeaponGetSlot_Sig, CBaseCombatPlayer_WeaponGetSlot_SigBytes);
	SIGFIND( g_WeaponDropFunc, WeaponDropFunction, CBasePlayer_WeaponDrop_Sig, CBasePlayer_WeaponDrop_SigBytes);
	SIGFIND( g_WeaponDeleteFunc, WeaponDeleteFunction, CBaseCombatWeapon_Delete_Sig, CBaseCombatWeapon_Delete_SigBytes);
	SIGFIND( g_TermRoundFunc, TermRoundFunction, CCSGame_TermRound_Sig, CCSGame_TermRound_SigBytes);
	SIGFIND( g_WantsLag, WantsLagFunction, WantsLagComp_Sig, WantsLagComp_SigBytes );
	SIGFIND( g_CCSPlayer_RoundRespawn, CCSPlayer_RoundRespawnFunc, CCSPlayer_RoundRespawn_Sig, CCSPlayer_RoundRespawn_SigBytes );
	SIGFIND( g_GetFileWeaponInfoFromHandle, GetFileWeaponInfoFromHandleFunc, GetFileWeaponInfoFromHandle_Sig, GetFileWeaponInfoFromHandle_SigBytes );
	SIGFIND( v_GiveNamedItem, void *, CCSPlayer_GiveNamedItem_Sig, CCSPlayer_GiveNamedItem_SigBytes );
	SIGFIND( v_SetFOV, void *, SetFOV_Sig, SetFOV_SigBytes );
	SIGFIND( v_SwitchTeam, void *, CCSPlayer_SwitchTeam_Sig, CCSPlayer_SwitchTeam_SigBytes );
	SIGFIND( v_ApplyAbsVelocity, void *, CBaseEntity_ApplyAbsVelocity_Sig, CBaseEntity_ApplyAbsVelocity_SigBytes );
	SIGFIND( g_TakeDamage, OnTakeDamageFunction, OnTakeDamage_Sig, OnTakeDamage_SigBytes );
	
	
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

			SIGFIND( var_address, "te" );
			SIGFIND( vUClient, "_ZN11CBasePlayer16UpdateClientDataEv" );
			SIGFIND( g_SetModelFunc, "_ZN14CBaseAnimating8SetModelEPKc" );
			SIGFIND( g_TermRoundFunc, "_ZN12CCSGameRules14TerminateRoundEfi" );
			SIGFIND( g_GiveNamedItemFunc, "_ZN11CBasePlayer13GiveNamedItemEPKci" );
			SIGFIND( g_WeaponDropFunc, "_ZN11CBasePlayer11Weapon_DropEP17CBaseCombatWeaponPK6VectorS4_" );
			SIGFIND( g_WeaponSlotFunc, "_ZNK20CBaseCombatCharacter14Weapon_GetSlotEi" );
			SIGFIND( g_WeaponDeleteFunc, "_ZN17CBaseCombatWeapon6DeleteEv" );
			SIGFIND( g_WantsLag, "_ZNK11CBasePlayer28WantsLagCompensationOnEntityEPKS_PK8CUserCmdPK7CBitVecILi2048EE" );
			SIGFIND( g_TakeDamage, "_ZN9CCSPlayer12OnTakeDamageERK15CTakeDamageInfo" );
			SIGFIND( g_GetFileWeaponInfoFromHandle, "_Z27GetFileWeaponInfoFromHandlet" );
			SIGFIND( g_CCSPlayer_RoundRespawn, "_ZN9CCSPlayer12RoundRespawnEv" );
			SIGFIND( v_GiveNamedItem, "_ZN11CBasePlayer13GiveNamedItemEPKci" );
			SIGFIND( v_SetFOV, "_ZN11CBasePlayer6SetFOVEP11CBaseEntityif" );
			SIGFIND( v_SwitchTeam, CCSPlayer_SwitchTeam_Sig );
			SIGFIND( v_ApplyAbsVelocity, CBaseEntity_ApplyAbsVelocity_Sig );
            dlclose( handle );
    }
#endif

	if ( vUClient )
	{
		FindGameRules( (char *)vUClient );
	}
	else
	{
		snprintf(error, maxlen, "Could not find CGameRules !");
		return false;
		
	}
	//META_LOG(g_PLAPI, "Starting plugin.\n");

	ismm->AddListener(this, &g_Listener);

	ConCommandBaseMgr::OneTimeInit(&g_Accessor);



	//SH_ADD_HOOK_MEMFUNC(IEngineSound, EmitSound, m_EngineSound, &g_ZombiePlugin, &ZombiePlugin::EmitSound, false );
	//m_Engine_CC = SH_GET_CALLCLASS( m_Engine );
	m_VoiceServer_CC = SH_GET_CALLCLASS( m_VoiceServer );

	SH_ADD_HOOK_MEMFUNC(IVoiceServer, SetClientListening, m_VoiceServer, &g_ZombiePlugin, &ZombiePlugin::SetClientListening, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::LevelInit, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::ServerActivate, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::GameFrame, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::LevelShutdown, false);
	//SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientActive, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientActive, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientDisconnect, false );
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientPutInServer, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::SetCommandClient, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientSettingsChanged, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientConnect, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientCommand, false);

	//g_SMAPI->AddListener(g_PLAPI, this);

	m_GameEventManager->AddListener( this, "player_spawn", true );
	m_GameEventManager->AddListener( this, "hegrenade_detonate", true );
	m_GameEventManager->AddListener( this, "flashbang_detonate", true );
	m_GameEventManager->AddListener( this, "round_start", true );
	m_GameEventManager->AddListener( this, "round_end", true );
	m_GameEventManager->AddListener( this, "round_freeze_end", true );
	m_GameEventManager->AddListener( this, "item_pickup", true );
	m_GameEventManager->AddListener( this, "player_death", true );

	g_Offsets.m_hZoomOwner = 0;

	g_Timers = new STimers();

	return true;
}

bool ZombiePlugin::Unload(char *error, size_t maxlen)
{

	 FFA_Disable();

	m_GameEventManager->RemoveListener( this );

	if ( pKillCmd != NULL )
		SH_ADD_HOOK_STATICFUNC( ConCommand, Dispatch, pKillCmd, cmdKill, false );
	
	SH_REMOVE_HOOK_MEMFUNC(IVoiceServer, SetClientListening, m_VoiceServer, &g_ZombiePlugin, &ZombiePlugin::SetClientListening, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::LevelInit, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::ServerActivate, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::GameFrame, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, m_ServerDll, &g_ZombiePlugin, &ZombiePlugin::LevelShutdown, false);
	//SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientActive, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientActive, true);
	SH_REMOVE_HOOK_MEMFUNC( IServerGameClients, ClientDisconnect, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientDisconnect, false );
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientPutInServer, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::SetCommandClient, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientSettingsChanged, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientConnect, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientConnect, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientCommand, m_ServerClients, &g_ZombiePlugin, &ZombiePlugin::ClientCommand, false);

	SH_RELEASE_CALLCLASS(m_VoiceServer_CC);

	
	g_SetModelFunc = NULL;
	g_TermRoundFunc = NULL;
	g_GiveNamedItemFunc = NULL;
	g_GetNameFunc = NULL;
	g_WeaponSlotFunc = NULL;
	g_WeaponDropFunc = NULL;
	g_WeaponDeleteFunc = NULL;
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
	return true;
}

void cmdKill( void )
{
	if ( zombie_mode.GetBool() && zombie_suicide.GetBool() )
	{
		edict_t *pEnt = g_ZombiePlugin.m_Engine->PEntityOfEntIndex( g_ZombiePlugin.iCurrentPlayer );
		if ( pEnt && !pEnt->IsFree() && g_ZombiePlugin.m_Engine->GetPlayerUserId( pEnt ) != -1 )
		{
			g_ZombiePlugin.Hud_Print( pEnt, "[ZOMBIE] %s", zombie_suicide_text.GetString() );
		}
		RETURN_META( MRES_SUPERCEDE );
	}
	RETURN_META( MRES_IGNORED );
}

void ZombiePlugin::AllPluginsLoaded()
{
	mp_friendlyfire = m_CVar->FindVar("mp_friendlyfire");
	mp_limitteams = m_CVar->FindVar("mp_limitteams");
	mp_autoteambalance = m_CVar->FindVar("mp_autoteambalance");
	ConCommandBase *pCmd = m_CVar->GetCommands();
	while (pCmd)
	{
		if ( /*pKillCmd == NULL &&*/ pCmd->IsCommand() && (FStrEq(pCmd->GetName(), "kill")) )
		{
			pKillCmd = (ConCommand *)pCmd;
			SH_ADD_HOOK_STATICFUNC( ConCommand, Dispatch, pKillCmd, cmdKill, false );
			META_CONPRINTF( "[ZOMBIE] Found kill command. %p\n", pKillCmd );
			//break;
		}
		else if ( pCmd->IsCommand() && (FStrEq(pCmd->GetName(), "fov")) )
		{
			cmd_fov = (ConCommand *)pCmd;
			META_CONPRINTF( "[ZOMBIE] Found fov command. %p\n", cmd_fov );
		}
		pCmd = const_cast<ConCommandBase *>(pCmd->GetNext());
	}
	if ( !cmd_fov )
	{
		META_CONPRINTF( "-----> No FOV ?\n" );
	}

	return;
	//FFA_Enable();
}

void Timed_SetFOV( void **params )
{
	edict_t *pEnt = (edict_t *)params[0];
	int iFOV = (int)params[1];
	if ( pEnt && !pEnt->IsFree() && g_ZombiePlugin.m_Engine->GetPlayerUserId( pEnt ) != -1 )
	{
		/*CBasePlayer *pPlayer = (CBasePlayer*)GetContainingEntity( pEnt );
		if ( pPlayer )
		{
			UTIL_SetProperty( g_ZombiePlugin.g_Offsets.m_hZoomOwner, pEnt, pPlayer );
		}*/
		int iPlayer = g_ZombiePlugin.m_Engine->IndexOfEdict( pEnt );
		CBasePlayer_SetFOV( (CBasePlayer *)g_ZombiePlayers[iPlayer].pPlayer, (CBasePlayer *)g_ZombiePlayers[iPlayer].pPlayer, iFOV );
		//UTIL_SetProperty( g_ZombiePlugin.g_Offsets.m_iFOV + g_ZombiePlugin.g_Offsets.m_Local, pEnt, iFOV );
	}
}


void ZombiePlugin::MakeZombie( CBasePlayer *pPlayer )
{
	char sModel[128];
	int iPlayer = m_Engine->IndexOfEdict( pPlayer->edict() );
	if ( g_ZombieModels.Count() < 1 )
	{
	        Q_strncpy( sModel, "models/zombie/classic.mdl", sizeof( sModel ) );
	} 
	else
	{
	        Q_strncpy( sModel, g_ZombieModels[ RandomInt(0, g_ZombieModels.Count()-1) ].c_str(), sizeof( sModel ) );
	}
	UTIL_SetModel( ( CBaseFlex * )pPlayer, sModel );
	g_ZombiePlayers[iPlayer].isZombie = true;
	ZombieDropWeapons( pPlayer, true );
	edict_t *pEntity = pPlayer->edict();
	//CBasePlayer *pPlayer = (CBasePlayer *)GetContainingEntity( pEntity );
	UTIL_SetProperty( g_Offsets.m_iHealth, pEntity, zombie_health.GetInt() );
	bool bOn = false;
	int iInt = 0;
	if ( !g_ZombiePlayers[iPlayer].isBot )
	{
//		UTIL_SetProperty( g_Offsets.m_bDrawViewmodel + g_Offsets.m_Local, pEntity, (bool)bOn );
//		UTIL_SetProperty( g_Offsets.m_iHideHUD + g_Offsets.m_Local, pEntity, (int)HIDEHUD_CROSSHAIR );
		if ( UTIL_GetProperty( g_Offsets.m_iAccount, pEntity, &iInt ) )
		{
			iInt += 1250;
			UTIL_SetProperty( g_Offsets.m_iAccount, pEntity, iInt );
		}
		bOn = true;
		UTIL_SetProperty( g_Offsets.m_bHasNightVision, pEntity, bOn );
		UTIL_SetProperty( g_Offsets.m_bNightVisionOn, pEntity, bOn );
		iInt = zombie_fov.GetInt();
		//UTIL_SetProperty( g_Offsets.m_hZoomOwner, pEntity, pPlayer );
		//UTIL_SetProperty( g_Offsets.m_iFOV + g_Offsets.m_Local, pEntity, (int)iInt );
		CBasePlayer_SetFOV( pPlayer, pPlayer, iInt );
		//pFOV_Player = pEntity;
		//g_Timers->AddTimer( 0.1, Timed_SetFOV, params, 1 );
	}
	if ( iZombieCount > 1 && zombie_teams.GetBool() )
	{
		//UTIL_SetProperty( g_Offsets.m_iTeamNum, pEntity, TERRORISTS );
		CBasePlayer_SwitchTeam( pPlayer, TERRORISTS );
	}
	Hud_Print( pEntity, "[ZOMBIE] You are now a zombie. Eat some brains !" );
	return;
}

void ZombiePlugin::ZombieDropWeapons( CBasePlayer *pPlayer, bool bHook )
{
	int iWeaponSlots[] = { SLOT_NADE, SLOT_NADE, SLOT_NADE, SLOT_NADE, SLOT_SECONDARY, SLOT_PRIMARY, SLOT_KNIFE };
	if ( !zombie_mode.GetBool() )
	{
		return;
	}
	bool bDeleted = false;
	CBaseCombatWeapon *pWeapon =  NULL;
	for (int x = 6; x >= 0; x--)
	{
		pWeapon = UTIL_WeaponSlot( (CBaseCombatCharacter*) pPlayer, iWeaponSlots[x] );
		if ( pWeapon )
		{
			if ( pWeapon->edict()->GetClassName() )
			{
				if ( FStrEq( pWeapon->edict()->GetClassName(), "weapon_knife" ) )
				{
					CBaseEntity *pBase = (CBaseEntity *)pWeapon;
					if ( bHook  )
					{
						int iDX = g_HookedWeapons.Find( pBase );
						if ( iDX != -1 )
						{
							SH_REMOVE_MANUALHOOK_MEMFUNC( GetMaxSpeed_hook, pBase, &g_ZombiePlugin, &ZombiePlugin::GetMaxSpeed, true);
							SH_REMOVE_MANUALHOOK_MEMFUNC( Delete_hook, pBase, &g_ZombiePlugin, &ZombiePlugin::Delete, true);
							g_HookedWeapons.Remove(x);
						}
						iDX = g_HookedWeapons.Find( pBase );
						if ( iDX == -1 )
						{
							int iPlayer = m_Engine->IndexOfEdict( pPlayer->edict() );
							g_ZombiePlayers[iPlayer].pKnife = pWeapon;
							
							SH_ADD_MANUALHOOK_MEMFUNC(GetMaxSpeed_hook, pBase, &g_ZombiePlugin, &ZombiePlugin::GetMaxSpeed, true);
							SH_ADD_MANUALHOOK_MEMFUNC(Delete_hook, pBase, &g_ZombiePlugin, &ZombiePlugin::Delete, true);
							g_HookedWeapons.AddToTail(pBase);
						}
					}
				}
				else
				{
					bDeleted = true;
					UTIL_WeaponDrop( (CBasePlayer*)pPlayer, pWeapon, &Vector( 100, 100, 100 ), &Vector( 100, 100, 100 ) );
					//UTIL_WeaponDelete( pWeapon );
				}
			}
		}
	}
	if ( bDeleted )
	{
		m_Engine->ClientCommand( pPlayer->edict(), "use weapon_knife; slot3\n" );
	}
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
 
	if ( !pEntity || pEntity->IsFree() )
	{
		recipient_filter.AddAllPlayers( g_SMAPI->pGlobals()->maxClients, m_Engine, m_PlayerInfoManager );
	}
	else
	{
		recipient_filter.AddPlayer( m_Engine->IndexOfEdict( pEntity ), m_Engine, m_PlayerInfoManager );
	}

    netmsg = m_Engine->UserMessageBegin ( &recipient_filter, 3 );
    netmsg->WriteByte ( 0 );
    netmsg->WriteString ( szText );
    netmsg->WriteByte ( 1 );
    m_Engine->MessageEnd ();
    return;
}

int ZombiePlugin::EntIdxOfUserIdx(int useridx)
{
	edict_t			*pEnt;
	IPlayerInfo		*pInfo;
	int				i = 1;
	for ( i = 1; i <= g_SMAPI->pGlobals()->maxClients; i++ )
	{
		pEnt = m_Engine->PEntityOfEntIndex(i);
		if ( pEnt && !pEnt->IsFree() )
		{
			if ( FStrEq( pEnt->GetClassName(), "player" ) )
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
	}
	return 0;
}

int ZombiePlugin::GetTeam(edict_t *pEntity)
{
	if ( !pEntity || pEntity->IsFree() || m_Engine->GetPlayerUserId( pEntity ) == -1 )
	{
		return 0;
	}
	int iTeam;
	if ( UTIL_GetProperty( g_Offsets.m_iTeamNum, pEntity, &iTeam ) )
	{
		if ( iTeam > 0 && iTeam < 4 )
		{
			return iTeam;
		}
	}
	return 0;
}

void RandomZombie(void **params)
{
	bZombieDone = true;
	if ( !zombie_mode.GetBool() )
	{
		return;
	}
	int aliveCount = 0;
	bool		aAlive[MAX_PLAYERS+1] = {false};
	edict_t		*aEdict[MAX_PLAYERS+1] = {NULL};
	for ( int x = 1; x <= g_SMAPI->pGlobals()->maxClients; x++ )
	{
		edict_t *pEdict = g_ZombiePlugin.m_Engine->PEntityOfEntIndex(x);
		aEdict[x] = pEdict;
		if ( pEdict && !pEdict->IsFree() && g_ZombiePlugin.m_Engine->GetPlayerUserId( pEdict ) != -1 )
		{
			IPlayerInfo *iInfo = g_ZombiePlugin.m_PlayerInfoManager->GetPlayerInfo( pEdict );
			aAlive[x] = false;
			if ( iInfo && !iInfo->IsDead() )
			{
				aAlive[x] = true;
				aliveCount++;
			}
			/*if ( UTIL_GetProperty( g_ZombiePlugin.g_Offsets.m_lifeState, pEdict, &IsAlive ) )
			{
				aAlive[x] = ( IsAlive ==  LIFE_ALIVE );
				if ( aAlive[x] )
					aliveCount++;
			}*/
		}
	}
	int num = RandomInt( 1, aliveCount );
	for ( int x = 1; x <= g_SMAPI->pGlobals()->maxClients; x++ )
	{
		edict_t *pEdict = aEdict[x];
		if ( aAlive[x] )
		{
			num--;
		}
		if ( num <= 0 )
		{
			CBasePlayer *pPlayer = ( CBasePlayer* )pEdict->GetUnknown()->GetBaseEntity();
			META_LOG( g_PLAPI, "Zombie: %d", x );
			g_ZombiePlugin.MakeZombie( pPlayer );
			//UTIL_SetPropertyInt( "CBasePlayer", "m_iHealth", pEdict, zombie_health.GetInt() * 2 );
			UTIL_SetProperty( g_ZombiePlugin.g_Offsets.m_iHealth, pEdict, (int)zombie_health.GetInt() * 2 );
			g_ZombiePlugin.g_ZombieRoundOver = false;
			return;
		}
	}
}

void ZombiePlugin::ShowZomb()
{
	int x = 1;
	for ( x = 1; x <= g_SMAPI->pGlobals()->maxClients; x++ )
	{
		edict_t *pEdict = g_ZombiePlugin.m_Engine->PEntityOfEntIndex( x );
		if ( pEdict && !pEdict->IsFree() && m_Engine->GetPlayerUserId( pEdict ) != -1 )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)GetContainingEntity( pEdict );
			META_LOG( g_PLAPI, "\n==================================" );
			META_LOG( g_PLAPI, "=== Player: %d", x );
			META_LOG( g_PLAPI, "=== %s", ( g_ZombiePlayers[x].isBot ? "BOT" : "Player" )  );
			META_LOG( g_PLAPI, "=== %s", ( g_ZombiePlayers[x].isHooked ? "Hooked" : "--> NOT HOOKED <--" ) );
			META_LOG( g_PLAPI, "=== Knife: %p", g_ZombiePlayers[x].pKnife );
			META_LOG( g_PLAPI, "=== Player: %p // %p", pPlayer, g_ZombiePlayers[x].pPlayer );
			META_LOG( g_PLAPI, "==================================\n" );
		}
	}
}

void CheckZombies( void **params )
{
	if ( !zombie_mode.GetBool() )
	{
		return;
	}

	if ( teamCT && teamT && bRoundStarted  )
	{
		int c = UTIL_GetNumPlayers( teamCT ) + UTIL_GetNumPlayers( teamT );
		if ( ( c > 1 ) && !g_ZombiePlugin.g_ZombieRoundOver )
		{
			int zombies = 0;
			int humans = 0;
			for ( int x = 1; x <= g_SMAPI->pGlobals()->maxClients; x++ )
			{
				edict_t *pEdict = g_ZombiePlugin.m_Engine->PEntityOfEntIndex( x );
				IPlayerInfo *info = g_ZombiePlugin.m_PlayerInfoManager->GetPlayerInfo( pEdict );
				if ( pEdict && pEdict->GetUnknown() && pEdict->GetUnknown()->GetBaseEntity() && ( info && !info->IsDead() ) )
				{
					CBasePlayer *pPlayer = (CBasePlayer *)GetContainingEntity( pEdict );
					if ( g_ZombiePlayers[x].isZombie )
					{
						if ( zombie_teams.GetBool() )
						{
							//int iTeam = UTIL_GetPropertyInt( "CBaseEntity", "m_iTeamNum", pEdict );
							int iTeam = g_ZombiePlugin.GetTeam( pEdict );
							if ( iTeam == COUNTERTERRORISTS )
							{
								//UTIL_SetPropertyInt( "CBaseEntity", "m_iTeamNum", pEdict, TERRORISTS );
								//UTIL_SetProperty( g_ZombiePlugin.g_Offsets.m_iTeamNum, pEdict, (int)TERRORISTS );
								CBasePlayer_SwitchTeam( pPlayer, TERRORISTS );
							}
						}
						//CBasePlayer *pPlayer = (CBasePlayer*)pEdict->GetUnknown()->GetBaseEntity();
						if (RandomInt(0, zombie_sound_rand.GetInt()) == 3)
						{
							char sSound[128];
							MRecipientFilter filter;
							filter.AddAllPlayers( g_SMAPI->pGlobals()->maxClients, g_ZombiePlugin.m_Engine, g_ZombiePlugin.m_PlayerInfoManager );
							Q_snprintf( sSound, sizeof(sSound), "npc/zombie/%s.wav", g_sZombieSounds[ RandomInt( 0, NUM_ZOMBIE_SOUNDS - 1) ] );
							g_ZombiePlugin.m_EngineSound->EmitSound(filter, x, CHAN_VOICE, sSound, RandomFloat( 0.1, 1.0 ), 0.5, 0, RandomInt( 70, 150 ) );
						}
						zombies++;
					}
					else
					{
						if ( zombie_teams.GetBool() )
						{
							int iTeam = g_ZombiePlugin.GetTeam( pEdict );
							if ( iTeam == TERRORISTS )
							{
								CBasePlayer_SwitchTeam( pPlayer, COUNTERTERRORISTS );
								//UTIL_SetProperty( g_ZombiePlugin.g_Offsets.m_iTeamNum, pEdict, (int)COUNTERTERRORISTS );
								//UTIL_SetPropertyInt( "CBaseEntity", "m_iTeamNum", pEdict, COUNTERTERRORISTS );
							}
						}
						humans++;
					}
				}
			}
			g_ZombiePlugin.iCountZombies = zombies;
			g_ZombiePlugin.iCountPlayers = humans;
			if ( humans == 0 )
			{
				g_ZombiePlugin.g_ZombieRoundOver = true;
				g_ZombiePlugin.Hud_Print( NULL, "[ZOMBIE] Zombies have taken over the world. The human race has been destroyed." );
				UTIL_TermRound( 3.0, Terrorists_Win );
			}
			else if ( zombies == 0 )
			{
				g_ZombiePlugin.Hud_Print( NULL, "[ZOMBIE] Humans have killed all the zombies.. for now.");
				g_ZombiePlugin.g_ZombieRoundOver = true;
				UTIL_TermRound( 3.0, CTs_PreventEscape );
			}
			iZombieCount = zombies;
			iHumanCount = humans;
		}
	}
	g_ZombiePlugin.g_Timers->AddTimer( 0.5, CheckZombies, NULL, 0, ZOMBIECHECK_TIMER_ID );
}

bool ZombiePlugin::LoadZombieModelList( const char *filename, const char *downloadfile )
{
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
		
		GET_CFG_ENDPARAM(File, MAX_SERVERCMD_LENGTH)

		FixSlashes(File);
		const char *sExts[] = { ".dx80.vtx", ".dx90.vtx", ".phy", ".sw.vtx", ".vvd", ".mdl" };
		for (int y = 0; y <= 5; y++ )
		{
			char buf[128];
			Q_snprintf( buf, sizeof(buf), "%s%s", File, sExts[y] );
			if ( !m_FileSystem->FileExists( buf ) )
			{
				META_LOG( g_PLAPI, "ERROR: Zombie model file %s does not exist !", File);
			}
			else
			{
				AddDownload( buf );
				if ( y == 5 )
				{
					META_LOG( g_PLAPI, "Adding Model:%s", buf );
					m_Engine->PrecacheModel( buf );
					int id = g_ZombieModels.AddToTail();
					g_ZombieModels[id].assign( buf );
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
		
		GET_CFG_ENDPARAM( File, MAX_SERVERCMD_LENGTH )

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

void ZombiePlugin::ZombieOff() 
{
	if ( !zombie_mode.GetBool() )
	{
		return;
	}
	bUsingCVAR = true;
	zombie_mode.SetValue(0);
	bUsingCVAR = false;
	m_Engine->LightStyle(0, "m");
	mp_spawnprotectiontime->SetValue( g_oldSpawnProtectionTime );
	mp_tkpunish->SetValue( g_oldTKPunish );
	sv_alltalk->SetValue( g_oldAlltalkValue );
	ammo_buckshot_max->SetValue( g_oldAmmoBuckshotValue );
	
	for (int x = 0; x < CSW_MAX; x++)
	{
		g_RestrictT[x] = -1;
		g_RestrictCT[x] = -1;
	}
	FFA_Disable();
    CPlayerMatchList matches("#a");
	for ( int x = 0; x < matches.Count(); x++ )
	{
		m_Engine->ClientCommand( matches.GetMatch(x), "r_radiosity 4" );
	}
	if ( zombie_dark.GetBool() )
	{
		m_Engine->ChangeLevel( g_CurrentMap, NULL );
	}
}

void ZombiePlugin::ZombieOn()
{
	if ( zombie_mode.GetBool() )
	{
		return;
	}
	
	g_oldFFValue = mp_friendlyfire->GetInt();
	g_oldSpawnProtectionTime = mp_spawnprotectiontime->GetInt();
	g_oldTKPunish = mp_tkpunish->GetInt();
	g_oldAlltalkValue = sv_alltalk->GetInt();
	g_oldAmmoBuckshotValue = ammo_buckshot_max->GetInt();
	
	bUsingCVAR = true;
	zombie_mode.SetValue(1);
	bUsingCVAR = false;
	
	for ( int x = 0; x <= MAX_PLAYERS; x++ )
	{
		g_ZombiePlayers[x].isZombie = false;
	}
	if ( zombie_dark.GetBool() )
	{
	    //*g_pGameOver = true;
	    m_Engine->ChangeLevel( g_CurrentMap, NULL );
	}
	else
	{
	    g_ZombieRoundOver = true;
	    ZombieLevelInit( NULL );
	    g_Timers->AddTimer( 0.5, CheckZombies, NULL, 0, ZOMBIECHECK_TIMER_ID );
	}
}
void ZombieLevelInit( void **params )
{
	if ( zombie_dark.GetBool() )
	{
		g_ZombiePlugin.m_Engine->LightStyle( 0, "a" );
	}
	FFA_Enable();
	//mp_friendlyfire->SetValue(0);
	g_ZombiePlugin.m_Engine->ServerCommand( "mp_flashlight 1\n" );
	g_ZombiePlugin.m_Engine->ServerCommand( "mp_limitteams 0\n" );
	mp_spawnprotectiontime->SetValue(0);
	mp_tkpunish->SetValue(0);
	sv_alltalk->SetValue(1);
	ammo_buckshot_max->SetValue(128);
	//mp_friendlyfire->SetValue(1);
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
		char tmp[20];
		int x = 0;
		const char *str = zombie_restrictions.GetString();
		int len = strlen(str);
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
				char buf[128];
				Q_snprintf( buf, sizeof(buf), "zombie_restrict a %s\n", tmp );
				g_ZombiePlugin.m_Engine->ServerCommand( buf );
			}
			x++;
		}
	}
	return;
}

bool ZombiePlugin::CanUseWeapon( CBaseEntity* pPlayer, CBaseEntity *pEnt, bool bDrop )
{
	if ( !pEnt || !pEnt->edict() || pEnt->edict()->IsFree() || !pPlayer || !pPlayer->edict() || pPlayer->edict()->IsFree() || !pEnt->edict()->GetClassName() )
	{
		return true;
	}
	const char *szWeapon = pEnt->edict()->GetClassName();
	if ( Q_strncmp( szWeapon, "weapon_", 7 ) )
	{
		return true;
	}
	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pEnt;
	if ( pWeapon && pWeapon->edict() )
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
	if ( m_Engine->GetPlayerUserId( pPlayer->edict() ) == -1 || !pPlayer->edict() || pPlayer->edict()->IsFree() || WeaponID == CSW_NONE )
	{
		return true;
	}
	if ( zombie_mode.GetBool() )
	{
		if ( WeaponID == CSW_C4 )
		{
			return false;
		}
		int iPlayer = m_Engine->IndexOfEdict( pPlayer->edict() );
		if ( iPlayer < 1 || iPlayer >= MAX_PLAYERS )
		{
			return false;
		}
		if ( g_ZombiePlayers[iPlayer].isZombie  )
		{
			if ( WeaponID != CSW_KNIFE )
			{
				return false;
			}
		}
		else if ( WeaponID == CSW_NVGS )
		{
			return false;
		}
	}
	int teamid = GetTeam( pPlayer->edict() );
	if ( (teamid != TERRORISTS) && (teamid != COUNTERTERRORISTS))
	{
		return true;
	}
 	if (teamid == TERRORISTS)
	{
		if ( g_RestrictT[WeaponID] == 0 )
		{
			return false;
		}
	}
	else
	{
		if (g_RestrictCT[WeaponID] == 0)
		{
			return false;
		}
	} 
	return true;
}

void ZombiePlugin::CheckAutobuy( int nIndex, CBasePlayer* pPlayer )
{
	if (nIndex < 1 || nIndex > g_SMAPI->pGlobals()->maxClients )
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
void *MyListener::OnMetamodQuery(const char *iface, int *ret)
{
	return NULL;
}

CBaseCombatWeapon* UTIL_WeaponSlot(CBaseCombatCharacter *pCombat, int slot)
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

}

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

void UTIL_WeaponDelete(CBaseCombatWeapon *pWeapon)
{
	#ifdef WIN32
		__asm {
			push ecx;
			mov ecx, pWeapon;
			call g_WeaponDeleteFunc;
			pop ecx;
		};
	#else
		(g_WeaponDeleteFunc)(pWeapon);
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

void CBasePlayer_SwitchTeam( CBasePlayer *pPlayer, int iTeam )
{
	if ( !IsBadPtr( mp_limitteams ) && mp_limitteams->GetInt() != 0 )
	{
		mp_limitteams->SetValue( 0 );
	}
	if ( !IsBadPtr( mp_autoteambalance ) && mp_autoteambalance->GetInt() != 0 )
	{
		mp_autoteambalance->SetValue( 0 );
	}
	if ( IsBadPtr(pPlayer) )
		return;
	CTeam *pTeam = ( iTeam == COUNTERTERRORISTS ? teamT : teamCT );
	if ( pTeam && UTIL_GetNumPlayers( pTeam ) <= 1 )
	{
		return;
	}
	//CBaseEntity_ChangeTeam ( pPlayer, iTeam );
	//return;

	void* funcptr = v_SwitchTeam;
	void* thisptr = pPlayer;
	u_SwitchTeam.addr = funcptr;
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
	if ( IsBadPtr( pPlayer ) )
		return false;
	void* funcptr = v_SetFOV;
	void* thisptr = pPlayer;
	u_SetFOV.addr = funcptr;
	return (bool)(reinterpret_cast<vEmptyClass*>(thisptr)->*u_SetFOV.mfpnew)( pRequester, FOV, zoomRate );
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
	if ( !pPlayer )
		return NULL;
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

void UTIL_SetModel( CBaseFlex *pBaseFlex, const char *model )
{
	#ifdef WIN32
		__asm
		{
			push ecx;
			mov ecx, pBaseFlex;
			push model;
			call g_SetModelFunc;
			pop ecx;
		};
	#else
		( g_SetModelFunc )( pBaseFlex, model );
	#endif
}

int GetUnknownInt(edict_t *pEdict, int offset)
{
	if (offset < 1)
		return 0;

	return *((int*)pEdict->GetUnknown() + offset);
}

int LookupBuyID(const char *name) {
	if (Q_stristr(name, "228")) {
		return CSW_P228;
	} else if (Q_stristr(name, "glock") || Q_stristr(name, "9x19")) {
		return CSW_GLOCK;
	} else if (Q_stristr(name, "scout")) {
		return CSW_SCOUT;
	} else if (Q_stristr(name, "hegren") || Q_stristr(name, "nade")) {
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
	if (Q_stristr(name, "SmokeGrenade")) {
		return CSW_SMOKEGRENADE;
	} else if (Q_stristr(name, "HEGrenade")) {
		return CSW_HEGRENADE;
	} else if (Q_stristr(name, "Flashbang")) {
		return CSW_FLASHBANG;
	} else if (Q_stristr(name, "PrimaryAmmo")) {
		return CSW_PRIMAMMO;
	} else if (Q_stristr(name, "SecondaryAmmo")) {
		return CSW_SECAMMO;
	} else if (Q_stristr(name, "NightVision")) {
		return CSW_NVGS;
	} else if (Q_stristr(name, "Armor")) {
		return CSW_VEST;
	} else if (Q_stristr(name, "Defuser")) {
		return CSW_DEFUSEKIT; 
	}
	return CSW_NONE;
}

const char *LookupWeaponName(int id) {
	switch (id) {
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
float DamageForce(const Vector &size, float damage)
{
	float force = damage * ((32 * 32 * 72.0) / (size.x * size.y * size.z)) * zombie_knockback.GetFloat();
	if (force > 1000.0)
	{
			force = 1000.0;
	}
	return force;
}

/*void FindGameRules(char *addr)
{
	memcpy(&g_pGameRules, (addr + CPlayer_g_pGameRules), sizeof(char*));
	int *ptr = *( (int **)(g_pGameRules) );
	g_pGameRules = (CGameRules *)ptr;
} */
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

void UTIL_MemProtect(void *addr, int length, int prot)
{
#ifdef __linux__
        void *addr2 = (void *)ALIGN(addr);
        int ret = mprotect(addr2, sysconf(_SC_PAGESIZE), prot);
#else
        DWORD old_prot;
    VirtualProtect(addr, length, prot, &old_prot);
#endif
}

void FFA_Disable()
{
        if (!g_ffa)
                return;

        char *hack = (char *)g_TakeDamage + OnTakeDamage_Offset;
        UTIL_MemProtect(hack, 20, PAGE_EXECUTE_READWRITE);
        memcpy(hack, g_hackbuf, OnTakeDamage_OffsetBytes);

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
        char *hack = (char *)g_TakeDamage + OnTakeDamage_Offset;
        UTIL_MemProtect(hack, 20, PAGE_EXECUTE_READWRITE);
        char _hackbuf[] =
#if defined __linux__
        {0x90, 0x90, 0x90, 0x90, 0x90, 0x90}
#else
        {0xEB}
#endif
        ;
        memcpy(g_hackbuf, hack, OnTakeDamage_OffsetBytes);
        memcpy(hack, _hackbuf, OnTakeDamage_OffsetBytes);

        //now patch lag compensation
        hack = (char *)g_WantsLag + WantsLagComp_Offset;
        char _lagbuf[] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90};

        UTIL_MemProtect(hack, 20, PAGE_EXECUTE_READWRITE);
        memcpy(g_lagbuf, hack, WantsLagComp_OffsetBytes);
        memcpy(hack, _lagbuf, WantsLagComp_OffsetBytes);
		META_CONPRINTF( "[ZOMBIE] Free For All has been enabled.\n" );
        g_ffa = true;
}

void CheckRespawn( void **params ) 
{
	if (!zombie_respawn.GetBool())
		return;
	edict_t *pEntity = (edict_t*)params[0];
	if ( !pEntity || pEntity->IsFree() || !pEntity->GetUnknown() || !pEntity->GetUnknown()->GetBaseEntity() || g_ZombiePlugin.m_Engine->GetPlayerUserId( pEntity ) == -1 )
		return;
	IPlayerInfo *info = g_ZombiePlugin.m_PlayerInfoManager->GetPlayerInfo( pEntity );
	if ( !info )
		return;
	CBasePlayer* pPlayer = (CBasePlayer*)pEntity->GetUnknown()->GetBaseEntity();
	if ( !pPlayer )
		return;
	int team = g_ZombiePlugin.GetTeam( pEntity );
	//int team = GetTeam(pPlayer);
	if ( team == COUNTERTERRORISTS || team == TERRORISTS )
	{
		//CCSPlayer_RoundRespawn_(pPlayer);
		UTIL_RestartRound( pPlayer );
	}
}