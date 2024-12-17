#ifndef KERNEL_INCLUDE

#include <config.h>

#ifdef CONFIG_ARCH_x86_32
	#define KERNEL_INCLUDE(__subpath)		<kernel/x8632/__subpath>
	#define KERNEL_SOURCE_INCLUDE(__subpath)	<kernel/x8632/__subpath>
#else
	#error "No recognized arch selected in config script. Try \
	--arch=MY_ARCH."
#endif

#endif

