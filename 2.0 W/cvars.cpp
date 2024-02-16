/* ======== props_mm ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): David "BAILOPAN" Anderson
* ============================
*/
#include "server_class.h"
#include "ZombiePlugin.h"
#include "cvars.h"
#include "myutils.h"

#include "mapentities.h"

ZombieAccessor					g_Accessor;

extern CTeam					*teamCT;
extern CTeam					*teamT;
extern ArrayLengthSendProxyFn	TeamCount;
extern PlayerInfo_t				g_Players[65];
extern int						g_RestrictT[CSW_MAX];
extern int						g_RestrictCT[CSW_MAX];
extern int						g_RestrictW[CSW_MAX];

extern bool						g_bZombieClasses;
extern int						g_iZombieClasses;
extern ZombieClasses_t			g_ZombieClasses[100];
extern KeyValues				*kvPlayerClasses;

ConVar zombie_model_file("zombie_model_file", "cfg/zombiemod/models.cfg", FCVAR_PLUGIN, "File with list of models to use for zombie mod" );
ConVar zombie_download_file("zombie_download_file", "cfg/zombiemod/downloads.cfg", FCVAR_PLUGIN, "File with list of materials. Clients download everything in this list when they connect.");
ConVar zombie_sound_rand("zombie_sound_rand", "50", FCVAR_PLUGIN, "How often zombies play zombie sounds. The lower the value, the more often they emit a sound.", true, 1, false, 0);
ConVar zombie_enabled("zombie_enabled", "0", FCVAR_REPLICATED | FCVAR_SPONLY | FCVAR_NOTIFY, "If non-zero, zombie mode is enabled (DO NOT SET THIS DIRECTLY!)", true, 0, true, 1, CVar_CallBack );
ConVar zombie_health("zombie_health", "2500", FCVAR_PLUGIN, "Health that zombies get. First zombie is this times two.");
ConVar zombie_speed("zombie_speed", "325.0", FCVAR_PLUGIN, "The speed that zombies travel at.");
ConVar zombie_fov("zombie_fov", "125", FCVAR_PLUGIN, "Field of vision of the zombies. Normal human FOV is 90.", true, 40, true, 160);
ConVar zombie_knockback("zombie_knockback", "4", FCVAR_PLUGIN, "The knockback multiplier of zombies. Set to 0 to disable knockback", true, 0, false, 0);
ConVar zombie_restrictions("zombie_restrictions", "rifles m249 flash", FCVAR_PLUGIN, "Space separated list of guns that are restricted during zombie mode. Takes effect immediately on change.", CVar_CallBack);
ConVar zombie_dark("zombie_dark", "1", FCVAR_PLUGIN, "Makes maps very dark if enabled. If disabled, the map doesn't have to be reloaded to start zombiemode", true, 0, true, 2);
ConVar zombie_teams("zombie_teams", "1", FCVAR_PLUGIN, "Sets teams Humans = CT; Zombies = T" );
ConVar zombie_respawn("zombie_respawn", "0", FCVAR_PLUGIN, "If enabled, respawn after a set amount of time." );
ConVar zombie_respawn_delay("zombie_respawn_delay", "1.0", FCVAR_PLUGIN, "Time before players are respawned in autorespawn mode", true, 0, false, 0);
ConVar zombie_respawn_protection("zombie_respawn_protection", "10.0", FCVAR_PLUGIN, "If enabled, players are protected for this amount of time after they respawn.", true, 0, true, 60);
ConVar zombie_respawn_as_zombie("zombie_respawn_as_zombie", "0.0", FCVAR_PLUGIN, "If enabled, when players spawn in they respawn as a Zombie.", true, 0, true, 1);
ConVar zombie_respawn_onconnect( "zombie_respawn_onconnect", "1.0", FCVAR_PLUGIN, "If enabled, allows players to spawn in once when they join regardless of zombie_respawn value.", true, 0.0, true, 1.0 );
ConVar zombie_respawn_onconnect_as_zombie( "zombie_respawn_onconnect_as_zombie", "1.0", FCVAR_PLUGIN, "If enabled, the first time a player spawns in they will be spawned as a zombie.", true, 0.0, true, 1.0 );

