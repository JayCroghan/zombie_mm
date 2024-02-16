
#ifndef WEAPON_AMMO_H
	#define WEAPON_AMMO_H

	#include "ZombiePlugin.h"

	#define WEAPONAMMO_HEGRENADE_LIMIT      1
	#define WEAPONAMMO_FLASHBANG_LIMIT      2
	#define WEAPONAMMO_SMOKEGRENADE_LIMIT   1

	extern bool g_bGrenadePack;

	extern int g_iToolsClip1;
	extern int g_iToolsClip2;
	extern int g_iToolsAmmo;
	extern int g_iToolsActiveWeapon;

	#define WEAPONS_MAX_LENGTH 32
	#define WEAPONS_SLOTS_MAX 5
	#define WEAPONS_SPAWN_T_WEAPON "weapon_glock"
	#define WEAPONS_SPAWN_CT_WEAPON "weapon_usp"

	enum WeaponsData
	{
		WEAPONS_DATA_NAME = 0,
		WEAPONS_DATA_ENTITY,
		WEAPONS_DATA_TYPE,
		WEAPONS_DATA_SLOT,
		WEAPONS_DATA_RESTRICTDEFAULT,
		WEAPONS_DATA_TOGGLEABLE,
		WEAPONS_DATA_AMMOTYPE,
		WEAPONS_DATA_AMMOPRICE,
		WEAPONS_DATA_KNOCKBACK,
		WEAPONS_DATA_ZMARKETPRICE,
		WEAPONS_DATA_ZMARKETPURCHASEMAX,
		WEAPONS_DATA_RESTRICTED,
	};

	enum WeaponsSlot
	{
		Slot_Invalid        = -1,   /** Invalid weapon (slot). */
		Slot_Primary        = 0,    /** Primary weapon slot. */
		Slot_Secondary      = 1,    /** Secondary weapon slot. */
		Slot_Melee          = 2,    /** Melee (knife) weapon slot. */
		Slot_Projectile     = 3,    /** Projectile (grenades, flashbangs, etc) weapon slot. */
		Slot_Explosive      = 4,    /** Explosive (c4) weapon slot. */
		Slot_NVGs           = 5,    /** NVGs (fake) equipment slot. */
	};


	enum WeaponAmmoGrenadeType
	{
		GrenadeType_Invalid         = -1,   /** Invalid grenade slot. */
		GrenadeType_HEGrenade       = 11,   /** HEGrenade slot */
		GrenadeType_Flashbang       = 12,   /** Flashbang slot. */
		GrenadeType_Smokegrenade    = 13   /** Smokegrenade slot. */
	};

	void WeaponAmmoSetAmmo(edict_t *pWeapon, bool clip, int value, bool add = false);
	void WeaponAmmoOnAllPluginsLoaded();
	void WeaponAmmoOnOffsetsFound();
	void WeaponAmmoSetGrenadeCount(edict_t *pEdict, WeaponAmmoGrenadeType type, int value, bool add);
	int WeaponAmmoGetAmmo(edict_t *pWeapon, bool clip);
	int WeaponAmmoGetGrenadeCount(edict_t *pEdict, WeaponAmmoGrenadeType type);
	WeaponAmmoGrenadeType WeaponAmmoEntityToGrenadeType(const char *sWeaponEntity);
	int WeaponAmmoGetGrenadeLimit(WeaponAmmoGrenadeType grenadetype);
	void WeaponsOnOffsetsFound();

#endif