#undef GetClassName

#include "ZMClass.h"
#include "cvars.h"

ZombieClassManager			m_ClassManager;

ZombieClass::ZombieClass()
{
	this->sClassname[0] = '\0';
	this->sModel[0] = '\0';
}

void ZombieClass::SetName( const char *sName )
{
	//this->sClassname = new char[Q_strlen(sName) + 1];
	Q_strncpy( this->sClassname, sName, Q_strlen( sName ) + 1 );
}

void ZombieClass::SetModelName( const char *sModel )
{
	//this->sModel = new char[Q_strlen(sModel) + 1];
	Q_strncpy( this->sModel, sModel, Q_strlen( sModel ) + 1 );
}

const char *ZombieClass::GetName()
{
	return this->sClassname;
}

const char *ZombieClass::GetModelName()
{
	return this->sModel;
}


ZombieClassManager::ZombieClassManager( void )
{
}

ZombieClass *ZombieClassManager::GetClass( int iClass, bool bIgnoreEnabled )
{
	if ( (!this->Enabled() && !bIgnoreEnabled) || !m_Classes.IsValidIndex( iClass ) )
	{
		return NULL;
	}
	else
	{
		return &m_Classes[iClass];
	}
}

ZombieClass *ZombieClassManager::GetClass( const char *sName )
{
	int x = 0;
	for ( x = 0; x < m_Classes.Count(); x++ )
	{
		if ( FStrEq( m_Classes[x].GetName(), sName ) )
		{
			return &m_Classes[x];
		}
	}
	return NULL;
}

void ZombieClassManager::Clear()
{
	if ( this->Count() > 0 )
	{
		this->m_Classes.RemoveAll();
	}
}

int ZombieClassManager::AddClass( ZombieClass *zClass )
{
	return m_Classes.AddToTail( *zClass );
}

int ZombieClassManager::Count()
{
	return m_Classes.Count();
}

bool ZombieClassManager::Enabled()
{
	return (g_bZombieClasses && zombie_classes.GetBool() && this->Count() > 0);
}

ZombieClass *ZombieClassManager::RandomClass( void )
{
	int iRand = 0;
	iRand = RandomInt( 0, ( m_ClassManager.Count() - 1 ) );
	return this->GetClass( iRand );
}