ConVar zombie_unlimited_ammo( "zombie_unlimited_ammo", "0.0", FCVAR_PLUGIN, "If enabled, ammo is unlimited.", true, 0, false, 0, CVar_CallBack );
ConVar zombie_talk( "zombie_talk", "1.0", FCVAR_PLUGIN, "If enabled, zombies can only voice with zombies and humans with humans.", true, 0, false, 0 );
ConVar zombie_suicide( "zombie_suicide", "1", FCVAR_PLUGIN, "When 1, disables players from suiciding.", true, 0, false, 0 );
ConVar zombie_changeteam_block( "zombie_changeteam_block", "1", FCVAR_PLUGIN, "When enabled, disables players from suiciding by switching teams.", true, 0, false, 0 );
ConVar zombie_suicide_text( "zombie_suicide_text", "Dont be an asshole.", FCVAR_PLUGIN, "Text for suiciding assholes.", true, 0, false, 0 );
ConVar zombie_startup( "zombie_startup", "0", FCVAR_PLUGIN, "If this is in your config.cfg ZombieMod attempts to auto load itself.", true, 0, false, 0 );
ConVar zombie_timer_max( "zombie_timer_max", "12.0", FCVAR_PLUGIN, "Maximum amount of seconds after round_freeze_end for first random Zombification.", true, 4, true, 100 );
ConVar zombie_timer_min( "zombie_timer_min", "4.0", FCVAR_PLUGIN, "Minimum amount of seconds after round_freeze_end for first random Zombification", true, 3, true, 100 );

ConVar zombie_kill_bonus( "zombie_kill_bonus", "0.0", FCVAR_PLUGIN, "Amount of extra frags awarded for killing a zombie.", true, 0, false, 0 );
ConVar zombie_headshot_count( "zombie_headshot_count", "6.0", FCVAR_PLUGIN, "Amount of headshots before a zombie's head comes off. 0 means on death only.", true, 0, false, 0 );
ConVar zombie_headshots( "zombie_headshots", "1.0", FCVAR_PLUGIN, "If enabled, zombies heads get blown off on headshot deaths.", true, 0.0, true, 1.0 );
ConVar zombie_allow_disable_nv( "zombie_allow_disable_nv", "0.0", FCVAR_PLUGIN, "When 1, allows zombies to disable their own night vision.", true, 0.0, true, 1.0 );


ConVar zombie_notices( "zombie_notices", "1.0", FCVAR_PLUGIN, "When enabled, shows an icon in the top right corner of a players screen when someone gets turned into a zombie.", true, 0.0, true, 1.0 );

ConVar zombie_fog( "zombie_fog", "1.0", FCVAR_PLUGIN, "When enabled, over-rides fog for all maps with a predefined Zombie-Fog.", true, 0.0, true, 1.0 );
ConVar zombie_fog_sky( "zombie_fog_sky", "1.0", FCVAR_PLUGIN, "When enabled, over-rides the sky for all maps so it corresponds with the fog.", true, 0.0, true, 1.0 );
ConVar zombie_fog_colour( "zombie_fog_colour", "176 192 202", FCVAR_PLUGIN, "Primary fog colour." );
ConVar zombie_fog_colour2( "zombie_fog_colour2", "206 216 222", FCVAR_PLUGIN, "Secondary fog colour." );
ConVar zombie_fog_start( "zombie_fog_start", "30", FCVAR_PLUGIN, "How close to a players Point-Of-View fog is rendered.", true, 10.0, false, 0.0 );
ConVar zombie_fog_end( "zombie_fog_end", "4000", FCVAR_PLUGIN, "How far from a players Point-Of-View that fog stops rendering.", true, 100.0, false, 0.0 );
ConVar zombie_fog_blend( "zombie_fog_blend", "1", FCVAR_PLUGIN, "Enables fog blending between colours..", true, 0.0, true, 1.0 );

ConVar zombie_effect( "zombie_effect", "1", FCVAR_PLUGIN, "Enables the cool effects when zombies are turned.", true, 0.0, true, 1.0 );
ConVar zombie_shake( "zombie_shake", "1", FCVAR_PLUGIN, "Enable screen shake on zombification.", true, 0.0, true, 1.0 );

