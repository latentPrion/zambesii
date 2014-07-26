
#include <arch/memory.h>
#include <platform/pageTables.h>
#include <kernel/common/processTrib/processTrib.h>


ProcessTrib		processTrib(
	(void *)(ARCH_MEMORY___KLOAD_VADDR_BASE + 0x400000), 0x3FBFF000);

