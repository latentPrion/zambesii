
#include <__ksymbols.h>
#include <arch/paddr_t.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kcxxlib/cstring>
#include <__kthreads/__korientation.h>
#include <__kthreads/__korientationpreConstruct.h>
#include <kernel/common/__koptimizationHacks.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>

extern "C" void __korientationMain(ubit32, multibootDataS *)
{
	error_t		ret;
	void		(**ctorPtr)();

	__koptimizationHacks();

	// Zero out the .BSS section of the kernel ELF.
	memset(&__kbssStart, 0, &__kbssEnd - &__kbssStart);

	// Prepare the kernel for its most basic semantics to begin working.
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

	// Initialize the kernel swamp.
	ret = memoryTrib.initialize1(
		reinterpret_cast<void *>( 0xC0000000 + 0x400000 ),
		static_cast<paddr_t>( 0x3FB00000 ),
		__KNULL);

	DO_OR_DIE(ret);

	ret = numaTrib.initialize();
	DO_OR_DIE(ret);
}