ConVar zombie_jetpack( "zombie_jetpack", "0", FCVAR_PLUGIN, "Enables the JetPack for Zombies.", true, 0.0, true, 1.0 );
ConVar humans_jetpack( "humans_jetpack", "0", FCVAR_PLUGIN, "Enables the JetPack for Humans.", true, 0.0, true, 1.0 );
ConVar zombie_jetpack_timer( "zombie_jetpack_timer", "6", FCVAR_PLUGIN, "Amount of seconds zombies are allowed to use the JetPack in one round.", true, 0.0, true, 100.0 );
ConVar zombie_fog_sky_material( "zombie_fog_sky_material", "zombie_sky", FCVAR_PLUGIN, "The material to use for the sky." );
ConVar zombie_remove_radar( "zombie_remove_radar" , "0", FCVAR_PLUGIN, "When enabled, removes the radar for all players and disables their ability to redraw it. (Warning: All clients need to restart to enable radar again.)" );
ConVar zombie_stuckcheck_radius( "zombie_stuckcheck_radius", "35.0", FCVAR_PLUGIN, "The radius around a players location to check for other players on Zombification.", true, 1.0, true, 50.0 );

ConVar zombie_welcome_delay( "zombie_welcome_delay", "7", FCVAR_PLUGIN, "Delay in seconds after a player joins a server to send the welcome message. 0 to disable.", true, 0.0, true, 30.0 );
ConVar zombie_welcome_text( "zombie_welcome_text", "[ZOMBIE] This server is using the ZombieMod Server Plugin. Type !zhelp in chat for help.\nGet it at www.ZombieMod.com", FCVAR_PLUGIN, "The text to send to new players." );
ConVar zombie_help_url( "zombie_help_url", "http://www.zombiemod.com/zombie_help.php", FCVAR_PLUGIN, "The url to direct people to when they enter !zhelp in chat." );

ConVar zombie_undead_sound_enabled( "zombie_undead_sound_enabled", "1", FCVAR_PLUGIN, "When enabled, all players hear 'undead ambient sounds'.", true, 0.0, true, 1.0 );
ConVar zombie_undead_sound_volume( "zombie_undead_sound_volume", "0.7", FCVAR_PLUGIN, "Can be used to higher or lower the volume of the sound played. ", true, 0.0, true, 1.0 );
ConVar zombie_undead_sound( "zombie_undead_sound", "ambient/zombiemod/zombie_ambient.mp3", FCVAR_PLUGIN, "The sound to play to players, is precached and added to the client download list automatically." );
ConVar zombie_undead_sound_exclusions( "zombie_undead_sound_exclusions", "de_dust", FCVAR_PLUGIN, "A comma delimited string of maps with wich not to use zombie_undead_sound." );

ConVar zombie_headshots_only( "zombie_headshots_only", "0", FCVAR_PLUGIN, "When enabled, Zombies only take damage via headshots.", true, 0.0, true, 1.0 );

ConVar zombie_balancer_health_ratio( "zombie_balancer_health_ratio", "0", FCVAR_PLUGIN, "When enabled, after zombie_balancer_player_ratio amount of players are zombies, each additional new zombies health is reduced by this percentage. 0 Disables.", true, 0.0, true, 100.0 );
ConVar zombie_balancer_player_ratio( "zombie_balancer_player_ratio", "0", FCVAR_PLUGIN, "The ratio of players whom must be a zombie to start reducing new Zombies health. So when this percentage of the server is a zombie, new zombies health is reduced by the percentage of zombie_balancer_health_ratio.", true, 0.0, true, 100.0 );

ConVar zombie_balancer_type( "zombie_balancer_type", "1", FCVAR_PLUGIN, "If this is 1, zombie_balancer_player_ratio is the ratio of players who must be zombies before health is decreased. Otherwise, zombie_balancer_player_ratio is the count of Zombies ex:6.", true, 0.0, true, 1.0 );

ConVar zombie_balancer_min_health( "zombie_balancer_min_health", "100", FCVAR_PLUGIN, "This is the minimum health you want Zombies to ever end up with while using the balancer.", true, 1.0, true, 50000.0 );

ConVar zombie_zstuck_enabled( "zombie_zstuck_enabled", "1.0", FCVAR_PLUGIN, "Enables/Disabled the zstuck player command.", true, 0.0, true, 1.0 );

ConVar zombie_teleport_zombies_only( "zombie_teleport_zombies_only", "0", FCVAR_PLUGIN, "When enabled only allows !zstuck to be used by zombies.", true, 0.0, true, 1.0 );
ConVar zombie_teleportcount( "zombie_teleportcount", "3", FCVAR_PLUGIN, "Number of times a player can use zstuck per round.", true, 0.0, true, 100.0 );
ConVar zombie_damagelist( "zombie_damagelist", "1", FCVAR_PLUGIN, "If enabled keeps tracks of damage that players do to zombies and shows them a list on round end or zombification.", true, 0.0, true, 1.0 );
ConVar zombie_damagelist_time( "zombie_damagelist_time", "-1", FCVAR_PLUGIN, "Length of time to display damage list. -1 is forever.", true, -1, true, 30.0 );

