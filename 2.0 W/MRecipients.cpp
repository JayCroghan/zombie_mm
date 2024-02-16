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

#undef GetClassName

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

void MRecipientFilter::AddAllPlayers( int maxClients )
{
    m_Recipients.RemoveAll();

    for( int i=1; i <= maxClients; i++ ) 
    {
		AddPlayer( i );
    }
}

void MRecipientFilter::AddPlayer( int index )
{
	edict_t *pEntity;
	if ( IsValidPlayer( index, &pEntity ) )
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


void MRecipientFilter::AddRecipientsByPAS( const Vector& origin, int maxClients, IVEngineServer *m_Engine )
{
	if ( maxClients == 1 )
	{
		AddAllPlayers( maxClients );
	}
	else
	{
		CBitVec< ABSOLUTE_PLAYER_LIMIT > playerbits;
		m_Engine->Message_DetermineMulticastRecipients( true, origin, playerbits );
		AddPlayersFromBitMask( playerbits );
	}
}



void MRecipientFilter::AddPlayersFromBitMask( CBitVec< ABSOLUTE_PLAYER_LIMIT >& playerbits )
{
	int index = playerbits.FindNextSetBit( 0 );

	while ( index > -1 )
	{
		AddPlayer( index + 1 );
		index = playerbits.FindNextSetBit( index + 1 );
	}
}


