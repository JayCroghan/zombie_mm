#include "Events.h"

#undef CreateEvent
#undef GetClassName

#define IMPLEMENT_EVENT(name) \
	cls_event_##name g_cls_event_##name; \
	void cls_event_##name::FireGameEvent( IGameEvent *pEvent )

#define HOOK_EVENT2(name) \
	if ( !m_GameEventManager->FindListener( &g_cls_event_##name, #name ) ) \
	{ \
		if (!m_GameEventManager->AddListener(&g_cls_event_##name, #name, true)) \
		{ \
			g_SMAPI->Format(error, maxlength, "Could not hook event \"%s\"", #name); \
			META_LOG( g_PLAPI, error ); \
		} \
	}

#define UNHOOK_EVENT2(name) m_GameEventManager->RemoveListener(&g_cls_event_##name);

#define GETCLASS( iClass ) \
	ZombieClass *zClass; \
	zClass = m_ClassManager.GetClass( iClass );


#include "ZombiePlugin.h"
#include "cvars.h"
#include "VFunc.h"
#include "CTakeDamageInfo.h"
#include "mathlib.h"
#include "ZMClass.h"

#undef CreateEvent
#undef GetClassName

extern CUtlVector<TimerInfo_t>	g_RespawnTimers;
extern PlayerInfo_t				g_Players[65];
extern BuyMenuInfo_t			g_tBuyMenu;
extern bool						g_bBuyMenu;
extern unsigned int				g_RoundTimer;
extern int						iNextHealth;
extern bool						bRoundTerminating;
extern int						iZombieCount;
extern int						iHealthDecrease;
extern bool						bFirstEverRound;
extern bool						bLoadedLate;
extern ConVar					*mp_roundtime;
extern bool						g_bZombieClasses;
//extern int						g_iZombieClasses;
//extern ZombieClasses_t			g_ZombieClasses[100];
extern CBaseEntity				*pJumper[MAX_PLAYERS];
extern int						iJumpers;
extern int						iJumper[MAX_PLAYERS];
extern KeyValues				*kvPlayerClasses;
extern IPlayerInfoManager		*m_PlayerInfoManager;
extern int						g_iDefaultClass;
extern IServerPluginHelpers		*m_Helpers;
extern myString					sRestrictedWeapons;
extern bool						bKickStart[MAX_PLAYERS];
extern unsigned int				iKickTimer;
extern ConVar					*mp_buytime;

IMPLEMENT_EVENT( zombification )
{
	return;
}

IMPLEMENT_EVENT( player_team )
{
	return;
}

IMPLEMENT_EVENT( round_freeze_end )
{
	if ( !pEvent || !pEvent->GetName() )
	{
		return;
	}
	g_ZombiePlugin.Event_Round_Freeze_End();
}

IMPLEMENT_EVENT( player_say )
{
	if ( !pEvent || !pEvent->GetName() )
	{
		return;
	}
	int iPlayer = g_ZombiePlugin.EntIdxOfUserIdx( pEvent->GetInt( "userid" ) );
	const char *sText = pEvent->GetString( "text" );
	META_RES mRes = g_ZombiePlugin.Event_Player_Say( iPlayer, sText );
}

IMPLEMENT_EVENT( player_spawn )
{
	if ( !pEvent || !pEvent->GetName() )
	{
		return;
	}
	int iPlayer = g_ZombiePlugin.EntIdxOfUserIdx( pEvent->GetInt( "userid" ) );
	g_ZombiePlugin.Event_Player_Spawn( iPlayer );
}

IMPLEMENT_EVENT( player_death )
{
	if ( !pEvent || !pEvent->GetName() )
	{
		return;
	}
	int iVic = g_ZombiePlugin.EntIdxOfUserIdx( pEvent->GetInt( "userid" ) );
	int iAtt = g_ZombiePlugin.EntIdxOfUserIdx( pEvent->GetInt( "attacker" ) );
	const char *sWeapon = pEvent->GetString( "weapon" );
	g_ZombiePlugin.Event_Player_Death( iVic, iAtt, sWeapon );
}

IMPLEMENT_EVENT( player_hurt )
{
	if ( !pEvent || !pEvent->GetName() )
	{
		return;
	}
	const char *sWeapon = pEvent->GetString( "weapon" );
	int iPlayer = g_ZombiePlugin.EntIdxOfUserIdx( pEvent->GetInt("userid") );

	g_ZombiePlugin.Event_Player_Hurt( iPlayer, sWeapon );
}

IMPLEMENT_EVENT( weapon_fire )
{
	if ( !pEvent || !pEvent->GetName() )
	{
		return;
	}
	const char *sWeapon = pEvent->GetString( "weapon" );
	int iPlayer = g_ZombiePlugin.EntIdxOfUserIdx( pEvent->GetInt("userid") );
	g_ZombiePlugin.Event_Weapon_Fire( iPlayer, sWeapon );
	return;
}

IMPLEMENT_EVENT( round_end )
{
	if ( !pEvent || !pEvent->GetName() )
	{
		return;
	}
	int iWinner = pEvent->GetInt( "winner" );
	g_ZombiePlugin.Event_Round_End( iWinner );
}

IMPLEMENT_EVENT( hostage_follows )
{
	if ( !pEvent || !pEvent->GetName() )
	{
		return;
	}
	int iHosti = pEvent->GetInt( "hostage" );
	g_ZombiePlugin.Event_Hostage_Follows( iHosti );
}

#undef CreateEvent
#undef GetClassName

IMPLEMENT_EVENT( round_start )
{
	if ( !pEvent || !pEvent->GetName() )
	{
		return;
	}
	g_ZombiePlugin.Event_Round_Start( );
}

IMPLEMENT_EVENT( player_jump )
{
	if ( !pEvent || !pEvent->GetName() )
	{
		return;
	}
	int iPlayer = g_ZombiePlugin.EntIdxOfUserIdx( pEvent->GetInt( "userid" ) );
	g_ZombiePlugin.Event_Player_Jump( iPlayer );
}


/*******************************************************************
						End Declarations
*******************************************************************/

void ZombiePlugin::Event_Player_Jump( int iPlayer )
{
	CBaseEntity *cEntity;
	if ( IsValidPlayer( iPlayer, &cEntity ) )
	{
		GETCLASS( g_Players[iPlayer].iClass );
		if ( zClass )
		{
			if ( g_Players[iPlayer].isZombie && zClass->iJumpHeight != 0 )
			{
				int iJumpHeight = ( zClass->iJumpHeight / 100 ) * zombie_jump_height_percent.GetInt();
				iJumpers = iJumpers + 1;
				pJumper[iJumpers] = cEntity;
				iJumper[iJumpers] = iJumpHeight;
			}
		}
	}
}

void ZombiePlugin::Event_Round_Start( )
{
	iJumpers = -1;
	zombie_first_zombie.SetValue( 0 );
	iNextHealth = 0;
	iZombieCount = 0;
	iHealthDecrease = 0;
	int x = 0;
	if ( bFirstEverRound && !bLoadedLate )
	{
		if ( zombie_startup.GetBool() )
		{
			m_Engine->ServerCommand( "zombie_mode 1\n" );
		}
		bFirstEverRound = false;
		return;
	}
	if ( zombie_enabled.GetBool() )
	{
		RemoveAllSpawnTimers();
		bRoundTerminating = false;
		bRoundStarted = false;
		int iHumanCount = 0;
		bGameOn = true;

		for( x = 1; x < m_Engine->GetEntityCount(); x++ )
		{
			edict_t *pEnt = m_Engine->PEntityOfEntIndex( x );
			if ( IsValidEnt( pEnt ) )
			{
				char sRemoveEnts[512];
				Q_strncpy( sRemoveEnts, ",", sizeof( sRemoveEnts ) );
				Q_strncat( sRemoveEnts, zombie_remove_objectives.GetString(), sizeof( sRemoveEnts ) );
				Q_strncat( sRemoveEnts, ",", sizeof( sRemoveEnts ) );

				CBaseEntity *pBase = NULL;
				if ( IsValidPlayer( x, NULL, &pBase ) && !g_Players[x].isBot )
				{
					g_Players[x].pPlayer = (CBasePlayer *)pBase;
					g_Players[x].bProtected = false;
					g_Players[x].bAsZombie = false;
					SetProtection( x, false, false, false );
					ZombieVision( pEnt, false );
					iHumanCount++;
					int iTeam = GetTeam( pEnt );
					if ( IsAlive( pEnt ) )
					{
						CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_Weapon_GetSlot( (CBaseCombatCharacter*) g_Players[x].pPlayer, SLOT_SECONDARY );
						if ( !pWeapon )
						{
							//GiveNamedItem_Test( g_Players[x].pPlayer, ( iTeam == TERRORISTS ?  "weapon_glock" : "weapon_usp" ) , 0 );
							UTIL_GiveNamedItem( g_Players[x].pPlayer, ( iTeam == TERRORISTS ?  "weapon_glock" : "weapon_usp" ) , 0 );
						}
						if ( !g_Players[x].bSentWelcome )
						{
							g_Players[x].bSentWelcome = true;
							if ( zombie_welcome_delay.GetInt() > 0 )
							{
								void *params[1];
								params[0] = (void *)pEnt;
								g_Timers->AddTimer( zombie_welcome_delay.GetFloat(), Timed_ConsGreet, params, 1 );
							}
						}
					}
				}
				else if ( Q_strlen( sRemoveEnts ) > 2 ) //, "hostage_entity" ) )
				{
					const char *sClass = pEnt->GetClassName();
					if ( iPlayerCount != 0 && ( sClass && Q_stristr( sRemoveEnts, sClass ) ) )
					{
						//CBaseEntity *pBase = m_GameEnts->EdictToBaseEntity( pEnt );
						//if ( pBase )
						//{
							//CCSPlayer_Event_Killed( (CCSPlayer*)pBase, CTakeDamageInfo(m_GameEnts->EdictToBaseEntity(m_Engine->PEntityOfEntIndex(0)), m_GameEnts->EdictToBaseEntity(m_Engine->PEntityOfEntIndex(0)), 0, DMG_NEVERGIB) );
							//CBasePlayer_Event_Dying( (CBasePlayer *)pBase );
							//g_ZombiePlugin.PerformSuicide( (CBasePlayer *)pBase, false, false );
							m_Engine->RemoveEdict( pEnt );
						//}
					}
				}
			}
		}
		if ( mp_roundtime )
		{
			g_Timers->RemoveTimer( g_RoundTimer );
			float fTime = (mp_roundtime->GetFloat() * 60.0f) + 10;
			g_RoundTimer = g_Timers->AddTimer( fTime, Timed_RoundTimer );
		}
	}
}

void ZombiePlugin::RemoveObjectives()
{
}

void ZombiePlugin::Event_Hostage_Follows( int iHosti )
{
	if ( iHosti > 0 )
	{
		edict_t *pEnt = m_Engine->PEntityOfEntIndex( iHosti );
		if ( IsValidEnt( pEnt ) )
		{
			m_Engine->RemoveEdict( pEnt );
			/*
			CBaseEntity *pBase = m_GameEnts->EdictToBaseEntity( pEnt );
			if ( pBase )
			{
				//CCSPlayer_Event_Killed( (CCSPlayer*)pBase, CTakeDamageInfo(m_GameEnts->EdictToBaseEntity(m_Engine->PEntityOfEntIndex(0)), m_GameEnts->EdictToBaseEntity(m_Engine->PEntityOfEntIndex(0)), 0, DMG_NEVERGIB) );
				//CBasePlayer_Event_Dying( (CBasePlayer *)pBase );
				g_ZombiePlugin.PerformSuicide( (CBasePlayer *)pBase, false, false );
			}
			*/
		}
	}
}

void ZombiePlugin::Event_Round_End( int iWinner )
{
	
	zombie_first_zombie.SetValue( 0 );
	iNextHealth = 0;
	g_Timers->RemoveTimer( g_RoundTimer );
	bAllowedToJetPack = false;
	if ( zombie_enabled.GetBool() )
	{
		int x = 0;
		bRoundStarted = false;
		bRoundTerminating = true;
		
		if ( iWinner == 3 )
		{
			RoundEndOverlay( true, 0 );
		}
		else
		{
			RoundEndOverlay( true, 1 );
		}
		if ( zombie_teams.GetBool() )
		{
			bool bCT = false;
			for ( x = 0; x <= MAX_CLIENTS; x++ )
			{
				edict_t *pEnt = m_Engine->PEntityOfEntIndex( x );
				if ( IsValidPlayer( x, &pEnt ) && g_Players[x].bConnected )
				{
					if ( g_Players[x].iRegenTimer != 0 )
					{
						g_Timers->RemoveTimer( g_Players[x].iRegenTimer );
						g_Players[x].iRegenTimer = 0;
					}
					if ( g_Players[x].bProtected )
					{
							g_Players[x].bProtected = false;
					}
					GETCLASS( g_Players[x].iChangeToClass );
					if ( zClass  )
					{
						g_Players[x].iClass = zClass->iClassId;
						g_Players[x].iChangeToClass = -1;
					}
					if ( zombie_damagelist.GetBool() )
					{
						ShowDamageMenu( pEnt );
					}
					
					RemoveAllSpawnTimers();

					g_Players[x].bAsZombie = false;
					SetProtection( x, false, false, true );
					if ( g_Players[x].bJetPack )
						m_Engine->ClientCommand( pEnt, "-jetpack\n" );
					int iTeam = GetTeam( pEnt );
					if ( iTeam == COUNTERTERRORISTS || iTeam == TERRORISTS )
					{
						bCT = !bCT;
						CBasePlayer_SwitchTeam( g_Players[x].pPlayer, ( bCT ? COUNTERTERRORISTS : TERRORISTS ) );
					}
				}
			}
		}
	}
}

#undef CreateEvent
#undef GetClassName

void ZombiePlugin::Event_Player_Death( int iVic, int iAtt, const char *sWeapon )
{
	if ( iVic == -1 )
	{
		return;
	}
	if ( zombie_notices.GetBool() )
	{
		if ( sWeapon && FStrEq( sWeapon, zombie_weapon.GetString() ) )
		{
			return;
		}
	}

	edict_t *pPlayer = m_Engine->PEntityOfEntIndex ( iVic );
	edict_t *pAttacker = m_Engine->PEntityOfEntIndex ( iAtt );
	if ( zombie_enabled.GetBool() )
	{
		if ( (iAtt > -1) && (iVic != iAtt) && pAttacker && !pAttacker->IsFree() && pPlayer && !pPlayer->IsFree() )
		{
			int iAttTeam = GetTeam( pAttacker );
			int iVicTeam = GetTeam( pPlayer );
			if ( iAttTeam == iVicTeam )
			{
				GiveMoney( pAttacker, zombie_tk_money.GetInt() );

				int iFrags = 0;
				if ( UTIL_GetProperty( g_Offsets.m_iFrags, pAttacker, &iFrags ) )
				{
					iFrags += ( 2 + zombie_kill_bonus.GetInt() );
					UTIL_SetProperty( g_Offsets.m_iFrags, pAttacker, iFrags );
				}
			}
			else
			{
				if ( zombie_kill_money.GetInt() > 0 )
				{
					GiveMoney( pAttacker, zombie_kill_money.GetInt() );
				}
				if ( zombie_kill_bonus.GetInt() > 0 )
				{
					int iFrags = 0;
					if ( UTIL_GetProperty( g_Offsets.m_iFrags, pAttacker, &iFrags ) )
					{
						iFrags += zombie_kill_bonus.GetInt();
						UTIL_SetProperty( g_Offsets.m_iFrags, pAttacker, iFrags );
					}
				}
			}
			if ( zombie_dissolve_corpse.GetBool() )
			{
				int iDelay = zombie_dissolve_corpse_delay.GetInt();
				if ( iDelay == 0 )
				{
					Dissolve( pPlayer );
				}
				else
				{
					void *params[1];
					params[0] = pPlayer;
					g_Timers->AddTimer( zombie_dissolve_corpse_delay.GetFloat(), Timed_Dissolve, params, 1 );
				}
			}
		}
		//if ( g_bZombieClasses && (zombie_classes_random.GetInt() == 1 || (zombie_classes_random.GetInt() == 3 && !g_Players[iVic].bChoseClass)))
		if ( m_ClassManager.Enabled() && (zombie_classes_random.GetInt() == 1 || (zombie_classes_random.GetInt() == 3 && !g_Players[iVic].bChoseClass)))
		{
			//int iChangeTo = 0;
			//iChangeTo = RandomInt( 0, g_iZombieClasses );
			ZombieClass *zClass = m_ClassManager.RandomClass();
			SelectZombieClass( pPlayer, iVic, zClass, true );
		}
		else if ( g_Players[iVic].iChangeToClass > -1 ) //&& g_Players[iVic].iChangeToClass <= g_iZombieClasses )
		{
			GETCLASS( g_Players[iVic].iChangeToClass );
			g_Players[iVic].iClass = zClass->iClassId;//g_Players[iVic].iChangeToClass;
			g_Players[iVic].iChangeToClass = -1;
		}
		if ( g_Players[iVic].isZombie )
		{
			HudMessage( iVic, 66, 0.03, 0.89, 188, 112, 0, 128, 188, 112, 0, 128, 0, 0.0, 0.0, 100.0, 0.0, "" );
			if ( g_Players[iVic].bFaded )
			{
				DoFade( g_Players[iVic].pPlayer, false, iVic );
				ZombieVision( pPlayer, false );
			}
			g_Players[iVic].isZombie = false;

			SetFOV( pPlayer, 90 );
			MRecipientFilter filter;
			filter.AddAllPlayers( MAX_CLIENTS );
			char sound[512];
			Q_snprintf( sound, sizeof(sound), "npc/zombie/zombie_die%d.wav", RandomInt( 1, 3 ) );
			m_EngineSound->EmitSound( filter, iVic, CHAN_VOICE, sound, RandomFloat( 0.6, 1.0 ), 0.5, 0, RandomInt( 70, 150 ) );
		}
		if ( g_Players[iVic].iProtectTimer != 99 )
		{
			g_Timers->RemoveTimer( g_Players[iVic].iProtectTimer );
			g_Players[iVic].iProtectTimer = 99;
		}
	}
	if ( zombie_respawn.GetBool() && pPlayer ) 
	{
		int iCvarVal = zombie_startmoney.GetInt();
		if ( iCvarVal != 0 )
		{
			UTIL_SetProperty( g_Offsets.m_iAccount, pPlayer, ( iCvarVal == 1 ? 16000 : iCvarVal ) );
		}
		void *params[1];
		params[0] = pPlayer;
		if ( zombie_respawn_delay.GetFloat() != 0.0f )
		{
			int iId = g_RespawnTimers.AddToTail( );
			g_RespawnTimers[iId].iTimer = g_Timers->AddTimer( zombie_respawn_delay.GetFloat(), CheckRespawn, params, 1 );
			g_RespawnTimers[iId].iPlayer = iVic;
		}
		else
		{
			CheckRespawn( params );
		}
	}
}

void ZombiePlugin::Event_Player_Hurt( int iUser, const char *sWeapon )
{
	if ( !zombie_enabled.GetBool() || !zombie_napalm_enabled.GetBool() || iUser <= 0 || iUser > MAX_PLAYERS || !FStrEq( sWeapon, "hegrenade" ) )
	{
		return;
	}
	
	edict_t *pEntity = m_Engine->PEntityOfEntIndex ( iUser );
	if ( pEntity ) 
	{
		CBaseEntity *pPlayer = m_GameEnts->EdictToBaseEntity( pEntity );
		if ( pPlayer )
		{
			CBaseAnimating_Ignite( (CBaseAnimating *) pPlayer, zombie_napalm_time.GetFloat(), false, 0.0f, true );
		}
	}
}

void ZombiePlugin::Event_Weapon_Fire( int iUser, const char *sWeapon )
{
	if ( !zombie_enabled.GetBool() || !zombie_napalm_enabled.GetBool() || iUser < 0 || iUser > MAX_PLAYERS )
	{
		return;
	}

	if ( sWeapon && FStrEq( sWeapon, "hegrenade" ) )
	{
		edict_t *pEntity = m_Engine->PEntityOfEntIndex ( iUser );
		CBaseEntity *pPlayer = m_GameEnts->EdictToBaseEntity( pEntity );
		void *params[1];
		params[0] = pPlayer;
		g_Timers->AddTimer( 0.1, IgniteGrenade, params, 1 );
	}
}

void IgniteGrenade( void **params ) 
{
	CBaseEntity *pPlayer = (CBaseEntity *)params[0];
	int iPlayer = 0;
	
	if ( !IsValidPlayer( pPlayer, &iPlayer ) )
		return;

	CBaseEntity *pGrenade = UTIL_FindEntityByClassName( (CGlobalEntityList *)g_EntList, pPlayer, "CBaseCSGrenadeProjectile" );
	if ( pGrenade )
	{
		CBaseAnimating_Ignite( (CBaseAnimating *)pGrenade, 60.0, false, 0.0, true );
		edict_t *pEntity = m_GameEnts->BaseEntityToEdict( pGrenade );
		int iGrenade = m_Engine->IndexOfEdict( pEntity );
		
		float fDamage = 0.0f;
		float fDmgRadius = 0.0f;
		
		GetEntProp( iGrenade, Prop_Data, "m_flDamage", &fDamage );
		GetEntProp( iGrenade, Prop_Data, "m_DmgRadius", &fDmgRadius );


		if ( zombie_napalm_damage.GetInt() > 0 && ( SetEntProp( iGrenade, Prop_Data, "m_flDamage", zombie_napalm_damage.GetFloat() ) != 0 ) )
		{
			META_LOG( g_PLAPI, "Could not set m_flDamage for Grenade[%d] for Client[%d].", iGrenade, iPlayer );
		}
		if ( zombie_napalm_radius.GetInt() > 0 && ( SetEntProp( iGrenade, Prop_Data, "m_DmgRadius", zombie_napalm_radius.GetFloat() ) != 0 ) )
		{
			META_LOG( g_PLAPI, "Could not set m_DmgRadius for Grenade[%d] for Client[%d].", iGrenade, iPlayer );
		}
	}
}

void ZombiePlugin::Event_Round_Freeze_End( )
{
	bAllowedToJetPack = false;
	bZombieDone = false;
	bRoundStarted = true;
	zombie_first_zombie.SetValue( 0 );
	Hud_Print( NULL, "\x03[ZOMBIE]\x01 %s \x04%s\x01.", GetLang("zombie_enabled"), GetLang("humans_vs_zombies") ); //ZombieMod is enabled, run for your lives, the game is
	//HUMANS vs. ZOMBIES
	g_ZombieRoundOver = true;
	g_Timers->RemoveTimer( RANDOM_ZOMBIE_TIMER_ID );
	float fRand = RandomFloat(zombie_timer_min.GetFloat(), zombie_timer_max.GetFloat());
	META_LOG( g_PLAPI, "Random zombie in %f.", fRand );
	g_Timers->AddTimer( fRand, RandomZombie, NULL, 0, RANDOM_ZOMBIE_TIMER_ID );
	
	iNextSpawn = 0;
	for ( int x = 0; x <= MAX_PLAYERS; x++ )
	{
		edict_t *pEnt = NULL;
		if ( IsValidPlayer( x, &pEnt ) && IsAlive( pEnt ) )
		{
			CBaseEntity *pBase = m_GameEnts->EdictToBaseEntity( pEnt );
			ICollideable *cCol = CBaseEntity_GetCollideable( pBase );
			Vector vLoc = vec3_origin;
			QAngle vAng = vec3_angle;
			if ( cCol )
			{
				vLoc = cCol->GetCollisionOrigin();
				vAng = cCol->GetCollisionAngles();
			}

			vSpawnVec[iNextSpawn] = vLoc; //pEnt->GetCollideable()->GetCollisionOrigin();
			++iNextSpawn;
			g_Players[x].vSpawn = vLoc; //pEnt->GetCollideable()->GetCollisionOrigin();
			g_Players[x].qSpawn = vAng; //pEnt->GetCollideable()->GetCollisionAngles();
		}
	}
	if ( iNextSpawn != 0 )
	{
		iUseSpawn = iUseSpawn % iNextSpawn;
	}

	g_fBuyTimeLimit = (g_Timers->TheTime() + (mp_buytime->GetFloat() * 60));
	return;
}

void ZombiePlugin::Event_Player_Spawn( int iPlayer )
{
	if ( iPlayer > -1 )
	{
		edict_t *pEntity = m_Engine->PEntityOfEntIndex( iPlayer );
		CBaseEntity *pBase = pEntity->GetUnknown()->GetBaseEntity();
		if ( zombie_enabled.GetBool() && pEntity && !pEntity->IsFree() && pBase ) 
		{
			bool bOn = true;
			UTIL_SetProperty( g_Offsets.m_iHealth, pEntity, (int)100 );
			
			//UTIL_SetProperty( g_Offsets.m_iAccount, pEntity, 16000 );
			int iCvarVal = zombie_startmoney.GetInt();
			if ( iCvarVal != 0 )
			{
				UTIL_SetProperty( g_Offsets.m_iAccount, pEntity, ( iCvarVal == 1 ? 16000 : iCvarVal ) );
			}

			GETCLASS( g_Players[iPlayer].iChangeToClass );
			if ( zClass )
			{
				g_Players[iPlayer].iClass = zClass->iClassId;
				g_Players[iPlayer].iChangeToClass = -1;
			}
			
			ZombieVision( pEntity, false );
			if ( g_Players[iPlayer].iRegenTimer != 0 )
			{
				g_Timers->RemoveTimer( g_Players[iPlayer].iRegenTimer  );
			}
			if ( g_Players[iPlayer].bFaded )
			{
				DoFade( pBase, false, iPlayer );
			}
			if ( g_Players[iPlayer].isZombie )
			{
				HudMessage( iPlayer, 66, 0.03, 0.89, 188, 112, 0, 128, 188, 112, 0, 128, 0, 0.0, 0.0, 100.0, 0.0, "" );
				g_Players[iPlayer].isZombie = false;
				if ( !g_Players[iPlayer].isBot )
				{
					bOn = false;
					//UTIL_SetProperty( g_Offsets.m_bHasNightVision, pEntity, bOn );
					//UTIL_SetProperty( g_Offsets.m_bNightVisionOn, pEntity, bOn );
					SetFOV( pEntity, 90 );
				}
			}
			bool bIsAlive = IsAlive( pEntity );

			if ( bIsAlive )
			{
				g_Players[iPlayer].bHasSpawned = true;
			}

			g_Players[iPlayer].iStuckRemain = zombie_teleportcount.GetInt();


			g_Players[iPlayer].bJetPack = false;
			g_Players[iPlayer].iJetPack = 0;

			g_Players[iPlayer].fOriginalKnifeSpeed = -1.0;

			g_Players[iPlayer].vSpawn = pEntity->GetCollideable()->GetCollisionOrigin();
			g_Players[iPlayer].qSpawn = pEntity->GetCollideable()->GetCollisionAngles();

			if ( zombie_undead_sound_enabled.GetBool() && !Q_stristr( zombie_undead_sound_exclusions.GetString(), g_CurrentMap ) )
			{
				m_EngineSound->StopSound( iPlayer, CHAN_AUTO, zombie_undead_sound.GetString() );
				if ( zombie_undead_sound_enabled.GetBool() )
				{
					MRecipientFilter mrf;
					mrf.AddPlayer( iPlayer );
					m_EngineSound->EmitSound( mrf, iPlayer, CHAN_AUTO, zombie_undead_sound.GetString(), zombie_undead_sound_volume.GetFloat(), ATTN_NORM, 0, PITCH_NORM, &pEntity->GetCollideable()->GetCollisionOrigin(), 0, 0, true, 0, iPlayer);
				}
			}

			if ( g_Players[iPlayer].bSameWeapons && !g_Players[iPlayer].isZombie )
			{
				GiveSameWeapons( pEntity, iPlayer );
			}
			else if ( zombie_buymenu_roundstart.GetBool() && IsAlive( pEntity ) )
			{
				ShowBuyMenu( pEntity, -1 );
			}
		}
	}
}


META_RES ZombiePlugin::Event_Player_Say( int iPlayer, const char *sText )
{
	if ( iPlayer > 0 && iPlayer <= MAX_PLAYERS )
	{
		if ( Q_strncmp( sText, "!z3", 6 ) == 0 )
		{
			ShowMOTD( iPlayer, "ZombieMod 3.0", "http://www.zombiemod.com/z3.htm", TYPE_URL, "" ); //ZombieMod Information
			return MRES_SUPERCEDE;
		}
		if ( Q_strncmp( sText, "!zhelp", 6 ) == 0 )
		{
			ShowMOTD( iPlayer, (char*)GetLang("zombie_info"), "myinfo", TYPE_INDEX, "" ); //ZombieMod Information
			return MRES_SUPERCEDE;
		}
		else if ( Q_strncmp( sText, "!zclasses", 6 ) == 0 )
		{
			ShowMOTD( iPlayer, (char*)GetLang("class_info"), "zm_classinfo", TYPE_INDEX, "" ); //ZombieMod Information
			return MRES_SUPERCEDE;
		}
		else if ( Q_strncmp( sText, "!ztele", 7 ) == 0 )
		{
			edict_t *pEntity =  m_GameEnts->BaseEntityToEdict( g_Players[iPlayer].pPlayer );
			if ( g_Players[iPlayer].iStuckRemain > 0 && iNextSpawn > 0 )
			{
				if ( !IsAlive( pEntity ) )
				{
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("ztele_dead") );
					return MRES_SUPERCEDE;
				}
				if ( !g_ZombieRoundOver && zombie_teleport_zombies_only.GetBool() && !g_Players[iPlayer].isZombie )
				{
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("ztele_chosen") ); //Once a zombie has been chosen only zombies can use !ztele.
					return MRES_SUPERCEDE;
				}
				--g_Players[iPlayer].iStuckRemain;
				IPlayerInfo *iInfo = m_PlayerInfoManager->GetPlayerInfo( pEntity );
				if ( iInfo )
				{
					QAngle qAngles = iInfo->GetAbsAngles();
					Vector vecVelocity = vec3_origin;
					//Teleport( g_Players[iPlayer].pPlayer, &vSpawnVec[iUseSpawn], &qAngles, &vecVelocity );
					CBaseEntity_Teleport( g_Players[iPlayer].pPlayer, &vSpawnVec[iUseSpawn], &qAngles, &vecVelocity );
					//Teleport( g_Players[iPlayer].pPlayer, &vSpawnVec[iUseSpawn], &qAngles, &vecVelocity );
					iUseSpawn = (++iUseSpawn) % iNextSpawn;
					char sPrint[100];
					Q_snprintf( sPrint, sizeof( sPrint ), GetLang( "ztele_uses" ), g_Players[iPlayer].iStuckRemain, zombie_teleportcount.GetInt() );
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", sPrint ); //You have %d / %d uses of ztele left this round.
				}
			}
			else
			{
				Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("ztele_used") ); //You have used up all your uses of ztele this round.
			}
			return MRES_SUPERCEDE;
		}
		else if ( Q_strncmp( sText, "!zstuck", 7 ) == 0 )
		{
			edict_t *pEntity =  m_GameEnts->BaseEntityToEdict( g_Players[iPlayer].pPlayer );
			if ( IsValidPlayer( pEntity ) )
			{
				if ( !IsAlive( pEntity ) )
				{
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("zstuck_dead") ); //You cannot use !zstuck when you're dead.
					return MRES_SUPERCEDE;
				}
				if ( !zombie_zstuck_enabled.GetBool() )
				{
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("zstuck_disabled") ); //!zstuck is currently disabled.
					return MRES_SUPERCEDE;
				}
				Vector vecPlayer = pEntity->GetCollideable()->GetCollisionOrigin();
				Vector vecClosest = ClosestPlayer( iPlayer, vecPlayer );
				if ( vecPlayer.DistTo( vecClosest ) <= zombie_stuckcheck_radius.GetFloat() )
				{
					/*
					Set_sv_cheats( true, NULL );
					m_Helpers->ClientCommand( pEntity, "noclip" );
					m_Helpers->ClientCommand( pEntity, "noclip" );
					Set_sv_cheats( false, NULL );
					*/
					UnstickPlayer( iPlayer );
				}
				Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("zstuck_retry") ); //If you are still stuck try crouching and using !zstuck again.
			}
			return MRES_SUPERCEDE;
		}
		else if ( Q_strncmp( sText, "!zmenu", 6 ) == 0 )
		{
			edict_t *pEntity =  m_GameEnts->BaseEntityToEdict( g_Players[iPlayer].pPlayer );
			if ( pEntity )
			{
				ShowMainMenu( pEntity );
			}
			return MRES_SUPERCEDE;
		}
		else if ( Q_strncmp( sText, "!zbuy", 6 ) == 0 )
		{
			edict_t *pEntity =  m_GameEnts->BaseEntityToEdict( g_Players[iPlayer].pPlayer );
			if ( zombie_buymenu.GetBool() && g_bBuyMenu && pEntity )
			{
				ShowBuyMenu( pEntity, -1 );
			}
			else
			{
				Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("buy_disabled") );
			}
			return MRES_SUPERCEDE;
		}
		else if ( Q_strncmp( sText, "!zclassmenu", 6 ) == 0 )
		{
			edict_t *pEntity =  m_GameEnts->BaseEntityToEdict( g_Players[iPlayer].pPlayer );
			if ( m_ClassManager.Enabled() )
			{
				if ( zombie_classes_random.GetInt() == 1 || zombie_classes_random.GetInt() == 2 )
				{
					if ( zombie_classes_random.GetInt() == 1 )
					{
						Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("random_class1") );
					}
					else
					{
						Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("random_class2") );
					}
					GETCLASS( g_Players[iPlayer].iClass );
					SelectZombieClass( pEntity, iPlayer, zClass, true, true );
				}
				else
				{
					ShowClassMenu( pEntity );
				}
			}
			else
			{
				Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("classes_disabled") );
			}
			return MRES_SUPERCEDE;
		}
		else if ( Q_strncmp( sText, "!zspawn", 7 ) == 0 )
		{
			if ( IsValidPlayer( g_Players[iPlayer].pPlayer ) )
			{
				//CBasePlayer_SwitchTeam
				edict_t *pEntity =  m_GameEnts->BaseEntityToEdict( g_Players[iPlayer].pPlayer );
				int iTeam = GetTeam( pEntity );
				bool bAlive = IsAlive( pEntity );
				if ( !zombie_respawn.GetBool() && (!zombie_respawn_onconnect.GetBool() && g_Players[iPlayer].bHasSpawned))
				{
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("zspawn_disabled") );
				}
				else
				{
					if ( g_ZombieRoundOver )
					{
						Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("autospawn_1stzombie") );
						return MRES_SUPERCEDE;
					}
					if ( !bAlive && (iTeam == COUNTERTERRORISTS || iTeam == TERRORISTS) )
					{
						if ( zombie_respawn_delay.GetInt() > 0 ) //&& ( iTeam == TERRORISTS || iTeam == COUNTERTERRORISTS ) )
						{
							char sPrint[100];
							for ( int x = 0; x < g_RespawnTimers.Count(); x++ )
							{
								if ( g_RespawnTimers[x].iPlayer == iPlayer )
								{
									double dRes = g_Timers->StartTimeLeft( g_RespawnTimers[x].iTimer );
									if ( dRes < 0.0 )
									{
										Q_snprintf( sPrint, sizeof( sPrint ), "ERROR - Respawn timer less than 0 - %d", dRes );
										Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", sPrint ); //You have already used !zspawn - time remaining %d seconds.
										META_LOG( g_PLAPI, sPrint );
									}
									else
									{
										Q_snprintf( sPrint, sizeof( sPrint ), GetLang("autospawn_secs_2nd"), dRes );
										Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", sPrint ); //You have already used !zspawn - time remaining %d seconds.
									}
									return MRES_SUPERCEDE;
								}
							}
							
							Q_snprintf( sPrint, sizeof( sPrint ), GetLang("autospawn_secs"), zombie_respawn_delay.GetInt() );
							META_LOG ( g_PLAPI, GetLang("autospawn_secs") );
							//META_LOG ( g_PLAPI, sPrint );
							Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", sPrint ); //You will auto-respawn in %d seconds.
							void *params[1];
							params[0] = pEntity;
							int iId = g_RespawnTimers.AddToTail( );
							g_RespawnTimers[iId].iTimer = g_Timers->AddTimer( zombie_respawn_delay.GetFloat(), CheckRespawn, params, 1 );
							g_RespawnTimers[iId].iPlayer = iPlayer;
							return MRES_SUPERCEDE;
						}
						void *params[1];
						params[0] = pEntity;
						CheckRespawn( params );
					}
					else
					{
						if ( !(iTeam == COUNTERTERRORISTS || iTeam == TERRORISTS) )
						{
							Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("zspawn_team") );
						}
						else
						{
							Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("zspawn_dead") );
						}
					}
				}
			}
			return MRES_SUPERCEDE;
		}
		else if ( Q_strncmp( sText, "!zplayer", 7 ) == 0 )
		{
			char buf[200];
			myString sFind;
			
			edict_t *pEntity =  m_GameEnts->BaseEntityToEdict( g_Players[iPlayer].pPlayer );

			sFind.assign( sText );
			sFind.assign( sFind.substr( 9, sFind.size() - 9 ) );


			CPlayerMatchList matches( sFind.c_str(), false );
			if ( matches.Count() < 1 ) 
			{
				Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s \x04%s\x01.", GetLang("cannot_match_player"), sFind.c_str() );
				return MRES_SUPERCEDE;
			} //if ( matches.Count() < 1 ) 
			for ( int x = 0; x < matches.Count(); x++ )
			{
				Q_snprintf( buf, sizeof(buf), "%s\n----------------------\n", GetLang("zombie_playerinfo")  );
				m_Engine->ClientPrintf( pEntity, buf );
				int iIndex = m_Engine->IndexOfEdict( matches.GetMatch(x) );
				if ( g_Players[iIndex].bConnected )
				{
					Q_snprintf( buf, sizeof(buf), "%s: %s\n%s: %s\n%s: %s\n----------------------\n\n", GetLang("player_name"), g_Players[iIndex].sUserName.c_str(), 
						GetLang("invinc"), ( g_Players[iIndex].bProtected ? "Yes" : "No" ), GetLang("iszombie"), ( g_Players[iIndex].isZombie ? "Yes" : "No" ) );
					m_Engine->ClientPrintf( pEntity, buf );
				}
			} // for
			return MRES_SUPERCEDE;
		}
		else if ( Q_strncmp( sText, "!zstart", 7 ) == 0 )
		{
			edict_t *pEntity =  m_GameEnts->BaseEntityToEdict( g_Players[iPlayer].pPlayer );
			if ( IsValidPlayer( pEntity ) )
			{
				if ( zombie_kickstart_percent.GetInt() == 0 )
				{
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("kickstart_disabled") );
					return MRES_SUPERCEDE;
				}
				if ( bKickStart[iPlayer] )
				{
					Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("kickstarted") );
					return MRES_SUPERCEDE;
				}
				bKickStart[iPlayer] = true;

				int iNeeded = 0;
				iNeeded = (int)iPlayerCount / 100 * zombie_kickstart_percent.GetInt();
				if ( iNeeded < 1 )
				{
					iNeeded = 1;
				}
				if ( iKickTimer > 0 || iNeeded == 1 )
				{
					int iKickers = 0;
					iNeeded = (int)iPlayerCount / 100 * zombie_kickstart_percent.GetInt();
					for ( int x=0; x< MAX_PLAYERS; x++ )
					{
						if ( bKickStart[x] )
						{
							iKickers++;
						}
					}
					if ( iKickers >= iNeeded || iNeeded == 1 )
					{
						iGlobalVoteTimer = zombie_vote_interval.GetInt() * 2;
						for ( int x=0; x< MAX_PLAYERS; x++ )
						{
							bKickStart[x] = false;
						}
						Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("kickstarting") );
						bool bRoundStarted = g_ZombiePlugin.bRoundStarted;
						g_ZombiePlugin.bRoundStarted = false;
						RandomZombie( NULL );
						g_ZombiePlugin.bRoundStarted = bRoundStarted;
						g_Timers->RemoveTimer( iKickTimer );
						iKickTimer = 0;
					}
					else
					{
						char sPrint[100];
						Q_snprintf( sPrint, sizeof( sPrint ), GetLang("kickers"), iKickers, iNeeded );
						Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", sPrint );
					}
				}
				else
				{
					if ( iGlobalVoteTimer > 0 )
					{
						char sPrint[100];
						Q_snprintf( sPrint, sizeof( sPrint ), GetLang("vote_restriction"), (iGlobalVoteTimer / 2) );
						Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", sPrint );
					}
					else
					{
						iKickTimer = g_Timers->AddTimer( zombie_kickstart_timer.GetFloat(), KickStartEnd );
						char sPrint[100];
						Q_snprintf( sPrint, sizeof( sPrint ), GetLang("kickstart_start"), 1, iNeeded );
						Hud_Print( NULL, "\x03[ZOMBIE]\x01 %s", sPrint );
						Q_snprintf( sPrint, sizeof( sPrint ), GetLang("kickstart_vote"), zombie_kickstart_timer.GetInt() );
						Hud_Print( NULL, "\x03[ZOMBIE]\x01 %s", sPrint );
					}
				}
				return MRES_SUPERCEDE;
			}
		}
	}
	return MRES_IGNORED;
}