ConVar zombie_vision_r( "zombie_vision_r", "0", FCVAR_PLUGIN, "Amount of red saturation to use for zombie vision.", true, 0.0, true, 255.0 );
ConVar zombie_vision_g( "zombie_vision_g", "160", FCVAR_PLUGIN, "Amount of green saturation to use for zombie vision.", true, 0.0, true, 255.0 );
ConVar zombie_vision_b( "zombie_vision_b", "0", FCVAR_PLUGIN, "Amount of blue saturation to use for zombie vision.", true, 0.0, true, 255.0 );
ConVar zombie_vision_a( "zombie_vision_a", "50", FCVAR_PLUGIN, "Alpha colour to use for zombie vision.", true, 0.0, true, 255.0 );

ConVar zombie_vision_timer( "zombie_vision_timer", "10.0", FCVAR_PLUGIN, "Time in seconds between checks to se if zombie_vision is still enabled on a client. If you notice it disabling often try reducing this number.", true, 0.0, true, 360 );

ConVar zombie_regen_timer( "zombie_regen_timer", "1.0", FCVAR_PLUGIN, "This is the time between health regeneration for zombies. 0 disables.", true, 0.0, true, 60.0 );
ConVar zombie_regen_health( "zombie_regen_health", "10", FCVAR_PLUGIN, "Amount of health to regen every health regen.", true, 0.0, true, 60.0 );

ConVar zombie_first_zombie( "zombie_first_zombie", "0", FCVAR_PLUGIN, "This cvar contains the user id to the first Zombie in a round.", true, 0.0, false, 0.0 );
ConVar zombie_show_health( "zombie_show_health", "1.0", FCVAR_PLUGIN, "If 1 sends a message to a zombie every time they zombify someone displaying their current health.", true, 0.0, true, 1.0 );

ConVar zombie_version( "zombie_version", ZOMBIE_VERSION, FCVAR_REPLICATED | FCVAR_SPONLY | FCVAR_NOTIFY, "ZombieMod Plugin version");

ConVar zombie_respawn_primary( "zombie_respawn_primary", "weapon_tmp", FCVAR_PLUGIN, "Primary weapon to give players who spawn after round start." );
ConVar zombie_respawn_secondary( "zombie_respawn_secondary", "weapon_usp", FCVAR_PLUGIN, "Secondary weapon to give players who spawn after round start." );
ConVar zombie_respawn_grenades( "zombie_respawn_grenades", "1.0", FCVAR_PLUGIN, "When enabled, gives players a frag grenade once their protection ends.", true, 0.0, true, 1.0 );

ConVar zombie_respawn_dontdrop_indexes( "zombie_respawn_dontdrop_indexes", "2,3", FCVAR_PLUGIN, "Comma delimited string to ignore weapon slots when dropping weapons after respawn." );
ConVar zombie_respawn_dropweapons( "zombie_respawn_dropweapons", "1.0", FCVAR_PLUGIN, "When enabled ZombieMod will drop the players weapons before giving new weapons.", true, 0.0, true, 1.0 );

ConVar zombie_classes( "zombie_classes", "1.0", FCVAR_PLUGIN, "When enabled, uses different zombie classes as defined in zombie_classes.cfg", true, 0.0, true, 1.0 );

ConVar zombie_vision_material( "zombie_vision_material", "zombie_view", FCVAR_PLUGIN, "Material to use for zombie_vision, located in vgui/hud/zombiemod/. If blank zombie_vision will be disabled." );

ConVar zombie_save_classlist( "zombie_save_classlist", "1.0", FCVAR_PLUGIN, "If enabled saves players class selections to a file on map change and reloads it the next time the server starts. Also server command zombie_writeclasses", true, 0.0, true, 1.0 );

ConVar zombie_health_percent( "zombie_health_percent", "100", FCVAR_PLUGIN, "Percentage of specified health to give zombie classes. Usefull for balancing maps. Only applies when zombie classes are enabled.", true, 1, true, 500.0 );
ConVar zombie_speed_percent( "zombie_speed_percent", "100", FCVAR_PLUGIN, "Percentage of specified speed to give zombie classes. Usefull for balancing maps. Only applies when zombie classes are enabled.", true, 1.0, true, 500.0 );
ConVar zombie_jump_height_percent( "zombie_jump_height_percent", "100", FCVAR_PLUGIN, "Percentage of specified jump height to give zombie classes. Usefull for balancing maps. Only applies when zombie classes are enabled.", true, 1.0, true, 500.0 );
ConVar zombie_knockback_percent( "zombie_knockback_percent", "100", FCVAR_PLUGIN, "Percentage of specified knockback to give zombie classes. Usefull for balancing maps. Only applies when zombie classes are enabled.", true, 1.0, true, 500.0 );

