
#include <__ksymbols.h>
#include <arch/paging.h>
#include <arch/paddr_t.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/cstring>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kthreads/__korientation.h>
#include <__kthreads/__korientationpreConstruct.h>
#include <kernel/common/__koptimizationHacks.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <firmware/x86emu.h>


extern "C" void __korientationMain(ubit32, multibootDataS *)
{
	error_t		ret;
	uarch_t		devMask;
	void		(**ctorPtr)();
	x86emu		*lm;

	__koptimizationHacks();

	memset(&__kbssStart, 0, &__kbssEnd - &__kbssStart);

	// Set up the kernel process, orientation thread, and BSP CPU Stream.
	__korientationPreConstruct::__kprocessInit();
	__korientationPreConstruct::__korientationThreadInit();
	__korientationPreConstruct::bspInit();

	DO_OR_DIE(firmwareTrib, initialize(), ret);
	DO_OR_DIE(timerTrib, initialize(), ret);
	DO_OR_DIE(interruptTrib, initialize(), ret);

	// Call all global constructors.
	ctorPtr = reinterpret_cast<void (**)()>( &__kctorStart );
	for (; ctorPtr < reinterpret_cast<void(**)()>( &__kctorEnd );
		ctorPtr++)
	{
		(**ctorPtr)();
	};

	// Initialize the kernel swamp.
	DO_OR_DIE(
		memoryTrib,
		initialize1(
			reinterpret_cast<void *>( 0xC0000000 + 0x400000 ),
			static_cast<paddr_t>( 0x3FB00000 ),
			__KNULL),
		ret);

	DO_OR_DIE(numaTrib, initialize(), ret);
	DO_OR_DIE(__kdebug, initialize(), ret);

	devMask = __kdebug.tieTo(DEBUGPIPE_DEVICE_BUFFER | DEBUGPIPE_DEVICE1);
	if (!__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE_BUFFER)
		|| !__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE1))
	{
		for (;;){};
		if (__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE1))
		{
			__kdebug.printf(WARNING"No debug buffer allocated.\n");
		};
	};
	__kdebug.refresh();
	__kdebug.printf(NOTICE"Kernel debug output tied to devices BUFFER and "
		"DEVICE1.\n");

	__kdebug.printf(NOTICE"__korientationMain: __kvaddrSpaceStream: "
		" tests:\n");

	/**	EXPLANATION:
	 * The things that must be tested are:
	 * 1. Allocate 4 pages. 2. Allocate 32 pages. 3. Allocate 2 pages.
	 * 4. Free the 32 from (2). Dump and see if there are now two nodes
	 * with the first having 32 pages on it. 5. Now free the two from (3).
	 * Dump and see if the swamp has compacted via reverse compacting.
	 **/
#define vaddrSpaceStreamAlloc(__n)			\
	(memoryTrib.__kmemoryStream.vaddrSpaceStream \
		.*memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(__n)

#define vaddrSpaceStreamFree(__v,__n)			\
	memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(__v, __n)

#define memAlloc(nPages, opt)				\
	(memoryTrib.__kmemoryStream.*memoryTrib.__kmemoryStream.memAlloc)(\
		nPages, opt)

	lm = new (memAlloc(PAGING_BYTES_TO_PAGES(sizeof(struct x86emu)), 0))
		x86emu;

	__kdebug.printf(NOTICE"Struct x86emu: size: %dB, v: 0x%p.\n",
		sizeof(struct x86emu), lm);

	x86emu_init_default(lm);
}

