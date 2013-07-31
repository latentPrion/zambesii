
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

#include <commonlibs/libacpi/libacpi.h>
static void rDumpSrat(void)
{
	acpi_rsdtS	*rsdt;
	acpi_rSratS	*srat;
	void		*context, *handle;
	uarch_t		nSrats;

	rsdt = acpi::getRsdt();
	__kprintf(NOTICE"RSDT mapped to 0x%p.\n", rsdt);

	context = handle = __KNULL;
	srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);
	for (nSrats=0; srat != __KNULL;
		srat = acpiRsdt::getNextSrat(rsdt, &context, &handle), nSrats++)
	{
		acpi_rSratCpuS		*cpuEntry;
		void			*handle2;
		uarch_t			nCpuEntries;

		__kprintf(NOTICE"Srat #%d, at vaddr 0x%p. CPU entries:\n",
			nSrats, srat);

		handle2 = __KNULL;
		cpuEntry = acpiRSrat::getNextCpuEntry(srat, &handle2);
		for (nCpuEntries=0; cpuEntry != __KNULL;
			cpuEntry = acpiRSrat::getNextCpuEntry(srat, &handle2),
				nCpuEntries++)
		{
			__kprintf(NOTICE"Entry %d: bank %d cpuid %d "
				"firmware-enabled? %s.\n",
				nCpuEntries,
				cpuEntry->domain0
					| (cpuEntry->domain1 & 0xFFFFFF00),
				cpuEntry->lapicId,
				((__KFLAG_TEST(
					cpuEntry->flags,
					ACPI_SRAT_CPU_FLAGS_ENABLED))
					? "yes" : "no"));
		};

		__kprintf(NOTICE"%d CPU entries in all. "
			"N: Memory range entries:\n",
			nCpuEntries);

		acpi_rSratMemS		*memEntry;
		uarch_t			nMemEntries;

		handle2 = __KNULL;
		memEntry = acpiRSrat::getNextMemEntry(srat, &handle2);
		for (nMemEntries=0; memEntry != __KNULL;
			memEntry = acpiRSrat::getNextMemEntry(srat, &handle2),
				nMemEntries++)
		{
			__kprintf(NOTICE"Entry %d: bank %d, firmware-enabled? "
				"%s, hotplug? %s.\n"
				"\tBase paddr 0x%P_%P, length 0x%P_%P.\n",
				nMemEntries,
				memEntry->domain0
				| (memEntry->domain1 << 16),
				((__KFLAG_TEST(
					memEntry->flags,
					ACPI_SRAT_MEM_FLAGS_ENABLED))
					? "yes" : "no"),
				((__KFLAG_TEST(
					memEntry->flags,
					ACPI_SRAT_MEM_FLAGS_HOTPLUG))
					? "yes" : "no"),
				memEntry->baseHigh, memEntry->baseLow,
				memEntry->lengthHigh, memEntry->lengthLow);
		};

		__kprintf(NOTICE"%d memory range entries.\n", nMemEntries);
	};
}

static void dumpSrat(void)
{
	acpi::initializeCache();

	if (acpi::findRsdp() != ERROR_SUCCESS)
	{
		__kprintf(ERROR"Unable to find RSDP.\n");
		return;
	};

	if (acpi::testForRsdt())
	{
		if (acpi::mapRsdt() != ERROR_SUCCESS)
		{
			__kprintf(ERROR"Failed to map the RSDT into vmem.\n");
			return;
		};
		rDumpSrat();
	}
	else if (acpi::testForXsdt()) {
		__kprintf(FATAL"Machine has an XSDT, but no RSDT; aborting.\n");
	} else {
		__kprintf(FATAL"Unable to find either RSDT or XSDT.\n");
	};
}

