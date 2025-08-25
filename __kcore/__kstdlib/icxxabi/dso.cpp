
#include <config.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>


struct sAtexitFuncTableEntry
{
	void (*func)(void *);
	void *arg, *dsoHandle;
} atexitFuncTable[CONFIG_CXXABI_ATEXIT_MAX_NFUNCS];

/**	EXPLANATION:
 * Required by Itanium ABI when using -nostartfiles.
 * This symbol must be defined. Cursor's AI thinks it must also point to itself.
 * **/
void *__dso_handle __attribute__((__visibility__("hidden"))) = &__dso_handle;

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
	for (int i=0; i<CONFIG_CXXABI_ATEXIT_MAX_NFUNCS; i++)
	{
		if (atexitFuncTable[i].func != NULL) { continue; }

		atexitFuncTable[i].func = func;
		atexitFuncTable[i].arg = arg;
		atexitFuncTable[i].dsoHandle = dsoHandle;
		return 0;
	};

	return -1;
}

/* __cxa_finalize(): Calls a function in the list if given a pointer,
 * or calls all destructors if passed a null pointer.
 **/
extern "C" void __cxa_finalize(void *dsoHandle)
{
	/**	EXPLANATION:
	 * If dsoHandle is NULL, call and destroy every object in the table,
	 * then return.
	 *
	 * Otherwise, destroy all of a particular DSO's objects.
	 * **/
	if (dsoHandle == NULL)
	{
		for (int i = 0; i < CONFIG_CXXABI_ATEXIT_MAX_NFUNCS; i++)
		{
			if (atexitFuncTable[i].func == NULL) { continue; }

			(*atexitFuncTable[i].func)(atexitFuncTable[i].arg);
			atexitFuncTable[i].func = NULL;
		}

		return;
	}

	// Call the funcs for the particular DSO.
	for (int i = 0; i < CONFIG_CXXABI_ATEXIT_MAX_NFUNCS; i++)
	{
		if (atexitFuncTable[i].dsoHandle != dsoHandle)
			{ continue; }

		(*atexitFuncTable[i].func)(atexitFuncTable[i].arg);

		// To remove, bring the rest one index up using memmove.
		int last = i;
		while (last < CONFIG_CXXABI_ATEXIT_MAX_NFUNCS - 1
			&& atexitFuncTable[last + 1].func != NULL)
		{
			last++;
		}

		if (last <= i) { continue; }

		memmove(
			&atexitFuncTable[i],
			&atexitFuncTable[i + 1],
			(last - i) * sizeof(atexitFuncTable[0]));

		// Clear the now-duplicate last entry
		memset(
			&atexitFuncTable[last], 0,
			sizeof(atexitFuncTable[last]));
	}
}

