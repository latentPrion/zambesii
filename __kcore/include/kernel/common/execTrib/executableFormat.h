#ifndef _EXECUTABLE_FORMAT_H
	#define _EXECUTABLE_FORMAT_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * A struct of function pointers which are entry points into a loaded
 * relocatable ELF file which has been mapped into the kernel's own address
 * space.
 *
 * When initialize() is called for any module, the kernel will pass a buffer
 * with memory that the loaded module can use. The buffer is exactly 128B in
 * size. This should be enough for the module to use to 
 **/

struct sExecutableParser
{
	error_t (*initialize)(const char *archString, ubit16 wordSize);
	sarch_t (*identify)(void *buff);
	sarch_t (*isLocalArch)(void *buff);
};

extern struct sExecutableParser		elfParser;

#endif

