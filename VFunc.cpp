#include "ZombiePlugin.h"
#include "VFunc.h"
#include "css_offsets.h"

class VfuncEmptyClass {};

ICollideable *CBaseEntity_GetCollideable( CBaseEntity *pThisPtr )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cGetCollidable->iFunctionOffset ]; //VFUNC_OFFSET_GETCOLLIDABLE + VTABLE_OFFSET ]; 

	union {ICollideable * (VfuncEmptyClass::*mfpnew)( );
	#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
	#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (ICollideable *) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( );
}

int CBaseCombatCharacter_GiveAmmo( CBaseCombatCharacter *pThisPtr, int iCount, int iAmmoIndex, bool bSuppressSound )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cGiveAmmo->iFunctionOffset ];//VFUNC_OFFSET_GIVEAMMO+VTABLE_OFFSET]; 

	union {int (VfuncEmptyClass::*mfpnew)( int, int, bool );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else 
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (int) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( iCount, iAmmoIndex, bSuppressSound );
}

bool CBaseEntity_AcceptInput( CBaseEntity *pThisPtr, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cAcceptInput->iFunctionOffset ]; //VFUNC_OFFSET_ACCEPTINPUT+VTABLE_OFFSET]; 

	union {bool (VfuncEmptyClass::*mfpnew)( const char *, CBaseEntity *, CBaseEntity *, variant_t, int );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else 
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (bool) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( szInputName, pActivator, pCaller, Value, outputID );
}

bool CCSPlayer_Weapon_Switch( CCSPlayer *pThisPtr, CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cWeaponSwitch->iFunctionOffset ]; //VFUNC_OFFSET_WEAPON_SWITCH+VTABLE_OFFSET]; 

	union {bool (VfuncEmptyClass::*mfpnew)( CBaseCombatWeapon *, int );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else 
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (bool) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( pWeapon, viewmodelindex );
}

CBaseCombatWeapon* CBaseCombatCharacter_Weapon_GetSlot( CBaseCombatCharacter *pThisPtr, int slot )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cGetSlot->iFunctionOffset ];//VFUNC_OFFSET_GETSLOT+VTABLE_OFFSET]; 
	
	union {CBaseCombatWeapon *(VfuncEmptyClass::*mfpnew)( int );
#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
#else 
		struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (CBaseCombatWeapon *) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( slot );
}

datamap_t *CBaseEntity_GetDataDescMap( CBaseEntity *pThisPtr )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cGetDataDescMap->iFunctionOffset ]; //VFUNC_OFFSET_GETDATADESCMAP+VTABLE_OFFSET]; 

	union {datamap_t *(VfuncEmptyClass::*mfpnew)();
#ifndef __linux__
        void *addr;	} u; 	u.addr = func;
#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

	return (datamap_t *) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)();

}

bool CBaseEntity_KeyValueFloat( CBaseEntity *pThisPtr, const char *szKeyName, float *flValue )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cKeyValueFloat->iFunctionOffset ]; //VFUNC_OFFSET_KEYVALUEFLOAT]; //+VTABLE_OFFSET]; 

	union {bool (VfuncEmptyClass::*mfpnew)( const char *, float *);
	#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
	#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (bool) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( szKeyName, flValue );
}

bool CBaseEntity_KeyValueVector( CBaseEntity *pThisPtr, const char *szKeyName, const Vector &vecValue )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cKeyValueVector->iFunctionOffset ]; //VFUNC_OFFSET_KEYVALUEVECTOR]; //+VTABLE_OFFSET]; 

	union {bool (VfuncEmptyClass::*mfpnew)( const char *, const Vector &);
	#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
	#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (bool) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( szKeyName, vecValue );
}

bool CBaseEntity_GetKeyValue( CBaseEntity *pThisPtr, const char *szKeyName, char *szValue, int iMaxLen )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cGetKeyValue->iFunctionOffset ]; //VFUNC_OFFSET_GETKEYVALUE+VTABLE_OFFSET]; 

	union {bool (VfuncEmptyClass::*mfpnew)( const char *, char *, int);
	#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
	#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (bool) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( szKeyName, szValue, iMaxLen );
}

bool CBaseEntity_KeyValue( CBaseEntity *pThisPtr, const char *szKeyName, const char *szValue )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cKeyValue->iFunctionOffset ];//VFUNC_OFFSET_KEYVALUE];//+VTABLE_OFFSET]; 

	union {bool (VfuncEmptyClass::*mfpnew)( const char *, const char *);
	#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
	#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (bool) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( szKeyName, szValue);
}

static CViewVectors g_DefaultViewVectors(
	Vector( 0, 0, 64 ),
	
	Vector(-16, -16, 0 ),
	Vector( 16,  16,  72 ),
	
	Vector(-16, -16, 0 ),
	Vector( 16,  16,  36 ),
	Vector( 0, 0, 28 ),
	
	Vector(-10, -10, -10 ),
	Vector( 10,  10,  10 ),
	
	Vector( 0, 0, 14 )
);

const CViewVectors* CGameRules_GetViewVectors( CGameRules *pThisPtr )
{
	//void *rules = NULL;
	//memcpy(&rules, reinterpret_cast<void*>(pThisPtr),sizeof(char*));

	return &g_DefaultViewVectors;
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cGetViewVectors->iFunctionOffset ]; //VFUNC_OFFSET_GETVIEWVECTORS + VTABLE_OFFSET ]; 

	union {const CViewVectors* (VfuncEmptyClass::*mfpnew)( );
	#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
	#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (const CViewVectors *) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( );
}

