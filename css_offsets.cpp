#include "ZombiePlugin.h"
#include "css_offsets.h"
#include "myutils.h"

#ifndef WIN32
	cVirtualMap *cTempEnts = NULL;
	cVirtualMap *cEntList = NULL;
	cVirtualMap *cSetCollisionBounds = NULL;
	cVirtualMap *cCGameRules = NULL;
#else
	cVirtualMap *cLevelShutDown = NULL;
	cVirtualMap *cSetMinMaxSize = NULL;
	cVirtualMap *cCGameRules = NULL;
#endif

cVirtualMap *cUtilRemove = NULL;
cVirtualMap *cApplyAbsVelocity = NULL;
cVirtualMap *cSwitchTeam = NULL;
cVirtualMap *cWantsLagComp = NULL;
cVirtualMap *cWantsLagCompPatch = NULL;
cVirtualMap *cOnTakeDamage = NULL;
cVirtualMap *cGetFileWeaponInfoFromHandle = NULL;
cVirtualMap *cRoundRespawn = NULL;
cVirtualMap *cWeaponDrop = NULL;
cVirtualMap *cTermRound = NULL;
cVirtualMap *cGiveNamedItem = NULL;
cVirtualMap *cCalcDominationAndRevenge = NULL;
cVirtualMap *cFindEntityByClassname = NULL;
cVirtualMap *cGetPlayerMaxSpeed = NULL;
cVirtualMap *cWeaponCanSwitch = NULL;
cVirtualMap *cWeaponCanUse = NULL;
cVirtualMap *cTraceAttack = NULL;
cVirtualMap *cChangeTeam = NULL;
cVirtualMap *cCommitSuicide = NULL;
cVirtualMap *cEventKill = NULL;
cVirtualMap *cTeleport = NULL;
cVirtualMap *cPreThink = NULL;
cVirtualMap *cIPointsForKill = NULL;
cVirtualMap *cGetCollidable = NULL;
cVirtualMap *cGetModelIndex = NULL;
cVirtualMap *cSetModelIndex = NULL;
cVirtualMap *cGetDataDescMap = NULL;
cVirtualMap *cSetModel = NULL;
cVirtualMap *cKeyValue = NULL;
cVirtualMap *cKeyValueFloat = NULL;
cVirtualMap *cKeyValueVector = NULL;
cVirtualMap *cGetKeyValue = NULL;
cVirtualMap *cSetParent = NULL;
cVirtualMap *cAcceptInput = NULL;
cVirtualMap *cEventKilled = NULL;
cVirtualMap *cGetVelocity = NULL;
cVirtualMap *cWorldSpaceCenter = NULL;
cVirtualMap *cIgnite = NULL;
cVirtualMap *cGiveAmmo = NULL;
cVirtualMap *cWeaponSwitch = NULL;
cVirtualMap *cEventDying = NULL;
cVirtualMap *cGetSlot = NULL;
cVirtualMap *cIsBot = NULL;
cVirtualMap *cGetViewVectors = NULL;
cVirtualMap *cNoClipOff = NULL;

void InitialiseSigs()
{

	#ifndef WIN32
		cTempEnts = new cVirtualMap( "gTempEnts", true );
		cEntList = new cVirtualMap( "gEntList", true );
		cSetCollisionBounds = new cVirtualMap( "CBaseEntity_SetCollisionBounds", true );
		cCGameRules = new cVirtualMap( "CGameRules", true );
	#else
		cLevelShutDown = new cVirtualMap( "LevelShutDown", true, false, true );
		cSetMinMaxSize = new cVirtualMap( "SetMinMaxSize", true );
		cCGameRules = new cVirtualMap( "CGameRules", true, false, true );
	#endif

	//cVirtualMap cCreateEntityByName = new cVirtualMap( "CreateEntityByName", true );
	cUtilRemove = new cVirtualMap( "UtilRemove", true );
	cApplyAbsVelocity = new cVirtualMap( "CBaseEntity_ApplyAbsVelocity", true );
	cSwitchTeam = new cVirtualMap( "CCSPlayer_SwitchTeam", true );
	cWantsLagComp = new cVirtualMap( "WantsLagComp", true, false, true, false, false, true );
	cWantsLagCompPatch = new cVirtualMap( "WantsLagComp", true, false, true, false, false, true );
	cOnTakeDamage = new cVirtualMap( "OnTakeDamage", true, true, false, true, true, true, true );
	cGetFileWeaponInfoFromHandle = new cVirtualMap( "GetFileWeaponInfoFromHandle", true );
	cRoundRespawn = new cVirtualMap( "CCSPlayer_RoundRespawn", true );
	cWeaponDrop = new cVirtualMap( "CBasePlayer_WeaponDrop", true );
	cTermRound = new cVirtualMap( "CCSGame_TermRound", true );
	cGiveNamedItem = new cVirtualMap( "CCSPlayer_GiveNamedItem", true, true );
	cCalcDominationAndRevenge = new cVirtualMap( "CalcDominationAndRevenge", true, false, true, false, false, true );
	cFindEntityByClassname = new cVirtualMap( "FindEntityByClassname", true );
	cGetPlayerMaxSpeed = new cVirtualMap( "GetPlayerMaxSpeed", false, true  );
	cWeaponCanSwitch = new cVirtualMap( "Weapon_CanSwitchTo", false, true );
	cWeaponCanUse = new cVirtualMap( "Weapon_CanUse", false, true );
	cTraceAttack = new cVirtualMap( "TraceAttack", false, true );
	cChangeTeam = new cVirtualMap( "ChangeTeam", false, true );
	cCommitSuicide = new cVirtualMap( "CommitSuicide", false, true );
	cEventKill = new cVirtualMap( "Event_Killed", false, true );
	cTeleport = new cVirtualMap( "Teleport", false, true );
	cPreThink = new cVirtualMap( "PreThink", false, true );
	cIPointsForKill = new cVirtualMap( "IPointsForKill", false, true );
	cGetCollidable = new cVirtualMap( "GetCollidable", false, true );
	cGetModelIndex = new cVirtualMap( "GetModelIndex", false, true );
	cSetModelIndex = new cVirtualMap( "SetModelIndex", false, true );
	cGetDataDescMap = new cVirtualMap( "GetDataDescMap", false, true );
	cSetModel = new cVirtualMap( "SetModel", false, true );
	cKeyValue = new cVirtualMap( "KeyValue", false, true );
	cKeyValueFloat = new cVirtualMap( "KeyValueFloat", false, true );
	cKeyValueVector = new cVirtualMap( "KeyValueVector", false, true );
	cGetKeyValue = new cVirtualMap( "GetKeyValue", false, true );
	cSetParent = new cVirtualMap( "SetParent", false, true );
	cAcceptInput = new cVirtualMap( "AcceptInput", false, true );
	cEventKilled = new cVirtualMap( "EventKilled", false, true );
	cGetVelocity = new cVirtualMap( "GetVelocity", false, true );
	cWorldSpaceCenter = new cVirtualMap( "WorldSpaceCenter", false, true );
	cIgnite = new cVirtualMap( "Ignite", false, true );
	cGiveAmmo = new cVirtualMap( "GiveAmmo", false, true );
	cWeaponSwitch = new cVirtualMap( "WeaponSwitch", false, true );
	cEventDying = new cVirtualMap( "EventDying", false, true );
	cGetSlot = new cVirtualMap( "GetSlot", false, true );
	cIsBot = new cVirtualMap( "IsBot", false, true );
	cGetViewVectors = new cVirtualMap( "GetViewVectors", false, true );
	cNoClipOff = new cVirtualMap( "NoClipOff", true );
}

