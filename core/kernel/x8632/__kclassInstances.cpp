
#include <arch/memory.h>
#include <platform/pageTables.h>
#include <kernel/common/processTrib/processTrib.h>


processTribC		processTrib(
	(void *)(ARCH_MEMORY___KLOAD_VADDR_BASE + 0x400000), 0x3FB00000,
#ifdef CONFIG_ARCH_x86_32_PAE
	reinterpret_cast<pagingLevel0S *>( 0xFFFFC000 ),
#else
	reinterpret_cast<pagingLevel0S *>( 0xFFFFD000 ),
#endif
	reinterpret_cast<paddr_t>( __kpagingLevel0Tables ));