ConVar zombie_end_round_overlay( "zombie_end_round_overlay", "1024", FCVAR_PLUGIN, "The file to use for end_round overlay, set blank to disable. If file names are 1024_zombies and 1024_humans this setting should be 1024. The _zombies and _humans must be there." );
ConVar zombie_count( "zombie_count", "1.0", FCVAR_PLUGIN, "*EXPERIMENTAL* Amount of zombies to spawn at round_start, 0 for regular method. Defaults to ( player count - 2 ) if the total amount of players is less than this value.", true, 0.0, true, MAX_PLAYERS );
ConVar zombie_count_doublehp( "zombie_count_doublehp", "1.0", FCVAR_PLUGIN, "Type of double health to give first zombie(s): 0 disable double health, 1 first chosen gets double health, 2 all first zombies get double health.", true, 0.0, true, 3.0 );

ConVar zombie_kickstart_percent( "zombie_kickstart_percent", "70", FCVAR_PLUGIN, "Percent of players needed to kickstart zombie selection - 0 disables.", true, 0.0, true, 100.0 );
ConVar zombie_kickstart_timer( "zombie_kickstart_timer", "120", FCVAR_PLUGIN, "Time in seconds that kickstart is enabled after first vote.", true, 1.0, true, 360.0 );

ConVar zombie_vote_interval( "zombie_vote_interval", "60", FCVAR_PLUGIN, "Global time in seconds interval between votes.", true, 1.0, true, 360.0 );

ConVar zombie_weapon( "zombie_weapon", "zombie_claws_of_death", FCVAR_PLUGIN, "String to use for zombie's weapon." );
ConVar zombie_disconnect_protection( "zombie_disconnect_protection", "1.0", FCVAR_PLUGIN, "When enabled it will call the random zombie functionality as if it was round start when the last living zombie disconnects.", true, 0.0, true, 1.0 );

ConVar zombie_force_endround( "zombie_force_endround", "1.0", FCVAR_PLUGIN, "When enabled forces round end at mp_roundtime -4 seconds.", true, 0.0, true, 1.0 );

ConVar zombie_language_file( "zombie_language_file", "english.cfg", FCVAR_PLUGIN, "Language file to use, it must exist in /cfg/zombiemod/language" );
ConVar zombie_language_error( "zombie_language_error", "*** %s not found in language file ***", FCVAR_PLUGIN, "Error string to give when missing language key values." );

ConVar zombie_first_zombie_tele( "zombie_first_zombie_tele", "1.0", FCVAR_PLUGIN, "When enabled will teleport first zombie back to spawn.", true, 0.0, true, 1.0 );

ConVar zombie_startmoney( "zombie_startmoney", "16000", FCVAR_PLUGIN, "When 0 zombiemod only gives the cash cvar cash values for events as specified, when it's 1 it does what pre 2.0 Q did (16000 start), otherwise the value you specify.", true, 0.0, true, 16000 );
ConVar zombie_tk_money( "zombie_tk_money", "3000", FCVAR_PLUGIN, "The amount of cash to give a player that kills a zombie that's on their own team - i.e. what the server takes for tks.", true, 0.0, true, 16000.0 );
ConVar zombie_kill_money( "zombie_kill_money", "300", FCVAR_PLUGIN, "Amount of extra cash to give players that kill zombies (additional to 300 css default).", true, 0.0, true, 16000.0 );
ConVar zombie_zombie_money( "zombie_zombie_money", "500", FCVAR_PLUGIN, "Amount of extra cash to give zombies that zombify a human.", true, 0.0, true, 16000.0 );

ConVar zombie_dissolve_corpse( "zombie_dissolve_corpse", "1.0", FCVAR_PLUGIN, "When enabled uses a nice dissolve effect on ragdools.", true, 0.0, true, 1.0 );
ConVar zombie_dissolve_corpse_delay( "zombie_dissolve_corpse_delay", "1.0", FCVAR_PLUGIN, "Time to delay disolver effect on zombies death.", true, 0.0, true, 10.0 );