void ZombiePlugin::LoadZombieEvents()
{
	char error[300];
	int maxlength = sizeof( error );

	HOOK_EVENT2( player_spawn );
	HOOK_EVENT2( round_start );
	HOOK_EVENT2( round_end );
	HOOK_EVENT2( player_death );
	HOOK_EVENT2( player_say );
	HOOK_EVENT2( round_freeze_end );
	HOOK_EVENT2( hostage_follows );
	HOOK_EVENT2( player_jump );
	HOOK_EVENT2( player_team );
	HOOK_EVENT2( weapon_fire );
	HOOK_EVENT2( player_hurt );
	HOOK_EVENT2( zombification );
}

void ZombiePlugin::UnLoadZombieEvents()
{
	UNHOOK_EVENT2( player_spawn );
	UNHOOK_EVENT2( round_start );
	UNHOOK_EVENT2( round_end );
	UNHOOK_EVENT2( player_death );
	UNHOOK_EVENT2( player_say );
	UNHOOK_EVENT2( round_freeze_end );
	UNHOOK_EVENT2( hostage_follows );
	UNHOOK_EVENT2( player_jump );
	UNHOOK_EVENT2( player_team );
	UNHOOK_EVENT2( weapon_fire );
	UNHOOK_EVENT2( player_hurt );
	UNHOOK_EVENT2( zombification );
}

