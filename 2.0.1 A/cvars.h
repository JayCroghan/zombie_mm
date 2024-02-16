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

#include <convar.h>

extern ConVar 					zombie_model_file;
extern ConVar 					zombie_download_file;
extern ConVar 					zombie_sound_rand;
extern ConVar 					zombie_enabled;
extern ConVar 					zombie_health;
extern ConVar 					zombie_speed;
extern ConVar 					zombie_fov;
extern ConVar 					zombie_knockback;
extern ConVar 					zombie_restrictions;
extern ConVar 					zombie_dark;
extern ConVar 					zombie_teams;
extern ConVar 					zombie_respawn;
extern ConVar 					zombie_respawn_delay;
extern ConVar 					zombie_respawn_protection;
extern ConVar 					zombie_respawn_as_zombie;
extern ConVar 					zombie_respawn_onconnect;
extern ConVar					zombie_respawn_onconnect_as_zombie;
extern ConVar 					zombie_unlimited_ammo;
extern ConVar 					zombie_talk;
extern ConVar 					zombie_suicide;
extern ConVar 					zombie_changeteam_block;
extern ConVar 					zombie_suicide_text;
extern ConVar 					zombie_startup;
extern ConVar 					zombie_timer_max;
extern ConVar 					zombie_timer_min;
extern ConVar 					zombie_kill_bonus;
extern ConVar 					zombie_headshot_count;
extern ConVar 					zombie_headshots;
extern ConVar 					zombie_allow_disable_nv;
extern ConVar 					zombie_notices;
extern ConVar 					zombie_fog;
extern ConVar 					zombie_fog_sky;
extern ConVar 					zombie_fog_colour;
extern ConVar 					zombie_fog_colour2;
extern ConVar 					zombie_fog_start;
extern ConVar 					zombie_fog_end;
extern ConVar 					zombie_fog_blend;
extern ConVar 					zombie_effect;
extern ConVar 					zombie_shake;
extern ConVar 					zombie_jetpack;
extern ConVar 					humans_jetpack;
extern ConVar 					zombie_jetpack_timer;
extern ConVar 					zombie_fog_sky_material;
extern ConVar 					zombie_remove_radar;
extern ConVar 					zombie_stuckcheck_radius;
extern ConVar 					zombie_welcome_delay;
extern ConVar 					zombie_welcome_text;
extern ConVar 					zombie_help_url;
extern ConVar 					zombie_undead_sound_enabled;
extern ConVar 					zombie_undead_sound_volume;
extern ConVar 					zombie_undead_sound;
extern ConVar 					zombie_undead_sound_exclusions;
extern ConVar 					zombie_headshots_only;
extern ConVar 					zombie_balancer_health_ratio;
extern ConVar 					zombie_balancer_player_ratio;
extern ConVar 					zombie_balancer_type;
extern ConVar 					zombie_balancer_min_health;
extern ConVar 					zombie_teleportcount;
extern ConVar 					zombie_damagelist;
extern ConVar					zombie_damagelist_time;
extern ConVar 					zombie_vision_r;
extern ConVar 					zombie_vision_g;
extern ConVar 					zombie_vision_b;
extern ConVar 					zombie_vision_a;
extern ConVar 					zombie_vision_timer;
extern ConVar 					zombie_version;
extern ConVar					zombie_regen_timer;
extern ConVar					zombie_regen_health;
extern ConVar					zombie_first_zombie;
extern ConVar					zombie_show_health;
extern ConVar					zombie_teleport_zombies_only;
extern ConVar					zombie_respawn_primary;
extern ConVar					zombie_respawn_secondary;
extern ConVar					zombie_respawn_grenades;
extern ConVar					zombie_respawn_dontdrop_indexes;
extern ConVar					zombie_respawn_dropweapons;
extern ConVar					zombie_classes;
extern ConVar					zombie_classes_random;
extern ConVar					zombie_vision_material;
extern ConVar					zombie_save_classlist;

extern ConVar					zombie_health_percent;
extern ConVar					zombie_speed_percent;
extern ConVar					zombie_jump_height_percent;
extern ConVar					zombie_knockback_percent;
extern ConVar					zombie_end_round_overlay;
extern ConVar					zombie_count;
extern ConVar					zombie_count_doublehp;
extern ConVar					zombie_zstuck_enabled;

extern ConVar					zombie_kickstart_percent;
extern ConVar					zombie_kickstart_timer;
extern ConVar					zombie_vote_interval;
extern ConVar					zombie_weapon;
extern ConVar					zombie_disconnect_protection;

extern ConVar					zombie_force_endround;
extern ConVar					zombie_language_file;
extern ConVar					zombie_language_error;

extern ConVar					zombie_first_zombie_tele;

extern ConVar					zombie_startmoney;
extern ConVar					zombie_tk_money;
extern ConVar					zombie_kill_money;
extern ConVar					zombie_zombie_money;

extern ConVar					zombie_dissolve_corpse;
extern ConVar					zombie_dissolve_corpse_delay;

extern ConVar					zombie_delete_dropped_weapons;
extern ConVar					zombie_grenade_damage_multiplier;
extern ConVar					zombie_grenade_knockback_multiplier;
extern ConVar					zombie_health_bonus;

class ZombieAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool RegisterConCommandBase(ConCommandBase *pVar);
};

extern ZombieAccessor g_Accessor;

#endif //_INCLUDE_CVARS_H