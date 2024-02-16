#ifdef WIN32
	#define WINDOWS_LEAN_AND_MEAN	1
	#include <windows.h>
#else
	#include <dlfcn.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
#endif

#ifndef WIN32
	#include <fcntl.h>
	#include <link.h>
	#include <sys/mman.h>
#else
	#include "string.h"
#endif
#include "MemoryUtils.h"
#include "zm_linuxutils.h"
#include <eiface.h>
#include "CSigMngr.h"

CSigMngr g_SigMngr;

#undef CreateEvent
#undef GetClassName

bool FindAddr(const char *name, void *func, const char *format, const char *searchdata, int offset, int occ, bool ptp, bool rel, int area)
{
	int datalen = strlen(format);
	unsigned long start, end;

	#ifdef WIN32
    if (area == 1) { // sound emitter sys
        start = 0x2C000000;
        end = 0x2C023000;
    } else { // unknown, default to game dll
	    start = 0x22000000;
	    end = 0x224B2000; // 224B2FFE
    }
	#else
	start = 0x5a800000;
	end = 0x5affffff;
	#endif

	int num = 0;
	unsigned long range = end - start - datalen;
	char *data = (char*)start;
	for (unsigned long x = 0; x < range; x++) {
		data++;
		bool found = true;
		for (int c = 0; c < datalen; c++) {
			if (format[c] != 'x') continue;
			if (data[c] != searchdata[c]) {
				found = false;
				break;
			}
		}
		if (found) {
			if (num >= occ) {
                //Msg("Got %s at 0x%x\n", name, (unsigned long)data);
                
                unsigned long res;
                if (!rel) {
                    if (offset < 1)
                        res = (unsigned long)data;
                    else
                        res = *(unsigned long*)(data + offset);
                } else {
                    unsigned long tmp = *(unsigned long*)(data + offset);
                    res = (unsigned long)(((unsigned long)data) + offset + 4 + tmp);
                }
                if (ptp)
                    res = *(unsigned long*)res;
                //PMsg("   Addr: 0x%x\n", res);
                *(unsigned long*)func = res;
                return true;
			}
			num++;
		}
	}
    //META_LOG( g_PLAPI, "Couldn't find address for %s!\n", name);
	return false;
}

bool CSigMngr::ResolveAddress(signature_t *sigmem)
{
#ifdef WIN32
	MEMORY_BASIC_INFORMATION mem;

	if (!VirtualQuery(sigmem->memInBase, &mem, sizeof(MEMORY_BASIC_INFORMATION)))
		return false;

	if (mem.AllocationBase == NULL)
		return false;

	HMODULE dll = (HMODULE)mem.AllocationBase;

	//code adapted from hullu's linkent patch
	union 
	{
		unsigned long mem;
		IMAGE_DOS_HEADER *dos;
		IMAGE_NT_HEADERS *pe;
	} dllmem;

	dllmem.mem = (unsigned long)dll;

	if (IsBadReadPtr(dllmem.dos, sizeof(IMAGE_DOS_HEADER)) || (dllmem.dos->e_magic != IMAGE_DOS_SIGNATURE))
		return false;

	dllmem.mem = ((unsigned long)dll + (unsigned long)(dllmem.dos->e_lfanew));
	if (IsBadReadPtr(dllmem.pe, sizeof(IMAGE_NT_HEADERS)) || (dllmem.pe->Signature != IMAGE_NT_SIGNATURE))
		return false;

	//end adapted hullu's code

	IMAGE_NT_HEADERS *pe = dllmem.pe;

	sigmem->allocBase = mem.AllocationBase;
	sigmem->memSize = (DWORD)(pe->OptionalHeader.SizeOfImage);

	return true;
#else
	Dl_info info;
	struct stat buf;

	if (!dladdr(sigmem->memInBase, &info))
		return false;

	if (!info.dli_fbase || !info.dli_fname)
		return false;

	if (stat(info.dli_fname, &buf) != 0)
		return false;

	sigmem->allocBase = info.dli_fbase;
	sigmem->memSize = buf.st_size;

#ifdef DEBUG
	//Msg("Alloc base: %p\n", sigmem->allocBase);
#endif

	return true;
#endif
}

