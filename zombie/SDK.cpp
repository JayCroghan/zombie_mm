#include "ZombiePlugin.h"

void CTakeDamageInfo::Init( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iKillType )
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

	m_flDamage = flDamage;

	m_flBaseDamage = BASEDAMAGE_NOT_SPECIFIED;

	m_bitsDamageType = bitsDamageType;
	m_iCustomKillType = iKillType;

	m_flMaxDamage = flDamage;
	m_vecDamageForce = damageForce;
	m_vecDamagePosition = damagePosition;
	m_vecReportedPosition = reportedPosition;
	m_iAmmoType = -1;
}

CTakeDamageInfo::CTakeDamageInfo()
{
	Init( NULL, NULL, vec3_origin, vec3_origin, vec3_origin, 0, 0, 0 );
}


CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType )
{
	Set( pInflictor, pAttacker, flDamage, bitsDamageType, iKillType );
}

CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Set( pInflictor, pAttacker, damageForce, damagePosition, flDamage, bitsDamageType, iKillType, reportedPosition );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType )
{
	Init( pInflictor, pAttacker, vec3_origin, vec3_origin, vec3_origin, flDamage, bitsDamageType, iKillType );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Vector vecReported = vec3_origin;
	if ( reportedPosition )
	{
		vecReported = *reportedPosition;
	}
	Init( pInflictor, pAttacker, damageForce, damagePosition, vecReported, flDamage, bitsDamageType, iKillType );
}

CTraceFilterSkipTwoEntities::CTraceFilterSkipTwoEntities( const IHandleEntity *passentity, const IHandleEntity *passentity2, int collisionGroup ) :
	BaseClass( passentity, collisionGroup ), m_pPassEnt2(passentity2)
{
}

bool CTraceFilterSkipTwoEntities::ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
{
	Assert( pServerEntity );
	if ( !PassServerEntityFilter( pServerEntity, m_pPassEnt2 ) )
		return false;

	return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
}

CTraceFilterSimple::CTraceFilterSimple( const IHandleEntity *passedict, int collisionGroup )
{
	m_pPassEnt = passedict;
	m_collisionGroup = collisionGroup;
}


bool StandardFilterRules( IHandleEntity *pHandleEntity, int fContentsMask )
{
        CBaseEntity *pCollide = EntityFromEntityHandle( pHandleEntity );

        // Static prop case...
        if ( !pCollide )
                return true;

        SolidType_t solid = pCollide->GetSolid();
        const model_t *pModel = g_ZombiePlugin.m_ModelInfo->GetModel( pCollide->GetModelIndex() );

        if ( ( g_ZombiePlugin.m_ModelInfo->GetModelType( pModel ) != mod_brush ) || (solid != SOLID_BSP && solid != SOLID_VPHYSICS) )
        {
                if ( (fContentsMask & CONTENTS_MONSTER) == 0 )
                        return false;
        }

        // This code is used to cull out tests against see-thru entities
        if ( !(fContentsMask & CONTENTS_WINDOW) && pCollide->IsTransparent() )
                return false;

        // FIXME: this is to skip BSP models that are entities that can be
        // potentially moved/deleted, similar to a monster but doors don't seem to
        // be flagged as monsters
        // FIXME: the FL_WORLDBRUSH looked promising, but it needs to be set on
        // everything that's actually a worldbrush and it currently isn't
        if ( !(fContentsMask & CONTENTS_MOVEABLE) && (pCollide->GetMoveType() == MOVETYPE_PUSH))// !(touch->flags & FL_WORLDBRUSH) )
                return false;

        return true;
}

bool PassServerEntityFilter( const IHandleEntity *pTouch, const IHandleEntity *pPass )
{
        if ( !pPass )
                return true;

        if ( pTouch == pPass )
                return false;

        const CBaseEntity *pEntTouch = EntityFromEntityHandle( pTouch );
        const CBaseEntity *pEntPass = EntityFromEntityHandle( pPass );
        if ( !pEntTouch || !pEntPass )
                return true;

        // don't clip against own missiles
        if ( pEntTouch->GetOwnerEntity() == pEntPass )
                return false;

        // don't clip against owner
        if ( pEntPass->GetOwnerEntity() == pEntTouch )
                return false;


        return true;
}

