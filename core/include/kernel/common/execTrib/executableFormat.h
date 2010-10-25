#ifndef _EXECUTABLE_FORMAT_H
	#define _EXECUTABLE_FORMAT_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * A struct of function pointers which are entry points into a loaded
 * relocatable ELF file which has been mapped into the kernel's own address
 * space.
 **/

struct executableFormatS
{
	error_t (*identify)(void *buff);
	
};

#endif