/**	EXPLANATION:
 * __korientationMain is responsible for:
 *	1. Preparing the kernel executable image in RAM for execution (zeroing
 *	   (.BSS, preparing locking, executing constructors).
 *	2. Initializing CPU exceptions.
 *	3. Initializing the kernel's process control block.
 *	4. Initializing the Memory Manager.
 *	   a. Setup the __kspace memory bank (boot PMM).
 *	   b. Setup the kernel process' Memory Stream (VMM).
 *	   c. Setup the kernel heap.
 *	5. Setting up the kernel's debug log (we actually do this out of order,
 *	   before the heap because debug output is really valuable).
 *	6. Initializing the scheduler on the BSP CPU.
 *
 * At the end of __korientationInit(), regardless of the arch/chipset being
 * booted on, we should be able to rely on having __kspace level MM, cooperative
 * scheduling with threading and exceptions.
 *
 * We then pass control to __korientationMain().
 **/
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
	devMask = __kdebug.tieTo(/*DEBUGPIPE_DEVICE_BUFFER |*/ DEBUGPIPE_DEVICE1);
	if (!__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE_BUFFER)) {
		__kprintf(WARNING ORIENT"No debug buffer allocated.\n");
	};

	// __kdebug.refresh();
	__kprintf(NOTICE ORIENT"Kernel debug output tied to devices BUFFER and "
		"DEVICE1.\n");

//	dumpSrat();

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

/**	EXPLANATION:
 * This function is entered after __korientationInit() so it has access to
 * memory management (__kspace level), cooperative scheduling on the BSP,
 * thread spawning and process spawning, along with exception and IRQs.
 *
 * Certain chipsets may need to do extra work inside of __korientationMain to
 * have IRQs etc up to speed, but that is the general initialization state at
 * this point.
 *
 * The rest of this function guides the kernel through the bootstrap sequence
 * (timers, VFS, drivers, etc). Generally the floodplainn VFS is initialized
 * first, and then populated with the ZKCM devices, and then the rest of the
 * kernel is initialized with an emphasis on getting the timers initialized
 * ASAP so we can get the BSP CPU to pre-emptive scheduling status quickly.
 **/
utf8Char	buff[512];
void __korientationMain(void)
{
	error_t			ret;
	threadC			*self;

	self = static_cast<threadC *>( cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask() );

	__kprintf(NOTICE ORIENT"Main running. Task ID 0x%x (@0x%p).\n",
		self->getFullId(), self);

	DO_OR_DIE(vfsTrib, initialize(), ret);
	__kprintf(NOTICE"Testing sprintf.\n");
	ubit32 len = snprintf(
		buff, sizeof(buff),
		NOTICE ORIENT OPTS(NOLOG)"%d: %s. %r, %p.\n",
		1, "This is a flat string",
		"This " OPTS(NOLOG)"%s string recurses (l%d)",
			"stupid", 1,
		buff);

	__kprintf(NOTICE ORIENT"\"%s\". (len %d).\n", buff, len);
	__kprintf(NOTICE ORIENT"About to dormant.\n");
	taskTrib.dormant(self->getFullId());

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

	sarch_t		waitForTimeout=1;
	ret = self->parent->timerStream.createRelativeOneshotEvent(
		timestampS(0, 3, 0), __KNULL, __KNULL, 0);

	if (ret != ERROR_SUCCESS)
	{
		waitForTimeout = 0;
		__kprintf(ERROR ORIENT"Failed to create 1 second timeout.\n");
	};

	for (ubit8 i=0; i<((waitForTimeout) ? 0xFF : 3); i++)
	{
		zcallback::headerS	*callback;

		self->getTaskContext()->callbackStream.pull(&callback);

		switch (callback->subsystem)
		{
		case ZMESSAGE_SUBSYSTEM_PROCESS:
			zcallback::genericS	*procMsg;

			procMsg = (zcallback::genericS *)callback;
			__kprintf(NOTICE ORIENT"pulled %dth callback: err %d. "
				"New process' ID: 0x%x.\n",
				procMsg->header.privateData,
				procMsg->header.err,
				procMsg->header.sourceId);

			break;

		case ZMESSAGE_SUBSYSTEM_TIMER:
			timerStreamC::eventS	*timerEvent;

			timerEvent = (timerStreamC::eventS *)callback;
			__kprintf(NOTICE ORIENT"pulled timer timeout. "
				"Actual expiration: %d:%dns.\n",
				timerEvent->actualStamp.time.seconds,
				timerEvent->actualStamp.time.nseconds);

			__kprintf(NOTICE ORIENT"Kernel should meet scheduler now, and be halted.\n");
			break;
		};
	};

	__kprintf(NOTICE ORIENT"Halting unfavourably.\n");
	for (;;) { asm volatile("hlt\n\t"); };
}