ConVar zombie_delete_dropped_weapons( "zombie_delete_dropped_weapons", "1.0", FCVAR_PLUGIN, "When enabled zombiemod will delete any weapons a player drops when they are zombified.", true, 0.0, true, 1.0 );

bool ZombieAccessor::RegisterConCommandBase(ConCommandBase *pVar)
{
	return META_REGCVAR( pVar );
}

CON_COMMAND( kill_round, "" )
{
	UTIL_TermRound( 3.0, CTs_PreventEscape );
}

CON_COMMAND( zombie_loadsettings, "Loads the ZombieMod configuration file.")
{
	m_Engine->ServerCommand( "exec zombiemod/zombiemod.cfg\n" );
}

CON_COMMAND(zombie_dump, "Dumps the map entity list to a file")
{
	DumpMapEnts();
}

CON_COMMAND( zombie_check, "" )
{
	g_ZombiePlugin.ShowZomb();
}

//CON_COMMAND( zombie_comm, "<name> - Executes command on a player." )
//{
//	if ( !zombie_enabled.GetBool() )
//	{
//		META_LOG( g_PLAPI, "%s: Zombie mode is not currently enabled\n", m_Engine->Cmd_Argv(0) );
//		return;
//	}
//	CPlayerMatchList matches(m_Engine->Cmd_Argv(1), true);
//	if ( matches.Count() < 1 ) 
//	{
//		META_LOG( g_PLAPI, "%s: Unable to find any living players that match %s\n", m_Engine->Cmd_Argv(0), m_Engine->Cmd_Argv(1));
//		//print(pEdict, "%s: Unable to find any living players that match %s\n", g_ZombiePlugin.m_Engine->Cmd_Argv(0), g_ZombiePlugin.m_Engine->Cmd_Argv(1));
//		return;
//	}
//	for ( int x = 0; x < matches.Count(); x++ )
//	{
//		edict_t* cEnt = matches.GetMatch(x); //->GetUnknown()->GetBaseEntity();
//		//int iPlayer = m_Engine->IndexOfEdict( cEnt );
//		//m_Engine->ClientCommand( cEnt, m_Engine->Cmd_Argv(2) );
//		//g_ZombiePlugin.ShowMOTD( iPlayer, "Welcome to ZombieMod", "Welcome to ZombieMod ;)", TYPE_TEXT, "toggle cl_restrict_server_commands 0" );
//		//g_ZombiePlugin.SetProtection( iPlayer, !g_Players[iPlayer].bProtected, false );
//		void *params[1];
//		params[0] = (void *)cEnt;
//		Timed_ConsGreet( params );
//	}
//	return;
//}

#ifdef _ICEPICK
	CON_COMMAND( zombie_knockback_player, "<id> <value> - Sets the player's knockback to specified value." )
	{
		_CRT_FLOAT fltval;

		if ( !zombie_enabled.GetBool() )
		{
			META_LOG( g_PLAPI, "%s: Zombie mode is not currently enabled\n", m_Engine->Cmd_Argv(0) );
			return;
		}
		CPlayerMatchList matches(m_Engine->Cmd_Argv(1), false);
		if ( matches.Count() < 1 ) 
		{
			META_LOG( g_PLAPI, "%s: Unable to find any players that match %s\n", m_Engine->Cmd_Argv(0), m_Engine->Cmd_Argv(1));
			return;
		}
		for ( int x = 0; x < matches.Count(); x++ )
		{
			CBasePlayer* pPlayer = (CBasePlayer*)matches.GetMatch(x)->GetUnknown()->GetBaseEntity();
			int iPlayer = m_Engine->IndexOfEdict( m_GameEnts->BaseEntityToEdict( pPlayer ) );
			_atoflt( &fltval, m_Engine->Cmd_Argv(2) );
			g_Players[iPlayer].fKnockback = fltval.f;
		}
		return;
	}

	CON_COMMAND( zombie_setmodel, "<id> <string model> - Sets the player's model to specified string." )
	{
		if ( !zombie_enabled.GetBool() )
		{
			META_LOG( g_PLAPI, "%s: Zombie mode is not currently enabled\n", m_Engine->Cmd_Argv(0) );
			return;
		}
		CPlayerMatchList matches(m_Engine->Cmd_Argv(1), true);
		if ( matches.Count() < 1 ) 
		{
			META_LOG( g_PLAPI, "%s: Unable to find any living players that match %s\n", m_Engine->Cmd_Argv(0), m_Engine->Cmd_Argv(1));
			return;
		}
		for ( int x = 0; x < matches.Count(); x++ )
		{
			CBasePlayer* pPlayer = (CBasePlayer*)matches.GetMatch(x)->GetUnknown()->GetBaseEntity();
			UTIL_SetModel( pPlayer, m_Engine->Cmd_Argv(2) );
		}
		return;
	}
