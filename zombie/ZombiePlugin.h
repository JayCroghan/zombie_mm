/* ======== sample_mm ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): David "BAILOPAN" Anderson
* ============================
*/

#ifndef _INCLUDE_ZombiePlugin_H
#define _INCLUDE_ZombiePlugin_H

#include <ISmmPlugin.h>
#include <sourcehook/sourcehook.h>
#include <igameevents.h>
#include "bitbuf.h"
#define GAME_DLL 1
#define CSTRIKE_DLL 1
#define ENGINE_DLL 1
#include "cbase.h"
#include "team.h"
#include "CSigMngr.h"
#include "MRecipients.h"
#include "css_offsets.h"
#include "sourcehook/sh_string.h"
#include <time.h>
#include "weapon_csbase.h"
#include "datamap.h"
#include "shake.h"
#include "IEffects.h"
#include "itempents.h"
#include "model_types.h"
#include "te_effect_dispatch.h"
#include "ivoiceserver.h"

#include "Timers.h"

typedef SourceHook::String myString;

#define PVFN( classptr , offset ) ((*(DWORD*) classptr ) + offset)
#define VFN( classptr , offset ) *(DWORD*)PVFN( classptr , offset )

#ifdef WIN32              // windows offsets
	#define AMMO_OFFSET 409
	#define MONEYOFFSET 902
	#define ARMOROFFSET1 759
	#define ARMOROFFSET2 769
	#define HELMOFFSET 898
	#define KILLSOFFSET 765
	#define CSS_TEAM_OFFSET 2
#else                     // linux offsets
	#define AMMO_OFFSET 409
	#define MONEYOFFSET 907
	#define ARMOROFFSET1 764
	#define ARMOROFFSET2 774
	#define HELMOFFSET 876
	#define KILLSOFFSET 770
	#define CSS_TEAM_OFFSET 2
#endif

#define TERRORISTS 2		// Team index number
#define COUNTERTERRORISTS 3 // Team index number

#define SLOT_PRIMARY 0  // weapon slots
#define SLOT_SECONDARY 1
#define SLOT_KNIFE 2
#define SLOT_NADE 3
#define SLOT_BOMB 4

#define Target_Bombed							1
#define VIP_Escaped								2
#define VIP_Assassinated						3
#define Terrorists_Escaped						4
#define CTs_PreventEscape						5
#define Escaping_Terrorists_Neutralized			6
#define Bomb_Defused							7
#define CTs_Win									8
#define Terrorists_Win							9
#define Round_Draw								10
#define All_Hostages_Rescued					11
#define Target_Saved							12
#define Hostages_Not_Rescued					13
#define Terrorists_Not_Escaped					14
#define VIP_Not_Escaped							15

#define ZOMBIE_VERSION	"0.9.7e"

#define AUTORESPAWN_CHECK_OFFSET	22969394
#define AUTORESPAWN_PROTECT_OFFSET	23969358
#define ZOMBIECHECK_TIMER_ID 		69785474

//int CTeam::GetNumPlayers( void )
//GetFileWeaponInfoFromHandleFunc GetFileWeaponInfoFromHandle_ = NULL;

class vEmptyClass { };

typedef FileWeaponInfo_t* (*GetFileWeaponInfoFromHandleFunc)( WEAPON_FILE_INFO_HANDLE handle );

#ifdef WIN32
	typedef void (* CCSPlayer_RoundRespawnFunc)( );
	typedef int (*OnTakeDamageFunction)( const CTakeDamageInfo &inputInfo );
	typedef bool (*WantsLagFunction)( const CBasePlayer *, const CUserCmd *, const CBitVec<MAX_EDICTS> * );
	typedef void (*TermRoundFunction)(float, int);
	typedef CBaseEntity	*( *GiveNamedItem_Func) ( const char *, int );
	typedef CBaseEntity *( *FindEntityByClassname_Func )( CBaseEntity *, const char * );
	typedef void (*SetModelFunction)(const char *);
	typedef void (*WeaponDropFunction)(CBaseCombatWeapon *, const Vector *, const Vector * );
	typedef CBaseCombatWeapon *(*WeaponSlotFunction)(int);
	typedef char *(*GetNameFunction)(void);
	typedef void (*WeaponDeleteFunction)(void);
