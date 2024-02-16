#include "ZombiePlugin.h"
#include "cvars.h"
#include "VFunc.h"

#undef GetClassName

extern CUtlVector<TimerInfo_t>	g_RespawnTimers;
extern PlayerInfo_t				g_Players[65];
extern unsigned int				g_RoundTimer;
extern int						iNextHealth;
extern bool						bRoundTerminating;
extern int						iZombieCount;
extern int						iHealthDecrease;
extern bool						bFirstEverRound;
extern bool						bLoadedLate;
extern ConVar					*mp_roundtime;
extern bool						g_bZombieClasses;
extern int						g_iZombieClasses;
extern ZombieClasses_t			g_ZombieClasses[100];
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

void ZombiePlugin::Event_Round_Freeze_End( )
{
	bAllowedToJetPack = false;
	bZombieDone = false;
	bRoundStarted = true;
	zombie_first_zombie.SetValue( 0 );
	Hud_Print( NULL, "\x03[ZOMBIE]\x01 %s \x04%s\x01.", GetLang("zombie_enabled"), GetLang("humans_vs_zombies") ); //ZombieMod is enabled, run for your lives, the game is
	//HUMANS vs. ZOMBIES
	g_ZombieRoundOver = true;
	g_Timers->RemoveTimer( 8672396 );
	float fRand = RandomFloat(zombie_timer_min.GetFloat(), zombie_timer_max.GetFloat());
	META_LOG( g_PLAPI, "Random zombie in %f.", fRand );
	g_Timers->AddTimer( fRand, RandomZombie, NULL, 0, 8672396 );
	
	iNextSpawn = 0;
	for ( int x = 0; x <= MAX_PLAYERS; x++ )
	{
		edict_t *pEnt = NULL;
		if ( IsValidPlayer( x, &pEnt ) && IsAlive( pEnt ) )
		{
			vSpawnVec[iNextSpawn] = pEnt->GetCollideable()->GetCollisionOrigin();
			++iNextSpawn;
			g_Players[x].vSpawn = pEnt->GetCollideable()->GetCollisionOrigin();
			g_Players[x].qSpawn = pEnt->GetCollideable()->GetCollisionAngles();
		}
	}
	if ( iNextSpawn != 0 )
	{
		iUseSpawn = iUseSpawn % iNextSpawn;
	}
	return;
}

META_RES ZombiePlugin::Event_Player_Say( int iPlayer, const char *sText )
{
	if ( iPlayer > 0 && iPlayer <= MAX_PLAYERS )
	{
		if ( Q_strncmp( sText, "!zhelp", 6 ) == 0 )
		{
			//ShowMOTD( iPlayer, "ZombieMod Information", (char*)zombie_help_url.GetString(), TYPE_URL, "" );
			ShowMOTD( iPlayer, (char*)GetLang("zombie_info"), "myinfo", TYPE_INDEX, "" ); //ZombieMod Information
			//DisplayHelp( iPlayer );
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
					CBaseAnimating_Teleport( g_Players[iPlayer].pPlayer, &vSpawnVec[iUseSpawn], &qAngles, &vecVelocity );
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
					Set_sv_cheats( true, NULL );
					m_Helpers->ClientCommand( pEntity, "noclip" );
					m_Helpers->ClientCommand( pEntity, "noclip" );
					Set_sv_cheats( false, NULL );
				}
				Hud_Print( pEntity, "\x03[ZOMBIE]\x01 %s", GetLang("zstuck_retry") ); //If you are still stuck try crouching and using !zstuck again.
			}
			return MRES_SUPERCEDE;
		}
		else if ( Q_strncmp( sText, "!zmenu", 6 ) == 0 )
		{
			edict_t *pEntity =  m_GameEnts->BaseEntityToEdict( g_Players[iPlayer].pPlayer );
			if ( g_bZombieClasses && zombie_classes.GetBool() && g_iZombieClasses > -1 )
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
					SelectZombieClass( pEntity, iPlayer, g_Players[iPlayer].iClass, true, true );
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
							META_LOG ( g_PLAPI, sPrint );
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
				if ( iKickTimer > -1 || iNeeded == 1 )
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
						iKickTimer = -1;
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

			CBasePlayer *pPlayer = (CBasePlayer *)GetContainingEntity( pEntity );
			
			//sv_cheats->m_fValue = 1.0;
			//m_Engine->ServerCommand( "sv_cheats 1\n" );
			//g_Players[iPlayer].sScreenOverlay.assign( "r_screenoverlay off" );
			//m_Engine->ClientCommand( pEntity, "zombie_vision" );
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
		}
	}
}

