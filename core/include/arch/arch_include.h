#ifndef ARCH_INCLUDE

#include <config.h>

#ifdef CONFIG_ARCH_x86_32
	#define ARCH_INCLUDE(__subpath)			<arch/x8632/__subpath>
	#define ARCH_SOURCE_INCLUDE(__subpath)		<arch/x8632/__subpath>
#else
	#error "No recognizible architecture defined. Try --arch-MY_ARCH in the \
	configure script."
#endif

#endif

