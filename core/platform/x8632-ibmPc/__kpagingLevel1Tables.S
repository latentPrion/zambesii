
#include <config.h>

#ifdef CONFIG_ARCH_x86_32_PAE
	#include "paeLevel1Tables.S"
#else
	#include "noPaeLevel1Tables.S"
#endif

