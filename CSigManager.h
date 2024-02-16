/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2000 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 */

#ifndef _INCLUDE_CSIGMANAGER_H
#define _INCLUDE_CSIGMANAGER_H

#include <IMemoryUtils.h>
#include "sp_file_headers.h"
#ifndef WIN32
#include <sh_vector.h>
#include "sm_symtable.h"

using namespace SourceHook;
#endif

#include <ISmmPlugin.h>
extern ISmmPlugin *g_PLAPI;
extern ISmmAPI *g_SMAPI;

using namespace SourceMod;

struct DynLibInfo
{
	void *baseAddress;
	size_t memorySize;
};

#ifndef WIN32
typedef uint32_t Elf32_Addr;

struct LibSymbolTable
{
	SymbolTable table;
	Elf32_Addr lib_base;
	uint32_t last_pos;
};
#endif

class MemoryUtils : 
	public IMemoryUtils
{
public:
	~MemoryUtils();
public: // IMemoryUtils
	void *FindPattern(const void *libPtr, const char *pattern, size_t len);
	void *ResolveSymbol(void *handle, const char *symbol);
public:
	bool GetLibraryInfo(const void *libPtr, DynLibInfo &lib);
#ifndef WIN32
private:
	CVector<LibSymbolTable *> m_SymTables;
#endif
};

size_t UTIL_DecodeHexString(unsigned char *buffer, size_t maxlength, const char *hexstr);
void *LoadSignature(void *addrInBase, char sig[512]);
extern void *LoadSignature(void *addrInBase, char sig[512]);

extern MemoryUtils g_MemUtils;

#endif // _INCLUDE_CSIGMANAGER_H