#endif

CON_COMMAND( zombie_player, "<name> - Turns player into a zombie if zombie mode is on." )
{
	if ( !zombie_enabled.GetBool() )
	{
		META_LOG( g_PLAPI, "%s: Zombie mode is not currently enabled\n", m_Engine->Cmd_Argv(0) );
		return;
	}
	CPlayerMatchList matches(m_Engine->Cmd_Argv(1), true);
	if ( matches.Count() < 1 ) 
	{
		META_LOG( g_PLAPI, "%s: Unable to find any living players that match %s\n", m_Engine->Cmd_Argv(0), m_Engine->Cmd_Argv(1));
		//print(pEdict, "%s: Unable to find any living players that match %s\n", g_ZombiePlugin.m_Engine->Cmd_Argv(0), g_ZombiePlugin.m_Engine->Cmd_Argv(1));
		return;
	}
	for ( int x = 0; x < matches.Count(); x++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)matches.GetMatch(x)->GetUnknown()->GetBaseEntity();
		int iPlayer = m_Engine->IndexOfEdict( m_GameEnts->BaseEntityToEdict( pPlayer ) );
		if ( !g_Players[iPlayer].isZombie )
		{
			g_ZombiePlugin.MakeZombie( pPlayer, zombie_health.GetInt() ); // this command changes the argv/argc so we can't get the command name from it
			//print(pEdict, "nm_zombie_player: %s is now a zombie\n", /*engine->Cmd_Argv(0),*/ engine->GetClientConVarValue(pPlayer->entindex(), "name"));
			//logact(pEdict, "nm_zombie_player", "zombified %s", engine->GetClientConVarValue(pPlayer->entindex(), "name"));
			META_LOG( g_PLAPI, "%s is now a zombie.", m_Engine->GetClientConVarValue( iPlayer, "name") );
		}
	}
	return;
}

CON_COMMAND( zombie_mode, "- Toggles zombie mode" )
{
	if ( m_Engine->Cmd_Argc() == 1 )
	{
 		META_LOG( g_PLAPI, "Zombie Mode is currently %s.", ( zombie_enabled.GetBool() ? "enabled" : "disabled" ) );
		return;
	}
	if ( FStrEq( m_Engine->Cmd_Argv(1), "1" ) )
	{
		if ( !zombie_enabled.GetBool() )
		{
			g_ZombiePlugin.Hud_Print( NULL, "Zombie mode enabled." );
			g_ZombiePlugin.ZombieOn();
		}
		else
		{
			META_LOG( g_PLAPI, "Zombie Mod is already enabled." );
		}
	}
	else if ( FStrEq( m_Engine->Cmd_Argv(1), "0" ) )
	{
		if ( zombie_enabled.GetBool() )
		{
			g_ZombiePlugin.Hud_Print( NULL, "Zombie mode disabled." );
			g_ZombiePlugin.ZombieOff();
		}
		else
		{
			META_LOG( g_PLAPI, "Zombie Mod is already disabled." );
		}
	}
	else
	{
		META_LOG( g_PLAPI, "Usage: zombie_mode 1/0 Enable or disable Zombie Mod." );
	}
    return;
}

CON_COMMAND( zombie_test, "" )
{
	META_LOG ( g_PLAPI, g_ZombiePlugin.GetLang("autospawn_secs") );
	return;
}

CON_COMMAND(zombie_restrict, "<a/t/c/w> <weapon> [amount] - Restricts a weapon for all players, terrorist, counter-terrorist, or winning team. Optional last parameter is how many weapons of the type the team can hold.")
{
	g_ZombiePlugin.RestrictWeapon( m_Engine->Cmd_Argv(1), m_Engine->Cmd_Argv(2), m_Engine->Cmd_Argv(3), m_Engine->Cmd_Argv(0) );
    return;
}

CON_COMMAND( zombie_list_restrictions, "Lists all current weapon restrictions." )
{
	int x = 0;
	META_CONPRINT( "\nCurrent Weapon Restrictions\n==========================\n" );
	for ( x = 0; x < CSW_MAX; x++ )
	{
		META_CONPRINTF( "%d. %d %d\n", ( x ), g_RestrictCT[x], g_RestrictT[x] );
	}
	META_CONPRINT( "==========================\n" );
}

