#include "CTakeDamageInfo.h"

void CTakeDamageInfo::Init( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iCustomDamage )
{
	m_hInflictor = pInflictor;
	if ( pAttacker )
	{
		m_hAttacker = pAttacker;
	}
	else
	{
		m_hAttacker = pInflictor;
	}

	m_hWeapon = pWeapon;

	m_flDamage = flDamage;

	m_flBaseDamage = BASEDAMAGE_NOT_SPECIFIED;

	m_bitsDamageType = bitsDamageType;
	m_iDamageCustom = iCustomDamage;

	m_flMaxDamage = flDamage;
	m_vecDamageForce = damageForce;
	m_vecDamagePosition = damagePosition;
	m_vecReportedPosition = reportedPosition;
	m_iAmmoType = -1;
}

CTakeDamageInfo::CTakeDamageInfo()
{
	Init( NULL, NULL, NULL, vec3_origin, vec3_origin, vec3_origin, 0, 0, 0 );
}

CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType )
{
	Set( pInflictor, pAttacker, flDamage, bitsDamageType, iKillType );
}

CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, float flDamage, int bitsDamageType, int iKillType )
{
	Set( pInflictor, pAttacker, pWeapon, flDamage, bitsDamageType, iKillType );
}

CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Set( pInflictor, pAttacker, damageForce, damagePosition, flDamage, bitsDamageType, iKillType, reportedPosition );
}

CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Set( pInflictor, pAttacker, pWeapon, damageForce, damagePosition, flDamage, bitsDamageType, iKillType, reportedPosition );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType )
{
	Init( pInflictor, pAttacker, NULL, vec3_origin, vec3_origin, vec3_origin, flDamage, bitsDamageType, iKillType );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, float flDamage, int bitsDamageType, int iKillType )
{
	Init( pInflictor, pAttacker, pWeapon, vec3_origin, vec3_origin, vec3_origin, flDamage, bitsDamageType, iKillType );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Set( pInflictor, pAttacker, NULL, damageForce, damagePosition, flDamage, bitsDamageType, iKillType, reportedPosition );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Vector vecReported = vec3_origin;
	if ( reportedPosition )
	{
		vecReported = *reportedPosition;
	}
	Init( pInflictor, pAttacker, pWeapon, damageForce, damagePosition, vecReported, flDamage, bitsDamageType, iKillType );
}
