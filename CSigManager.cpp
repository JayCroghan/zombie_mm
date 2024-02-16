/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

#include "CSigManager.h"
#include "ShareSys.h"
#ifdef PLATFORM_LINUX
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>
#endif

MemoryUtils g_MemUtils;

#if 0
MemoryUtils::MemoryUtils()
{
#ifdef PLATFORM_WINDOWS

	SYSTEM_INFO info;
	GetSystemInfo(&info);

	m_PageSize = info.dwPageSize;

#elif defined PLATFORM_POSIX

	m_PageSize = sysconf(_SC_PAGE_SIZE);

#endif
}
#endif

MemoryUtils::~MemoryUtils()
{
#ifdef PLATFORM_LINUX
	for (size_t i = 0; i < m_SymTables.size(); i++)
	{
		delete m_SymTables[i];
	}
	m_SymTables.clear();
#endif
}

void *MemoryUtils::FindPattern(const void *libPtr, const char *pattern, size_t len)
{
	DynLibInfo lib;
	bool found;
	char *ptr, *end;

	memset(&lib, 0, sizeof(DynLibInfo));

	if (!GetLibraryInfo(libPtr, lib))
	{
		return NULL;
	}

	ptr = reinterpret_cast<char *>(lib.baseAddress);
	end = ptr + lib.memorySize;

	while (ptr < end)
	{
		found = true;
		for (register size_t i = 0; i < len; i++)
		{
			if (pattern[i] != '\x2A' && pattern[i] != ptr[i])
			{
				found = false;
				break;
			}
		}

		if (found)
			return ptr;

		ptr++;
	}

	return NULL;
}

void *MemoryUtils::ResolveSymbol(void *handle, const char *symbol)
{
#ifdef PLATFORM_WINDOWS

	return GetProcAddress((HMODULE)handle, symbol);

#elif defined PLATFORM_LINUX

	struct link_map *dlmap;
	struct stat dlstat;
	int dlfile;
	uintptr_t map_base;
	Elf32_Ehdr *file_hdr;
	Elf32_Shdr *sections, *shstrtab_hdr, *symtab_hdr, *strtab_hdr;
	Elf32_Sym *symtab;
	const char *shstrtab, *strtab;
	uint16_t section_count;
	uint32_t symbol_count;
	LibSymbolTable *libtable;
	SymbolTable *table;
	Symbol *symbol_entry;

	dlmap = (struct link_map *)handle;
	symtab_hdr = NULL;
	strtab_hdr = NULL;
	table = NULL;
	
	/* See if we already have a symbol table for this library */
	for (size_t i = 0; i < m_SymTables.size(); i++)
	{
		libtable = m_SymTables[i];
		if (libtable->lib_base == dlmap->l_addr)
		{
			table = &libtable->table;
			break;
		}
	}

	/* If we don't have a symbol table for this library, then create one */
	if (table == NULL)
	{
		libtable = new LibSymbolTable();
		libtable->table.Initialize();
		libtable->lib_base = dlmap->l_addr;
		libtable->last_pos = 0;
		table = &libtable->table;
		m_SymTables.push_back(libtable);
	}

	/* See if the symbol is already cached in our table */
	symbol_entry = table->FindSymbol(symbol, strlen(symbol));
	if (symbol_entry != NULL)
	{
		return symbol_entry->address;
	}

	/* If symbol isn't in our table, then we have open the actual library */
	dlfile = open(dlmap->l_name, O_RDONLY);
	if (dlfile == -1 || fstat(dlfile, &dlstat) == -1)
	{
		close(dlfile);
		return NULL;
	}

	/* Map library file into memory */
	file_hdr = (Elf32_Ehdr *)mmap(NULL, dlstat.st_size, PROT_READ, MAP_PRIVATE, dlfile, 0);
	map_base = (uintptr_t)file_hdr;
	if (file_hdr == MAP_FAILED)
	{
		close(dlfile);
		return NULL;
	}
	close(dlfile);

	if (file_hdr->e_shoff == 0 || file_hdr->e_shstrndx == SHN_UNDEF)
	{
		munmap(file_hdr, dlstat.st_size);
		return NULL;
	}

	sections = (Elf32_Shdr *)(map_base + file_hdr->e_shoff);
	section_count = file_hdr->e_shnum;
	/* Get ELF section header string table */
	shstrtab_hdr = &sections[file_hdr->e_shstrndx];
	shstrtab = (const char *)(map_base + shstrtab_hdr->sh_offset);

	/* Iterate sections while looking for ELF symbol table and string table */
	for (uint16_t i = 0; i < section_count; i++)
	{
		Elf32_Shdr &hdr = sections[i];
		const char *section_name = shstrtab + hdr.sh_name;

		if (strcmp(section_name, ".symtab") == 0)
		{
			symtab_hdr = &hdr;
		}
		else if (strcmp(section_name, ".strtab") == 0)
		{
			strtab_hdr = &hdr;
		}
	}

	/* Uh oh, we don't have a symbol table or a string table */
	if (symtab_hdr == NULL || strtab_hdr == NULL)
	{
		munmap(file_hdr, dlstat.st_size);
		return NULL;
	}

	symtab = (Elf32_Sym *)(map_base + symtab_hdr->sh_offset);
	strtab = (const char *)(map_base + strtab_hdr->sh_offset);
	symbol_count = symtab_hdr->sh_size / symtab_hdr->sh_entsize;

	/* Iterate symbol table starting from the position we were at last time */
	for (uint32_t i = libtable->last_pos; i < symbol_count; i++)
	{
		Elf32_Sym &sym = symtab[i];
		unsigned char sym_type = ELF32_ST_TYPE(sym.st_info);
		const char *sym_name = strtab + sym.st_name;
		Symbol *cur_sym;

		/* Skip symbols that are undefined or do not refer to functions or objects */
		if (sym.st_shndx == SHN_UNDEF || (sym_type != STT_FUNC && sym_type != STT_OBJECT))
		{
			continue;
		}

		/* Caching symbols as we go along */
		cur_sym = table->InternSymbol(sym_name, strlen(sym_name), (void *)(dlmap->l_addr + sym.st_value));
		if (strcmp(symbol, sym_name) == 0)
		{
			symbol_entry = cur_sym;
			libtable->last_pos = ++i;
			break;
		}
	}

	munmap(file_hdr, dlstat.st_size);
	return symbol_entry ? symbol_entry->address : NULL;

#endif
}

