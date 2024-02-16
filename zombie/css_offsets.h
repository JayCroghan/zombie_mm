#ifndef _INCLUDE_OFFSETS_H
#define _INCLUDE_OFFSETS_H

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
#include <sys/mman.h>
#define PAGE_EXECUTE_READWRITE  PROT_READ|PROT_WRITE|PROT_EXEC
#endif

#define PAGE_SIZE       4096
#define ALIGN(ar) ((long)ar & ~(PAGE_SIZE-1))

#ifdef __linux__

	//_ZNK11CBasePlayer28WantsLagCompensationOnEntityEPKS_PK8CUserCmdPK7CBitVecILi2048EE
	#define WantsLagComp_Sig                "\x81\xEC\x8C\x00\x00\x00\xA1\x2A\x2A\x2A\x2A\x89\xB4\x24\x80\x00\x00\x00\x8B\xB4\x24\x94\x00\x00\x00\x89\xBC\x24\x84\x00\x00\x00\x8B\xBC\x24\x90\x00\x00\x00\x89\xAC\x24\x88\x00\x00\x00\x8B\xAC"
	#define WantsLagComp_SigBytes           48
	#define WantsLagComp_Offset             0x3E
	#define WantsLagComp_OffsetBytes        6

	//_ZN9CCSPlayer12OnTakeDamageERK15CTakeDamageInfo
	//find by looking for aPlayer_damageh (Player.DamageHelmet)
	#define OnTakeDamage_Sig                                "\x55\x57\x56\x53\x81\xEC\x1C\x01\x00\x00\x8B\x2A\x24\x2A\x01\x00\x00\x8B\xB4\x24\x30\x01\x00\x00\x8B\x2A\x2A\x89\x94\x24\xC0\x00\x00\x00\x8B\x2A\x04\x89\x84\x24\xC4\x00\x00\x00\x8B\x2A\x08\x89"
	#define OnTakeDamage_SigBytes                   48
	#define OnTakeDamage_Offset                             0x668
	#define OnTakeDamage_OffsetBytes                6

	#define CBaseCombatPlayer_WeaponGetSlot_Sig			"\x55\x89\xE5\x57\x56\x31\xF6\x53\x83\xEC\x0C\x8B\x5D\x08\x8B\x7D\x0C\x81\xC3\xD8\x06\x00\x00"
#define CBaseCombatPlayer_WeaponGetSlot_SigBytes	23

#define CBasePlayer_WeaponDrop_Sig			"\x55\x89\xE5\x57\x56\x31\xF6\x53\x83\xEC\x1C\x8B\x7D\x08\xEB\x06\x46\x83\xFE\x2F\x7F\x43\x89\x74"
#define CBasePlayer_WeaponDrop_SigBytes		24

	#define CPlayer_UpdateClient_Sig	"\x55\x89\xE5\x81\xEC\xA8\x00\x00\x00\x89\x5D\xF4\x8B\x5D\x08\x89\x75\xF8\x8D\x75\xC8\x89\x7D\xFC\x89\x34\x24\xE8\x90\x17\x03\x00"
	#define CPlayer_UpdateClient_SigBytes	32
	//bytes into the function that we'll find something like: mov ecx, dword_2242AA78 (to the dword)
	//#define CPlayer_g_pGameRules			0x83
	#define CPlayer_g_pGameRules			0x93

	#define CCSGame_TermRound_Sig			"\x55\x89\xE5\x8D\x45\xD4\x57\x56\x31\xF6\x53\x83\xEC\x7C\x8B\x55"
	#define CCSGame_TermRound_SigBytes		16

	//_ZN9CCSPlayer13GiveNamedItemEPKci
	#define CCSPlayer_GiveNamedItem_Sig             "\x55\x89\xE5\x57\x8D\x7D\xD8\x56\x53\x83\xEC\x3C\x8B\x45\x08\x8B\x5D\x0C\x89\x3C\x24\x89\x45\xD0\xE8\x2A\x2A\x2A\x2A\x85\xDB\x0F"
	#define CCSPlayer_GiveNamedItem_SigBytes        32

	//_Z11UTIL_RemoveP11CBaseEntity
	#define UtilRemove_Sig				"\x55\x89\xE5\x83\xEC\x18\x89\x5D\xF8\x8B\x5D\x08\x89\x75\xFC\x85\xDB\x74\x2A\x89\xDE\x81\xC6\x2A\x2A\x2A\x2A\x74\x2A\x8B\x83\x2A"
	#define UtilRemove_SigBytes			32

	//_ZN12CCSGameRules12IsThereABombEv 0077B520
	#define CEntList_Pattern                        "\x55\x31\xC0\x89\xE5\x53\xBA\x2A\x2A\x2A\x2A\x83\xEC\x14\x89\x54\x24\x08\x31\xDB\x89\x44\x24\x04\xC7\x04\x24\x2A\x2A\x2A\x2A\xE8"
	#define CEntList_PatternBytes                   32
	//bytes into the function for the op mov stack, <imm offset> (00997A80)
	//gEntList
	#define CEntList_gEntList			27
	//bytes into the function for the op after call <imm> (test eax, eax)
	//_ZN17CGlobalEntityList21FindEntityByClassnameEP11CBaseEntityPKc
	#define CEntList_FindEntity			36

