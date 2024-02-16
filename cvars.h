/* ======== sample_mm ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): David "BAILOPAN" Anderson
* ============================
*/

#ifndef _INCLUDE_CVARS_H
#define _INCLUDE_CVARS_H

#include "convar_sm.h"

#define ExternConvar( cvar ) \
	extern ConVar				zm_##cvar; \
	extern ConVar				zombie_##cvar;

ExternConvar( model_file );
ExternConvar( download_file );
ExternConvar( sound_rand );
ExternConvar( enabled );
ExternConvar( health );
ExternConvar( speed );
ExternConvar( speed_crouch );
ExternConvar( speed_run );
ExternConvar( fov );
ExternConvar( knockback );
ExternConvar( restrictions );
ExternConvar( dark );
ExternConvar( teams );
ExternConvar( respawn );
ExternConvar( respawn_delay );
ExternConvar( respawn_protection );
ExternConvar( respawn_as_zombie );
ExternConvar( respawn_onconnect );
ExternConvar( respawn_onconnect_as_zombie );
ExternConvar( unlimited_ammo );
ExternConvar( talk );
ExternConvar( suicide );
ExternConvar( changeteam_block );
ExternConvar( suicide_text );
ExternConvar( startup );
ExternConvar( timer_max );
ExternConvar( timer_min );
ExternConvar( kill_bonus );
ExternConvar( headshot_count );
ExternConvar( headshots );
ExternConvar( allow_disable_nv );
ExternConvar( notices );
ExternConvar( fog );
ExternConvar( fog_sky );
ExternConvar( fog_colour );
ExternConvar( fog_colour2 );
ExternConvar( fog_start );
ExternConvar( fog_end );
ExternConvar( fog_blend );
ExternConvar( effect );
ExternConvar( shake );
ExternConvar( jetpack );
ExternConvar( humans_jetpack );
ExternConvar( jetpack_timer );
ExternConvar( fog_sky_material );
ExternConvar( remove_radar );
ExternConvar( stuckcheck_radius );
ExternConvar( welcome_delay );
ExternConvar( welcome_text );
ExternConvar( help_url );
ExternConvar( undead_sound_enabled );
ExternConvar( undead_sound_volume );
ExternConvar( undead_sound );
ExternConvar( undead_sound_exclusions );
ExternConvar( headshots_only );
ExternConvar( balancer_health_ratio );
ExternConvar( balancer_player_ratio );
ExternConvar( balancer_type );
ExternConvar( balancer_min_health );
ExternConvar( teleportcount );
ExternConvar( damagelist );
ExternConvar( damagelist_time );
ExternConvar( vision_r );
ExternConvar( vision_g );
ExternConvar( vision_b );
ExternConvar( vision_a );
ExternConvar( vision_timer );
ExternConvar( version );
ExternConvar( regen_timer );
ExternConvar( regen_health );
ExternConvar( first_zombie );
ExternConvar( show_health );
ExternConvar( show_attacker_health );
ExternConvar( teleport_zombies_only );
ExternConvar( respawn_primary );
ExternConvar( respawn_secondary );
ExternConvar( respawn_grenades );
ExternConvar( respawn_dontdrop_indexes );
ExternConvar( respawn_dropweapons );
ExternConvar( classes );
ExternConvar( classes_random );
ExternConvar( vision_material );
ExternConvar( save_classlist );
ExternConvar( health_percent );
ExternConvar( speed_percent );
ExternConvar( jump_height_percent );
ExternConvar( knockback_percent );
ExternConvar( end_round_overlay );
ExternConvar( count );
ExternConvar( count_doublehp );
ExternConvar( zstuck_enabled );
ExternConvar( kickstart_percent );
ExternConvar( kickstart_timer );
ExternConvar( vote_interval );
ExternConvar( weapon );
ExternConvar( disconnect_protection );
ExternConvar( force_endround );
ExternConvar( language_file );
ExternConvar( language_error );
ExternConvar( first_zombie_tele );
ExternConvar( startmoney );
ExternConvar( tk_money );
ExternConvar( kill_money );
ExternConvar( money );
ExternConvar( dissolve_corpse );
ExternConvar( dissolve_corpse_delay );
ExternConvar( delete_dropped_weapons );
ExternConvar( grenade_damage_multiplier );
ExternConvar( grenade_knockback_multiplier );
ExternConvar( health_bonus );
ExternConvar( class_url );
ExternConvar( count_min );
ExternConvar( count_max );
ExternConvar( allow_picked_up_weapons );
ExternConvar( buymenu );
ExternConvar( buymenu_zone_only );
ExternConvar( falldamage );
ExternConvar( buymenu_timelimit );
ExternConvar( buymenu_roundstart );
ExternConvar( ddos_protect );
ExternConvar( remove_objectives );
ExternConvar( display_class );
ExternConvar( napalm_enabled );
ExternConvar( napalm_damage );
ExternConvar( napalm_radius );
ExternConvar( save_visionlist );
ExternConvar( ffa_enabled );
ExternConvar( jump_height );
ExternConvar( game_description_format );
ExternConvar( game_description );
ExternConvar( napalm_time );
//ExternConvar( humans_win_sounds );
//ExternConvar( zombies_win_sounds );

#ifdef _PAID1
	extern ConVar zombie_knife_knockback;
#endif

class ZombieAccessor : public IConCommandBaseAccessor
{
public:
	bool RegisterConCommandBase(ConCommandBase *pVar)
	{
		return META_REGCVAR(pVar);
	}
};


extern ZombieAccessor g_Accessor;

#endif //_INCLUDE_CVARS_H