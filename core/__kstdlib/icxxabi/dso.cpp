
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
//#include "icxxabi.h"

#define CXXABI_ATEXIT_MAX_NFUNCS 64

struct sAtexitFuncTableEntry
{
	void (*func)(void *);
	void *arg, *dsoHandle;
} atexitFuncTable[ CXXABI_ATEXIT_MAX_NFUNCS ];

/**	EXPLANATIONL:
 * No longer needed since we now properly use -nodefaultlibs and let the
 * compiler do its job, by allowing it to link the startfiles (crt*.o).
 * Apparently this symbol is contained within crtbegin.o for GCC's
 * implementation.
 * **/
// void * __dso_handle = 0;

extern "C" void __cxa_pure_virtual(void)
{
	panic(FATAL"Pure virtual function called with no resolve target.\n");
}

/* __cxa_atexit(): registers a new function in the atexit list.
 **/
extern "C" int __cxa_atexit(
	void (*func)(void *),
	void *arg,
	void *dsoHandle
	)
{
	(void)func;
	(void)arg;
	(void)dsoHandle;
#if 0
	for (int i=0; i<CXXABI_ATEXIT_MAX_NFUNCS; i++)
	{
		if (atexitFuncTable[i].func == 0)
		{
			atexitFuncTable[i].func = func;
			atexitFuncTable[i].arg = arg;
			atexitFuncTable[i].dsoHandle = dsoHandle;
			return 0;
		};
	};
	return -1;
#else
	return 0;
#endif
}

/* __cxa_finalize(): Calls a function in the list if given a pointer,
 * or calls all destructors if passed a null pointer.
 **/
extern "C" void __cxa_finalize(void *dsoHandle)
{
	// Function 1: Destroy all of a particular DSO's objects.
	if (dsoHandle != 0)
	{
		// Run through and find all entries with passed DSO handle.
		for (int i=0; i<CXXABI_ATEXIT_MAX_NFUNCS; i++)
		{
			if (atexitFuncTable[i].dsoHandle == dsoHandle)
			{
				// If handle matches, call function with arg.
				(*atexitFuncTable[i].func)(
					atexitFuncTable[i].arg
					);

				// To remove, bring the rest one index up.
				int j=i;
				if ((j < CXXABI_ATEXIT_MAX_NFUNCS - 1)
					&& (atexitFuncTable[j+1].func != 0))
				{
					while (j < CXXABI_ATEXIT_MAX_NFUNCS - 1)
					{
						// memcpy() may be faster here.
						atexitFuncTable[j].func =
							atexitFuncTable[j+1]
								.func;

						atexitFuncTable[j].arg =
							atexitFuncTable[j+1]
								.arg;

						atexitFuncTable[j].dsoHandle =
							atexitFuncTable[j+1]
								.dsoHandle;

						atexitFuncTable[j+1].func = 0;
						j++;
					};
				};
			};
		};
	}
	// Function 2: Call and destroy every object in the table.
	else
	{
		for (int i=0; i<CXXABI_ATEXIT_MAX_NFUNCS; i++)
		{
			if (atexitFuncTable[i].func != 0)
			{
				(*atexitFuncTable[i].func)(
					atexitFuncTable[i].arg
					);
				atexitFuncTable[i].func = 0;
			};
		};
	};
}