void ZombiePlugin::DisplayHelp( INetworkStringTable *pInfoPanel )
{
	char sTmp[500] = "";
	char sFinal[10240] = "";
	#define AddToString( sFrmt, Var ) \
		sTmp[0] = '\0'; \
		Q_snprintf( sTmp, sizeof( sTmp ), sFrmt, Var ); \
		Q_strncat( sFinal, sTmp, sizeof( sFinal ), -1 );

	AddToString( "%s", (char*)zombie_help_url.GetString() );
	AddToString( "?cnt=%d", ( zombie_count.GetInt() <= 0 ? 1 : zombie_count.GetInt() ) );
	AddToString( "&tele=%d", ( zombie_teleportcount.GetInt() > 0 ? zombie_teleportcount.GetInt() : 0 ) );
	AddToString( "&stuck=%d", zombie_zstuck_enabled.GetInt() );
	AddToString( "&spawn=%d", zombie_respawn.GetInt() );
	AddToString( "&spawnas=%d", zombie_respawn_as_zombie.GetInt() );
	AddToString( "&stimer=%d", zombie_respawn_delay.GetInt() );
	AddToString( "&jetpack=%d", (int)( zombie_jetpack.GetInt() || zombie_humans_jetpack.GetInt() )  );
	AddToString( "&wns=%s", sRestrictedWeapons.c_str() );
	AddToString( "&css=%d", (int)( zombie_classes.GetBool() )  );
	AddToString( "&rgn=%d", zombie_regen_timer.GetInt() );
	AddToString( "&hp=%d", zombie_regen_health.GetInt() );
	AddToString( "&prim=%s", zombie_respawn_primary.GetString() );
	AddToString( "&sec=%s", zombie_respawn_secondary.GetString() );
	AddToString( "&gren=%d", zombie_respawn_grenades.GetInt() );
	AddToString( "&v=%s", zombie_version.GetString() );
	Q_snprintf( sTmp, sizeof( sTmp ), "<!doctype html public \"-//w3c//dtd html 4.0 transitional//en\"><html><body><meta http-equiv=\"Refresh\" content=\"0; url=%s\"></body></html>", sFinal );

	int StrIdx = pInfoPanel->FindStringIndex( "myinfo" );
	if ( StrIdx == INVALID_STRING_INDEX  )
	{
#if defined ORANGEBOX_BUILD
		StrIdx = pInfoPanel->AddString( ADDSTRING_ISSERVER, "myinfo" );
#else
		StrIdx = pInfoPanel->AddString( "myinfo" );
#endif
	}
	pInfoPanel->SetStringUserData( StrIdx, Q_strlen( sTmp ), sTmp );
}