#define CBaseCombatWeapon_GetName_Sig		"\x55\x89\xE5\x83\xEC\x08\x8B\x55\x08\x0F\xB7\x82\x1C\x05\x00\x00\x89\x04\x24\xE8\xF8\xB8\x09\x00\xC9\x83\xC0\x06\xC3"
#define CBaseCombatWeapon_GetName_SigBytes	29

	///////////////////////
	///END OF LINUX DEFS///
	///////////////////////

	#define CCSPlayer_SwitchTeam_Sig		"_ZN9CCSPlayer10SwitchTeamEi"
	#define CBaseEntity_ApplyAbsVelocity_Sig "_ZN11CBaseEntity23ApplyAbsVelocityImpulseERK6Vector"
#else 

	#define CBaseEntity_ApplyAbsVelocity_Sig	"\x83\xEC\x0C\x56\x57\x8B\x7C\x24\x18\xD9\x07\x8B\xF1\xD9\x05\xC8"
	#define CBaseEntity_ApplyAbsVelocity_SigBytes	16

	#define CCSPlayer_SwitchTeam_Sig		"\x83\xEC\x2A\x56\x57\x8B\x7C\x24\x2A\x57\x8B\xF1\xE8\x2A\x2A\x2A\x2A\x83\xC4\x04"
	#define CCSPlayer_SwitchTeam_SigBytes	20

	//use friendlyfire cvar to find it.
	#define SetFOV_Sig						"\x53\x8B\x5C\x24\x08\x85\xDB\x57\x8B\xF9\x75\x07\x5F\x32\xC0\x5B"
	#define SetFOV_SigBytes					16

	#define WantsLagComp_Sig				"\xA1\x88\x20\x49\x22\x83\xEC\x24\x83\x78\x2C\x00\x53\x56\x8B\x74"
	#define WantsLagComp_SigBytes			16

	//CCSPlayer not CBE - %s attacked a teammate
	#define OnTakeDamage_Sig				"\x8B\x44\x24\x04\x81\xEC\x80\x00\x00\x00\x53\x56\x8B\xF1\x50\x8D"
	#define OnTakeDamage_SigBytes			16

	#define OnTakeDamage_Offset				0x265
	#define WantsLagComp_Offset             0x29
	#define OnTakeDamage_OffsetBytes        1
	#define WantsLagComp_OffsetBytes        6

