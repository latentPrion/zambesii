#ifndef _PLATFORM_MEMORY_H
	#define _PLATFORM_MEMORY_H

	#include <arch/x8632/memory.h>
	#include <chipset/ibmPc/memory.h>

#define PLATFORM_MEMORY_GET___KSYMBOL_PADDR(__sym)		\
	((((uarch_t)&__sym) - ARCH_MEMORY___KLOAD_VADDR_BASE)		\
		+ CHIPSET_MEMORY___KLOAD_PADDR_BASE)

#endif

