
#ifndef _MRECIIENT_FILTER_H
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
	virtual void AddRecipientsByPAS( const Vector& origin, int maxClients, IVEngineServer *engine, IPlayerInfoManager *m_PlayerInfoManager );
	virtual void AddTeam(int teamIdx, IVEngineServer *engine, IPlayerInfoManager *m_PlayerInfoManager, int maxClients);
	virtual void AddPlayer( int index, IVEngineServer *engine, IPlayerInfoManager *playerinfomanager );
	virtual void AddAllPlayers( int maxClients, IVEngineServer *engine, IPlayerInfoManager *playerinfomanager );
	virtual void AddPlayersFromBitMask( CBitVec< ABSOLUTE_PLAYER_LIMIT >& playerbits, IVEngineServer *engine, IPlayerInfoManager *m_PlayerInfoManager );
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

	CPASFilters( const Vector& origin, int maxClients, IVEngineServer *engine, IPlayerInfoManager *m_PlayerInfoManager )
	{
		AddRecipientsByPAS( origin, maxClients, engine, m_PlayerInfoManager );
	}
};

#endif 