//Find gunshotsplash in _ZN13CWeaponCSBase13PhysicsSplashERK6Vector
// 1st sub in function.
// Only sub in there.
	#define GetFileWeaponInfoFromHandle_Sig			"\x66\x8B\x44\x24\x04\x66\x3B\x05\x32\x1C\x46\x22\x73\x17\x66\x3D"
	#define GetFileWeaponInfoFromHandle_SigBytes	16

	#define CCSPlayer_RoundRespawn_Sig			"\x56\x8B\xF1\x8B\x06\xFF\x90\x60\x04\x00\x00\x8B\x86\xE8\x0D\x00"
	#define CCSPlayer_RoundRespawn_SigBytes		16


	#define CBaseCombatWeapon_Delete_Sig        "\x56\x6A\x00\x6A\x00\x8B\xF1\x68\x2A\x2A\x25\x22\xC7\x46\x7C"
	#define CBaseCombatWeapon_Delete_SigBytes    15

	#define CBaseCombatWeapon_GetName_Sig		"\x66\x8B\x81\x08\x05\x00\x00\x50\xE8\x13\x3C\x1C\x00\x83\xC4\x04"
	#define CBaseCombatWeapon_GetName_SigBytes	16

	#define CBaseCombatPlayer_WeaponGetSlot_Sig			"\x53\x55\x8B\x6C\x24\x0C\x56\x8B\xD9\x57\x33\xF6\x8D\xBB\xC4\x06"
	#define CBaseCombatPlayer_WeaponGetSlot_SigBytes	16

	#define CBasePlayer_WeaponDrop_Sig			"\x81\xEC\x00\x01\x00\x00\x53\x55\x56\x8B\xB4\x24\x10\x01\x00\x00"
	//#define CBasePlayer_WeaponDrop_Sig			"\x53\x56\x57\x8B\xF1\x32\xDB\xE8\xD4\xDD\xEC\xFF\x8B\x7C\x24\x10"
	#define CBasePlayer_WeaponDrop_SigBytes		16

	#define CCSGame_TermRound_Sig			"\x83\xEC\x18\x53\x55\x8B\xD9\x8B\x4C\x24\x28\x56\x57\x33\xF6\x8D\x41\xFF\x33\xFF"
	#define CCSGame_TermRound_SigBytes		20

	#define CBaseEntity_CreateGib_Sig			"\x56\x6A\xFF\x68\x5C\x07\x3A\x22\xE8\xE3\x45\x01\x00\x8B\xF0\x8B"
	#define CBaseEntity_CreateGib_SigBytes		16

	#define CBaseFlex_SetModel_Sig				"\x8B\x44\x24\x04\x56\x57\x50\x8B\xF9\xE8\xB2\x88\xFD\xFF\x8B\xCF"
	#define CBaseFlex_SetModel_SigBytes			16

	#define CBaseAnimating_Teleport_Sig			"\x8B\x44\x24\x0C\x8B\x54\x24\x04\x56\x8B\xF1\x8B\x4C\x24\x0C\x50\x51\x52\x8B\xCE\xE8\xF7\x35\x02"
	#define CBaseAnimating_Teleport_SigBytes	24

	#define CCSPlayer_GiveNamedItem_Sig			"\x53\x8B\x5C\x24\x0C\x55\x56\x8B\x74\x24\x10\x53\x56\x8B\xE9\xE8"
	#define CCSPlayer_GiveNamedItem_SigBytes	16

	
	#define CEntList_Pattern				"\x53\x68\x2A\x2A\x2A\x2A\x6A\x00\xB9\x2A\x2A\x2A\x2A\x32\xDB\xE8\x2A\x2A\x2A\x2A\x85\xC0\x74\x2A\xB0\x01\x5B\xC3"
	#define CEntList_PatternBytes			28
	#define CEntList_gEntList				9
	#define CEntList_FindEntity				20

	

	//original address: 2225AB40 ("ResetHUD"), CBasePlayer::UpdateClientData( void )
	//we're going deep into it, however, starting at: mov eax, [esi]
	//but actually, only after the initial 8b for mov, because the scanner is 4byte aligned!
	#define CPlayer_UpdateClient_Sig "\x83\xEC\x34\x53\x56\x8B\xF1\x57\x8D\x4C\x24\x20\xE8\x3F\x5D\x02"
	//#define CPlayer_UpdateClient_Sig	"\x83\xEC\x34\x53\x56\x8B\xF1\x57\x8D\x4C\x24\x20\xE8\x0F\x5C\x02"
	#define CPlayer_UpdateClient_SigBytes	16
	//bytes into the function that we'll find something like: mov ecx, dword_2242AA78
	#define CPlayer_g_pGameRules			0x6D

#endif

#endif //_INCLUDE_OFFSETS_H
