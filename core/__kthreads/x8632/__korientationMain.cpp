
#include <__ksymbols.h>
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

extern "C" void __korientationMain(ubit32 mbMagic, multibootDataS *mbInfo)
{
	error_t		ret;
	uarch_t		devMask;
	void		(**ctorPtr)(), *mem1, *mem2, *mem3;
	paddr_t		p;

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

	mem1 = vaddrSpaceStreamAlloc(4);
	mem2 = vaddrSpaceStreamAlloc(32);
	mem3 = vaddrSpaceStreamAlloc(2);

	vaddrSpaceStreamFree(mem2, 32);
	vaddrSpaceStreamFree(mem3, 2);
	vaddrSpaceStreamFree(mem1, 4);

/*	mem1 = vaddrSpaceStreamAlloc(0x3FACA);
	mem2 = vaddrSpaceStreamAlloc(0x24);
	mem3 = vaddrSpaceStreamAlloc(0x2);
	vaddrSpaceStreamAlloc(1);
	vaddrSpaceStreamFree(mem2, 0x24);
	vaddrSpaceStreamFree(mem3, 0x2);
	vaddrSpaceStreamFree(mem1, 0x3FACA);

	numaTrib.fragmentedGetFrames(1, &p);
	numaTrib.releaseFrames(p, 1);
	numaTrib.fragmentedGetFrames(32, &p);
	numaTrib.fragmentedGetFrames(32, &p);
	memoryTrib.__kmemoryStream.dump();
*/
}

