// Stolen from hl2coding.com's forum tutorial on sounds.
// with additional public functions by L. Duke


#include "MRecipients.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"
#include <ISmmPlugin.h>
extern ISmmPlugin *g_PLAPI;
extern ISmmAPI *g_SMAPI;
#include "shake.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


inline bool FStrEq(const char *sz1, const char *sz2)
{
	if ( sz1 && sz2 )
	{
		return(stricmp(sz1, sz2) == 0);
	}
	return false;
}

MRecipientFilter::MRecipientFilter(void)
{
}

MRecipientFilter::~MRecipientFilter(void)
{
}

int MRecipientFilter::GetRecipientCount() const
{
   return m_Recipients.Size();
}

int MRecipientFilter::GetRecipientIndex(int slot) const
{
   if ( slot < 0 || slot >= GetRecipientCount() )
      return -1;

   return m_Recipients[ slot ];
}

bool MRecipientFilter::IsInitMessage() const
{
   return false;
}

bool MRecipientFilter::IsReliable() const
{
   return false;
}

void MRecipientFilter::AddAllPlayers( int maxClients, IVEngineServer *engine, IPlayerInfoManager *playerinfomanager )
{
    m_Recipients.RemoveAll();

    for( int i=1; i <= maxClients; i++ ) 
    {
		AddPlayer( i, engine, playerinfomanager );
    }
}

void MRecipientFilter::AddPlayer( int index, IVEngineServer *engine, IPlayerInfoManager *playerinfomanager )
{
	edict_t *pEntity =  engine->PEntityOfEntIndex( index );
    if( !pEntity || pEntity->IsFree() ) 
    {
            return;
    }
    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
    if( playerinfo && playerinfo->IsPlayer() && !playerinfo->IsFakeClient() )
    {
		m_Recipients.AddToTail( index );
	}
}

void MRecipientFilter::AddTeam(int teamIdx, IVEngineServer *engine, IPlayerInfoManager *m_PlayerInfoManager, int maxClients)
{
	edict_t *pEnt;
	IPlayerInfo *pInfo;
	for (int i=1; i<maxClients; i++)
	{
		pEnt = engine->PEntityOfEntIndex(i);
		if (FStrEq(pEnt->GetClassName(),"player"))
		{
			pInfo = m_PlayerInfoManager->GetPlayerInfo(pEnt);
			if (pInfo->GetTeamIndex() == teamIdx)
			{
				m_Recipients.AddToTail(i);
			}
		}
	}
}


void MRecipientFilter::AddRecipientsByPAS( const Vector& origin, int maxClients, IVEngineServer *m_Engine, IPlayerInfoManager *m_PlayerInfoManager )
{
	if ( maxClients == 1 )
	{
		AddAllPlayers( maxClients, m_Engine, m_PlayerInfoManager  );
	}
	else
	{
		META_LOG( g_PLAPI, "Creating BitVec.." );
		CBitVec< ABSOLUTE_PLAYER_LIMIT > playerbits;
		m_Engine->Message_DetermineMulticastRecipients( true, origin, playerbits );
		META_LOG( g_PLAPI, "Aad Players.." );
		AddPlayersFromBitMask( playerbits, m_Engine, m_PlayerInfoManager );
	}
}

void MRecipientFilter::AddPlayersFromBitMask( CBitVec< ABSOLUTE_PLAYER_LIMIT >& playerbits, IVEngineServer *engine, IPlayerInfoManager *m_PlayerInfoManager )
{
	int index = playerbits.FindNextSetBit( 0 );

	while ( index > -1 )
	{
		AddPlayer( index, engine, m_PlayerInfoManager);
		index = playerbits.FindNextSetBit( index + 1 );
	}
}


