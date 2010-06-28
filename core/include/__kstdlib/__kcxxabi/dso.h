#ifndef __KCXXABI_DSO_H
	#define __KCXXABI_DSO_H

	//This number may increase to about 512, realistically. Or we may make it into a linked list.
	#define CXXABI_ATEXIT_MAX_NFUNCS	64

struct atexitFuncTableEntryS
{
	void (*func)(void *);
	void *arg, *dsoHandle;
} atexitFuncTable[ CXXABI_ATEXIT_MAX_NFUNCS ];

extern void * __dso_handle;

extern "C" int __cxa_atexit(
	void (*func)(void *),
	void *arg,
	void *dsoHandle
	);

extern "C" void __cxa_finalize(void *dsoHandle);

#endif

