#ifndef _INCLUDE_OFFSETS_H
#define _INCLUDE_OFFSETS_H

#ifdef __linux__

	//_ZNK11CBasePlayer28WantsLagCompensationOnEntityEPKS_PK8CUserCmdPK7CBitVecILi2048EE
	//#define WantsLagComp_Sig                "\x81\xEC\x8C\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x89\xB4\x24\x80\x00\x00\x00\x8B\xB4\x24\x94\x00\x00\x00\x89\xBC\x24\x84\x00\x00\x00\x8B\xBC\x24\x90\x00\x00\x00\x89\xAC\x24\x88\x00\x00\x00\x8B\xAC"
	#define WantsLagComp_SigBytes						48
	#define WantsLagComp_Offset							0x3E
	#define WantsLagComp_OffsetBytes					6

	//_ZN9CCSPlayer12OnTakeDamageERK15CTakeDamageInfo
	//find by looking for aPlayer_damageh (Player.DamageHelmet)
	//#define OnTakeDamage_Sig                                "\x55\x57\x56\x53\x81\xEC\x1C\x01\x00\x00\x8B\x2A\x24\x2A\x01\x00\x00\x8B\xB4\x24\x30\x01\x00\x00\x8B\x2A\x2A\x89\x94\x24\xC0\x00\x00\x00\x8B\x2A\x04\x89\x84\x24\xC4\x00\x00\x00\x8B\x2A\x08\x89"
	#define OnTakeDamage_SigBytes               		48
	//#define OnTakeDamage_Offset                 		0x668
	#define OnTakeDamage_OffsetBytes            		6
	//#define OnTakeDamage_Offset1						0x668
	//#define OnTakeDamage_Offset2						0x14F
	#define OnTakeDamage_Offset1						0x60B
	#define OnTakeDamage_Offset2						0x16E

	#define CPlayer_UpdateClient_Sig					"_ZN11CBasePlayer16UpdateClientDataEv"
	#define CCSGame_TermRound_Sig						"_ZN12CCSGameRules14TerminateRoundEfi"
	#define	CCSPlayer_GiveNamedItem_Sig					"_ZN11CBasePlayer13GiveNamedItemEPKci"
	#define CBasePlayer_WeaponDrop_Sig					"_ZN11CBasePlayer11Weapon_DropEP17CBaseCombatWeaponPK6VectorS4_"
	#define	CBaseCombatPlayer_WeaponGetSlot_Sig			"_ZNK20CBaseCombatCharacter14Weapon_GetSlotEi"
	#define	CBaseCombatWeapon_Delete_Sig				"_ZN17CBaseCombatWeapon6DeleteEv"
	#define	WantsLagComp_Sig							"_ZNK11CBasePlayer28WantsLagCompensationOnEntityEPKS_PK8CUserCmdPK7CBitVecILi2048EE"
	#define OnTakeDamage_Sig							"_ZN9CCSPlayer12OnTakeDamageERK15CTakeDamageInfo"
	#define	GetFileWeaponInfoFromHandle_Sig				"_Z27GetFileWeaponInfoFromHandlet"
	#define	CCSPlayer_RoundRespawn_Sig					"_ZN9CCSPlayer12RoundRespawnEv"
	#define	CCSPlayer_GiveNamedItem_Sig					"_ZN11CBasePlayer13GiveNamedItemEPKci"
	#define	SetFOV_Sig									"_ZN11CBasePlayer6SetFOVEP11CBaseEntityif"
	#define CCSPlayer_SwitchTeam_Sig					"_ZN9CCSPlayer10SwitchTeamEi"
	#define CBaseEntity_ApplyAbsVelocity_Sig			"_ZN11CBaseEntity23ApplyAbsVelocityImpulseERK6Vector"
	#define CCSPlayer_IsInBuyZone_Sig					"_ZN9CCSPlayer11IsInBuyZoneEv"
	#define CreateEntityByName_Sig						"_Z18CreateEntityByNamePKci"
	#define CBaseEntity_KeyValues_Sig					"_ZN11CBaseEntity8KeyValueEPKcS1_"
	//#define UTIL_SetModel_Sig							"_Z13UTIL_SetModelP11CBaseEntityPKc"
	#define	CBaseEntity_SetCollisionBounds_Sig			"_ZN11CBaseEntity18SetCollisionBoundsERK6VectorS2_"
	#define	ClientPrint_Sig								"_Z11ClientPrintP11CBasePlayeriPKcS2_S2_S2_S2_"

	#define UtilRemove_Sig								"_Z11UTIL_RemoveP11CBaseEntity"

	#define CPlayer_g_pGameRules						0x93

	#define CEventQueue_AddEvent_Sig					"_ZN11CEventQueue8AddEventEP11CBaseEntityPKcfS1_S1_i"
