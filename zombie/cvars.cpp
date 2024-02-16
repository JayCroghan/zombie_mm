/* ======== props_mm ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): David "BAILOPAN" Anderson
* ============================
*/
#include "server_class.h"
#include "ZombiePlugin.h"
#include "cvars.h"
#include "myutils.h"

ZombieAccessor					g_Accessor;
extern ConVar					zombie_mode;
extern CTeam					*teamCT;
extern CTeam					*teamT;
extern ArrayLengthSendProxyFn	TeamCount;
extern PlayerInfo_t				g_ZombiePlayers[65];

ConVar zombie_version( "zombie_version", ZOMBIE_VERSION, FCVAR_REPLICATED | FCVAR_SPONLY | FCVAR_NOTIFY, "Props Plugin version");

bool ZombieAccessor::RegisterConCommandBase(ConCommandBase *pVar)
{
	return META_REGCVAR( pVar );
}

CON_COMMAND( zombie_check, "" )
{
	g_ZombiePlugin.ShowZomb();
}


CON_COMMAND( zombie_player, "<name> - Turns player into a zombie if zombie mode is on." )
{
	if ( !zombie_mode.GetBool() )
	{
		META_LOG( g_PLAPI, "%s: Zombie mode is not currently enabled\n", g_ZombiePlugin.m_Engine->Cmd_Argv(0) );
		return;
	}
	CPlayerMatchList matches(g_ZombiePlugin.m_Engine->Cmd_Argv(1), true);
	if ( matches.Count() < 1 ) 
	{
		META_LOG( g_PLAPI, "%s: Unable to find any living players that match %s\n", g_ZombiePlugin.m_Engine->Cmd_Argv(0), g_ZombiePlugin.m_Engine->Cmd_Argv(1));
		//print(pEdict, "%s: Unable to find any living players that match %s\n", g_ZombiePlugin.m_Engine->Cmd_Argv(0), g_ZombiePlugin.m_Engine->Cmd_Argv(1));
		return;
	}
	for ( int x = 0; x < matches.Count(); x++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)matches.GetMatch(x)->GetUnknown()->GetBaseEntity();
		int iPlayer = g_ZombiePlugin.m_Engine->IndexOfEdict( pPlayer->edict() );
		if ( !g_ZombiePlayers[iPlayer].isZombie )
		{
			g_ZombiePlugin.MakeZombie( pPlayer ); // this command changes the argv/argc so we can't get the command name from it
			//print(pEdict, "nm_zombie_player: %s is now a zombie\n", /*engine->Cmd_Argv(0),*/ engine->GetClientConVarValue(pPlayer->entindex(), "name"));
			//logact(pEdict, "nm_zombie_player", "zombified %s", engine->GetClientConVarValue(pPlayer->entindex(), "name"));
			META_LOG( g_PLAPI, "%s is now a zombie.", g_ZombiePlugin.m_Engine->Cmd_Argv(0), g_ZombiePlugin.m_Engine->GetClientConVarValue( iPlayer, "name") );
		}
	}
	return;
}
CON_COMMAND( termr, "- Toggles zombie mode" )
{
	UTIL_TermRound( 1.0, Terrorists_Escaped );
}

CON_COMMAND( zombies, "- Toggles zombie mode" )
{
	if ( zombie_mode.GetBool() )
	{
		g_ZombiePlugin.Hud_Print( NULL, "Zombie mode disabled." );
		g_ZombiePlugin.ZombieOff();
	}
	else
	{
		g_ZombiePlugin.Hud_Print( NULL, "Zombie mode enabled." );
		g_ZombiePlugin.ZombieOn();
	}
    return;
}

CON_COMMAND( zombie_test, "" )
{
	edict_t *pEntity = g_ZombiePlugin.m_Engine->PEntityOfEntIndex( 1 );
	if ( pEntity && !pEntity->IsFree() && g_ZombiePlugin.m_Engine->GetPlayerUserId( pEntity ) > 0 )
	{
		CBaseEntity *pBase = pEntity->GetUnknown()->GetBaseEntity();
		int iFrags = UTIL_FindOffsetMap( pBase, "CBasePlayer", "m_hZoomOwner" );
		META_LOG( g_PLAPI, "%d",iFrags );
		META_LOG( g_PLAPI, "%d",iFrags );
	}
} 

