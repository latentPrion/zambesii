#ifndef _ELF_FUNCTIONS_H
	#define _ELF_FUNCTIONS_H

	#include <__kstdlib/__ktypes.h>

error_t elf_initialize(const char *archString, ubit16 wordSize);
sarch_t elf_identify(void *buff);
sarch_t elf_isLocalArch(void *buff);

#endif