#else 

	#define CBaseEntityOutput_g_EventQueue			0xD9
	#define CBaseEntityOutput_FireOutput_Sig		"\x81\xEC\x1C\x03\x00\x00\x53\x55\x56\x8B\x71\x14\x85\xF6\x57\x89"
	//#define CBaseEntityOutput_FireOutput_Sig		"\x83\xEC\x1C\x53\x55\x56\x8B\x71\x14\x85\xF6\x57\x89\x4C\x24\x14"
	#define CBaseEntityOutput_FireOutput_SigBytes	16

	#define CEventQueue_AddEvent_Sig				"\x83\xEC\x14\x8B\x44\x24\x20\x56\x57\x8B\x7C\x24\x0C\x50\x8B\x44"
	#define CEventQueue_AddEvent_SigBytes			16

	#define	ClientPrint_Sig								"\x83\xEC\x20\x56\x8B\x74\x24\x28\x85\xF6\x74\x58\x8D\x4C\x24\x04"
	#define	ClientPrint_SigBytes						16

	#define DispatchEffect_Sig							"\x83\xEC\x20\x56\x8D\x4C\x24\x04\xE8\x2A\x2A\x2A\xFF\x8B\x74\x24"
	//#define DispatchEffect_Sig							"\x83\xEC\x20\x56\x8D\x4C\x24\x04\xE8\x2A\x2A\xFC\xFF\x8B\x74\x24"
	//#define DispatchEffect_Sig							"\x83\xEC\x20\x56\x8D\x4C\x24\x04\xE8\xC3\x40\xFC\xFF\x8B\x74\x24"
	//#define DispatchEffect_Sig						"\x83\xEC\x20\x56\x8D\x4C\x24\x04\xE8\xB3\x3F\xFC\xFF\x8B\x74\x24"
	#define DispatchEffect_SigBytes						16
	#define DispatchEffect_Offset						0x29

	#define	SetMinMaxSize_Sig							"\x53\x8B\x5C\x24\x0C\x55\x8B\x6C\x24\x14\x56\x57\x8B\xF5\x2B\xDD"
	#define	SetMinMaxSize_SigBytes						16

	#define FindEntityByClass_Sig						"\x53\x55\x56\x8B\xF1\x8B\x4C\x24\x10\x85\xC9\x57\x74\x18\x8B\x01\xFF\x50\x08\x8B\x08\x81\xE1\xFF\x0F\x00\x00\x83\xC1\x01\xC1\xE1"
	#define FindEntityByClass_SigBytes					32
	//_ZN11CEventQueue4InitEv
	//event_queue_saveload_proxy
	#define CEntList_Pattern							"\x8B\x44\x24\x04\x85\xC0\x56\x57\x74\x49\x8B\x7C\x24\x10\x6A\x00"
	#define CEntList_PatternBytes						16
	#define CEntList_gEntList							23

	//													"\x83\xEC\x0C\x55\x8B\x6C\x24\x14\x57\x6A\x23\x55\x8B\xF9\xE8\x1D"
	//#define CBaseEntity_KeyValues_Sig					"\x83\xEC\x0C\x55\x8B\x6C\x24\x14\x57\x6A\x23\x55\x8B\xF9\xE8\xFD"
	#define CBaseEntity_KeyValues_Sig					"\x83\xEC\x0C\x55\x8B\x6C\x24\x14\x57\x6A\x23\x55\x8B\xF9\xE8\x2A"
	#define CBaseEntity_KeyValues_SigBytes				16
	
	#define CreateEntityByName_Sig						"\x56\x8B\x74\x24\x0C\x83\xFE\xFF\x57\x8B\x7C\x24\x0C\x74\x25\x8B"
	#define CreateEntityByName_Sigbytes					16

	#define UtilRemove_Sig								"\x8B\x44\x24\x04\x85\xC0\x74\x2A\x05\x2A\x2A\x00\x00\x89\x44\x24\x04\xE9\x2A\xFF\xFF\xFF"
	#define UtilRemove_SigBytes							22

	#define UTILClientPrint_Sig							"\x8B\x44\x24\x04\x68\x74\x77\x3D\x22\x50\xE8\x61\x6C\xEF\xFF\x8B"
	#define UTILClientPrint_SigBytes					16

	//#define CCSPlayer_IsInBuyZone_Sig					"\x80\xB9\x51\x12\x00\x00\x00\x74\x0F\x80\xB9\x14\x14\x00\x00\x00"
	#define CCSPlayer_IsInBuyZone_Sig					"\x80\xB9\x2A\x12\x00\x00\x00\x74\x0F\x80\xB9\x2A\x14\x00\x00\x00"
	#define CCSPlayer_IsInBuyZone_SigBytes				16
	