//-----------------------------------------------------------------------------
// The trace filter!
//-----------------------------------------------------------------------------
bool ShouldCollider( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		int tmp = collisionGroup0;
		collisionGroup0 = collisionGroup1;
		collisionGroup1 = tmp;
	}
	
	// --------------------------------------------------------------------------
	// NOTE: All of this code assumes the collision groups have been sorted!!!!
	// NOTE: Don't change their order without rewriting this code !!!
	// --------------------------------------------------------------------------

	// Don't bother if either is in a vehicle...
	if (( collisionGroup0 == COLLISION_GROUP_IN_VEHICLE ) || ( collisionGroup1 == COLLISION_GROUP_IN_VEHICLE ))
		return false;

	if ( ( collisionGroup1 == COLLISION_GROUP_DOOR_BLOCKER ) && ( collisionGroup0 != COLLISION_GROUP_NPC ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && ( collisionGroup1 == COLLISION_GROUP_PASSABLE_DOOR ) )
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_DEBRIS || collisionGroup0 == COLLISION_GROUP_DEBRIS_TRIGGER )
	{
		// put exceptions here, right now this will only collide with COLLISION_GROUP_NONE
		return false;
	}

	// Dissolving guys only collide with COLLISION_GROUP_NONE
	if ( (collisionGroup0 == COLLISION_GROUP_DISSOLVING) || (collisionGroup1 == COLLISION_GROUP_DISSOLVING) )
	{
		if ( collisionGroup0 != COLLISION_GROUP_NONE )
			return false;
	}

	// doesn't collide with other members of this group
	// or debris, but that's handled above
	if ( collisionGroup0 == COLLISION_GROUP_INTERACTIVE_DEBRIS && collisionGroup1 == COLLISION_GROUP_INTERACTIVE_DEBRIS )
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_BREAKABLE_GLASS && collisionGroup1 == COLLISION_GROUP_BREAKABLE_GLASS )
		return false;

	// interactive objects collide with everything except debris & interactive debris
	if ( collisionGroup1 == COLLISION_GROUP_INTERACTIVE && collisionGroup0 != COLLISION_GROUP_NONE )
		return false;

	// Projectiles hit everything but debris, weapons, + other projectiles
	if ( collisionGroup1 == COLLISION_GROUP_PROJECTILE )
	{
		if ( collisionGroup0 == COLLISION_GROUP_DEBRIS || 
			collisionGroup0 == COLLISION_GROUP_WEAPON ||
			collisionGroup0 == COLLISION_GROUP_PROJECTILE )
		{
			return false;
		}
	}

	// Don't let vehicles collide with weapons
	// Don't let players collide with weapons...
	// Don't let NPCs collide with weapons
	// Weapons are triggers, too, so they should still touch because of that
	if ( collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		if ( collisionGroup0 == COLLISION_GROUP_VEHICLE || 
			collisionGroup0 == COLLISION_GROUP_PLAYER ||
			collisionGroup0 == COLLISION_GROUP_NPC )
		{
			return false;
		}
	}

	// collision with vehicle clip entity??
	if ( collisionGroup0 == COLLISION_GROUP_VEHICLE_CLIP || collisionGroup1 == COLLISION_GROUP_VEHICLE_CLIP )
	{
		// yes then if it's a vehicle, collide, otherwise no collision
		// vehicle sorts lower than vehicle clip, so must be in 0
		if ( collisionGroup0 == COLLISION_GROUP_VEHICLE )
			return true;
		// vehicle clip against non-vehicle, no collision
		return false;
	}

	return true;
}


bool CTraceFilterSimple::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
		return false;

	if ( m_pPassEnt )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
		{
			return false;
		}
	}

	// Don't test if the game code tells us we should ignore this collision...
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
		return false;
	/*if ( pEntity && !g_pGameRules->ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
		return false;*/
	if ( pEntity && !ShouldCollider( m_collisionGroup, pEntity->GetCollisionGroup() ) )
		return false; 

	return true;
}

void UTIL_TraceLines( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter *pFilter, trace_t *ptr, bool bRed )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd );
	g_ZombiePlugin.m_EngineTrace->TraceRay( ray, mask, pFilter, ptr );
}
/*inline void UTIL_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, const IHandleEntity *ignore, int collisionGroup, trace_t *ptr, bool bRed )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd );
	CTraceFilterSimple traceFilter( ignore, collisionGroup );
	g_ZombiePlugin.m_EngineTrace->TraceRay( ray, mask, &traceFilter, ptr );
}*/


void CBasePlayer::IncrementFragCount( int nCount )
{
        m_iFrags += nCount;
        pl.frags = m_iFrags;
}