void *CSigMngr::ResolveSig( void *memInBase, const char *pattern, size_t siglen )
{
	signature_t sig;

	memset(&sig, 0, sizeof(signature_t));

	sig.sig = (const char *)pattern;
	sig.siglen = siglen;
	sig.memInBase = memInBase;

	if (!ResolveAddress(&sig))
		return NULL;

	const char *paddr = (const char *)sig.allocBase;
	bool found;

	register unsigned int j;

	sig.memSize -= sig.siglen;	//prevent a crash maybe?

	for (size_t i=0; i<sig.memSize; i+=sizeof(unsigned long))
	{
		found = true;
		for (j=0; j<sig.siglen; j++)
		{
			if ( (pattern[j] != (char)0x2A) &&
				 (pattern[j] != paddr[j]) )
			{
				found = false;
				break;
			}
		}
		if (found)
		{
			sig.offset = (void *)paddr;
			break;
		}
		//we're always gonna be on a four byte boundary
		paddr += sizeof(unsigned long);
	}

	return sig.offset;
}


#ifndef WIN32
void *FindSigAddress(SymbolMap *sym_map_ptr, char *sig_name, const char *sName, eSigType sType, int iOffset )
{
	//sigscan_t *sigscan_details = gpManiGameType->GetSigDetails(sig_name);
	sigscan_t sigscan_details;
	memset(&sigscan_details, 0, sizeof(sigscan_t));

	Q_strncpy( sigscan_details.linux_symbol, sig_name, sizeof( sigscan_details.linux_symbol ) ); // I know it's the actual sig but hackin here.
	Q_strncpy( sigscan_details.sig_name, sName, sizeof( sigscan_details.sig_name ) );

	int	sig_type = sType;
	int	index = iOffset;

	void *ptr = sym_map_ptr->FindAddress(sigscan_details.linux_symbol);

	if (ptr != NULL)
	{
		switch( sType )
		{
			case SIG_DIRECT:
				if (index != 0)
				{
					META_LOG( g_PLAPI, "Initial [%p] Sig [%s]", ptr, sigscan_details.sig_name );
				}
				ptr = (void *) ((unsigned long) ptr + (unsigned long) index);
				break;
			case SIG_INDIRECT:
				META_LOG( g_PLAPI, "  Initial [%p] Sig [%s]", ptr, sigscan_details.sig_name);
				ptr = *(void **) ((unsigned long) ptr + (unsigned long) index);
				break;
			default : 
				ptr = NULL;
				break;
		}
	}

	if (ptr == NULL)
	{
		META_LOG( g_PLAPI, "Sig [%s] Failed!!", sigscan_details.sig_name);
	}
	else
	{
		META_LOG( g_PLAPI, "Final [%p] [%s]", ptr, sigscan_details.sig_name);
	}

	return ptr;
}

ITempEntsSystem *GetTempEntsLinux(const char *sLibName)
{
	ITempEntsSystem *temp_ents;
	SymbolMap *sym_ptr = new SymbolMap;
	
	if ( !sym_ptr->GetLib(sLibName) )
	{
		META_LOG( g_PLAPI, "Failed to open [%s]", sLibName );
	}

	void *var_address = sym_ptr->FindAddress("te");
	if (var_address == NULL)
	{
		META_LOG( g_PLAPI, "dlsym failure : Error [%s]", dlerror());
	}
	else
	{
		META_LOG( g_PLAPI, "TempEnts Pre = %p", var_address);
		temp_ents = *(ITempEntsSystem **) var_address;
	}
	delete sym_ptr;
	return temp_ents;
}

CGameRules *GetCGameRulesLinux(const char *sLibName)
{
	CGameRules *game_rules;
	
	SymbolMap *sym_ptr = new SymbolMap;
	
	if ( !sym_ptr->GetLib(sLibName) )
	{
		META_LOG( g_PLAPI, "Failed to open [%s]", sLibName );
	}

	void *var_address = sym_ptr->FindAddress("g_pGameRules");
	if (var_address == NULL)
	{
		META_LOG( g_PLAPI, "dlsym failure : Error [%s]", dlerror());
	}
	else
	{
		META_LOG( g_PLAPI, "var_address = %p", var_address);
		game_rules = (CGameRules *) var_address;
		META_LOG( g_PLAPI, "game_rules = %p", game_rules);
	}


	var_address = sym_ptr->FindAddress("gEntList");
	if (var_address == NULL)
	{
		META_LOG( g_PLAPI, "dlsym failure : Error [%s]", dlerror());
	}
	else
	{
		META_LOG( g_PLAPI, "var_address = %p", var_address);
		//g_EntList = (void *)var_address;

		g_EntList = reinterpret_cast<void *>(var_address);
		META_LOG( g_PLAPI, "g_EntList = %p", g_EntList);
	}
	delete sym_ptr;
	return game_rules;
}
#endif




void *CSigMngr::ResolveSymbol(void *handle, const char *symbol)
{
#ifdef WIN32

	return GetProcAddress((HMODULE)handle, symbol);

#else

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