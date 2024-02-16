#ifndef _INCLUDE_CSIGMNGR_H
#define _INCLUDE_CSIGMNGR_H

bool FindAddr(const char *name, void *func, const char *format, const char *searchdata, int offset, int occ, bool ptp, bool rel, int area);

struct signature_t
{
	void *allocBase;
	void *memInBase;
	size_t memSize;
	void *offset;
	const char *sig;
	size_t siglen;
};

class CSigMngr
{
public:
	void *ResolveSig(void *memInBase, const char *pattern, size_t siglen);
	int ResolvePattern(void *memInBase, const char *pattern, size_t siglen, int number, ...);
#ifdef DEBUG
	void TestLinux();
#endif
private:
	bool ResolveAddress(signature_t *sigmem);
};

extern CSigMngr g_SigMngr;

#endif //_INCLUDE_CSIGMNGR_H
