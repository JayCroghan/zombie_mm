#ifndef _INCLUDE_MYUTILS_H
#define _INCLUDE_MYUTILS_H

#include "ZombiePlugin.h"

// **********************************************************

// return pEntity for a user id (NOT ENTITY ID!)
edict_t *EdictOfUserId( int UserId );

ArrayLengthSendProxyFn UTIL_FindOffsetArray(const char *ClassName, const char *ArrayProp, void *vTeam );
int UTIL_FindOffsetMap( CBaseEntity *pBaseEntity, const char *ClassName, const char *PropertyName );

// get offset for ClassName and Property
//template <class TheClass> bool UTIL_GetProperty(const char *ClassName, const char *PropertyName, edict_t *pEntity, TheClass *Value);
//template <class TheClass> bool UTIL_SetProperty(const char *ClassName, const char *PropertyName, edict_t *pEntity, TheClass NewValue);
int UTIL_FindOffset(const char *ClassName, const char *Property);

// get offset for ClassName, DataTable, and PropertyName
//template <class TheClass> bool UTIL_GetPropertyTable(const char *ClassName, const char *DataTableName, const char *PropertyName, edict_t *pEntity, TheClass *Value);
//template <class TheClass> bool UTIL_SetPropertyTable(const char *ClassName, const char *DataTableName, const char *PropertyName, edict_t *pEntity, TheClass NewValue);
int UTIL_FindOffsetTable(const char *ClassName, const char *DataTableName, const char *PropertyName, const char *sLocal = NULL);




template <class TheClass> bool UTIL_GetProperty( int iOffset, edict_t *pEntity, TheClass *Value )
{
	if ( !pEntity || pEntity->IsFree() )
	{
		return false;
	}
	if ( iOffset > 0 )
	{
		TheClass *iptr = NULL;
		void *pVoid = pEntity->GetUnknown();
		int iloc = (int)pVoid + iOffset;
		iptr = (TheClass *)iloc;
		if ( iptr )
		{
			*Value = *iptr;
			return true;
		}
	}
	return false;
}

template <class TheClass> bool UTIL_SetProperty( int iOffset, edict_t *pEntity, TheClass NewValue )
{
	if ( !pEntity || pEntity->IsFree() )
	{
		return false;
	}
	if ( iOffset > 0 )
	{
		//int *iptr = (int *)(pEntity->GetUnknown() + (offset/sizeof(int)));
		//*iptr = NewValue;
		TheClass *iptr = NULL;
		void *pVoid = pEntity->GetUnknown();
		int iloc = (int)pVoid + iOffset;
		iptr = (TheClass *)iloc;
		if ( iptr )
		{
			*iptr = NewValue;
			pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
			return true;
		}
	}	
	return false;
}


#endif 