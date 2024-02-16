#ifdef WIN32
#define WINDOWS_LEAN_AND_MEAN	1
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <eiface.h>

#ifndef TYRANT_MM_EXPORTS
	#include "CSigMngr.h"
#else
	#include "../ZombieMod_mm/CSigMngr.h"
#endif

CSigMngr g_SigMngr;

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
	Msg("Alloc base: %p\n", sigmem->allocBase);
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