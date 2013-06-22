
#include <__ksymbols.h>
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/compiler/cxxrtl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <__kclasses/memReservoir.h>
#include <__kthreads/__korientation.h>
#include <kernel/common/__koptimizationHacks.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>

#include <arch/cpuControl.h>


int oo=0, pp=0, qq=0, rr=0;

extern "C" void __korientationInit(ubit32, multibootDataS *)
{
	error_t		ret;
	uarch_t		devMask;
	threadC		*mainTask;
	containerProcessC	&__kprocess = *processTrib.__kgetStream();

	/* Zero out uninitialized sections, prepare kernel locking and place a
	 * pointer to the BSP CPU Stream into the BSP CPU; then we can call all
	 * C++ global constructors.
	 **/
	__koptimizationHacks();
	memset(&__kbssStart, 0, &__kbssEnd - &__kbssStart);
	DO_OR_DIE(bspCpu, initializeBspCpuLocking(), ret);
	cxxrtl::callGlobalConstructors();
	bspCpu.powerManager.setPowerStatus(cpuStreamC::powerManagerC::C0);

	/* Initialize exception handling, then do chipset-wide early init.
	 * Finally, initialize the irqControl mod, and mask all IRQs off to
	 * place the chipset into a stable state.
	 **/
	DO_OR_DIE(interruptTrib, initializeExceptions(), ret);
	DO_OR_DIE(zkcmCore, initialize(), ret);
	DO_OR_DIE(zkcmCore.irqControl, initialize(), ret);
	zkcmCore.irqControl.maskAll();
	DO_OR_DIE(processTrib, initialize(), ret);
	DO_OR_DIE(
		__kprocess,
		initialize(
			CC"@h:boot/zambesii/zambesii.zxe",
			CC"__KRAMDISK_NAME=:__kramdisk\0",
			__KNULL),
		ret);

	/* Initialize __kspace level physical memory management, then the
	 * kernel Memory Stream.
	 **/
	DO_OR_DIE(memoryTrib, initialize(), ret);
	DO_OR_DIE(memoryTrib, __kspaceInitialize(), ret);
	DO_OR_DIE(processTrib.__kgetStream()->memoryStream, initialize(), ret);

	/* Initialize the kernel debug pipe for boot logging, etc.
	 **/
	DO_OR_DIE(__kdebug, initialize(), ret);
	devMask = __kdebug.tieTo(DEBUGPIPE_DEVICE_BUFFER | DEBUGPIPE_DEVICE1);
	if (!__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE_BUFFER)) {
		__kprintf(WARNING ORIENT"No debug buffer allocated.\n");
	};

	__kdebug.refresh();
	__kprintf(NOTICE ORIENT"Kernel debug output tied to devices BUFFER and "
		"DEVICE1.\n");

	/* Initialize the kernel Memory Reservoir (heap) and object cache pool.
	 **/
	DO_OR_DIE(memReservoir, initialize(), ret);
	DO_OR_DIE(cachePool, initialize(), ret);

	/* Initialize the CPU Tributary's internal BMPs etc, then initialize the
	 * BSP CPU's scheduler to co-op level scheduling capability, and
	 * spawn the __korientationMain thread, ending __korientationInit.
	 **/
	DO_OR_DIE(cpuTrib, initialize(), ret);
	DO_OR_DIE(zkcmCore.cpuDetection, initialize(), ret);
	DO_OR_DIE(cpuTrib, initializeBspCpuStream(), ret);

	/* Spawn the new thread for __korientationMain. There is no need to
	 * unschedule __korientationInit() because it will never be scheduled.
	**/
	DO_OR_DIE(
		__kprocess,
		spawnThread(
			(void (*)(void *))&__korientationMain, __KNULL,
			__KNULL,
			taskC::ROUND_ROBIN,
			0,
			SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT,
			&mainTask),
		ret);

	cpuTrib.getCurrentCpuStream()->taskStream.pull();
}

void __korientationMain(void)
{
	error_t			ret;
	threadC			*self;

	self = static_cast<threadC *>( cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask() );

	__kprintf(NOTICE ORIENT"Main running. Task ID 0x%x (@0x%p).\n",
		self->getFullId(), self);

	/* Initialize Interrupt Trib IRQ management (__kpin and __kvector),
	 * then load the chipset's bus-pin mappings and initialize timer
	 * services.
	 **/
	DO_OR_DIE(interruptTrib, initializeIrqManagement(), ret);
	DO_OR_DIE(zkcmCore.irqControl.bpm, loadBusPinMappings(CC"isa"), ret);
	DO_OR_DIE(zkcmCore.timerControl, initialize(), ret);
	DO_OR_DIE(timerTrib, initialize(), ret);

	// Detect physical memory.
	DO_OR_DIE(zkcmCore.memoryDetection, initialize(), ret);
	DO_OR_DIE(memoryTrib, pmemInit(), ret);

	// Detect and wake all CPUs.
	DO_OR_DIE(cpuTrib, initializeAllCpus(), ret);
/*__kprintf(NOTICE ORIENT"localInterrupts: %d, nLocksHeld: %d.\n",
	cpuControl::interruptsEnabled(),
	self->nLocksHeld);
for (;;) { asm volatile("hlt\n\t"); };
*/
	/* Initialize the VFS Trib to enable us to begin constructing the
	 * various currentts, and then populate the distributary namespace.
	 **/
	DO_OR_DIE(vfsTrib, initialize(), ret);
	DO_OR_DIE(distributaryTrib, initialize(), ret);



	distributaryProcessC		*dtribs[3];

	for (ubit8 i=0; i<3; i++)
	{
		DO_OR_DIE(
			processTrib,
			spawnDistributary(
				CC"@d/storage/cisternn", __KNULL,
				NUMABANKID_INVALID, 0, 0,
				(void *)i,
				&dtribs[i]),
			ret);

		__kprintf(NOTICE ORIENT"Spawned %dth process.\n", i);
	};

	for (ubit8 i=0; i<3; i++)
	{
		callbackStreamC::genericCallbackS	*callback;

		self->getTaskContext()->callbackStream.pull(
			(callbackStreamC::headerS **)&callback);

		__kprintf(NOTICE ORIENT"pulled %dth callback: err %d.\n",
			callback->header.privateData,
			callback->header.err);
	};

	__kprintf(NOTICE ORIENT"GG :).\n");
	for (;;) { asm volatile("hlt\n\t"); };
}