#else
	typedef void (* CCSPlayer_RoundRespawnFunc)( CBasePlayer* );
	typedef int (*OnTakeDamageFunction)( CBasePlayer* , const CTakeDamageInfo &inputInfo );
	typedef bool (*WantsLagFunction)( CBasePlayer *, const CBasePlayer *, const CUserCmd *, const CBitVec<MAX_EDICTS> * );
	typedef void (*TermRoundFunction)(CGameRules *, float, int);
	typedef CBaseEntity	*( *GiveNamedItem_Func) ( CBasePlayer *, const char *, int );
	typedef CBaseEntity *( *FindEntityByClassname_Func )( CGlobalEntityList *, CBaseEntity *, const char * );
	typedef void (*SetModelFunction)(CBaseFlex *, const char *);
	typedef void (*WeaponDropFunction)( CBasePlayer *, CBaseCombatWeapon *, const Vector *, const Vector * );
	typedef CBaseCombatWeapon *(*WeaponSlotFunction)(CBaseCombatCharacter *, int);
	typedef char*(*GetNameFunction)(CBaseCombatWeapon *);
	typedef void (*WeaponDeleteFunction)(CBaseCombatWeapon *);
#endif

struct OffsetInfo_t
{
	int		m_iHealth;
	int		m_iAccount;
	int		m_bHasNightVision;
	int		m_iTeamNum;
	int		m_iTeamTeamNum;
	int		m_lifeState;
	int		m_iFOV;
	int		m_Local;
	int		m_fFlags;
	int		m_bNightVisionOn;
	int		m_bDrawViewmodel;
	int		m_iHideHUD;
	int		m_iFrags;
	int		m_hZoomOwner;
	int		m_bInBuyZone;
	int		m_iTest1;
};

struct PlayerInfo_t
{
	CBasePlayer				*pPlayer;
	CBaseCombatWeapon 		*pKnife;
	int						iBulletCount;
	int						isZombie;
	int						isBot;
	int						isHooked;
};

class ZombiePlugin : public ISmmPlugin, public IGameEventListener2
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	void AllPluginsLoaded();
	bool Pause(char *error, size_t maxlen)
	{
		return true;
	}
	bool Unpause(char *error, size_t maxlen)
	{
		return true;
	}
public:
	int GetApiVersion() { return PLAPI_VERSION; }
public:
	const char *GetAuthor()
	{
		return "c0ldfyr3";
	}
	const char *GetName()
	{
		return "ZombieMod";
	}
	const char *GetDescription()
	{
		return "Zombie Mod for CS:S";
	}
	const char *GetURL()
	{
		return "http://www.c0ld.net/";
	}
	const char *GetLicense()
	{
		return "none";
	}
	const char *GetVersion()
	{
		return ZOMBIE_VERSION;
	}
	const char *GetDate()
	{
		return __DATE__;
	}
	const char *GetLogTag()
	{
		return "ZOMBIE";
	}
public:
	bool LevelInit(const char *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background);
	void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
	void GameFrame(bool simulating);
	void LevelShutdown(void);
	void ClientActive(edict_t *pEntity, bool bLoadGame);
	void ClientDisconnect(edict_t *pEntity);
	void ClientPutInServer(edict_t *pEntity, char const *playername);
	void SetCommandClient(int index);
	void ClientSettingsChanged(edict_t *pEdict);
	bool ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
	void ClientCommand(edict_t *pEntity);
	void FireGameEvent( IGameEvent *event );

	bool SetClientListening(int iReceiver, int iSender, bool bListen);
	void DoMuzzleFlash();
	void FireBullets( const FireBulletsInfo_t &info );
	void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, float flVolume, float flAttenuation, int iFlags = 0, int iPitch = PITCH_NORM, const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1 );
	void WriteByte( int val );
	bool WriteString( const char *pStr );
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	int OnTakeDamage( const CTakeDamageInfo &info );
	void UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity );
	void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	bool Weapon_CanUse( CBaseCombatWeapon *pWeapon );
	bool Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon );
	void Delete();
	void PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize );
	void Touch( CBaseEntity *pOther );
	
	void ZombieDropWeapons( CBasePlayer *pPlayer, bool bHook = false );
	bool LoadZombieModelList(const char *filename, const char *downloadfile);
	void MakeZombie( CBasePlayer *pPlayer );
	float GetMaxSpeed(void);
	void ZombieOn();
	void ZombieOff();
	bool IsAlive( edict_t *pEntity );
	void ShowZomb();

	void DoAmmo(  bool bEnable );
	void RestrictWeapons();

	void HookPlayer( int iPlayer );
	void UnHookPlayer( int iPlayer );

	bool CanUseWeapon( CBaseEntity* pPlayer, CBaseEntity *pEnt, bool bDrop = true );
	bool AllowWeapon( CBasePlayer *pPlayer, int WeaponID, int &nCount );
	void CheckAutobuy( int nIndex, CBasePlayer* pPlayer );
	int EntIdxOfUserIdx(int useridx);
	void Hud_Print( edict_t *pEntity, const char *sMsg, ... );
	int GetTeam(edict_t *pEntity);

	virtual void OnLevelShutdown();

	//CBasePlayer			*m_Bases[MAX_PLAYERS];

	bool g_ZombieRoundOver;

	STimers				*g_Timers;
	int					iCurrentPlayer;
	int					iCountPlayers;
	int					iCountZombies;


	//myString			sRestricted[100];
	//int					iRestricted;

	int g_RestrictT[CSW_MAX];
	int g_RestrictCT[CSW_MAX];
	int g_RestrictW[CSW_MAX];

	OffsetInfo_t		g_Offsets;
	IEngineSound		*m_EngineSound;
	IServerGameDLL		*m_ServerDll;
	IVEngineServer		*m_Engine;
	IPlayerInfoManager	*m_PlayerInfoManager;
	IEffects			*m_Effects;
	IVModelInfo			*m_ModelInfo;
	IEngineTrace		*m_EngineTrace;
