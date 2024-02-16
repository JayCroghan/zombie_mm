#include "ZombiePlugin.h"

#if defined WIN32
	#define	PATH_SEP_STR	"\\"
	#define PATH_SEP_CHAR	'\\'
	#define ALT_SEP_CHAR	'/'
#elif defined __linux__
	#define	PATH_SEP_STR	"/"
	#define PATH_SEP_CHAR	'/'
	#define ALT_SEP_CHAR	'\\'
#endif

/*
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
*/

/*
bool StandardFilterRules( IHandleEntity *pHandleEntity, int fContentsMask )
{
        CBaseEntity *pCollide = EntityFromEntityHandle( pHandleEntity );
		edict_t *pEntity = m_GameEnts->BaseEntityToEdict( pCollide );
		int iIndex = m_Engine->IndexOfEdict( pEntity );

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
        //if ( !(fContentsMask & CONTENTS_WINDOW) && pCollide->IsTransparent() )
		RenderMode_t m_nRenderMode;
		if ( !GetEntProp( iIndex, Prop_Data, "m_nRenderMode", m_nRenderMode ) )
			return false;
			

		if ( !(fContentsMask & CONTENTS_WINDOW) && (m_nRenderMode != kRenderNormal) )
                return false;

        // FIXME: this is to skip BSP models that are entities that can be
        // potentially moved/deleted, similar to a monster but doors don't seem to
        // be flagged as monsters
        // FIXME: the FL_WORLDBRUSH looked promising, but it needs to be set on
        // everything that's actually a worldbrush and it currently isn't
		if ( !UTIL_GetProperty( g_Offsets.m_MoveType, pCollide, &iMoveType ) )
			return false;

        if ( !(fContentsMask & CONTENTS_MOVEABLE) && (iMoveType == MOVETYPE_PUSH))// !(touch->flags & FL_WORLDBRUSH) )
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
*/
//-----------------------------------------------------------------------------
// The trace filter!
//-----------------------------------------------------------------------------
/*
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
	//if ( pEntity && !g_pGameRules->ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
	//	return false;
	if ( pEntity && !ShouldCollider( m_collisionGroup, pEntity->GetCollisionGroup() ) )
		return false; 

	return true;
}
*/
void UTIL_PathFmt(char *buffer, size_t len, const char *fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	size_t mylen = vsnprintf(buffer, len, fmt, ap);
	va_end(ap);

	for (size_t i=0; i<mylen; i++)
	{
		if (buffer[i] == ALT_SEP_CHAR)
			buffer[i] = PATH_SEP_CHAR;
	}
}

void DumpMapEnts()
{
	char path[255];
	char gamedir[100];

	m_Engine->GetGameDir(gamedir, sizeof(gamedir)-1);
	g_ZombiePlugin.m_FileSystem->CreateDirHierarchy( "addons/zombiemod/dump", "MOD" );
	UTIL_PathFmt(path, sizeof(path)-1, "%s/addons/zombiemod/dump", gamedir);
	int num = 0;
	char file[255];
	do
	{
		UTIL_PathFmt(file, sizeof(file)-1, "%s/%s.%04d.cfg", path, g_CurrentMap, num);
		FILE *fp = fopen(file, "rt");
		if (!fp)
			break;
		fclose(fp);
	} while (++num);

	FILE *fp = fopen(file, "wt");
	fprintf(fp, "%s", g_MapEntities.c_str());
	fclose(fp);

	META_LOG(g_PLAPI, "Logged map %s to file %s", g_CurrentMap, file);
}

