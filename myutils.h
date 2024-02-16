#ifndef ZM_INCLUDE_MYUTILS_H
#define ZM_INCLUDE_MYUTILS_H



#include "ZombiePlugin.h"
#include "cbase.h"
#include <sh_list.h>
#include <sh_tinyhash.h>
#include <sm_trie_tpl.h>
#include "sm_trie.h"
#include "dt_send.h"
#include "sp_vm_types.h"
#include <ISmmPlugin.h>
extern ISmmPlugin *g_PLAPI;
extern ISmmAPI *g_SMAPI;

// **********************************************************
using namespace SourceHook;

enum PropType
{
	Prop_Send = 0,
	Prop_Data
};

struct sm_sendprop_info_t
{
	SendProp *prop;					/**< Property instance. */
	unsigned int actual_offset;		/**< Actual computed offset. */
};

struct DataTableInfo
{
	ServerClass *sc;
	KTrie<sm_sendprop_info_t> lookup;
};

struct DataMapTrie
{
	DataMapTrie() : trie(NULL) {}
	Trie *trie;
};


#define FIND_PROP_DATA(td) \
	datamap_t *pMap; \
	if ((pMap = CBaseEntity_GetDataDescMap(pEntity)) == NULL) \
	{ \
		if (error) \
		{ \
			g_SMAPI->Format(error, maxlength, "Could not retrieve datamap" ); \
			return false; \
		} \
	} \
	if ((td = FindInDataMap(pMap, sProp)) == NULL) \
	{ \
		if (error) \
		{ \
			g_SMAPI->Format(error, maxlength, "Property \"%s\" not found (entity %d/%s)", sProp, iEntIndex, class_name ); \
			return false; \
		} \
	};


#define FIND_PROP_SEND(pProp) \
	IServerUnknown *pUnk = (IServerUnknown *)pEntity; \
	IServerNetworkable *pNet = pUnk->GetNetworkable(); \
	if (!pNet) \
	{ \
		if (error) \
		{ \
			g_SMAPI->Format(error, maxlength, "Edict %d (%d) is not networkable", ReferenceToIndex(iEntIndex), iEntIndex ); \
			return false; \
		} \
	} \
	if (!FindSendPropInfo(pNet->GetServerClass()->GetName(), sProp, pProp)) \
	{ \
		if (error) \
		{ \
			g_SMAPI->Format(error, maxlength, "Property \"%s\" not found (entity %d/%s)", sProp, iEntIndex, class_name ); \
			return false; \
		} \
	};


int MatchFieldAsInteger(int field_type);

bool FindSendPropInfo(const char *classname, const char *offset, sm_sendprop_info_t *info);

DataTableInfo *_FindServerClass(const char *classname);

void InitZMUtils();

void UnloadZMUtils();

bool UTIL_FindInSendTable(SendTable *pTable, const char *name, sm_sendprop_info_t *info, unsigned int offset);

CBaseEntity *ReferenceToEntity(cell_t entRef);

CEntInfo *LookupEntity(int entIndex);

int ReferenceToIndex(cell_t entRef);

CBaseEntity *ReferenceToEntity(cell_t entRef);

bool IndexToAThings(cell_t num, CBaseEntity **pEntData, edict_t **pEdictData);

typedescription_t *FindInDataMap(datamap_t *pMap, const char *offset);

void SetEdictStateChanged(edict_t *pEdict, unsigned short offset);

typedescription_t *UTIL_FindInDataMap(datamap_t *pMap, const char *name);


bool SetEntPropVector(int iEntIndex, PropType eSendType, const char *sProp, Vector vVector);
bool GetEntPropVector(int iEntIndex, PropType eSendType, const char *sProp, Vector &vVector);





template <class TheClass> bool GetEntProp( int iEntIndex, PropType eSendType, const char *sProp, TheClass *NewValue, int iBitCount = 0 )
{
	CBaseEntity *pEntity;
	int offset;
	const char *class_name;
	edict_t *pEdict;
	int bit_count;
	char error[200];
	int maxlength = sizeof( error );


	if (!IndexToAThings(iEntIndex, &pEntity, &pEdict))
	{
		META_LOG( g_PLAPI, "Entity %d (%d) is invalid", ReferenceToIndex(iEntIndex), iEntIndex );
		return false;
	}

	if ( !pEdict || ( class_name = pEdict->GetClassName( )) == NULL)
	{
		class_name = "";
	}

	switch (eSendType)
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			if ((bit_count = MatchFieldAsInteger(td->fieldType)) == 0)
			{
				META_LOG( g_PLAPI, "Data field %s is not an integer (%d)", 
					sProp,
					td->fieldType);
				return false;
			}

			offset = td->fieldOffset[TD_OFFSET_NORMAL];
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_Int)
			{
				META_LOG( g_PLAPI, "SendProp %s is not an integer ([%d,%d] != %d)",
					sProp,
					info.prop->GetType(),
					info.prop->m_nBits,
					DPT_Int);
				return false;
			}

			bit_count = info.prop->m_nBits;
			offset = info.actual_offset;
			break;
		}
	default:
		{
			META_LOG( g_PLAPI, "Invalid Property type %d", eSendType );
			return false;
		}
	}

	if (bit_count < 1)
	{
		bit_count = iBitCount * 8;
	}

	if (bit_count >= 17)
	{
		*NewValue =  *(int32_t *)((uint8_t *)pEntity + offset);
	}
	else if (bit_count >= 9)
	{
		*NewValue =  *(int16_t *)((uint8_t *)pEntity + offset);
	}
	else if (bit_count >= 2)
	{
		*NewValue =  *(int8_t *)((uint8_t *)pEntity + offset);
	}
	else
	{
		*NewValue =  *(bool *)((uint8_t *)pEntity + offset) ? 1 : 0;
	}

	return true;
}

