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
#include "VFunc.h"
#include "CTakeDamageInfo.h"
#include "mathlib.h"
#include "ZMClass.h"

ZombieAccessor					g_Accessor;

extern CTeam					*teamCT;
extern CTeam					*teamT;
extern ArrayLengthSendProxyFn	TeamCount;
extern PlayerInfo_t				g_Players[65];
extern BuyMenuInfo_t			g_tBuyMenu;
extern bool						g_bBuyMenu;

extern int						g_RestrictT[CSW_MAX];
extern int						g_RestrictCT[CSW_MAX];
extern int						g_RestrictW[CSW_MAX];

extern bool						g_bZombieClasses;
//extern int						g_iZombieClasses;
//extern ZombieClasses_t			g_ZombieClasses[100];
extern KeyValues				*kvPlayerClasses;
extern KeyValues				*kvPlayerVision;

#define GETCLASS( iClass ) \
	ZombieClass *zClass; \
	zClass = m_ClassManager.GetClass( iClass );

#define ShortCvar "zm_"
#define LongCvar "zombie_"
#define CreateConvar( sName, sDefault, lFlags, sHelp, bMin, fMin, bMax, fMax, oCallback ) \
	ConVar zm_##sName(ShortCvar#sName, sDefault, lFlags, sHelp, bMin, fMin, bMax, fMax,oCallback ); \
	ConVar zombie_##sName(LongCvar#sName, sDefault, lFlags, sHelp, bMin, fMin, bMax, fMax, oCallback );
		 