private:
	void AddDownload( const char *file );
	SourceHook::CallClass<IVoiceServer>		*m_VoiceServer_CC;
	IVoiceServer		*m_VoiceServer;
	INetworkStringTableContainer *m_NetworkStringTable; 
	ICvar				*m_CVar;
	IFileSystem			*m_FileSystem;
	IGameEventManager2	*m_GameEventManager;	
	IServerGameClients	*m_ServerClients;
};

class MyListener : public IMetamodListener
{
public:
	virtual void *OnMetamodQuery(const char *iface, int *ret);
};

extern ZombiePlugin g_ZombiePlugin;

void CVar_CallBack( ConVar *var, char const *pOldString );
void FindGameRules(char *addr);

void CBasePlayer_ApplyAbsVelocityImpulse( CBasePlayer *pPlayer, const Vector &vecImpulse );
void CBasePlayer_SwitchTeam( CBasePlayer *pPlayer, int iTeam );
bool CBasePlayer_SetFOV( CBasePlayer *pPlayer, CBaseEntity *pRequester, int FOV, float zoomRate = 0.0f );
void UTIL_RestartRound( CBasePlayer *pPlayer );
void UTIL_WeaponDrop( CBasePlayer *pBasePlayer, CBaseCombatWeapon *pWeapon, const Vector *target, const Vector *velocity );
CBaseCombatWeapon* UTIL_WeaponSlot(CBaseCombatCharacter *pCombat, int slot);
char* UTIL_GetName(CBaseCombatWeapon *pWeapon);
void UTIL_WeaponDelete(CBaseCombatWeapon *pWeapon);
void UTIL_TermRound(float delay, int reason);
void UTIL_SetModel( CBaseFlex *pBaseFlex, const char *model );
CBaseEntity	*UTIL_GiveNamedItem( CBasePlayer *pPlayer, const char *pszName, int iSubType );
int UTIL_GetNumPlayers( CTeam *pTeam );
CBaseEntity	*GiveNamedItem_Test( CBasePlayer *pPlayer, const char *pszName, int iSubType );

void UTIL_TraceLines( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter *pFilter, trace_t *ptr, bool bRed );
void FixSlashes( char *str );
int LookupBuyID(const char *name);
int LookupRebuyID(const char *name);
const char *LookupWeaponName(int id);
float DamageForce(const Vector &size, float damage);
void CheckZombies(void **params);
void ZombieLevelInit( void **params );
void RandomZombie(void **params);
void CheckZombieWeapons( void **params );
void CheckRespawn( void **params );
void Timed_SetFOV( void **params );

void ShowZomb();
void cmdKill( void );


void FFA_Enable();
void FFA_Disable();
void UTIL_MemProtect(void *addr, int length, int prot);


