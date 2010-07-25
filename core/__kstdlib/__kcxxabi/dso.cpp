
#include <__kstdlib/__kcxxabi/dso.h>

void * __dso_handle = 0;

/* __cxa_atexit(): registers a new function in the atexit list.
 **/
extern "C" int __cxa_atexit(
	void (*func)(void *),
	void *arg,
	void *dsoHandle
	)
{
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
}

/* __cxa_finalize(): Calls a function in the list if given a pointer, or calls all destructors if passed a null pointer.
 **/
extern "C" void __cxa_finalize(void *dsoHandle)
{
	//Function 1: Destroy all of a particular DSO's objects.
	if (dsoHandle != 0)
	{
		//Run through the list and find all entries with the DSO handle passed to us.
		for (int i=0; i<CXXABI_ATEXIT_MAX_NFUNCS; i++)
		{
			if (atexitFuncTable[i].dsoHandle == dsoHandle)
			{
				//If the DSO handle matches, call the function with its registered arg, and remove it from the list.
				(*atexitFuncTable[i].func)(
					atexitFuncTable[i].arg
					);
				
				//To remove a function from the list, we bring all the rest of the functions one index up.
				int j=i;
				if ((j < CXXABI_ATEXIT_MAX_NFUNCS - 1) && (atexitFuncTable[j+1].func != 0))
				{
					while (j < CXXABI_ATEXIT_MAX_NFUNCS - 1)
					{
						//A memcpy() would be faster here.
						atexitFuncTable[j].func = atexitFuncTable[j+1].func;
						atexitFuncTable[j].arg = atexitFuncTable[j+1].arg;
						atexitFuncTable[j].dsoHandle = atexitFuncTable[j+1].dsoHandle;
						atexitFuncTable[j+1].func = 0;
						j++;
					};
				};
			};
		};
	}
	//Function 2: Call and destroy every object in the table.
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