template <class TheClass> bool SetEntProp( int iEntIndex, PropType eSendType, const char *sProp, TheClass NewValue, int iBitCount = 0 )
{
	CBaseEntity *pEntity;
	int offset;
	const char *class_name;
	edict_t *pEdict;
	int bit_count;
	char error[200];
	int maxlength = sizeof( error );


	if (!IndexToAThings(iEntIndex, &pEntity, &pEdict))
	{
		META_LOG( g_PLAPI, "Entity %d (%d) is invalid", ReferenceToIndex(iEntIndex), iEntIndex );
		return false;
	}

	if ( !pEdict || ( class_name = pEdict->GetClassName( )) == NULL)
	{
		class_name = "";
	}

	switch (eSendType)
	{
	case Prop_Data:
		{
			typedescription_t *td;

			FIND_PROP_DATA(td);

			if ((bit_count = MatchFieldAsInteger(td->fieldType)) == 0)
			{
				META_LOG( g_PLAPI, "Data field %s is not an integer (%d)", 
					sProp,
					td->fieldType);
			}
			offset = td->fieldOffset[TD_OFFSET_NORMAL];
			break;
		}
	case Prop_Send:
		{
			sm_sendprop_info_t info;

			FIND_PROP_SEND(&info);

			if (info.prop->GetType() != DPT_Int)
			{
				META_LOG( g_PLAPI, "SendProp %s is not an integer ([%d,%d] != %d)",
					sProp,
					info.prop->GetType(),
					info.prop->m_nBits,
					DPT_Int);
			}

			bit_count = info.prop->m_nBits;
			offset = info.actual_offset;
			break;
		}
	default:
		{
			META_LOG( g_PLAPI, "Invalid Property type %d", eSendType );
		}
	}

	if (bit_count < 1)
	{
		bit_count = iBitCount * 8;
	}

	if (bit_count >= 17)
	{
		*(int32_t *)((uint8_t *)pEntity + offset) = NewValue;
	}
	else if (bit_count >= 9)
	{
		*(int16_t *)((uint8_t *)pEntity + offset) = (int16_t)NewValue;
	}
	else if (bit_count >= 2)
	{
		*(int8_t *)((uint8_t *)pEntity + offset) = (int8_t)NewValue;
	}
	else
	{
		*(bool *)((uint8_t *)pEntity + offset) = NewValue ? true : false;
	}

	return 0;
}


bool SetEntPropClr( int iEntIndex, PropType eSendType, const char *sProp, color32 NewValue );





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


bool IsValidEnt( CBaseEntity *pEntity );
bool IsValidEnt( edict_t *pEntity );
bool IsValidPlayer( int iPlayer, edict_t **pEntity = NULL, CBaseEntity **pBaseEntity = NULL );
bool IsValidPlayer( int iPlayer, CBaseEntity **pBaseEntity = NULL, edict_t **pEntity = NULL );
bool IsValidPlayer( edict_t *pEntity, int *iIndex = NULL, CBaseEntity **pBaseEntity = NULL );
bool IsValidPlayer( CBaseEntity *pBaseEntity, int *iIndex = NULL, edict_t **pEntity = NULL );

extern bool IsValidPlayer( int iPlayer, edict_t **pEntity, CBaseEntity **pBaseEntity );
extern bool IsValidPlayer( int iPlayer, CBaseEntity **pBaseEntity, edict_t **pEntity );
extern bool IsValidPlayer( edict_t *pEntity, int *iIndex, CBaseEntity **pBaseEntity );
extern bool IsValidPlayer( CBaseEntity *pBaseEntity, int *iIndex, edict_t **pEntity );

RenderMode_t GetRenderMode(CBaseEntity *pBase);
bool SetRenderMode(CBaseEntity *pBase, RenderMode_t mode);
bool SetClrRender(CBaseEntity *pBase, color32 clr);

#if defined __GNUC__ || defined HAVE_STDINT_
	#include <stdint.h>
#else
	#if !defined HAVE_STDINT_H
		typedef unsigned __int64	uint64_t;		/**< 64bit unsigned integer */
		typedef __int64				int64_t;		/**< 64bit signed integer */
		typedef unsigned __int32	uint32_t;		/**< 32bit unsigned integer */
		typedef __int32				int32_t;		/**< 32bit signed integer */
		typedef unsigned __int16	uint16_t;		/**< 16bit unsigned integer */
		typedef __int16				int16_t;		/**< 16bit signed integer */
		typedef unsigned __int8		uint8_t;		/**< 8bit unsigned integer */
		typedef __int8				int8_t;			/**< 8bit signed integer */
		#define HAVE_STDINT_H
	#endif
#endif

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
		if ( pVoid )
		{
			int iloc = (int)pVoid + iOffset;
			iptr = (TheClass *)iloc;
			if ( iptr )
			{
				*Value = *iptr;
				return true;
			}
		}
	}
	return false;
}

template <class TheClass> bool UTIL_SetProperty( int iOffset, edict_t *pEntity, TheClass NewValue, bool bChanged = true, bool NewWay = false )
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
		if ( pVoid )
		{
			/*if ( NewWay )
			{
				if ( bChanged )
					pEntity->m_fStateFlags |= FL_EDICT_CHANGED;

				*((uint8_t *)pEntity + iOffset) = NewValue;
				
				return true;
			}
			else	
			{*/
				int iloc = (int)pVoid + iOffset;
				iptr = (TheClass *)iloc;
				if ( iptr )
				{
					if ( bChanged )
						SetEdictStateChanged( pEntity, iOffset );

					*iptr = NewValue;
					return true;
				}
			//}
		}
	}	
	return false;
}


#endif 