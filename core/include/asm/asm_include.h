#ifndef ASM_INCLUDE

	#include <config.h>

#ifdef CONFIG_ARCH_x86_32
	#define ASM_INCLUDE(__subpath)		<asm/x8632/__subpath>
	#define ASM_SOURCE_INCLUDE(__subpath)	<asm/x8632/__subpath>
#else
	#error "No recognized arch chosen in configure. Try --arch=MY_ARCH."
#endif

#endif