#ifdef _BETA
	#define CBaseEntity_ApplyAbsVelocity_Sig			"\x83\xEC\x0C\x56\x57\x8B\x7C\x24\x18\xD9\x07\x8B\xF1\xD9\x05\x2A\x2A\x2A\x22\xDA\xE9\xDF\xE0\xF6\xC4\x44\x7A\x24\xD9\x47\x04\xD9\x05\x2A\x2A\x2A\x22\xDA\xE9\xDF\xE0\xF6\xC4\x44\x7A\x12\xD9\x47\x08\xD9\x05\x2A\x2A\x2A\x22\xDA\xE9\xDF\xE0\xF6\xC4\x44\x7B\x4C"
#else
	//#define CBaseEntity_ApplyAbsVelocity_Sig			"\x83\xEC\x0C\x56\x57\x8B\x7C\x24\x18\xD9\x07\x8B\xF1\xD9\x05\xC8"
	#define CBaseEntity_ApplyAbsVelocity_Sig			"\x83\xEC\x0C\x56\x57\x8B\x7C\x24\x18\xD9\x07\x8B\xF1\xD9\x05\x2A\x2A\x5B\x22\xDA\xE9\xDF\xE0\xF6\xC4\x44\x7A\x24\xD9\x47\x04\xD9\x05\x2A\x2A\x5B\x22\xDA\xE9\xDF\xE0\xF6\xC4\x44\x7A\x12\xD9\x47\x08\xD9\x05\x2A\x2A\x5B\x22\xDA\xE9\xDF\xE0\xF6\xC4\x44\x7B\x4C"
#endif
	//#define CBaseEntity_ApplyAbsVelocity_Sig			"\x83\xEC\x0C\x56\x57\x8B\x7C\x24\x18\xD9\x07\x8B\xF1\xD9\x05\x68\x36\x5B\x22\xDA\xE9\xDF\xE0\xF6\xC4\x44\x7A\x24\xD9\x47\x04\xD9\x05\x6C\x36\x5B\x22\xDA\xE9\xDF\xE0\xF6\xC4\x44\x7A\x12\xD9\x47\x08\xD9\x05\x70\x36\x5B\x22\xDA\xE9\xDF\xE0\xF6\xC4\x44\x7B\x4C"					
	#define CBaseEntity_ApplyAbsVelocity_SigBytes		64

	#define CCSPlayer_SwitchTeam_Sig					"\x83\xEC\x2A\x56\x57\x8B\x7C\x24\x2A\x57\x8B\xF1\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04"
	#define CCSPlayer_SwitchTeam_SigBytes				20

#ifdef _BETA
	#define SetFOV_Sig									"\x53\x8B\x5C\x24\x08\x85\xDB\x57\x8B\xF9\x2A\x07\x5F\x33\xC0\x5B"
#else
	#define SetFOV_Sig									"\x53\x8B\x5C\x24\x08\x85\xDB\x57\x8B\xF9\x75\x07\x5F\x32\xC0\x5B"
#endif

	#define SetFOV_SigBytes								16

	//use friendlyfire cvar to find it.
	//													"\xA1\xA8\x99\x49\x22\x83\xEC\x24\x83\x78\x2C\x00\x53\x56\x8B\x74"
	//#define WantsLagComp_Sig							"\xA1\x88\x20\x49\x22\x83\xEC\x24\x83\x78\x2C\x00\x53\x56\x8B\x74"
	//#define WantsLagComp_Sig							"\xA1\x2A\x2A\x49\x22\x83\xEC\x24\x83\x78\x2C\x00\x53\x56\x8B\x74"
	#define WantsLagComp_Sig							"\xA1\x2A\x2A\x2A\x22\x83\xEC\x24\x83\x78\x2C\x00\x53\x56\x8B\x74"
														   
															
	#define WantsLagComp_SigBytes						16

	//CCSPlayer not CBE - %s attacked a teammate
	#define OnTakeDamage_Sig							"\x8B\x44\x24\x04\x81\xEC\x2A\x00\x00\x00\x53\x56\x8B\xF1\x50\x8D"
	//#define OnTakeDamage_Sig							"\x8B\x44\x24\x04\x81\xEC\x80\x00\x00\x00\x53\x56\x8B\xF1\x50\x8D"
	#define OnTakeDamage_SigBytes						16

	#define OnTakeDamage_Offset							0x265
	#define WantsLagComp_Offset							0x29
	#define OnTakeDamage_OffsetBytes					1
	#define WantsLagComp_OffsetBytes					6

	//Find gunshotsplash in _ZN13CWeaponCSBase13PhysicsSplashERK6Vector
	// 1st sub in function.
	// Only sub in there.
#ifdef _BETA
	#define GetFileWeaponInfoFromHandle_Sig				"\x66\x8B\x44\x24\x04\x66\x3B\x05\x2A\x2A\x2A\x22\x73\x17\x66\x3D"