bool MemoryUtils::GetLibraryInfo(const void *libPtr, DynLibInfo &lib)
{
	unsigned long baseAddr;

	if (libPtr == NULL)
	{
		return false;
	}

#ifdef PLATFORM_WINDOWS

	MEMORY_BASIC_INFORMATION info;
	IMAGE_DOS_HEADER *dos;
	IMAGE_NT_HEADERS *pe;
	IMAGE_FILE_HEADER *file;
	IMAGE_OPTIONAL_HEADER *opt;

	if (!VirtualQuery(libPtr, &info, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		return false;
	}

	baseAddr = reinterpret_cast<unsigned long>(info.AllocationBase);

	/* All this is for our insane sanity checks :o */
	dos = reinterpret_cast<IMAGE_DOS_HEADER *>(baseAddr);
	pe = reinterpret_cast<IMAGE_NT_HEADERS *>(baseAddr + dos->e_lfanew);
	file = &pe->FileHeader;
	opt = &pe->OptionalHeader;

	/* Check PE magic and signature */
	if (dos->e_magic != IMAGE_DOS_SIGNATURE || pe->Signature != IMAGE_NT_SIGNATURE || opt->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		return false;
	}

	/* Check architecture, which is 32-bit/x86 right now
	 * Should change this for 64-bit if Valve gets their act together
	 */
	if (file->Machine != IMAGE_FILE_MACHINE_I386)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library */
	if ((file->Characteristics & IMAGE_FILE_DLL) == 0)
	{
		return false;
	}

	/* Finally, we can do this */
	lib.memorySize = opt->SizeOfImage;

#elif defined PLATFORM_LINUX

	Dl_info info;
	Elf32_Ehdr *file;
	Elf32_Phdr *phdr;
	uint16_t phdrCount;

	if (!dladdr(libPtr, &info))
	{
		return false;
	}

	if (!info.dli_fbase || !info.dli_fname)
	{
		return false;
	}

	/* This is for our insane sanity checks :o */
	baseAddr = reinterpret_cast<unsigned long>(info.dli_fbase);
	file = reinterpret_cast<Elf32_Ehdr *>(baseAddr);

	/* Check ELF magic */
	if (memcmp(ELFMAG, file->e_ident, SELFMAG) != 0)
	{
		return false;
	}

	/* Check ELF version */
	if (file->e_ident[EI_VERSION] != EV_CURRENT)
	{
		return false;
	}

	/* Check ELF architecture, which is 32-bit/x86 right now
	 * Should change this for 64-bit if Valve gets their act together
	 */
	if (file->e_ident[EI_CLASS] != ELFCLASS32 || file->e_machine != EM_386 || file->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library/shared object */
	if (file->e_type != ET_DYN)
	{
		return false;
	}

	phdrCount = file->e_phnum;
	phdr = reinterpret_cast<Elf32_Phdr *>(baseAddr + file->e_phoff);
	
	/* Add up the memory sizes of segments marked as PT_LOAD as those are the only ones that should be in memory */
	for (uint16_t i = 0; i < phdrCount; i++)
	{
		Elf32_Phdr &hdr = phdr[i];

		if (hdr.p_type == PT_LOAD)
		{
			lib.memorySize += hdr.p_memsz;
		}
	}

#endif

	lib.baseAddress = reinterpret_cast<void *>(baseAddr);

	return true;
}


void *LoadSignature(void *addrInBase, char sig[512])
{
	void *final_addr = NULL;
#ifndef WIN32
	if (sig[0] == '@')
	{
		Dl_info info;
		/* GNU only: returns 0 on error, inconsistent! >:[ */
		if (dladdr(addrInBase, &info) != 0)
		{
			void *handle = dlopen(info.dli_fname, RTLD_NOW);
			if (handle)
			{
#if defined ORANGEBOX_BUILD
				final_addr = g_MemUtils.ResolveSymbol(handle, &sig[1]);
#else
				final_addr = dlsym(handle, &sig[1]);
#endif
				dlclose(handle);
			} else {
				META_LOG( g_PLAPI, "[ZM] Unable to load library \"%s\"", "server");
				/*g_Logger.LogError("[SM] Unable to load library \"%s\".",
					s_TempSig.library);
				*/
			}
		} else {
			/*
			g_Logger.LogError("[SM] Unable to find library \"%s\" in memory (gameconf \"%s\")",
				s_TempSig.library,
				m_File);
			*/
			META_LOG( g_PLAPI,"[SM] Unable to find library \"%s\" in memory.", "server");
		}
	}
	if (final_addr)
	{
		goto skip_find;
	}
#endif
	/* First, preprocess the signature */
	unsigned char real_sig[511];
	size_t real_bytes;
	size_t length;

	real_bytes = 0;
	length = strlen(sig);

	real_bytes = UTIL_DecodeHexString(real_sig, sizeof(real_sig), sig);

	if (real_bytes >= 1)
	{
		final_addr = g_MemUtils.FindPattern(addrInBase, (char*)real_sig, real_bytes);
	}

#if defined PLATFORM_LINUX
skip_find:
#endif

	return final_addr;
}


size_t UTIL_DecodeHexString(unsigned char *buffer, size_t maxlength, const char *hexstr)
{
	size_t written = 0;
	size_t length = strlen(hexstr);

	for (size_t i = 0; i < length; i++)
	{
		if (written >= maxlength)
			break;
		buffer[written++] = hexstr[i];
		if (hexstr[i] == '\\' && hexstr[i + 1] == 'x')
		{
			if (i + 3 >= length)
				continue;
			/* Get the hex part. */
			char s_byte[3];
			int r_byte;
			s_byte[0] = hexstr[i + 2];
			s_byte[1] = hexstr[i + 3];
			s_byte[2] = '\0';
			/* Read it as an integer */
			sscanf(s_byte, "%x", &r_byte);
			/* Save the value */
			buffer[written - 1] = r_byte;
			/* Adjust index */
			i += 3;
		}
	}

	return written;
}

/*
void UTIL_GetGamePath ( char *path )
{
	char gamedir[256];
	char *p_path = NULL;
	g_ZombiePlugin.m_FileSystem->RelativePathToFullPath ( "gameinfo.txt", "GAME", gamedir, 256 );
	if ( gamedir[0] != 0 ) {
		char *p_slash = strrchr ( gamedir, PATH_SLASH );
		if ( p_slash )
			*p_slash = 0;

		p_path = strrchr ( gamedir, PATH_SLASH );

		if ( p_path )
			p_path ++;
	}

	Q_strncpy ( path, p_path, 256 );

}

// find the linux binary information from dlinfo()
#include <link.h>
#include <dlfcn.h>

void GetLinuxBins ( char *game, char *engine ) 
{
	link_map *map;

	pid_t pid = getpid();
	char file[255];
	snprintf(file, sizeof(file)-1, "/proc/%d/maps", pid);

	char gamedir[256];
	UTIL_GetGamePath ( gamedir );

#if !defined ( ORANGEBOX_BUILD )
	if ( UTIL_ScanFile ( file, "engine_i686.so" ) )
		Q_strncpy ( engine, "./bin/engine_i686.so", 256 );
	else if ( UTIL_ScanFile ( file, "engine_i486.so" ) )
		Q_strncpy ( engine, "./bin/engine_i486.so", 256 );
	else
		Q_strncpy ( engine, "./bin/engine_amd.so", 256 );

	Q_snprintf ( game, 256, "./%s/bin/server_i486.so", gamedir );

#else
	Q_strncpy ( engine, "./bin/engine.so", 256 );
	Q_snprintf ( game, 256, "./%s/bin/server.so", gamedir );
#endif
}#
*/