CON_COMMAND( zombie_listclasses, "List all currently loaded zombie classes." )
{
	int x;
	META_CONPRINT( "\nLoaded Zombie Classes (With percentages applied)\n==========================\n" );
	for ( x = 0; x <= g_iZombieClasses; x++ )
	{
		#define DOMATH( iVal, iVar, iPercent ) \
			iVal = ( (float)iVar / 100 ) * (float)iPercent;

		META_CONPRINTF( "\nName: %s\n", g_ZombieClasses[x].sClassname.c_str() );
		META_CONPRINTF( "Model: %s\n", g_ZombieClasses[x].sModel.c_str() );

		int iTmp = 0;
		//int iTmp = ( g_ZombieClasses[x].iHealth / 100 ) * zombie_health_percent.GetInt();
		DOMATH( iTmp, g_ZombieClasses[x].iHealth, zombie_health_percent.GetInt() );
		META_CONPRINTF( "Health: %d (%d%%%)\n", iTmp, zombie_health_percent.GetInt() );
		
		//iTmp = ( (float)g_ZombieClasses[x].iSpeed / 100 ) * (float)zombie_speed_percent.GetInt();
		DOMATH( iTmp, g_ZombieClasses[x].iSpeed, zombie_speed_percent.GetInt() );
		META_CONPRINTF( "Speed: %d (%d%%%)\n", iTmp, zombie_speed_percent.GetInt() );
		
		//iTmp = ( g_ZombieClasses[x].iJumpHeight / 100 ) * zombie_jump_height_percent.GetInt();
		DOMATH( iTmp, g_ZombieClasses[x].iJumpHeight, zombie_jump_height_percent.GetInt() );
		META_CONPRINTF( "Jump Height: %d (%d%%%)\n", iTmp, zombie_jump_height_percent.GetInt() );
		
		META_CONPRINTF( "Headhosts Required: %d\n", g_ZombieClasses[x].iHeadshots );

		float fTmp = ( g_ZombieClasses[x].fKnockback / 100 ) * zombie_knockback_percent.GetInt();
		META_CONPRINTF( "Knockback: %.2f (%d%%)\n\n", fTmp, zombie_knockback_percent.GetInt() );
	}
	if ( g_iZombieClasses == -1 )
	{
		META_CONPRINT( "Sorry, zombie classes are currently disabled.\n" );
	}
}

CON_COMMAND( zombie_clear_classlist, " - List all currently loaded zombie classes." )
{
	kvPlayerClasses->deleteThis();
	kvPlayerClasses = NULL;
	kvPlayerClasses = new KeyValues( "ClientClasses" );
	kvPlayerClasses->SaveToFile( g_ZombiePlugin.m_FileSystem, "cfg/zombiemod/player_classes.kv", "MOD" );
	META_CONPRINTF( "[ZOMBIE] Cleared player class history.\n" );
	return;
}

CON_COMMAND( zombie_class_count, " - Displays current count of players classes saved to disk." )
{
	if ( zombie_save_classlist.GetBool() )
	{
		KeyValues *p;
		int iCount = 0;
		p = kvPlayerClasses->GetFirstSubKey();
		while ( p )
		{
			iCount++;
			p = p->GetNextKey();
		}

		META_CONPRINTF( "[ZOMBIE] Current count of saved player classes: %d\n", iCount );
	}
	else
	{
		META_CONPRINT( "Sorry, zombie classes are currently disabled.\n" );
	}
	return;
}

CON_COMMAND( zombie_writeclasses, " - Writes the current list of player class selections server have stored to file." )
{
	if ( zombie_save_classlist.GetBool() )
	{
		if ( kvPlayerClasses->GetFirstSubKey() )
		{
			kvPlayerClasses->SaveToFile( g_ZombiePlugin.m_FileSystem, "cfg/zombiemod/player_classes.kv", "MOD" );
		}
		m_Engine->ServerCommand( "zombie_class_count\n" );
		META_CONPRINTF( "[ZOMBIE] Class Selection List Saved.\n" );
	}
	else
	{
		META_CONPRINT( "Sorry, zombie classes are currently disabled.\n" );
	}

}

CON_COMMAND( zombie_loadclasses, " - Loads the class list from the file." )
{
	g_ZombiePlugin.LoadZombieClasses();
}