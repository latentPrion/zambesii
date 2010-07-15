
#include <__ksymbols.h>
#include <arch/paddr_t.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kcxxlib/cstring>
#include <__kthreads/__korientation.h>
#include <__kthreads/__korientationpreConstruct.h>
#include <kernel/common/__koptimizationHacks.h>
#include <kernel/common/memoryTrib/memoryTrib.h>

extern "C" void __korientationMain(ubit32, multibootDataS *)
{
	error_t		ret;
	void		(**ctorPtr)();

	__koptimizationHacks();

	// Zero out the .BSS section of the kernel ELF.
	memset(&__kbssStart, 0, &__kbssEnd - &__kbssStart);

	// Prepare the kernel for its most basic sematics to begin working.
	__korientationPreConstruct::__kprocessInit();
	__korientationPreConstruct::__korientationThreadInit();
	__korientationPreConstruct::bspInit();

	// Call all global constructors.
	ctorPtr = reinterpret_cast<void (**)()>( &__kctorStart );
	for (; ctorPtr < reinterpret_cast<void(**)()>( &__kctorEnd );
		ctorPtr++)
	{
		(**ctorPtr)();
	};

	/**	EXPLANATION:
	 * To initialize the Memory Tributary, you must first calculate the
	 * size of the kernel's high address space. You then pass its base
	 * vaddr and size to the initialize() method, and check the return
	 * value. The kernel will typically not have a hole in the address
	 * space. So a __KNULL is passed to holeMapS *holeMap in the general
	 * case.
	 *
	 * What happens when we call memoryTrib.initialize()?
	 *
	 * The kernel swamp is initialized, and with it the kernel virtual
	 * address range, and consequently virtual memory management for the
	 * kernel; Then the kernel will attempt to spawn all memBmpCs for the
	 * chipset's memory regions.
	 **/
	ret = memoryTrib.initialize(
		reinterpret_cast<void *>( 0xC0000000 + 0x400000 ),
		static_cast<paddr_t>( 0x3FB00000 ),
		__KNULL);

	if (ret != ERROR_SUCCESS) {
		for (;;) {};
	};
}

