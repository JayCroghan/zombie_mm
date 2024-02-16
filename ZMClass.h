#ifndef _INCLUDE_ZMCLASS_H
#define _INCLUDE_ZMCLASS_H

#undef GetClassName

#include "ZombiePlugin.h"

class ZombieClass
{ 
public:

	ZombieClass();

	ZombieClass( int iID, const char*sName, const char* sModel )
	{
		this->iClassId = iID;
		this->SetName( sName );
		this->SetModelName( sModel );
	};

	ZombieClass( int iID, const char*sName )
	{
		this->iClassId = iID;
		this->SetName( sName );
	};

	ZombieClass( int iID )
	{
		this->iClassId = iID;
	}

	~ZombieClass()
	{
		this->iClassId = -1;
		this->sClassname[0] = '\0';
		this->sModel[0] = '\0';
	};

public:
	void SetName( const char *sName );
	void SetModelName( const char *sModel );
	const char *GetName();
	const char *GetModelName();

public:
	char		sClassname[500];
	char		sModel[MAX_PATH];

	int			iHealth;
	float		fSpeed;
	float		fSpeedDuck;
	float		fSpeedRun;
	int			iJumpHeight;
	float		fKnockback;
	bool		bInUse;
	int			iPrecache;
	int			iHeadshots;
	bool		bHeadShotsOnly;
	int			iRegenHealth;
	float		fRegenTimer;
	float		fGrenadeMultiplier;
	float		fGrenadeKnockback;
	int			iHealthBonus;
	bool		bFallDamage;

	int			iClassId;
};

class ZombieClassManager
{
public:
	ZombieClassManager( void );

	~ZombieClassManager()
	{
		m_Classes.RemoveAll();
	};

	ZombieClass *operator [] ( unsigned int index )
	{
		if ( !m_Classes.IsValidIndex(index ) )
		{
			return NULL;
		}
		else
		{
			return &m_Classes[index];
		}
	}

public:
	ZombieClass *GetClass( int iClass, bool bIgnoreEnabled = false );
	ZombieClass *GetClass( const char *sName );
	ZombieClass *RandomClass( void );

	int Count();
	int AddClass( ZombieClass *zClass );
	void Clear();
	bool Enabled();

private:
	CUtlVector<ZombieClass> m_Classes;
};

extern ZombieClassManager	m_ClassManager;

#endif