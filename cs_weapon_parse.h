#ifndef CS_WEAPON_PARSE_H
#define CS_WEAPON_PARSE_H

#include <weapon_parse.h>
#include "networkvar.h"

// these should actually be bits for things but who cares
#define WEAPONTYPE_KNIFE 0
#define WEAPONTYPE_PISTOL 1
#define WEAPONTYPE_RIFLE 3
#define WEAPONTYPE_XM1014 4
#define WEAPONTYPE_SNIPER 5
#define WEAPONTYPE_C4 7
#define WEAPONTYPE_GRENADE 8

class CCSWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CCSWeaponInfo, FileWeaponInfo_t );
	CCSWeaponInfo();
	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );

	float	m_flMaxSpeed;
	int     m_WeaponType;
	int	m_iWeaponPrice;
	int     m_iTeam;	// 0 all, 2 t, 3 ct
	short	m_unknown1; // MuzzleFlashScale? (float) 
	short 	m_unknown2; // MuzzleFlashStyle? (CS_MUZZLEFLASH_NORM, CS_MUZZLEFLASH_X, CS_MUZZLEFLASH_NONE) 17658?
	float	m_flWeaponArmorRatio;
	int 	m_iCrosshairMinDistance;
	int     m_iCrosshairDeltaDistance;
	short	m_iCanEquipWithShield;

	char    m_WrongTeamMsg[32];
	char	m_szAnimExtension[16];
	char 	m_szShieldViewModel[152];
	
	// Parameters for FX_FireBullets:
	int     m_iPenetration;
	int     m_iDamage;
	float	m_flRange;
	float	m_flRangeModifier;
	int     m_iBullets;
	float	m_flCycleTime;
	short   m_unknown3; // AccuracyQuadratic/BotAudibleRange?

	float   m_flAccuracyDivisor;
	float   m_flAccuracyOffset;
	float   m_flMaxInaccuracy;
	float	m_flTimeToIdle;
	float	m_flIdleInterval;
};


#endif // CS_WEAPON_PARSE_H
