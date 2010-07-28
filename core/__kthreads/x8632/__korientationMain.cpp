
#include <__ksymbols.h>
#include <arch/paddr_t.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kcxxlib/cstring>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kthreads/__korientation.h>
#include <__kthreads/__korientationpreConstruct.h>
#include <kernel/common/__koptimizationHacks.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>


extern "C" void __korientationMain(ubit32, multibootDataS *)
{
	error_t		ret;
	void		(**ctorPtr)();
	void		*r;

	// Prepare the kernel for its most basic semantics to begin working.
	__korientationPreConstruct::__kprocessInit();
	__korientationPreConstruct::__korientationThreadInit();
	__korientationPreConstruct::bspInit();

	// Ensure all the right rivulets are loaded into the descriptor.
	ret = firmwareTrib.initialize();
	DO_OR_DIE(ret);

	// Call initialize() on the interruptTrib ASAP.
//	ret = interruptTrib.initialize();
//	DO_OR_DIE(ret);

	// Zero out the .BSS section of the kernel ELF.
	memset(&__kbssStart, 0, &__kbssEnd - &__kbssStart);

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

	// The Interrupt Tributary should be initialized here.

	// Firmware Trib is initialized. Initialize kernel debugPipe.
	ret = __kdebug.initialize();
	DO_OR_DIE(ret);

	ret = __kdebug.tieTo(DEBUGPIPE_DEVICE_TERMINAL);
	DO_OR_DIE(ret);

	ret = __kdebug.tieTo(DEBUGPIPE_DEVICE_BUFFER);
	DO_OR_DIE(ret);

	(firmwareTrib.getTerminalFwRiv()->clear)();

	for (uarch_t i=0; i<2; i++)
	{
		__kdebug.printf(
			(utf8Char *)"\"Hello world!\" from Zambezii.\t", 0);
	}

	// I REALLY want to get rid of this function.
	__koptimizationHacks();
}

