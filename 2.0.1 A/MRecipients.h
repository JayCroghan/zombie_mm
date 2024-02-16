
#ifndef _MRECIPIENT_FILTER_H
#define _MRECIPIENT_FILTER_H
#include "irecipientfilter.h"
#include "bitvec.h"
#include "tier1/utlvector.h"
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include <igameevents.h>
#include <iplayerinfo.h>
#include <networkstringtabledefs.h>
#include <IEngineSound.h>
#include <eiface.h>
#include "myutils.h"

class MRecipientFilter :
   public IRecipientFilter
{
public:
	MRecipientFilter(void);
	~MRecipientFilter(void);

	virtual bool IsReliable( void ) const;
	virtual bool IsInitMessage( void ) const;

	virtual int GetRecipientCount( void ) const;
	virtual int GetRecipientIndex( int slot ) const;
	virtual void AddRecipientsByPAS( const Vector& origin, int maxClients, IVEngineServer *m_Engine );
	virtual void AddTeam(int teamIdx, IVEngineServer *engine, IPlayerInfoManager *m_PlayerInfoManager, int maxClients);
	virtual void AddPlayer( int index );
	virtual void AddAllPlayers( int maxClients );
	virtual void AddPlayersFromBitMask( CBitVec< ABSOLUTE_PLAYER_LIMIT >& playerbits );
private:
   edict_t* GetEdictFromName(const char *name);
   bool m_bReliable;
   bool m_bInitMessage;
   CUtlVector< int > m_Recipients;
};

class CPASFilters : public MRecipientFilter
{
public:
	CPASFilters( void )
	{
	}

	CPASFilters( const Vector& origin, int maxClients, IVEngineServer *m_Engine )
	{
		AddRecipientsByPAS( origin, maxClients, m_Engine );
	}
};

#endif 