CreateConvar( version, ZOMBIE_VERSION, (FCVAR_REPLICATED | FCVAR_SPONLY | FCVAR_NOTIFY), "ZombieMod Plugin version", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( model_file, "cfg/zombiemod/models.cfg", FCVAR_NONE, "File with list of models to use for zombie mod", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( download_file, "cfg/zombiemod/downloads.cfg", FCVAR_NONE, "File with list of materials. Clients download everything in this list when they connect.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( sound_rand, "50", FCVAR_NONE, "How often zombies play zombie sounds. The lower the value, the more often they emit a sound.", true, 1, false, 0, CVar_CallBack );
CreateConvar( enabled, "0", (FCVAR_REPLICATED | FCVAR_SPONLY | FCVAR_NOTIFY), "If non-zero, zombie mode is enabled (DO NOT SET THIS DIRECTLY!)", true, 0, true, 1, CVar_CallBack  );
CreateConvar( health, "2500", FCVAR_NONE, "Health that zombies get. First zombie is this times two.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( speed, "325.0", FCVAR_NONE, "The speed that zombies travel at when walking.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( speed_crouch, "325.0", FCVAR_NONE, "The speed that zombies travel at when crouching.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( speed_run, "325.0", FCVAR_NONE, "The speed that zombies travel at when running.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( fov, "125", FCVAR_NONE, "Field of vision of the zombies. Normal human FOV is 90.", true, 40, true, 160, CVar_CallBack );
CreateConvar( knockback, "4", FCVAR_NONE, "The knockback multiplier of zombies. Set to 0 to disable knockback", true, 0, false, 0, CVar_CallBack );
CreateConvar( restrictions, "rifles m249 flash", FCVAR_NONE, "Space separated list of guns that are restricted during zombie mode. Takes effect immediately on change.", false, 0.0f, false, 0.0f, CVar_CallBack );

// TODO
CreateConvar( dark, "0", FCVAR_NONE, "Makes maps very dark if enabled. If disabled, the map doesn't have to be reloaded to start zombiemode", true, 0, true, 2, CVar_CallBack );

CreateConvar( teams, "1", FCVAR_NONE, "Sets teams Humans = CT; Zombies = T", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( respawn, "0", FCVAR_NONE, "If enabled, respawn after a set amount of time.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( respawn_delay, "1.0", FCVAR_NONE, "Time before players are respawned in autorespawn mode", true, 0, false, 0, CVar_CallBack );
CreateConvar( respawn_protection, "10.0", FCVAR_NONE, "If enabled, players are protected for this amount of time after they respawn.", true, 0, true, 60, CVar_CallBack );
CreateConvar( respawn_as_zombie, "0.0", FCVAR_NONE, "If enabled, when players spawn in they respawn as a Zombie.", true, 0, true, 1, CVar_CallBack );
CreateConvar( respawn_onconnect, "1.0", FCVAR_NONE, "If enabled, allows players to spawn in once when they join regardless of respawn value.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( respawn_onconnect_as_zombie, "1.0", FCVAR_NONE, "If enabled, the first time a player spawns in they will be spawned as a zombie.", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( unlimited_ammo, "0.0", FCVAR_NONE, "If enabled, ammo is unlimited.", true, 0, false, 0, CVar_CallBack  );
CreateConvar( talk, "1.0", FCVAR_NONE, "If enabled, zombies can only voice with zombies and humans with humans.", true, 0, false, 0 , CVar_CallBack );
CreateConvar( suicide, "1", FCVAR_NONE, "When 1, disables players from suiciding.", true, 0, false, 0 , CVar_CallBack );
CreateConvar( changeteam_block, "1", FCVAR_NONE, "When enabled, disables players from suiciding by switching teams.", true, 0, false, 0 , CVar_CallBack );
CreateConvar( suicide_text, "Dont be an asshole.", FCVAR_NONE, "Text for suiciding assholes.", true, 0, false, 0 , CVar_CallBack );
CreateConvar( startup, "0", FCVAR_NONE, "If this is in your config.cfg ZombieMod attempts to auto load itself.", true, 0, false, 0 , CVar_CallBack );
CreateConvar( timer_max, "12.0", FCVAR_NONE, "Maximum amount of seconds after round_freeze_end for first random Zombification.", true, 4, true, 100 , CVar_CallBack );
CreateConvar( timer_min, "4.0", FCVAR_NONE, "Minimum amount of seconds after round_freeze_end for first random Zombification", true, 3, true, 100 , CVar_CallBack );

CreateConvar( kill_bonus, "0.0", FCVAR_NONE, "Amount of extra frags awarded for killing a zombie.", true, 0, false, 0 , CVar_CallBack );
CreateConvar( headshot_count, "6.0", FCVAR_NONE, "Amount of headshots before a zombie's head comes off. 0 means on death only.", true, 0, false, 0 , CVar_CallBack );
CreateConvar( headshots, "1.0", FCVAR_NONE, "If enabled, zombies heads get blown off on headshot deaths.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( allow_disable_nv, "0.0", FCVAR_NONE, "When 1, allows zombies to disable their own night vision.", true, 0.0, true, 1.0 , CVar_CallBack );


CreateConvar( notices, "1.0", FCVAR_NONE, "When enabled, shows an icon in the top right corner of a players screen when someone gets turned into a zombie.", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( fog, "0", FCVAR_NONE, "When enabled, over-rides fog for all maps with a predefined Zombie-Fog.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( fog_sky, "0", FCVAR_NONE, "When enabled, over-rides the sky for all maps so it corresponds with the fog.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( fog_colour, "176 192 202", FCVAR_NONE, "Primary fog colour.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( fog_colour2, "206 216 222", FCVAR_NONE, "Secondary fog colour.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( fog_start, "30", FCVAR_NONE, "How close to a players Point-Of-View fog is rendered.", true, 10.0, false, 0.0 , CVar_CallBack );
CreateConvar( fog_end, "4000", FCVAR_NONE, "How far from a players Point-Of-View that fog stops rendering.", true, 100.0, false, 0.0 , CVar_CallBack );
CreateConvar( fog_blend, "1", FCVAR_NONE, "Enables fog blending between colours..", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( effect, "1", FCVAR_NONE, "Enables the cool effects when zombies are turned.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( shake, "1", FCVAR_NONE, "Enable screen shake on zombification.", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( jetpack, "0", FCVAR_NONE, "Enables the JetPack for Zombies.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( humans_jetpack, "0", FCVAR_NONE, "Enables the JetPack for Humans.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( jetpack_timer, "6", FCVAR_NONE, "Amount of seconds zombies are allowed to use the JetPack in one round.", true, 0.0, true, 100.0 , CVar_CallBack );
CreateConvar( fog_sky_material, "sky", FCVAR_NONE, "The material to use for the sky.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( remove_radar, "0", FCVAR_NONE, "When enabled, removes the radar for all players and disables their ability to redraw it. (Warning: All clients need to restart to enable radar again.)", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( stuckcheck_radius, "35.0", FCVAR_NONE, "The radius around a players location to check for other players on Zombification.", true, 1.0, true, 50.0 , CVar_CallBack );

CreateConvar( welcome_delay, "7", FCVAR_NONE, "Delay in seconds after a player joins a server to send the welcome message. 0 to disable.", true, 0.0, true, 30.0 , CVar_CallBack );
CreateConvar( welcome_text, "[ZOMBIE] This server is using the ZombieMod Server Plugin. Type !zhelp in chat for help.\nGet it at www.ZombieMod.com", FCVAR_NONE, "The text to send to new players.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( help_url, "http://www.zombiemod.com/help.php", FCVAR_NONE, "The url to direct people to when they enter !zhelp in chat.", false, 0.0f, false, 0.0f, CVar_CallBack );

CreateConvar( undead_sound_enabled, "1", FCVAR_NONE, "When enabled, all players hear 'undead ambient sounds'.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( undead_sound_volume, "0.7", FCVAR_NONE, "Can be used to higher or lower the volume of the sound played. ", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( undead_sound, "ambient/zombiemod/ambient.mp3", FCVAR_NONE, "The sound to play to players, is precached and added to the client download list automatically.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( undead_sound_exclusions, "de_dust", FCVAR_NONE, "A comma delimited string of maps with wich not to use undead_sound.", false, 0.0f, false, 0.0f, CVar_CallBack );

CreateConvar( headshots_only, "0", FCVAR_NONE, "When enabled, Zombies only take damage via headshots.", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( balancer_health_ratio, "0", FCVAR_NONE, "When enabled, after balancer_player_ratio amount of players are zombies, each additional new zombies health is reduced by this percentage. 0 Disables.", true, 0.0, true, 100.0 , CVar_CallBack );
CreateConvar( balancer_player_ratio, "0", FCVAR_NONE, "The ratio of players whom must be a zombie to start reducing new Zombies health. So when this percentage of the server is a zombie, new zombies health is reduced by the percentage of balancer_health_ratio.", true, 0.0, true, 100.0 , CVar_CallBack );

CreateConvar( balancer_type, "1", FCVAR_NONE, "If this is 1, balancer_player_ratio is the ratio of players who must be zombies before health is decreased. Otherwise, balancer_player_ratio is the count of Zombies ex:6.", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( balancer_min_health, "100", FCVAR_NONE, "This is the minimum health you want Zombies to ever end up with while using the balancer.", true, 1.0, true, 50000.0 , CVar_CallBack );

CreateConvar( zstuck_enabled, "1.0", FCVAR_NONE, "Enables/Disabled the zstuck player command.", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( teleport_zombies_only, "0", FCVAR_NONE, "When enabled only allows !zstuck to be used by zombies.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( teleportcount, "3", FCVAR_NONE, "Number of times a player can use zstuck per round.", true, 0.0, true, 100.0 , CVar_CallBack );
CreateConvar( damagelist, "1", FCVAR_NONE, "If enabled keeps tracks of damage that players do to zombies and shows them a list on round end or zombification.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( damagelist_time, "-1", FCVAR_NONE, "Length of time to display damage list. -1 is forever.", true, -1, true, 30.0 , CVar_CallBack );

CreateConvar( vision_r, "0", FCVAR_NONE, "Amount of red saturation to use for zombie vision.", true, 0.0, true, 255.0 , CVar_CallBack );
CreateConvar( vision_g, "160", FCVAR_NONE, "Amount of green saturation to use for zombie vision.", true, 0.0, true, 255.0 , CVar_CallBack );
CreateConvar( vision_b, "0", FCVAR_NONE, "Amount of blue saturation to use for zombie vision.", true, 0.0, true, 255.0 , CVar_CallBack );
CreateConvar( vision_a, "50", FCVAR_NONE, "Alpha colour to use for zombie vision.", true, 0.0, true, 255.0 , CVar_CallBack );

CreateConvar( vision_timer, "10.0", FCVAR_NONE, "Time in seconds between checks to se if vision is still enabled on a client. If you notice it disabling often try reducing this number.", true, 0.0, true, 360 , CVar_CallBack );

CreateConvar( regen_timer, "1.0", FCVAR_NONE, "This is the time between health regeneration for zombies. 0 disables.", true, 0.0, true, 60.0 , CVar_CallBack );
CreateConvar( regen_health, "10", FCVAR_NONE, "Amount of health to regen every health regen.", true, 0.0, true, 60.0 , CVar_CallBack );

CreateConvar( first_zombie, "0", FCVAR_NONE, "This cvar contains the user id to the first Zombie in a round.", true, 0.0, false, 0.0 , CVar_CallBack );
CreateConvar( show_health, "1.0", FCVAR_NONE, "When 1 sends a custom message to a zombie every time they zombify someone displaying their current health, when 2 it uses HintText.", true, 0.0, true, 2.0 , CVar_CallBack );
CreateConvar( show_attacker_health, "1.0", FCVAR_NONE, "When 1 sends a custom message to the attacker every time they hurt someone displaying the victims current health, when 2 it uses HintText.", true, 0.0, true, 2.0 , CVar_CallBack );

CreateConvar( respawn_primary, "weapon_tmp", FCVAR_NONE, "Primary weapon to give players who spawn after round start.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( respawn_secondary, "weapon_usp", FCVAR_NONE, "Secondary weapon to give players who spawn after round start.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( respawn_grenades, "1.0", FCVAR_NONE, "When enabled, gives players a frag grenade once their protection ends.", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( respawn_dontdrop_indexes, "2,3", FCVAR_NONE, "Comma delimited string to ignore weapon slots when dropping weapons after respawn.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( respawn_dropweapons, "1.0", FCVAR_NONE, "When enabled ZombieMod will drop the players weapons before giving new weapons.", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( classes, "1.0", FCVAR_NONE, "When enabled, uses different zombie classes as defined in classes.cfg", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( classes_random, "0.0", FCVAR_NONE, "When 1, random class chosen each time a zombie dies. When 2, random zombie chosen each time a player connects. When 3 uses same behaviour as 1 but until player choses class themselves.", true, 0.0, true, 3.0 , CVar_CallBack );

CreateConvar( vision_material, "view", FCVAR_NONE, "Material to use for vision, located in vgui/hud/zombiemod/. If blank vision will be disabled.", false, 0.0f, false, 0.0f, CVar_CallBack );

CreateConvar( save_classlist, "1.0", FCVAR_NONE, "If enabled saves players class selections to a file on map change and reloads it the next time the server starts. Also server command writeclasses", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( health_percent, "100", FCVAR_NONE, "Percentage of specified health to give zombie classes. Usefull for balancing maps. Only applies when zombie classes are enabled.", true, 1, true, 500.0 , CVar_CallBack );
CreateConvar( speed_percent, "100", FCVAR_NONE, "Percentage of specified speed to give zombie classes. Usefull for balancing maps. Only applies when zombie classes are enabled.", true, 1.0, true, 500.0 , CVar_CallBack );
CreateConvar( jump_height_percent, "100", FCVAR_NONE, "Percentage of specified jump height to give zombie classes. Usefull for balancing maps. Only applies when zombie classes are enabled.", true, 1.0, true, 500.0 , CVar_CallBack );
CreateConvar( knockback_percent, "100", FCVAR_NONE, "Percentage of specified knockback to give zombie classes. Usefull for balancing maps. Only applies when zombie classes are enabled.", true, 1.0, true, 500.0 , CVar_CallBack );

CreateConvar( end_round_overlay, "1024", FCVAR_NONE, "The file to use for end_round overlay, set blank to disable. If file names are 1024_zombies and 1024_humans this setting should be 1024. The _zombies and _humans must be there.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( count, "1.0", FCVAR_NONE, "*EXPERIMENTAL* Amount of zombies to spawn at round_start, 0 for regular method. Defaults to ( player count - 2 ) if the total amount of players is less than this value.", true, 0.0, true, MAX_PLAYERS , CVar_CallBack );
CreateConvar( count_doublehp, "1.0", FCVAR_NONE, "Type of double health to give first zombie(s): 0 disable double health, 1 first chosen gets double health, 2 all first zombies get double health.", true, 0.0, true, 3.0 , CVar_CallBack );
CreateConvar( count_min, "2.0", FCVAR_NONE, "*EXPERIMENTAL* Minimum number of random zombies. Overrides count. Defaults to ( player count - 2 ) if the total amount of players is less than the resulting value. 0 or greater than max disables.", true, 0.0, true, MAX_PLAYERS , CVar_CallBack );
CreateConvar( count_max, "4.0", FCVAR_NONE, "*EXPERIMENTAL* Maximum number of random zombies. Overrides count. Defaults to ( player count - 2 ) if the total amount of players is less than the resulting value. 0 or less than min disables.", true, 0.0, true, MAX_PLAYERS , CVar_CallBack );

CreateConvar( kickstart_percent, "70", FCVAR_NONE, "Percent of players needed to kickstart zombie selection - 0 disables.", true, 0.0, true, 100.0 , CVar_CallBack );
CreateConvar( kickstart_timer, "120", FCVAR_NONE, "Time in seconds that kickstart is enabled after first vote.", true, 1.0, true, 360.0 , CVar_CallBack );

CreateConvar( vote_interval, "60", FCVAR_NONE, "Global time in seconds interval between votes.", true, 1.0, true, 360.0 , CVar_CallBack );

CreateConvar( weapon, "claws_of_death", FCVAR_NONE, "String to use for zombie's weapon.", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( disconnect_protection, "1.0", FCVAR_NONE, "When enabled it will call the random zombie functionality as if it was round start when the last living zombie disconnects.", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( force_endround, "1.0", FCVAR_NONE, "When enabled forces round end at mp_roundtime -4 seconds.", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( language_file, "english.cfg", FCVAR_NONE, "Language file to use, it must exist in /cfg/zombiemod/language", false, 0.0f, false, 0.0f, CVar_CallBack );
CreateConvar( language_error, "*** %s not found in language file ***", FCVAR_NONE, "Error string to give when missing language key values.", false, 0.0f, false, 0.0f, CVar_CallBack );

CreateConvar( first_zombie_tele, "1.0", FCVAR_NONE, "When enabled will teleport first zombie back to spawn.", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( startmoney, "16000", FCVAR_NONE, "When 0 zombiemod only gives the cash cvar cash values for events as specified, when it's 1 it does what pre 2.0 Q did (16000 start), otherwise the value you specify.", true, 0.0, true, 16000 , CVar_CallBack );
CreateConvar( tk_money, "3000", FCVAR_NONE, "The amount of cash to give a player that kills a zombie that's on their own team - i.e. what the server takes for tks.", true, 0.0, true, 16000.0 , CVar_CallBack );
CreateConvar( kill_money, "300", FCVAR_NONE, "Amount of extra cash to give players that kill zombies (additional to 300 css default).", true, 0.0, true, 16000.0 , CVar_CallBack );
CreateConvar( money, "500", FCVAR_NONE, "Amount of extra cash to give zombies that zombify a human.", true, 0.0, true, 16000.0 , CVar_CallBack );

CreateConvar( dissolve_corpse, "1.0", FCVAR_NONE, "When enabled uses a nice dissolve effect on ragdools.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( dissolve_corpse_delay, "1.0", FCVAR_NONE, "Time to delay disolver effect on zombies death.", true, 0.0, true, 10.0 , CVar_CallBack );

CreateConvar( delete_dropped_weapons, "1.0", FCVAR_NONE, "When enabled zombiemod will delete any weapons a player drops when they are zombified.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( grenade_damage_multiplier, "2.0", FCVAR_NONE, "When enabled grenades do damage times this number. 1 disables", true, 0.0, true, 100.0 , CVar_CallBack );
CreateConvar( grenade_knockback_multiplier, "2.0", FCVAR_NONE, "When enabled grenades have knockback times this number. 1 disables.", true, 0.0, true, 100.0 , CVar_CallBack );
CreateConvar( health_bonus, "50", FCVAR_NONE, "How much health to give zombies as a bonus for zombification. 0 disables.", true, 0.0, true, 5000.0 , CVar_CallBack );

CreateConvar( class_url, "http://www.c0ld.net/zombiemod/classes.php", FCVAR_NONE, "The url to direct people to when they enter !zclasses in chat.", false, 0.0f, false, 0.0f, CVar_CallBack );

CreateConvar( allow_picked_up_weapons, "1.0", FCVAR_NONE, "When this is enabled it allows you to pick up any weapon on the map but will still not allow you to buy them.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( buymenu, "1.0", FCVAR_NONE, "Enable or disable the buy menu", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( buymenu_zone_only, "0.0", FCVAR_NONE, "When enabled players can only receive weapons from buy menu while in a buy zone.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( buymenu_timelimit, "1.0", FCVAR_NONE, "Enable or disable the buy menu timelimit, when enabled players can only buy weapons during mp_buytime.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( buymenu_roundstart, "1.0", FCVAR_NONE, "Enable or disable the buy menu at round start.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( falldamage, "1.0", FCVAR_NONE, "When 0 and classes are disabled zombies receive no damage from falling.", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( ddos_protect, "1.0", FCVAR_NONE, "1 = Enable DoS Protect (default), 0 = Disable DoS Protect", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( remove_objectives, "func_bomb_target,func_hostage_rescue,hostage_entity,c4", FCVAR_NONE, "Comma seperated list of entities to remove at round_start if they exist.", false, 0.0f, false, 0.0f, CVar_CallBack );

CreateConvar( display_class, "1.0", FCVAR_NONE, "When enabled it displays class information in the client console.", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( napalm_enabled, "1.0", FCVAR_NONE, "When enabled turns grenades into napalms which ignite victims.", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( napalm_damage, "0.0", FCVAR_NONE, "Anything above 0 changes the grenade damage applied to zombies.", true, 0.0, true, 1000.0 , CVar_CallBack );
CreateConvar( napalm_radius, "0.0", FCVAR_NONE, "Anything above 0 changes the grenade damage radius.", true, 0.0, true, 1000.0 , CVar_CallBack );
CreateConvar( napalm_time, "30.0", FCVAR_NONE, "Time player is on fire after being napalmed.", true, 1.0, true, 1000.0 , CVar_CallBack );

CreateConvar( save_visionlist, "1.0", FCVAR_NONE, "If enabled saves players zombie vision selections to a file on map change and reloads it the next time the server starts. Also server command writevision", true, 0.0, true, 1.0 , CVar_CallBack );

CreateConvar( ffa_enabled, "1.0", FCVAR_NONE, "Enables CSS:DM styled FFA which is always enabled by default but after every update requires an offset file update so may need to be disabled.", true, 0.0, true, 1.0, CVar_CallBack  );

CreateConvar( jump_height, "200", FCVAR_NONE, "Additonal jump height for zombies when classes are disabled.", true, 0.0, true, 1000.0 , CVar_CallBack );
CreateConvar( game_description, "1", FCVAR_NONE, "When enabled changes the game description to ZombieMod (may interfere with SourceMod!)", true, 0.0, true, 1.0 , CVar_CallBack );
CreateConvar( game_description_format, "!d! !v!", FCVAR_NONE, "Format for game description, replacement vars so far are !d! and !v! - d is ZombieMod and v is the Version number.", false, 0.0f, false, 0.0f, CVar_CallBack );

//CreateConvar( humans_win_sounds, "survivor_win3 ,survivor_win1, survivor_win2", FCVAR_NONE, "Comma delimited list of sounds to play at random when humans win, these should be located in cstrike/sound/radio/zombiemod and are automatically precached.", false, 0.0, false, 0.0, CVar_CallBack );
//CreateConvar( zombies_win_sounds, "zombie_win1,zombie_win2,zombie_win3", FCVAR_NONE, "Comma delimited list of sounds to play at random when zombies win, these should be located in cstrike/sound/radio/zombiemod and are automatically precached.", false, 0.0, false, 0.0, CVar_CallBack );

#ifdef _PAID1
	ConVar zombie_knife_knockback( "zombie_knife_knockback", "1.0", FCVAR_NONE, "1 = Knockback equivelant to knife damage (2000) (default), 0 = Knockback equivelant to original knife damage.", true, 0.0, true, 1.0 );
#endif

#undef GetClassName

/*
CON_COMMAND( test, "" )
{
	CPlayerMatchList matches( args.Arg(1), true );
	if ( matches.Count() < 1 ) 
	{
		META_LOG( g_PLAPI, "%s: Unable to find any living players that match %s\n", args.Arg(0), args.Arg(1) );//args.Arg(0), args.Arg(1));
		return;
	}
	for ( int x = 0; x < matches.Count(); x++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)matches.GetMatch(x)->GetUnknown()->GetBaseEntity();
		int iPlayer = m_Engine->IndexOfEdict( m_GameEnts->BaseEntityToEdict( pPlayer ) );
		if( atoi( args.Arg(2) ) == 1 )
		{
			CBasePlayer_ApplyAbsVelocityImpulse( pPlayer, Vector( 250, 250, 250 ) );
			META_LOG( g_PLAPI, "Velocity..." );
		}
		else
		{
			g_ZombiePlugin.SetProtection( iPlayer );
			META_LOG( g_PLAPI, "Setting protection..." );
		}
	}
	return;
}

CON_COMMAND( testz, "" )
{
	char name[128] = "";
	int sizereturn = 0;
	bool boolrtn = false;
	int iTo = atoi( args.Arg( 1 ) );
	for( int x=1; x < iTo; x++ )
	{
		boolrtn = m_ServerDll->GetUserMessageInfo( x, name, 128, sizereturn ); 
		META_LOG( g_PLAPI, name );
	}
	return;
}

CON_COMMAND( kill_round, "" )
{
	int iWhy = atoi( args.Arg( 1 ) );
	UTIL_TermRound( 3.0, iWhy );
}


CON_COMMAND( run_set, "" )
{
	int iOffset = atoi( args.Arg(1) );
	if ( iOffset <= 0 )
	{
		META_LOG( g_PLAPI, "Please enter a valid offset." );
	}
	else
	{
		g_ZombiePlugin.ChangeRunHook( iOffset );
	}
}
*/

CON_COMMAND( zombie_listweaps, "" )
{
	for ( int x = 0; x < WEAPONINFO_COUNT; x++ )
	{
        CCSWeaponInfo *info = (CCSWeaponInfo *)g_GetFileWeaponInfoFromHandle(x);
		edict_t* pEdict = m_GameEnts->BaseEntityToEdict( (CBaseEntity *) info );

		if ( !info || !info->szClassName || !strlen( info->szClassName ) )
		{
			continue;
		}
		META_LOG( g_PLAPI, "%i: Weapon name [%s] Max Clip [%i] Default Clip [%i]", x, info->szClassName, info->iMaxClip1, info->iDefaultClip1 );
	}
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

#ifdef _ICEPICK
	CON_COMMAND( zombie_knockback_player, "<id> <value> - Sets the player's knockback to specified value." )
	{
		_CRT_FLOAT fltval;

		if ( !zombie_enabled.GetBool() )
		{
			META_LOG( g_PLAPI, "%s: Zombie mode is not currently enabled\n", args.Arg(0) );
			return;
		}
		CPlayerMatchList matches(args.Arg(1), false);
		if ( matches.Count() < 1 ) 
		{
			META_LOG( g_PLAPI, "%s: Unable to find any players that match %s\n", args.Arg(0), args.Arg(1));
			return;
		}
		for ( int x = 0; x < matches.Count(); x++ )
		{
			CBasePlayer* pPlayer = (CBasePlayer*)matches.GetMatch(x)->GetUnknown()->GetBaseEntity();
			int iPlayer = m_Engine->IndexOfEdict( m_GameEnts->BaseEntityToEdict( pPlayer ) );
			_atoflt( &fltval, args.Arg(2) );
			g_Players[iPlayer].fKnockback = fltval.f;
		}
		return;
	}

	CON_COMMAND( zombie_setmodel, "<id> <string model> - Sets the player's model to specified string." )
	{
		if ( !zombie_enabled.GetBool() )
		{
			META_LOG( g_PLAPI, "%s: Zombie mode is not currently enabled\n", args.Arg(0) );
			return;
		}
		CPlayerMatchList matches(args.Arg(1), true);
		if ( matches.Count() < 1 ) 
		{
			META_LOG( g_PLAPI, "%s: Unable to find any living players that match %s\n", args.Arg(0), args.Arg(1));
			return;
		}
		for ( int x = 0; x < matches.Count(); x++ )
		{
			CBasePlayer* pPlayer = (CBasePlayer*)matches.GetMatch(x)->GetUnknown()->GetBaseEntity();
			UTIL_SetModel( pPlayer, args.Arg(2) );
		}
		return;
	}
#endif

CON_COMMAND( zombie_player, "<name> - Turns player into a zombie if zombie mode is on." )
{
	if ( !zombie_enabled.GetBool() )
	{
		META_LOG( g_PLAPI, "%s: Zombie mode is not currently enabled\n", args.Arg(0) ); //args.Arg(0) );
		return;
	}
	CPlayerMatchList matches( args.Arg(1), true);
	if ( matches.Count() < 1 ) 
	{
		META_LOG( g_PLAPI, "%s: Unable to find any living players that match %s\n", args.Arg(0), args.Arg(1) );//args.Arg(0), args.Arg(1));
		return;
	}
	for ( int x = 0; x < matches.Count(); x++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)matches.GetMatch(x)->GetUnknown()->GetBaseEntity();
		int iPlayer = m_Engine->IndexOfEdict( m_GameEnts->BaseEntityToEdict( pPlayer ) );
		if ( !g_Players[iPlayer].isZombie )
		{
			g_ZombiePlugin.MakeZombie( pPlayer, zombie_health.GetInt() ); // this command changes the argv/argc so we can't get the command name from it
			META_LOG( g_PLAPI, "%s is now a zombie.", m_Engine->GetClientConVarValue( iPlayer, "name") );
		}
	}
	return;
}

CON_COMMAND( zombie_mode, "- Toggles zombie mode" )
{
	if ( args.ArgC() == 1 )
	{
 		META_LOG( g_PLAPI, "Zombie Mode is currently %s.", ( zombie_enabled.GetBool() ? "enabled" : "disabled" ) );
		return;
	}
	if ( FStrEq( args.Arg(1), "1" ) )
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
	else if ( FStrEq( args.Arg(1), "0" ) )
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

CON_COMMAND(zombie_restrict, "<a/t/c/w> <weapon> [amount] - Restricts a weapon for all players, terrorist, counter-terrorist, or winning team. Optional last parameter is how many weapons of the type the team can hold.")
{
	g_ZombiePlugin.RestrictWeapon( args.Arg(1), args.Arg(2), args.Arg(3), args.Arg(0) );
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

	for ( x = 0; x < m_ClassManager.Count(); x++ )
	{
		#define DOMATH( iVal, iVar, iPercent ) \
			iVal = ( (float)iVar / 100 ) * (float)iPercent;

		GETCLASS( x );

		META_CONPRINTF( "\nName: %s\n", zClass->GetName() );
		META_CONPRINTF( "Model: %s\n", zClass->GetModelName() );

		int iTmp = 0;
		DOMATH( iTmp, zClass->iHealth, zombie_health_percent.GetInt() );
		META_CONPRINTF( "Health: %d (%d%%%)\n", iTmp, zombie_health_percent.GetInt() );
		
		float fTmp = ( zClass->fSpeed / 100 ) * zombie_speed_percent.GetInt();
		META_CONPRINTF( "Speed: %.2f (%d%%%)\n", fTmp, zombie_speed_percent.GetInt() );

		fTmp = ( zClass->fSpeedDuck / 100 ) * zombie_speed_percent.GetInt();
		META_CONPRINTF( "Crouch Speed: %.2f (%d%%%)\n", fTmp, zombie_speed_percent.GetInt() );

		fTmp = ( zClass->fSpeedRun / 100 ) * zombie_speed_percent.GetInt();
		META_CONPRINTF( "Run Speed: %.2f (%d%%%)\n", fTmp, zombie_speed_percent.GetInt() );
		
		DOMATH( iTmp, zClass->iJumpHeight, zombie_jump_height_percent.GetInt() );
		META_CONPRINTF( "Jump Height: %d (%d%%%)\n", iTmp, zombie_jump_height_percent.GetInt() );
		
		META_CONPRINTF( "Headhosts Required: %d\n", zClass->iHeadshots );

		fTmp = ( zClass->fKnockback / 100 ) * zombie_knockback_percent.GetInt();
		META_CONPRINTF( "Knockback: %.2f (%d%%)\n", fTmp, zombie_knockback_percent.GetInt() );

		META_CONPRINTF( "Headhosts Only: %s\n", (zClass->bHeadShotsOnly ? "Yes" : "No"));

		META_CONPRINTF( "Regen Health: %d\n", zClass->iRegenHealth );

		META_CONPRINTF( "Regen Seconds: %.2f\n", zClass->fRegenTimer );

		META_CONPRINTF( "Grenade Multiplier: %.2f\n", zClass->fGrenadeMultiplier );

		META_CONPRINTF( "Grenade Knockback: %.2f\n", zClass->fGrenadeKnockback );

		META_CONPRINTF( "Health Bonus: %d\n", zClass->iHealthBonus );

		META_CONPRINTF( "Fall Damage: %d\n\n", zClass->bFallDamage );
	}
	if ( !m_ClassManager.Enabled() )
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

/*
CON_COMMAND( zm_test, "" )
{
	g_ZombiePlugin.ChangeRoundEndSounds();
}
*/

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
		META_CONPRINT( "Sorry, saving of zombie classes is currently disabled.\n" );
	}

}

CON_COMMAND( zombie_loadclasses, " - Loads the class list from the file." )
{
	g_ZombiePlugin.LoadZombieClasses();
}

CON_COMMAND( zombie_slay, "" )
{
	CPlayerMatchList matches(args.Arg(1), true);
	if ( matches.Count() < 1 ) 
	{
		META_LOG( g_PLAPI, "%s: Unable to find any living players that match %s\n", args.Arg(0), args.Arg(1));
		return;
	}
	for ( int x = 0; x < matches.Count(); x++ )
	{
		edict_t *pEdict = matches.GetMatch(x);
		CBaseEntity *pPlayer = NULL;
		int iPlayer = 0;
		if ( IsValidPlayer( pEdict, &iPlayer, &pPlayer ) && g_ZombiePlugin.IsAlive( pEdict ) )
		{
			g_ZombiePlugin.PerformSuicide( (CBasePlayer*)pPlayer, false, false );
		}
	}
}

CON_COMMAND( zombie_writevision, " - Writes the current player Zombie Vision selections to file." )
{
	if ( zombie_save_visionlist.GetBool() )
	{
		if ( kvPlayerVision->GetFirstSubKey() )
		{
			kvPlayerVision->SaveToFile( g_ZombiePlugin.m_FileSystem, "cfg/zombiemod/player_classes.kv", "MOD" );
		}
		m_Engine->ServerCommand( "zombie_class_count\n" );
		META_CONPRINTF( "[ZOMBIE] Class Selection List Saved.\n" );
	}
	else
	{
		META_CONPRINT( "Sorry, saving zombie vision selections is currently disabled.\n" );
	}

}

CON_COMMAND( zombie_vision_count, " - Displays current count of players classes saved to disk." )
{
	if ( zombie_save_visionlist.GetBool() )
	{
		KeyValues *p;
		int iCount = 0;
		p = kvPlayerVision->GetFirstSubKey();
		while ( p )
		{
			iCount++;
			p = p->GetNextKey();
		}

		META_CONPRINTF( "[ZOMBIE] Current count of saved player zombie vision selections: %d\n", iCount );
	}
	else
	{
		META_CONPRINT( "[ZOMBIE] Sorry, saving zombie vision selections is currently disabled.\n" );
	}
	return;
}