class CPlayerMatchList
{
private:
	CUtlVector<edict_t*>	m_pEdictList;
public:
	bool 			m_bAliveOnly;
	bool			m_bDeadOnly;
	bool 			m_bAllPlayers;
	bool			m_bMatchBots;
	bool			m_bMatchOne;
	bool			m_bSteamID;
	bool			m_bUserID;
	int				m_nUserID;
	int 			m_iTeam;
	char			m_szMatchString[128];
	
	CPlayerMatchList(const char *pMatchString, bool bAliveOnly = false, bool bDeadOnly = false, bool bMatchBots = true, bool bMatchOne = false)
	{
		Q_strncpy((char*)&m_szMatchString, pMatchString, sizeof(m_szMatchString));
		m_bAliveOnly = bAliveOnly;
		m_bDeadOnly = bDeadOnly;
		m_bMatchBots = bMatchBots;
		m_bMatchOne = bMatchOne;
        m_bAllPlayers = (!Q_strnicmp(pMatchString, "#a", 2));
		m_bSteamID = (!Q_strnicmp(pMatchString, "STEAM_", 6) && pMatchString[7] == ':' && pMatchString[9] == ':');
		m_iTeam = ((Q_strnicmp(pMatchString, "#t", 2) == 0) ? 2 : ((Q_strnicmp(pMatchString, "#c", 2) == 0) ? 3 : ((Q_strnicmp(pMatchString, "#s", 2) == 0) ? 0 : -1)));
		m_bUserID = (m_iTeam == -1 && pMatchString[0] == '#' && atoi(pMatchString+1));
		if (m_bUserID)
			m_nUserID = atoi(pMatchString+1);
        
		for (int i = 1; i <= g_SMAPI->pGlobals()->maxClients; i++) {
			edict_t *pEdict = g_ZombiePlugin.m_Engine->PEntityOfEntIndex(i);
			if (!pEdict || pEdict->IsFree() || !pEdict->GetUnknown() || !pEdict->GetUnknown()->GetBaseEntity() || (g_ZombiePlugin.m_Engine->GetPlayerUserId(pEdict) < 0)) {
				continue;
			} 
			CBasePlayer *pPlayer = (CBasePlayer*)pEdict->GetUnknown()->GetBaseEntity();
			if (!bMatchBots && pPlayer->IsBot()) {
				continue;
			} else if (m_bAliveOnly && (((CBasePlayer*)pEdict->GetUnknown()->GetBaseEntity())->IsAlive() != 1)) {
				continue;
			} else if (m_bDeadOnly && (((CBasePlayer*)pEdict->GetUnknown()->GetBaseEntity())->IsAlive() == 1)) {
				continue;
			} else if (m_bSteamID && g_ZombiePlugin.m_Engine->GetPlayerNetworkIDString(pEdict) && !Q_stricmp(g_ZombiePlugin.m_Engine->GetPlayerNetworkIDString(pEdict), m_szMatchString)) {
				m_pEdictList.Purge();
				m_pEdictList.AddToTail(pEdict);
				return;
			} else if (m_bUserID && (g_ZombiePlugin.m_Engine->GetPlayerUserId(pEdict) == m_nUserID)) {
				m_pEdictList.Purge();
				m_pEdictList.AddToTail(pEdict);
				return;
			} else if (m_bMatchOne && g_ZombiePlugin.m_Engine->GetClientConVarValue(i, "name") && !Q_stricmp(m_szMatchString, g_ZombiePlugin.m_Engine->GetClientConVarValue(i, "name"))) {
				m_pEdictList.Purge();
				m_pEdictList.AddToTail(pEdict);
				return;
			} else if (m_bAllPlayers) {
				m_pEdictList.AddToTail(pEdict);
				continue;
			} else if (m_iTeam > -1) {
				int team = g_ZombiePlugin.GetTeam( pPlayer->edict() );
				if ((m_iTeam == team) || ((m_iTeam == 0) && (team == 1))) {
					m_pEdictList.AddToTail(pEdict);
				}
				continue;
			}
			const char *pName = g_ZombiePlugin.m_Engine->GetClientConVarValue(i, "name");
			if (pName && Q_stristr(pName, m_szMatchString))
				m_pEdictList.AddToTail(pEdict);
		}
	}
	int Count() { return (int)m_pEdictList.Count(); }
	edict_t *GetMatch(int num) { 
		if ((num < 0) || (num >= m_pEdictList.Count())) 
			return NULL;
		return m_pEdictList[num];
	}
};


PLUGIN_GLOBALVARS();

#endif //_INCLUDE_ZombiePlugin_H
