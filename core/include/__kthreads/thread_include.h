#ifndef THREAD_INCLUDE

#include <config.h>

#ifdef CONFIG_ARCH_x86_32
	#define THREAD_INCLUDE(__subpath)		<__kthreads/x8632/__subpath>
	#define THREAD_SOURCE_INCLUDE(__subpath)	<__kthreads/x8632/__subpath>
#else
	#error "No architecture was defined in the config file. Please either \
	edit the config file, or use the configure script with option \
	--arch=MY_ARCH."
#endif

#endif

