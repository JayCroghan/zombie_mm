#include "WeaponAmmo.h"
#include "ZombiePlugin.h"


bool g_bGrenadePack = false;

int g_LocalWeaponData;
int g_iToolsClip1;
int g_iToolsClip2;
int g_iLocalData;
int g_iToolsAmmo;

int g_iToolsActiveWeapon;


/**
 * Find ammo-reserve-specific offsets here.
 */
void WeaponAmmoOnOffsetsFound()
{
	g_LocalWeaponData = UTIL_FindOffsetTable( "CBaseCombatWeapon", "LocalWeaponData", "" );
	g_iToolsClip1 = UTIL_FindOffsetTable( "CBaseCombatWeapon", "LocalWeaponData", "m_iClip1" );
    if (g_iToolsClip1 == 0)
    {
        META_LOG( g_PLAPI, "Offset \"CBaseCombatWeapon::m_iClip1\" was not found.");
    }
    
    //g_iToolsClip2 = UTIL_FindOffset("CBaseCombatWeapon", "m_iClip2");
	g_iToolsClip2 = UTIL_FindOffsetTable( "CBaseCombatWeapon", "LocalWeaponData", "m_iClip2" );
    if (g_iToolsClip2 == 0)
    {
        META_LOG( g_PLAPI, "Offset \"CBaseCombatWeapon::m_iClip2\" was not found.");
    }
    
	g_iLocalData = UTIL_FindOffsetTable( "CBasePlayer", "localdata", "" );
    g_iToolsAmmo = UTIL_FindOffsetTable( "CBasePlayer", "localdata", "m_iAmmo" );
    if (g_iToolsAmmo == 0)
    {
        META_LOG( g_PLAPI, "Offset \"CBasePlayer::m_iAmmo\" was not found.");
    }
}

/**
 * Set clip/reserve ammo on a weapon.
 * 
 * @param weapon    The weapon index.
 * @param clip      True sets clip ammo, false sets reserve.
 * @param value     The amount of ammo to set to.
 * @param add       (Optional) If true, the value is added to the weapon's current ammo count.
 */
void WeaponAmmoSetAmmo(edict_t *pWeapon, bool clip, int value, bool add)
{
    // Set variable to offset we are changing.
    int ammooffset = clip ? g_iToolsClip1 : g_iToolsClip2;
    
    int ammovalue = 0;
    
    // If we are adding, then update variable with current ammo value.
    if (add)
    {
        ammovalue = WeaponAmmoGetAmmo(pWeapon, clip);
    }
    
    // Return ammo offset value.
	UTIL_SetProperty( ammooffset, pWeapon, ammovalue, true );
    //SetEntData(sWeapon, ammooffset, ammovalue + value, _, true);
}

/**
 * Get clip/reserve ammo on a weapon.
 * 
 * @param weapon    The weapon index.
 * @param clip      True gets clip ammo, false gets reserve.
 */
int WeaponAmmoGetAmmo(edict_t *pWeapon, bool clip)
{
    // Set variable to offset we are changing.
    int ammooffset = clip ? g_iToolsClip1 : g_iToolsClip2;
	int ammovalue = 0;
    
    // Return ammo offset value.
	if ( UTIL_GetProperty( ammooffset, pWeapon, &ammovalue ) )
	{
		return ammovalue;
	}
	else
	{
		return -1;
	}
    //return GetEntData(sWeapon, ammooffset);
}

/**
 * Set the count of any grenade-type a client has.
 * 
 * @param client    The client index.
 * @param slot      The type of
 * @param value     The amount of ammo to set to.
 * @param add       (Optional) If true, the value is added to the grenades' current ammo count. 
 */
void WeaponAmmoSetGrenadeCount(edict_t *pEdict, WeaponAmmoGrenadeType type, int value, bool add)
{
    // Initialize variable (value is 0)
    int ammovalue;
    
    // If we are adding, then update variable with current ammo value.
    if (add)
    {
        ammovalue = WeaponAmmoGetGrenadeCount( pEdict, type);
    }
    
	UTIL_SetProperty( g_iToolsAmmo + (type * 4), pEdict, ammovalue + value );
    //SetEntData(client, g_iToolsAmmo + (_:type * 4), ammovalue + value, _, true);
}

/**
 * Get the count of any grenade-type a client has.
 * 
 * @param client    The client index.
 * @param slot      The type of
 */
int WeaponAmmoGetGrenadeCount(edict_t *pEdict, WeaponAmmoGrenadeType type)
{
	int ammovalue;
	if ( UTIL_GetProperty( g_iToolsAmmo + ( type * 4 ), pEdict, &ammovalue ) )
	{
		return ammovalue;
	}
	else
	{
		return 0;
	}
    //return GetEntData(client, g_iToolsAmmo + (_:type * 4));
}

/**
 * Takes a weapon entity and returns an entry in enum WeaponAmmoGrenadeType.
 * 
 * @param weaponentity  The weapon entity to find entry for.
 * @return              An entry in WeaponAmmoGrenadeType.
 */
WeaponAmmoGrenadeType WeaponAmmoEntityToGrenadeType(const char *sWeaponEntity)
{
    if (FStrEq(sWeaponEntity, "weapon_hegrenade"))
    {
        return GrenadeType_HEGrenade;
    }
    else if (FStrEq(sWeaponEntity, "weapon_flashbang"))
    {
        return GrenadeType_Flashbang;
    }
    else if (FStrEq(sWeaponEntity, "weapon_smokegrenade"))
    {
        return GrenadeType_Smokegrenade;
    }
    
    return GrenadeType_Invalid;
}

/**
 * Returns the max amount of this type of grenades the client is allowed to carry.
 * 
 * @param weaponentity  The weapon entity to get the limit for.
 * @return              The grenade limit, -1 if an unhandled grenade entity was given. 
 */
int WeaponAmmoGetGrenadeLimit(WeaponAmmoGrenadeType grenadetype)
{
    switch(grenadetype)
    {
        case GrenadeType_HEGrenade:
        {
            // Attempt to find a cvar provided by an outside plugin.
            //new Handle:gplimit = FindConVar(GRENADE_PACK_CVAR_LIMIT);
            
            // If Grenade Pack is loaded and the cvar was found, then get the value of the outside cvar, if not return CS:S default.
            return WEAPONAMMO_HEGRENADE_LIMIT;
        }
        case GrenadeType_Flashbang:
        {
            return WEAPONAMMO_FLASHBANG_LIMIT;
        }
        case GrenadeType_Smokegrenade:
        {
            return WEAPONAMMO_SMOKEGRENADE_LIMIT;
        }
    }
    
    return -1;
}


void WeaponsOnOffsetsFound()
{
    // If offset "m_hActiveWeapon" can't be found, then stop the plugin.
	g_iToolsActiveWeapon = UTIL_FindOffset("CBaseCombatCharacter", "m_hActiveWeapon");
    if (g_iToolsActiveWeapon == 0)
    {
        META_LOG( g_PLAPI, "Offset \"CBasePlayer::m_hActiveWeapon\" was not found.");
    }
    
    // Forward event to sub-modules
    WeaponAmmoOnOffsetsFound();
}