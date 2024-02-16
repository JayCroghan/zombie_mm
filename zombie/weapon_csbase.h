#ifndef WEAPON_CSBASE_H
#define WEAPON_CSBASE_H
#ifdef _WIN32
#pragma once
#endif

#include "cs_playeranimstate.h"
#include "cs_weapon_parse.h"

class CCSPlayer;

#define BULLET_PLAYER_50AE		"BULLET_PLAYER_50AE"
#define BULLET_PLAYER_762MM		"BULLET_PLAYER_762MM"
#define BULLET_PLAYER_556MM		"BULLET_PLAYER_556MM"
#define BULLET_PLAYER_338MAG	"BULLET_PLAYER_338MAG"
#define BULLET_PLAYER_9MM		"BULLET_PLAYER_9MM"
#define BULLET_PLAYER_BUCKSHOT	"BULLET_PLAYER_BUCKSHOT"
#define BULLET_PLAYER_45ACP		"BULLET_PLAYER_45ACP"
#define BULLET_PLAYER_357SIG	"BULLET_PLAYER_357SIG"
#define BULLET_PLAYER_57MM		"BULLET_PLAYER_57MM"
#define AMMO_TYPE_HEGRENADE		"AMMO_TYPE_HEGRENADE"
#define AMMO_TYPE_FLASHBANG		"AMMO_TYPE_FLASHBANG"

bool IsAmmoType( int iAmmoType, const char *pAmmoName );


typedef enum
{
	WEAPON_NONE = 0,

	WEAPON_P228,
	WEAPON_GLOCK,
	WEAPON_SCOUT,
	WEAPON_HEGRENADE,
	WEAPON_XM1014,
	WEAPON_C4,
	WEAPON_MAC10,
	WEAPON_AUG,
	WEAPON_SMOKEGRENADE,
	WEAPON_ELITE,
	WEAPON_FIVESEVEN,
	WEAPON_UMP45,
	WEAPON_SG550,
	WEAPON_GALIL,
	WEAPON_FAMAS,
	WEAPON_USP,
	WEAPON_AWP,
	WEAPON_MP5NAVY,
	WEAPON_M249,
	WEAPON_M3,
	WEAPON_M4A1,
	WEAPON_TMP,
	WEAPON_G3SG1,
	WEAPON_FLASHBANG,
	WEAPON_DEAGLE,
	WEAPON_SG552,
	WEAPON_AK47,
	WEAPON_KNIFE,
	WEAPON_P90,

	WEAPON_MAX,
} CSWeaponID;

class CWeaponCSBase : public CBaseCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponCSBase, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponCSBase();

	DECLARE_DATADESC();

	virtual void CheckRespawn();
	virtual CBaseEntity* Respawn();
	
	virtual const Vector& GetBulletSpread();

	virtual float	GetDefaultAnimSpeed();
	virtual void	BulletWasFired( const Vector &vecStart, const Vector &vecEnd );
	virtual bool    ShouldRemoveOnRoundRestart();

	void Materialize();
	void AttemptToMaterialize();

	virtual bool	IsPredicted() const;

	bool			IsPistol() const;

	virtual void			DefaultReload(int n1, int n2, int n3);	
	virtual  bool                    IsAwp() const;

	virtual float GetMaxSpeed() const;	

	virtual CSWeaponID GetWeaponID( void ) const;
	virtual bool                    IsSilenced() const;
	virtual void SetWeaponModelIndex(int index);
	virtual int UpdateShieldState();
	virtual Activity GetDeployActivity();
	virtual void DefaultPistolReload();

	CCSPlayer* GetPlayerOwner() const;

	CCSWeaponInfo const	&GetCSWpnData() const;

	virtual bool	Reload();
	virtual bool	Deploy();
	bool IsUseable();

	bool PlayEmptySound();
	bool SendReloadEvents();
	virtual void	ItemPostFrame();

	CWeaponCSBase( const CWeaponCSBase & );
};


#endif