void ZombiePlugin::Event_Player_Death( int iVic, int iAtt )
{
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
		if ( g_bZombieClasses && (zombie_classes_random.GetInt() == 1 || (zombie_classes_random.GetInt() == 3 && !g_Players[iVic].bChoseClass)))
		{
			int iChangeTo = 0;
			iChangeTo = RandomInt( 0, g_iZombieClasses );
			SelectZombieClass( pPlayer, iVic, iChangeTo, true );
		}
		else if ( g_Players[iVic].iChangeToClass > -1 && g_Players[iVic].iChangeToClass <= g_iZombieClasses )
		{
			g_Players[iVic].iClass = g_Players[iVic].iChangeToClass;
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
			//bool bOn = false;
			//UTIL_SetProperty( g_Offsets.m_bHasNightVision, pPlayer, bOn );
			//UTIL_SetProperty( g_Offsets.m_bNightVisionOn, pPlayer, bOn );
		}
		if ( g_Players[iVic].iProtectTimer != 99 )
		{
			//= g_ZombiePlugin.g_Timers->AddTimer( zombie_respawn_protection.GetFloat(), TimerProtection, params, 1 );
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

void ZombiePlugin::Event_Round_End( int iWinner )
{
	zombie_first_zombie.SetValue( 0 );
	iNextHealth = 0;
	g_Timers->RemoveTimer( g_RoundTimer );
	bAllowedToJetPack = false;
	int x = 0;
	if ( zombie_enabled.GetBool() )
	{
		bRoundStarted = false;
		bRoundTerminating = true;
	}
	if ( iWinner == 3 )
	{
		g_ZombiePlugin.RoundEndOverlay( true, 0 );
	}
	else
	{
		g_ZombiePlugin.RoundEndOverlay( true, 1 );
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
						META_LOG( g_PLAPI, "Protected Player at round end %d!", x );
				}
				if ( g_Players[x].iChangeToClass > -1  && g_Players[x].iChangeToClass <= g_iZombieClasses  )
				{
					g_Players[x].iClass = g_Players[x].iChangeToClass;
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

void ZombiePlugin::Event_Hostage_Follows( int iHosti )
{
	if ( iHosti > 0 )
	{
		edict_t *pEnt = m_Engine->PEntityOfEntIndex( iHosti );
		if ( pEnt && !pEnt->IsFree() )
		{
			CBaseEntity *pBase = GetContainingEntity( pEnt );
			if ( pBase )
			{
				CBaseEntity_Event_Killed( pBase, CTakeDamageInfo(GetContainingEntity(m_Engine->PEntityOfEntIndex(0)), GetContainingEntity(m_Engine->PEntityOfEntIndex(0)), 0, DMG_NEVERGIB) );
				CBasePlayer_Event_Dying( (CBasePlayer *)pBase );
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
		for( x = 1; x < m_Engine->GetEntityCount(); x++ )
		{
			edict_t *pEnt = m_Engine->PEntityOfEntIndex( x );
			if ( pEnt && !pEnt->IsFree() )
			{
				if ( x > 0 && x <= MAX_PLAYERS && IsValidPlayer( g_Players[x].pPlayer ) && !g_Players[x].isBot )
				{
					g_Players[x].bProtected = false;
					g_Players[x].bAsZombie = false;
					SetProtection( x, false, false, false );
					ZombieVision( pEnt, false );
					iHumanCount++;
					int iTeam = GetTeam( pEnt );
					if ( IsAlive( pEnt ) )
					{
						CBaseCombatWeapon *pWeapon = CBaseCombatCharacter_WeaponSlot( (CBaseCombatCharacter*) g_Players[x].pPlayer, SLOT_SECONDARY );
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
				else if ( pEnt->GetClassName() && FStrEq( pEnt->GetClassName(), "hostage_entity" ) )
				{
					
					if ( iPlayerCount != 0 )
					{
						CBaseEntity *pBase = m_GameEnts->EdictToBaseEntity( pEnt );
						if ( pBase )
						{
							CBaseEntity_Event_Killed( pBase, CTakeDamageInfo(GetContainingEntity(m_Engine->PEntityOfEntIndex(0)), GetContainingEntity(m_Engine->PEntityOfEntIndex(0)), 0, DMG_NEVERGIB) );
							CBasePlayer_Event_Dying( (CBasePlayer *)pBase );
						}
					}
				}
			}
		}
		if ( mp_roundtime )
		{
			g_Timers->RemoveTimer( g_RoundTimer );
			float fTime = (mp_roundtime->GetFloat() * 60.0f);
			g_RoundTimer = g_Timers->AddTimer( fTime, Timed_RoundTimer );
		}
	}
}

void ZombiePlugin::Event_Player_Jump( int iPlayer, CBaseEntity *cEntity )
{
	if ( g_bZombieClasses && zombie_classes.GetBool() && g_Players[iPlayer].iClass >= 0 && g_Players[iPlayer].iClass <= g_iZombieClasses )
	{
		if ( g_Players[iPlayer].isZombie && g_ZombieClasses[ g_Players[iPlayer].iClass ].iJumpHeight != 0 )
		{
			int iJumpHeight = ( g_ZombieClasses[ g_Players[iPlayer].iClass ].iJumpHeight / 100 ) * zombie_jump_height_percent.GetInt();
			iJumpers = iJumpers + 1;
			pJumper[iJumpers] = cEntity;
			iJumper[iJumpers] = iJumpHeight;
		}
	}
}

void ZombiePlugin::DisplayHelp( INetworkStringTable *pInfoPanel )
{
	char sTmp[500] = "";
	char sFinal[1024] = "";
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
	AddToString( "&jetpack=%d", (int)( zombie_jetpack.GetInt() || humans_jetpack.GetInt() )  );
	AddToString( "&wns=%s", sRestrictedWeapons.c_str() );
	AddToString( "&css=%d", (int)( zombie_classes.GetBool() )  );
	AddToString( "&rgn=%d", zombie_regen_timer.GetInt() );
	AddToString( "&hp=%d", zombie_regen_health.GetInt() );
	AddToString( "&prim=%s", zombie_respawn_primary.GetString() );
	AddToString( "&sec=%s", zombie_respawn_secondary.GetString() );
	AddToString( "&gren=%d", zombie_respawn_grenades.GetInt() );
	AddToString( "&v=%s", zombie_version.GetString() );
	//AddToString( "%s%s", , sTmp );
	//ShowMOTD( iPlayer, "ZombieMod Information", "myinfo", TYPE_INDEX, "" );
	Q_snprintf( sTmp, sizeof( sTmp ), "<!doctype html public \"-//w3c//dtd html 4.0 transitional//en\"><html><body><meta http-equiv=\"Refresh\" content=\"0; url=%s\"></body></html>", sFinal );

	//pInfoPanel->SetStringUserData( StrIdx, 4096, ZombieModHelpText().c_str() );	
	int StrIdx = pInfoPanel->FindStringIndex( "myinfo" );
	if ( StrIdx == INVALID_STRING_INDEX  )
	{
		StrIdx = pInfoPanel->AddString( "myinfo" );
	}
	pInfoPanel->SetStringUserData( StrIdx, Q_strlen( sTmp ), sTmp );
	//ShowMOTD( iPlayer, "ZombieMod Help", sFinal, TYPE_URL, "" );
}
