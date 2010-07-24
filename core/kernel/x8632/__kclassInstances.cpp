
#include <arch/paddr_t.h>
#include <arch/paging.h>
#include <chipset/__kmemory.h>
#include <platform/pageTables.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

memoryTribC		memoryTrib(
#ifdef CONFIG_ARCH_x86_32_PAE
	reinterpret_cast<pagingLevel0S *>( 0xFFFFC000 ),
#else
	reinterpret_cast<pagingLevel0S *>( 0xFFFFD000 ),
#endif
	reinterpret_cast<paddr_t>( __kpagingLevel0Tables ));

// Invoke NUMA Trib with the default constructor.
numaTribC		numaTrib;
cpuTribC		cpuTrib;
debugPipeC		__kdebug;

