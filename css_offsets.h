#ifndef _INCLUDE_OFFSETS_H
#define _INCLUDE_OFFSETS_H

#include "myutils.h"
#include "ZombiePlugin.h"

class cVirtualMap
{
public:

	cVirtualMap() 
	{
	};

	~cVirtualMap()
	{ 
		Q_strncpy( this->sSectionName, "", 1 );
		Q_strncpy( this->sPatch1, "", 1 );
		Q_strncpy( this->sPatch2, "", 1 );
	};

	cVirtualMap( const char *sName, bool bSig, bool bFuncOff = false, bool bOffset = false, bool bOffset1 = false, bool bOffset2  = false, bool bPatch1 = false, bool bPatch2 = false )
	{
		Q_strncpy( this->sSectionName, sName, 1024 );
		this->bSignature = bSig;
		this->bFunctionOffset = bFuncOff;
		this->bPatchOffset = bOffset;
		this->bPatchOffset1 = bOffset1;
		this->bPatchOffset2 = bOffset2;

		this->bPatch1 = bPatch1;
		this->bPatch2 = bPatch2;
	};

	char sSectionName[1024];
	char sPatch1[1024];
	char sPatch2[1024];
	int iFunctionOffset;
	void *pSigFunctionPointer;
	int iPatchOffset;
	int iPatchOffset1;
	int iPatchOffset2;

	bool bSignature;
	bool bFunctionOffset;
	bool bPatchOffset;
	bool bPatchOffset1;
	bool bPatchOffset2;
	bool bPatch1;
	bool bPatch2;

	bool cVirtualMap::operator==( const cVirtualMap& src ) const
	{
		return ( FStrEq( this->sSectionName, src.sSectionName ) );
	};

	bool cVirtualMap::operator!=( const cVirtualMap& src ) const
	{
		return ( !FStrEq( this->sSectionName, src.sSectionName ) );
	};
};

void InitialiseSigs();
void DestroySigs();

#ifndef WIN32
	extern cVirtualMap *cTempEnts;
	extern cVirtualMap *cEntList;
	extern cVirtualMap *cSetCollisionBounds;
	extern cVirtualMap *cCGameRules;
#else
	extern cVirtualMap *cLevelShutDown;
	extern cVirtualMap *cSetMinMaxSize;
	extern cVirtualMap *cCGameRules;
#endif

extern cVirtualMap *cUtilRemove;
extern cVirtualMap *cApplyAbsVelocity;
extern cVirtualMap *cSwitchTeam;
extern cVirtualMap *cWantsLagComp;
extern cVirtualMap *cWantsLagCompPatch;
extern cVirtualMap *cOnTakeDamage;
extern cVirtualMap *cGetFileWeaponInfoFromHandle;
extern cVirtualMap *cRoundRespawn;
extern cVirtualMap *cWeaponDrop;
extern cVirtualMap *cTermRound;
extern cVirtualMap *cGiveNamedItem;
extern cVirtualMap *cCalcDominationAndRevenge;
extern cVirtualMap *cFindEntityByClassname;
extern cVirtualMap *cGetPlayerMaxSpeed;
extern cVirtualMap *cWeaponCanSwitch;
extern cVirtualMap *cWeaponCanUse;
extern cVirtualMap *cTraceAttack;
extern cVirtualMap *cChangeTeam;
extern cVirtualMap *cCommitSuicide;
extern cVirtualMap *cEventKill;
extern cVirtualMap *cTeleport;
extern cVirtualMap *cPreThink;
extern cVirtualMap *cIPointsForKill;
extern cVirtualMap *cGetCollidable;
extern cVirtualMap *cGetModelIndex;
extern cVirtualMap *cSetModelIndex;
extern cVirtualMap *cGetDataDescMap;
extern cVirtualMap *cSetModel;
extern cVirtualMap *cKeyValue;
extern cVirtualMap *cKeyValueFloat;
extern cVirtualMap *cKeyValueVector;
extern cVirtualMap *cGetKeyValue;
extern cVirtualMap *cSetParent;
extern cVirtualMap *cAcceptInput;
extern cVirtualMap *cEventKilled;
extern cVirtualMap *cGetVelocity;
extern cVirtualMap *cWorldSpaceCenter;
extern cVirtualMap *cIgnite;
extern cVirtualMap *cGiveAmmo;
extern cVirtualMap *cWeaponSwitch;
extern cVirtualMap *cEventDying;
extern cVirtualMap *cGetSlot;
extern cVirtualMap *cIsBot;
extern cVirtualMap *cGetViewVectors;
extern cVirtualMap *cNoClipOff;

#define MESSAGE_CLIENT	3
#define	MESSAGE_MENU	10

typedef enum
{
        CSW_NONE = 0,
        CSW_P228,
        CSW_GLOCK,
        CSW_SCOUT,
        CSW_HEGRENADE,
        CSW_XM1014,
        CSW_C4,
        CSW_MAC10,
        CSW_AUG,
        CSW_SMOKEGRENADE,
        CSW_ELITE,
        CSW_FIVESEVEN,
        CSW_UMP45,
        CSW_SG550,
        CSW_GALIL,
        CSW_FAMAS,
        CSW_USP,
        CSW_AWP,
        CSW_MP5,
        CSW_M249,
        CSW_M3,
        CSW_M4A1,
        CSW_TMP,
        CSW_G3SG1,
        CSW_FLASHBANG,
        CSW_DEAGLE,
        CSW_SG552,
        CSW_AK47,
        CSW_KNIFE,
        CSW_P90,
        CSW_UNUSED1, // for if more weapons are added
     	CSW_UNUSED2,
     	CSW_UNUSED3,
     	CSW_UNUSED4,
     	
        // extra added here
		CSW_PRIMAMMO,
		CSW_SECAMMO,
		CSW_NVGS,
		CSW_VEST,
		CSW_VESTHELM,
		CSW_DEFUSEKIT,

        CSW_MAX,
};

#ifdef __linux__
	#include			<sys/mman.h>
	#define				PAGE_EXECUTE_READWRITE  PROT_READ|PROT_WRITE|PROT_EXEC
#endif
#define PAGE_SIZE       4096
#define ALIGN(ar)		((long)ar & ~(PAGE_SIZE-1))


#endif //_INCLUDE_OFFSETS_H
