#include "ZombiePlugin.h"
#include "VFunc.h"

class VfuncEmptyClass {};

datamap_t *CBaseEntity_GetDataDescMap(CBaseEntity *pThisPtr)
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[VFUNC_OFFSET_GETDATADESCMAP+VTABLE_OFFSET]; 

	union {datamap_t *(VfuncEmptyClass::*mfpnew)();
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (datamap_t *) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)();

}

bool CBaseEntity_KeyValue( CBaseEntity *pThisPtr, const char *szKeyName, const char *szValue )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[VFUNC_OFFSET_KEYVALUE+VTABLE_OFFSET]; 

	union {bool (VfuncEmptyClass::*mfpnew)( const char *, const char *);
	#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
	#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (bool) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( szKeyName, szValue);

}

void CBasePlayer_Event_Dying( CBasePlayer *pThisPtr )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[VFUNC_OFFSET_EVENT_DYING+VTABLE_OFFSET]; 
	union {void (VfuncEmptyClass::*mfpnew)( void );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
				struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( );
}

void CBaseEntity_Event_Killed( CBaseEntity *pThisPtr, const CTakeDamageInfo &info )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[VFUNC_OFFSET_EVENT_KILLED+VTABLE_OFFSET]; 
	union {void (VfuncEmptyClass::*mfpnew)( const CTakeDamageInfo & );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
				struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(info);
}

void CBaseEntity_ChangeTeam (CBaseEntity *pThisPtr, int TeamNum )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[VFUNC_OFFSET_CHANGETEAM+VTABLE_OFFSET]; 

	union {void (VfuncEmptyClass::*mfpnew)( int );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
				struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(TeamNum);
}

void CBaseEntity_Ignite(CBaseEntity *pThisPtr, float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner)
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[VFUNC_OFFSET_IGNITE+VTABLE_OFFSET]; 

	union {void (VfuncEmptyClass::*mfpnew)(float , bool , float , bool );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
				struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner);

}

CBaseEntity *CCSPlayer_GiveNamedItem(CBaseEntity *pThisPtr, const char *ItemName, int iSubType)
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[VFUNC_OFFSET_GIVENAMEDITEM+VTABLE_OFFSET]; 

	union {CBaseEntity *(VfuncEmptyClass::*mfpnew)(const char *, int );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
				struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (CBaseEntity *) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(ItemName, iSubType);
}