void ZombiePlugin::DisplayClassHelp( INetworkStringTable *pInfoPanel )
{
	char sTmp[10240] = "";
	char sTmp2[500] = "";
	char sFinal[10240] = "";
	#define AddToString( sFrmt, Var ) \
		sTmp[0] = '\0'; \
		Q_snprintf( sTmp, sizeof( sTmp ), sFrmt, Var ); \
		Q_strncat( sFinal, sTmp, sizeof( sFinal ), -1 );

	#define AddToString2( sFrmt, Var, Var2 ) \
		sTmp[0] = '\0'; \
		Q_snprintf( sTmp, sizeof( sTmp ), sFrmt, Var, Var2 ); \
		Q_strncat( sFinal, sTmp, sizeof( sFinal ), -1 );

	AddToString( "<form name=\"zmform\" id=\"zmform\" action=\"%s\" method=\"POST\">", (char*)zombie_class_url.GetString() );

	AddToString( "<input type=\"hidden\" name=\"vars\" value=\"?v=%s", zombie_version.GetString() );

	for ( int x = 0; x < m_ClassManager.Count(); x++ )
	{
		#define DOMATH( iVal, iVar, iPercent ) \
			iVal = ( (float)iVar / 100 ) * (float)iPercent;
		
		GETCLASS( x );
		AddToString2( "&name[%d]=%s", x, zClass->GetName() );
		AddToString2( "&model[%d]=%s", x, zClass->GetModelName() );

		int iTmp = 0;
		DOMATH( iTmp, zClass->iHealth, zombie_health_percent.GetInt() );
		Q_snprintf( sTmp2, sizeof( sTmp2 ), "%d (%d%%%)", iTmp, zombie_health_percent.GetInt() );
		AddToString2( "&health[%d]=%s", x, sTmp2 );
		
		float fTmp = ( zClass->fSpeed / 100 ) * zombie_speed_percent.GetInt();
		Q_snprintf( sTmp2, sizeof( sTmp2 ), "%.2f (%d%%%)", fTmp, zombie_speed_percent.GetInt() );
		AddToString2( "&speed[%d]=%s", x, sTmp2 );

		fTmp = ( zClass->fSpeedDuck / 100 ) * zombie_speed_percent.GetInt();
		Q_snprintf( sTmp2, sizeof( sTmp2 ), "%.2f (%d%%%)", fTmp, zombie_speed_percent.GetInt() );
		AddToString2( "&duck_speed[%d]=%s", x, sTmp2 );

		fTmp = ( zClass->fSpeedRun / 100 ) * zombie_speed_percent.GetInt();
		Q_snprintf( sTmp2, sizeof( sTmp2 ), "%.2f (%d%%%)", fTmp, zombie_speed_percent.GetInt() );
		AddToString2( "&run_speed[%d]=%s", x, sTmp2 );


		DOMATH( iTmp, zClass->iJumpHeight, zombie_jump_height_percent.GetInt() );
		Q_snprintf( sTmp2, sizeof( sTmp2 ), "%d (%d%%%)", iTmp, zombie_jump_height_percent.GetInt() );
		AddToString2( "&jumph[%d]=%s", x, sTmp2 );
		
		AddToString2( "&hss[%d]=%d", x, zClass->iHeadshots );

		fTmp = ( zClass->fKnockback / 100 ) * zombie_knockback_percent.GetInt();
		Q_snprintf( sTmp2, sizeof( sTmp2 ), "%.2f (%d%%%)", fTmp, zombie_knockback_percent.GetInt() );
		AddToString2( "&kb[%d]=%s", x, sTmp2 );
		
		AddToString2( "&hso[%d]=%s", x, (zClass->bHeadShotsOnly ? "1" : "0") );

		AddToString2( "&regh[%d]=%d", x, zClass->iRegenHealth );

		AddToString2( "&regs[%d]=%.2f", x, zClass->fRegenTimer );

		AddToString2( "&grenm[%d]=%.2f", x, zClass->fGrenadeMultiplier );

		AddToString2( "&grenk[%d]=%.2f", x, zClass->fGrenadeKnockback );

		AddToString2( "&hb[%d]=%d", x, zClass->iHealthBonus );
	}
	AddToString( "&classes=%d\" />\r\n", m_ClassManager.Count() );

	AddToString( "<input type=\"submit\" value=\"View Classes\" /></%s>", "form" );


	Q_snprintf( sTmp, sizeof( sTmp ), "<!doctype html public \"-//w3c//dtd html 4.0 transitional//en\"><html><body>%s<br /><br />Click 'View Classes' to view classlist.<br /><br />Sorry about having to click the submit button - unless someone has a way to automaticaly submit a form in this window?</body></html>", sFinal );

}