void DestroySigs()
{

#ifndef WIN32
	delete cTempEnts;
	delete cEntList;
	delete cSetCollisionBounds;
	delete cCGameRules;
#else
	delete cLevelShutDown;
	delete cSetMinMaxSize;
	delete cCGameRules;
#endif

	delete cUtilRemove;
	delete cApplyAbsVelocity;
	delete cSwitchTeam;
	delete cWantsLagComp;
	delete cWantsLagCompPatch;
	delete cOnTakeDamage;
	delete cGetFileWeaponInfoFromHandle;
	delete cRoundRespawn;
	delete cWeaponDrop;
	delete cTermRound;
	delete cGiveNamedItem;
	delete cCalcDominationAndRevenge;
	delete cFindEntityByClassname;
	delete cGetPlayerMaxSpeed;
	delete cWeaponCanSwitch;
	delete cWeaponCanUse;
	delete cTraceAttack;
	delete cChangeTeam;
	delete cCommitSuicide;
	delete cEventKill;
	delete cTeleport;
	delete cPreThink;
	delete cIPointsForKill;
	delete cGetCollidable;
	delete cGetModelIndex;
	delete cSetModelIndex;
	delete cGetDataDescMap;
	delete cSetModel;
	delete cKeyValue;
	delete cKeyValueFloat;
	delete cKeyValueVector;
	delete cGetKeyValue;
	delete cSetParent;
	delete cAcceptInput;
	delete cEventKilled;
	delete cGetVelocity;
	delete cWorldSpaceCenter;
	delete cIgnite;
	delete cGiveAmmo;
	delete cWeaponSwitch;
	delete cEventDying;
	delete cGetSlot;
	delete cIsBot;
	delete cGetViewVectors;
	delete cNoClipOff;

	
#ifndef WIN32
	cTempEnts = NULL;
	cEntList = NULL;
	cSetCollisionBounds = NULL;
	cCGameRules = NULL;
#else
	cLevelShutDown = NULL;
	cSetMinMaxSize = NULL;
	cCGameRules = NULL;
#endif

	cUtilRemove = NULL;
	cApplyAbsVelocity = NULL;
	cSwitchTeam = NULL;
	cWantsLagComp = NULL;
	cWantsLagCompPatch = NULL;
	cOnTakeDamage = NULL;
	cGetFileWeaponInfoFromHandle = NULL;
	cRoundRespawn = NULL;
	cWeaponDrop = NULL;
	cTermRound = NULL;
	cGiveNamedItem = NULL;
	cCalcDominationAndRevenge = NULL;
	cFindEntityByClassname = NULL;
	cGetPlayerMaxSpeed = NULL;
	cWeaponCanSwitch = NULL;
	cWeaponCanUse = NULL;
	cTraceAttack = NULL;
	cChangeTeam = NULL;
	cCommitSuicide = NULL;
	cEventKill = NULL;
	cTeleport = NULL;
	cPreThink = NULL;
	cIPointsForKill = NULL;
	cGetCollidable = NULL;
	cGetModelIndex = NULL;
	cSetModelIndex = NULL;
	cGetDataDescMap = NULL;
	cSetModel = NULL;
	cKeyValue = NULL;
	cKeyValueFloat = NULL;
	cKeyValueVector = NULL;
	cGetKeyValue = NULL;
	cSetParent = NULL;
	cAcceptInput = NULL;
	cEventKilled = NULL;
	cGetVelocity = NULL;
	cWorldSpaceCenter = NULL;
	cIgnite = NULL;
	cGiveAmmo = NULL;
	cWeaponSwitch = NULL;
	cEventDying = NULL;
	cGetSlot = NULL;
	cIsBot = NULL;
	cGetViewVectors = NULL;
	cNoClipOff = NULL;
}