CON_COMMAND(zombie_restrict, "<a/t/c/w> <weapon> [amount] - Restricts a weapon for all players, terrorist, counter-terrorist, or winning team. Optional last parameter is how many weapons of the type the team can hold.")
{
	//edict_t *pEdict = g_ZombiePlugin.m_Engine->PEntityOfEntIndex(g_ZombiePlugin.iCurrentPlayer);

	CUtlVector<int> matches;
	if ( !Q_stricmp(g_ZombiePlugin.m_Engine->Cmd_Argv(2), "all"))
	{
		matches.AddToTail(CSW_P228);
		matches.AddToTail(CSW_GLOCK);
		matches.AddToTail(CSW_ELITE);
		matches.AddToTail(CSW_FIVESEVEN);
		matches.AddToTail(CSW_USP);
		matches.AddToTail(CSW_DEAGLE);
		matches.AddToTail(CSW_M3);
		matches.AddToTail(CSW_XM1014);
		matches.AddToTail(CSW_MAC10);
		matches.AddToTail(CSW_UMP45);
		matches.AddToTail(CSW_MP5);
		matches.AddToTail(CSW_TMP);
		matches.AddToTail(CSW_P90);
		matches.AddToTail(CSW_SCOUT);
		matches.AddToTail(CSW_AUG);
		matches.AddToTail(CSW_SG550);
		matches.AddToTail(CSW_GALIL);
		matches.AddToTail(CSW_FAMAS);
		matches.AddToTail(CSW_AWP);
		matches.AddToTail(CSW_M4A1);
		matches.AddToTail(CSW_G3SG1);
		matches.AddToTail(CSW_SG552);
		matches.AddToTail(CSW_AK47);
		matches.AddToTail(CSW_M249);
		matches.AddToTail(CSW_SMOKEGRENADE);
		matches.AddToTail(CSW_HEGRENADE);
		matches.AddToTail(CSW_FLASHBANG);
		matches.AddToTail(CSW_PRIMAMMO);
		matches.AddToTail(CSW_SECAMMO);
		matches.AddToTail(CSW_VEST);
		matches.AddToTail(CSW_VESTHELM);
		matches.AddToTail(CSW_NVGS);
		matches.AddToTail(CSW_DEFUSEKIT);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted all weapons for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else if (!Q_strnicmp(g_ZombiePlugin.m_Engine->Cmd_Argv(2), "equip", 5)) {
		matches.AddToTail(CSW_SMOKEGRENADE);
		matches.AddToTail(CSW_HEGRENADE);
		matches.AddToTail(CSW_FLASHBANG);
		matches.AddToTail(CSW_PRIMAMMO);
		matches.AddToTail(CSW_SECAMMO);
		matches.AddToTail(CSW_VEST);
		matches.AddToTail(CSW_VESTHELM);
		matches.AddToTail(CSW_NVGS);
		matches.AddToTail(CSW_DEFUSEKIT);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted equipment for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else if (!Q_strnicmp(g_ZombiePlugin.m_Engine->Cmd_Argv(2), "pistol", 6)) {
		matches.AddToTail(CSW_P228);
		matches.AddToTail(CSW_GLOCK);
		matches.AddToTail(CSW_ELITE);
		matches.AddToTail(CSW_FIVESEVEN);
		matches.AddToTail(CSW_USP);
		matches.AddToTail(CSW_DEAGLE);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted pistols for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else if (!Q_strnicmp(g_ZombiePlugin.m_Engine->Cmd_Argv(2), "shotgun", 7)) {
		matches.AddToTail(CSW_M3);
		matches.AddToTail(CSW_XM1014);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted shotguns for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else if (!Q_strnicmp(g_ZombiePlugin.m_Engine->Cmd_Argv(2), "smg", 3)) {
		matches.AddToTail(CSW_MAC10);
		matches.AddToTail(CSW_UMP45);
		matches.AddToTail(CSW_MP5);
		matches.AddToTail(CSW_TMP);
		matches.AddToTail(CSW_P90);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted smgs for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else if (!Q_strnicmp(g_ZombiePlugin.m_Engine->Cmd_Argv(2), "rifle", 5)) {
		matches.AddToTail(CSW_SCOUT);
		matches.AddToTail(CSW_AUG);
		matches.AddToTail(CSW_SG550);
		matches.AddToTail(CSW_GALIL);
		matches.AddToTail(CSW_FAMAS);
		matches.AddToTail(CSW_AWP);
		matches.AddToTail(CSW_M4A1);
		matches.AddToTail(CSW_G3SG1);
		matches.AddToTail(CSW_SG552);
		matches.AddToTail(CSW_AK47);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted rifles for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else if (!Q_strnicmp(g_ZombiePlugin.m_Engine->Cmd_Argv(2), "sniper", 5)) {
		matches.AddToTail(CSW_SCOUT);
		matches.AddToTail(CSW_SG550);
		matches.AddToTail(CSW_AWP);
		matches.AddToTail(CSW_G3SG1);
		//logact(pEdict, engine->Cmd_Argv(0), "restricted sniper rifles for %s",
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
	} else {
		int id = LookupBuyID(g_ZombiePlugin.m_Engine->Cmd_Argv(2));
		if (id == CSW_NONE)
		{
			//print(pEdict, "%s: Unknown weapon specified\n", engine->Cmd_Argv(0));
			META_LOG( g_PLAPI, "%s: Unknown weapon specified\n", g_ZombiePlugin.m_Engine->Cmd_Argv(0) );
			return;
		}
		//logact(pEdict, engine->Cmd_Argv(0), "restricted %s for %s", LookupWeaponName(id),
		//	(*engine->Cmd_Argv(1) == 't') ? "Terrorists" : ((*engine->Cmd_Argv(1) == 'c') ? "Counter-Terroists" : ((*engine->Cmd_Argv(1) == 'w') ? "winning team" : "all players")));
		matches.AddToTail(id);
	}
	int num = 0;
	if ( g_ZombiePlugin.m_Engine->Cmd_Argc() > 3 )
	{
		num = abs(atoi(g_ZombiePlugin.m_Engine->Cmd_Argv(3)));
	}

	for (int x = 0; x < matches.Count(); x++) {
		int id = matches[x];
		if (*g_ZombiePlugin.m_Engine->Cmd_Argv(1) == 't') {
			g_ZombiePlugin.g_RestrictT[id] = num;
			META_LOG( g_PLAPI, "%s: The %s has been restricted for the Terrorists\n", g_ZombiePlugin.m_Engine->Cmd_Argv(0), LookupWeaponName(id));
		} else if (*g_ZombiePlugin.m_Engine->Cmd_Argv(1) == 'c') {
			g_ZombiePlugin.g_RestrictCT[id] = num;
			META_LOG( g_PLAPI, "%s: The %s has been restricted for the Counter-Terrorists\n", g_ZombiePlugin.m_Engine->Cmd_Argv(0), LookupWeaponName(id));
		} else if (*g_ZombiePlugin.m_Engine->Cmd_Argv(1) == 'w') {
			g_ZombiePlugin.g_RestrictW[id] = num;
			META_LOG( g_PLAPI, "%s: The %s has been restricted for the winning team\n", g_ZombiePlugin.m_Engine->Cmd_Argv(0), LookupWeaponName(id));
		} else {
			g_ZombiePlugin.g_RestrictT[id] = num;
			g_ZombiePlugin.g_RestrictCT[id] = num;
			META_LOG( g_PLAPI, "%s: The %s has been restricted\n", g_ZombiePlugin.m_Engine->Cmd_Argv(0), LookupWeaponName(id));
		}
	}
	for (int i = 1; i <= g_SMAPI->pGlobals()->maxClients; i++) 
	{
		edict_t *pEdict = g_ZombiePlugin.m_Engine->PEntityOfEntIndex(i);
		if (!pEdict || !pEdict->GetUnknown() || !pEdict->GetUnknown()->GetBaseEntity())
		{
			continue;
		}
		//g_EmptyServerPlugin.ClientSettingsChanged(pEdict);
		CBasePlayer *pP = (CBasePlayer*)pEdict->GetUnknown()->GetBaseEntity();
		if (pP->IsAlive() == 1) {
			/*CBaseCombatWeapon* pWeapon = pP->GetActiveWeapon();
			if (pWeapon && !g_ZombiePlugin.CanUseWeapon(pP, pWeapon)) 
			{
				UTIL_WeaponDrop(pP, pWeapon, NULL, NULL );
				//pP->Weapon_Drop(pWeapon, NULL, NULL);
			}*/
		}
	}
    return;
}
CON_COMMAND( zombie_list_restrictions, "Lists all current weapon restrictions." )
{
	int x = 0;
	META_CONPRINT( "\nCurrent Weapon Restrictions\n==========================\n" );
	for ( x = 0; x < CSW_MAX; x++ )
	{
		META_CONPRINTF( "%d. %d %d\n", ( x ), g_ZombiePlugin.g_RestrictCT[x], g_ZombiePlugin.g_RestrictT[x] );
	}
	META_CONPRINT( "==========================\n" );
}

/*CON_COMMAND( zombie_restrict, "" )
{
	const char *sWeapon = g_ZombiePlugin.m_Engine->Cmd_Argv( 1 );
	int x;
	for ( x = 0; x <= g_ZombiePlugin.iRestricted; x++ )
	{
		if ( FStrEq( g_ZombiePlugin.sRestricted[x].c_str(), sWeapon ) )
		{
			META_CONPRINTF( "[PROJECTX] You already have '%s' added to the weapon restrictions list.\n", sWeapon );
			return;
		}
	}
	if ( ( g_ZombiePlugin.iRestricted + 1 ) == 100 )
	{
		META_CONPRINTF( "[PROJECTX] Maximum amount of restrictions in place !\n", sWeapon );
		return;
	}
	g_ZombiePlugin.iRestricted++;
	int iCount = g_ZombiePlugin.iRestricted;
	g_ZombiePlugin.sRestricted[iCount].assign( sWeapon );
}*/