int CBaseEntity_GetModelIndex( CBaseEntity *pThisPtr )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cGetModelIndex->iFunctionOffset ];//VFUNC_OFFSET_GETMODELINDEX + VTABLE_OFFSET ]; 

	union {int (VfuncEmptyClass::*mfpnew)( );
	#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
	#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (int) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( );
}

//void CBaseEntity_Teleport( CBaseEntity *pThisPtr, Vector *newPosition, QAngle *newAngles, Vector *newVelocity )
//{
//	void **this_ptr = *(void ***)&pThisPtr;
//	void **vtable = *(void ***)pThisPtr;
//	void *func = vtable[ VFUNC_OFFSET_TELEPORT + VTABLE_OFFSET ]; 
//
//	union {void (VfuncEmptyClass::*mfpnew)( const Vector *, const QAngle *, const Vector * );
//	#ifndef __linux__
//		void *addr;	} u; 	u.addr = func;
//	#else 
//			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
//	#endif
//
//	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( newPosition, newAngles, newVelocity );
//}

/*
void CBaseEntity_SetOwnerEntity( CBaseEntity *pThisPtr, CBaseEntity* pOwner )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ VFUNC_OFFSET_SETOWNERENTITY + VTABLE_OFFSET ]; 

	union {void (VfuncEmptyClass::*mfpnew)( CBaseEntity * );
	#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
	#else 
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( pOwner );
}
*/


void CBaseEntity_SetParent( CBaseEntity *pThisPtr, CBaseEntity* pNewParent, int iAttachment )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cSetParent->iFunctionOffset ]; //VFUNC_OFFSET_SETPARENT + VTABLE_OFFSET ]; 

	union {void (VfuncEmptyClass::*mfpnew)( CBaseEntity *, int );
	#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
	#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( pNewParent, iAttachment );
}

void CBaseAnimating_GetVelocity( CBaseAnimating *pThisPtr, Vector *vVelocity, AngularImpulse *vAngVelocity )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cGetVelocity->iFunctionOffset ]; //VFUNC_OFFSET_GETVELOCITY + VTABLE_OFFSET ]; 

	union {void (VfuncEmptyClass::*mfpnew)( Vector *, AngularImpulse * );
	#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
	#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( vVelocity, vAngVelocity );
}

void CBasePlayer_SetModel( CBasePlayer *pThisPtr, const char *sModel )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cSetModel->iFunctionOffset ]; //VFUNC_OFFSET_SETMODEL + VTABLE_OFFSET ]; 

	union {void (VfuncEmptyClass::*mfpnew)( const char * );
	#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
	#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( sModel );
}

void CBaseEntity_SetModelIndex( CBaseEntity *pThisPtr, int Index )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cSetModelIndex->iFunctionOffset ]; //VFUNC_OFFSET_SETMODELINDEX + VTABLE_OFFSET ]; 

	union {void (VfuncEmptyClass::*mfpnew)( int );
	#ifndef __linux__
		void *addr;	} u; 	u.addr = func;
	#else /* GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 */
			struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( Index );
}

void CBasePlayer_Event_Dying( CBasePlayer *pThisPtr )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cEventDying->iFunctionOffset ]; //VFUNC_OFFSET_EVENT_DYING+VTABLE_OFFSET]; 
	union {void (VfuncEmptyClass::*mfpnew)( void );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
				struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( );
}

void CCSPlayer_Event_Killed( CCSPlayer *pThisPtr, const CTakeDamageInfo &info )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cEventKilled->iFunctionOffset ]; //VFUNC_OFFSET_EVENT_KILLED+VTABLE_OFFSET]; 
	union {void (VfuncEmptyClass::*mfpnew)( const CTakeDamageInfo & );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
				struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(info);
}

void CCSPlayer_ChangeTeam (CCSPlayer *pThisPtr, int TeamNum )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cChangeTeam->iFunctionOffset ]; //VFUNC_OFFSET_CHANGETEAM+VTABLE_OFFSET]; 

	union {void (VfuncEmptyClass::*mfpnew)( int );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
				struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(TeamNum);
}

void CBaseAnimating_Ignite(CBaseAnimating *pThisPtr, float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner)
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cIgnite->iFunctionOffset ]; //VFUNC_OFFSET_IGNITE+VTABLE_OFFSET]; 

	union {void (VfuncEmptyClass::*mfpnew)( float , bool , float , bool );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
				struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	(void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );

}

const Vector&	CBaseEntity_WorldSpaceCenter( CBaseEntity *pThisPtr )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cWorldSpaceCenter->iFunctionOffset ];//VFUNC_OFFSET_WORLDSPACECENTER + VTABLE_OFFSET ]; 

	union {const Vector &(VfuncEmptyClass::*mfpnew)( );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
				struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( );
}

CBaseEntity *CCSPlayer_GiveNamedItem(CCSPlayer *pThisPtr, const char *ItemName, int iSubType)
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cGiveNamedItem->iFunctionOffset ]; //VFUNC_OFFSET_GIVENAMEDITEM+VTABLE_OFFSET]; 

	union {CBaseEntity *(VfuncEmptyClass::*mfpnew)(const char *, int );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
				struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (CBaseEntity *) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(ItemName, iSubType);
}

bool CBasePlayer_IsBot( CBasePlayer *pThisPtr )
{
	void **this_ptr = *(void ***)&pThisPtr;
	void **vtable = *(void ***)pThisPtr;
	void *func = vtable[ cIsBot->iFunctionOffset ];//VFUNC_OFFSET_ISBOT+VTABLE_OFFSET]; 

	union {bool (VfuncEmptyClass::*mfpnew)( );
	#ifndef __linux__
			void *addr;	} u; 	u.addr = func;
	#else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
				struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
	#endif

	return (bool) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( );
}
