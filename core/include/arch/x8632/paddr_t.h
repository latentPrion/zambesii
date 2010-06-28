#ifndef _PADDR_T_H
	#define _PADDR_T_H

	#include <__kstdlib/__ktypes.h>

#ifndef __ASM__

#ifdef CONFIG_ARCH_x86_32_PAE
typedef ubit64		paddr_t;
#else
typedef uarch_t		paddr_t;
#endif

#endif /* ! defined (__ASM__) */

#endif