#else
	#define GetFileWeaponInfoFromHandle_Sig				"\x66\x8B\x44\x24\x04\x66\x3B\x05\x8A\x2A\x2A\x22\x73\x17\x66\x3D"
#endif
	//#define GetFileWeaponInfoFromHandle_Sig				"\x66\x8B\x44\x24\x04\x66\x3B\x05\x8A\xC8\x2A\x22\x73\x17\x66\x3D"
	//#define GetFileWeaponInfoFromHandle_Sig				"\x66\x8B\x44\x24\x04\x66\x3B\x05\x8A\xC8\x53\x22\x73\x17\x66\x3D"
	//#define GetFileWeaponInfoFromHandle_Sig				"\x66\x8B\x44\x24\x04\x66\x3B\x05\x2A\x2A\x46\x22\x73\x17\x66\x3D"
	#define GetFileWeaponInfoFromHandle_SigBytes		16
	
	#define CCSPlayer_RoundRespawn_Sig					"\x56\x8B\xF1\x8B\x06\xFF\x90\x2A\x04\x00\x00\x8B\x86\xE8\x0D\x00"
	//#define CCSPlayer_RoundRespawn_Sig					"\x56\x8B\xF1\x8B\x06\xFF\x90\xB8\x04\x00\x00\x8B\x86\xE8\x0D\x00"
	//#define CCSPlayer_RoundRespawn_Sig					"\x56\x8B\xF1\x8B\x06\xFF\x90\x60\x04\x00\x00\x8B\x86\xE8\x0D\x00"
	#define CCSPlayer_RoundRespawn_SigBytes				16


	#define CBaseCombatWeapon_Delete_Sig				"\x56\x6A\x00\x6A\x00\x8B\xF1\x68\x2A\x2A\x25\x22\xC7\x46\x7C"
	#define CBaseCombatWeapon_Delete_SigBytes			15

	#define CBaseCombatWeapon_GetName_Sig				"\x66\x8B\x81\x08\x05\x00\x00\x50\xE8\x13\x3C\x1C\x00\x83\xC4\x04"
	#define CBaseCombatWeapon_GetName_SigBytes			16

	#define CBaseCombatPlayer_WeaponGetSlot_Sig			"\x53\x55\x8B\x6C\x24\x0C\x56\x8B\xD9\x57\x33\xF6\x8D\xBB\xC4\x06"
	#define CBaseCombatPlayer_WeaponGetSlot_SigBytes	16

	#define CBasePlayer_WeaponDrop_Sig					"\x81\xEC\x00\x01\x00\x00\x53\x55\x56\x8B\xB4\x24\x10\x01\x00\x00"
	#define CBasePlayer_WeaponDrop_SigBytes				16

	#define CCSGame_TermRound_Sig						"\x83\xEC\x18\x53\x55\x8B\xD9\x8B\x4C\x24\x28\x56\x57\x33\xF6\x8D\x41\xFF\x33\xFF"
	#define CCSGame_TermRound_SigBytes					20

	#define CBaseEntity_CreateGib_Sig					"\x56\x6A\xFF\x68\x5C\x07\x3A\x22\xE8\xE3\x45\x01\x00\x8B\xF0\x8B"
	#define CBaseEntity_CreateGib_SigBytes				16
	
	#define UTIL_SetModel_Sig							"\x8B\x0D\xD4\x91\x49\x22\x8B\x01\x83\xEC\x18\x53\x56\x57\x8B\x7C"
	#define UTIL_SetModel_SigBytes						16

	#define CBaseAnimating_Teleport_Sig					"\x8B\x44\x24\x0C\x8B\x54\x24\x04\x56\x8B\xF1\x8B\x4C\x24\x0C\x50\x51\x52\x8B\xCE\xE8\xF7\x35\x02"
	#define CBaseAnimating_Teleport_SigBytes			24

	#define CCSPlayer_GiveNamedItem_Sig					"\x53\x8B\x5C\x24\x0C\x55\x56\x8B\x74\x24\x10\x53\x56\x8B\xE9\xE8"
	#define CCSPlayer_GiveNamedItem_SigBytes			16

	
	//original address: 2225AB40 ("ResetHUD"), CBasePlayer::UpdateClientData( void )
	//0.76 - g_pGameRules is now at 0x2225AB40
	#define CPlayer_UpdateClient_Sig                "\x83\xEC\x38\x53\x56\x8B\xF1\x57\x8D\x4C\x24\x24\xE8\x2A\x2A\x2A\x2A\x56\x8D\x4C\x24\x28\xC7\x44\x24\x28\x2A\x2A\x2A\x2A\xE8"
	#define CPlayer_UpdateClient_SigBytes   31
	//bytes into the function that we'll find something like: mov ecx, dword_2242AA78
	#define CPlayer_g_pGameRules            109

#endif

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
