
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
	void		(**ctorPtr)(), *mem;

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
			__kdebug.printf(WARNING"No debug buffer allocated.\n",
				0);
		};
	};
	__kdebug.refresh();
	__kdebug.printf(
		NOTICE"Kernel debug output tied to devices BUFFER and "
		"DEVICE1.\n",
		0);

	__kdebug.printf(
		NOTICE"Multiboot magic: %X, pointer: 0x%p\n",
		0, mbMagic, mbInfo);

	__kdebug.printf(NOTICE"CPU %d, thread %X\n"
		"\tHolds %d locks.\n", 0,
		cpuTrib.getCurrentCpuStream()->id,
		cpuTrib.getCurrentCpuStream()->currentTask->id,
		cpuTrib.getCurrentCpuStream()->currentTask->nLocksHeld);

	mem = (memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(1);

	memoryTrib.__kmemoryStream.memFree(mem);
	mem = (memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(2);

	memoryTrib.__kmemoryStream.memFree(mem);
	mem = (memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(1);

	memoryTrib.__kmemoryStream.memFree(mem);

	memoryTrib.__kmemoryStream.dump();
}

