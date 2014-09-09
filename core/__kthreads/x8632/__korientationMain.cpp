
#include <__ksymbols.h>
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/callback.h>
#include <__kstdlib/compiler/cxxrtl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/memory>
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
#include <kernel/common/floodplainn/floodplainn.h>

#include <arch/cpuControl.h>

int oo=0, pp=0, qq=0, rr=0;

static void				__korientationMain1(void);
static __kcbFn				__korientationMain2;
static Floodplainn::initializeReqCbFn	__korientationMain3;
static __kcbFn				__korientationMain4;

#include <commonlibs/libacpi/libacpi.h>
static void rDumpSrat(void)
{
	acpi::sRsdt	*rsdt;
	acpiR::sSrat	*srat;
	void		*context, *handle;
	uarch_t		nSrats;

	rsdt = acpi::getRsdt();
	printf(NOTICE"RSDT mapped to 0x%p.\n", rsdt);

	context = handle = NULL;
	srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);
	for (nSrats=0; srat != NULL;
		srat = acpiRsdt::getNextSrat(rsdt, &context, &handle), nSrats++)
	{
		acpiR::srat::sCpu	*cpuEntry;
		void			*handle2;
		uarch_t			nCpuEntries;

		printf(NOTICE"Srat #%d, at vaddr 0x%p. CPU entries:\n",
			nSrats, srat);

		handle2 = NULL;
		cpuEntry = acpiR::srat::getNextCpuEntry(srat, &handle2);
		for (nCpuEntries=0; cpuEntry != NULL;
			cpuEntry = acpiR::srat::getNextCpuEntry(srat, &handle2),
				nCpuEntries++)
		{
			printf(NOTICE"Entry %d: bank %d cpuid %d "
				"firmware-enabled? %s.\n",
				nCpuEntries,
				cpuEntry->domain0
					| (cpuEntry->domain1 & 0xFFFFFF00),
				cpuEntry->lapicId,
				((FLAG_TEST(
					cpuEntry->flags,
					ACPI_SRAT_CPU_FLAGS_ENABLED))
					? "yes" : "no"));
		};

		printf(NOTICE"%d CPU entries in all. "
			"N: Memory range entries:\n",
			nCpuEntries);

		acpiR::srat::sMem	*memEntry;
		uarch_t			nMemEntries;

		handle2 = NULL;
		memEntry = acpiR::srat::getNextMemEntry(srat, &handle2);
		for (nMemEntries=0; memEntry != NULL;
			memEntry = acpiR::srat::getNextMemEntry(srat, &handle2),
				nMemEntries++)
		{
			printf(NOTICE"Entry %d: bank %d, firmware-enabled? "
				"%s, hotplug? %s.\n"
				"\tBase paddr 0x%P_%P, length 0x%P_%P.\n",
				nMemEntries,
				memEntry->domain0
				| (memEntry->domain1 << 16),
				((FLAG_TEST(
					memEntry->flags,
					ACPI_SRAT_MEM_FLAGS_ENABLED))
					? "yes" : "no"),
				((FLAG_TEST(
					memEntry->flags,
					ACPI_SRAT_MEM_FLAGS_HOTPLUG))
					? "yes" : "no"),
				memEntry->baseHigh, memEntry->baseLow,
				memEntry->lengthHigh, memEntry->lengthLow);
		};

		printf(NOTICE"%d memory range entries.\n", nMemEntries);
	};
}

static void dumpSrat(void)
{
	(void)dumpSrat;

	acpi::initializeCache();

	if (acpi::findRsdp() != ERROR_SUCCESS)
	{
		printf(ERROR"Unable to find RSDP.\n");
		return;
	};

	if (acpi::testForRsdt())
	{
		if (acpi::mapRsdt() != ERROR_SUCCESS)
		{
			printf(ERROR"Failed to map the RSDT into vmem.\n");
			return;
		};
		rDumpSrat();
	}
	else if (acpi::testForXsdt()) {
		printf(FATAL"Machine has an XSDT, but no RSDT; aborting.\n");
	} else {
		printf(FATAL"Unable to find either RSDT or XSDT.\n");
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

extern "C" void __korientationMain(ubit32, sMultibootData *)
{
	error_t			ret;
	uarch_t			devMask;
	Thread			*self;
	ContainerProcess	&__kprocess = *processTrib.__kgetStream();

	/* Zero out uninitialized sections, prepare kernel locking and place a
	 * pointer to the BSP CPU Stream into the BSP CPU; then we can call all
	 * C++ global constructors.
	 **/
	__koptimizationHacks();
	bspCpu.initializeBaseState();
	memset(&__kbssStart, 0, &__kbssEnd - &__kbssStart);
	cxxrtl::callGlobalConstructors();
	self = (Thread *)cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread() ;

	/* Initialize exceptions, then move on to __kspace level physical
	 * memory management, then the kernel Memory Stream. Then when we have
	 * MM up, we initialize the debug pipe.
	 **/
	DO_OR_DIE(interruptTrib, initializeExceptions(), ret);
	DO_OR_DIE(memoryTrib, initialize(), ret);
	DO_OR_DIE(memoryTrib, __kspaceInitialize(), ret);

	DO_OR_DIE(zkcmCore, initialize(), ret);
	DO_OR_DIE(__kdebug, initialize(), ret);
	devMask = __kdebug.tieTo(DEBUGPIPE_DEVICE_BUFFER | DEBUGPIPE_DEVICE1);
	if (!FLAG_TEST(devMask, DEBUGPIPE_DEVICE_BUFFER)) {
		printf(WARNING ORIENT"No debug buffer allocated.\n");
	}
	else
	{
		printf(NOTICE ORIENT"Kernel debug output tied to devices "
			"BUFFER and DEVICE1.\n");
	};

	DO_OR_DIE((*__kprocess.getVaddrSpaceStream()), initialize(), ret);
	DO_OR_DIE(__kprocess.memoryStream, initialize(), ret);
	// This does nothing for IRQ ctl, but remaps VGA buff into kvaddrspace.
	zkcmCore.irqControl.chipsetEventNotification(IRQCTL_EVENT_MEMMGT_AVAIL, 0);
	DO_OR_DIE(memReservoir, initialize(), ret);
	DO_OR_DIE(cachePool, initialize(), ret);

	/* Next block is dedicated to initializing Process management,
	 * Threading and scheduling.
	 **/
	DO_OR_DIE(processTrib, initialize(), ret);
	DO_OR_DIE(
		__kprocess,
		initialize(
			CC"@h:boot/zambesii/zambesii.zxe",
			CC"__KRAMDISK_NAME=:__kramdisk\0",
			NULL),
		ret);

	/* This next block is dedicated to initializing the BSP CPU stream.
	 **/
	DO_OR_DIE(zkcmCore.cpuDetection, initialize(), ret);
	DO_OR_DIE(cpuTrib, initialize(), ret);
	DO_OR_DIE(cpuTrib, loadBspInformation(), ret);
	DO_OR_DIE(
		cpuTrib,
		spawnStream(
			CpuStream::bspBankId,
			CpuStream::bspCpuId, CpuStream::bspAcpiId),
		ret);

	DO_OR_DIE(bspCpu, bind(), ret);
	DO_OR_DIE(bspCpu.taskStream, cooperativeBind(), ret);

	/* The next block is dedicated to initializing the core of the
	 * microkernel. Scheduling, Message passing, Processes and Threads.
	 **/
	DO_OR_DIE(taskTrib, initialize(), ret);

#if 0
	/* Initialize IRQs.
	 **/
	DO_OR_DIE(zkcmCore.irqControl, initialize(), ret);
	zkcmCore.irqControl.maskAll();


	zkcmCore.irqControl.chipsetEventNotification(
		IRQCTL_EVENT___KSPACE_MEMMGT_AVAIL, 0);
#endif

	printf(NOTICE ORIENT"Entering message loop. Task ID 0x%x (@0x%p).\n",
		self->getFullId(), self);

	__korientationMain1();
	for (;FOREVER;)
	{
		MessageStream::sHeader		*iMessage;
		Callback			*callback;

		self->messageStream.pull(&iMessage);
		callback = (Callback *)iMessage->privateData;

		switch (iMessage->subsystem)
		{
		default:
			// Discard message if it has no callback.
			if (callback == NULL) { break; };

			(*callback)(iMessage);
			delete callback;
			break;
		};
	};

	printf(NOTICE ORIENT"Main: Exited async loop. Killing.\n");
	taskTrib.kill(self->getFullId());
}

void __korientationMain1(void)
{
	error_t			ret;
	DistributaryProcess	*dp;

	// Initialize the VFS roots.
	DO_OR_DIE(vfsTrib, initialize(), ret);
	DO_OR_DIE(vfsTrib, getFvfs()->initialize(), ret);
	DO_OR_DIE(vfsTrib, getDvfs()->initialize(), ret);

	/* Initialize the Distributary Trib and start up the UDI driver indexer
	 * dtrib to allow us to search the kernel driver index.
	 **/
	DO_OR_DIE(distributaryTrib, initialize(), ret);
	DO_OR_DIE(
		processTrib, spawnDistributary(
			CC"///@d//././udi-driver-indexer//./.", NULL,
			NUMABANKID_INVALID,
			0, 0, new __kCallback(&__korientationMain2),
			&dp),
		ret);

	floodplainn.setZudiIndexServerTid(dp->id);
}

void __korientationMain2(MessageStream::sHeader *msg)
{
	error_t		ret;

	DIE_ON(msg->error);
	DO_OR_DIE(floodplainn, initializeReq(&__korientationMain3), ret);
}

void __korientationMain3(error_t ret)
{
	DIE_ON(ret);

	/* Start the chipset up.
	 **/
	DO_OR_DIE(floodplainn, enumerateBaseDevices(), ret);

	zuiServer::newDeviceInd(
		CC"by-id/0", zuiServer::INDEX_KERNEL,
		new __kCallback(&__korientationMain4));
}

void __korientationMain4(MessageStream::sHeader *msgIt)
{
	Thread				*self;
	fplainn::Device			*chipsetDev; (void)chipsetDev;
	error_t				ret;
	Floodplainn::sZudiIndexMsg	*msg;

	msg = (Floodplainn::sZudiIndexMsg *)msgIt;
	self = static_cast<Thread *>( cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread() );

	if (msg->info.action != zuiServer::NDACTION_INSTANTIATE)
	{
		printf(FATAL ORIENT"Failed to instantiate "
			"root device.\n\tError is %s; got as far as %d.\n",
			strerror(msg->header.error), msg->info.action);
	};

	DIE_ON(msg->header.error);
//	floodplainn.getDevice(CC"by-id/0", &chipsetDev);
//	chipsetDev->driver->dump();

	printf(NOTICE ORIENT"About to dormant.\n");
	taskTrib.dormant(self->getFullId());

	/* Initialize Interrupt Trib IRQ management (__kpin and __kvector),
	 * then load the chipset's bus-pin mappings and initialize timer
	 * services.
	 **/
	DO_OR_DIE(interruptTrib, initializeIrqs(), ret);
	DO_OR_DIE(zkcmCore.irqControl.bpm, loadBusPinMappings(CC"isa"), ret);
	DO_OR_DIE(zkcmCore.timerControl, initialize(), ret);
	DO_OR_DIE(timerTrib, initialize(), ret);

	// Detect physical memory.
	DO_OR_DIE(zkcmCore.memoryDetection, initialize(), ret);
	DO_OR_DIE(memoryTrib, pmemInit(), ret);

	// Detect and wake all CPUs.
	DO_OR_DIE(cpuTrib, initializeAllCpus(), ret);

	DistributaryProcess		*dtribs[3];

	for (ubit8 i=0; i<3; i++)
	{
		DO_OR_DIE(
			processTrib,
			spawnDistributary(
				CC"@d/storage/cisternn", NULL,
				NUMABANKID_INVALID, 0, 0,
				(void *)(uintptr_t)i,
				&dtribs[i]),
			ret);

		printf(NOTICE ORIENT"Spawned %dth process.\n", i);
	};

	sarch_t		waitForTimeout=1;
	ret = self->parent->timerStream.createRelativeOneshotEvent(
		sTimestamp(0, 3, 0), 0, 0, NULL);

	if (ret != ERROR_SUCCESS)
	{
		waitForTimeout = 0;
		printf(ERROR ORIENT"Failed to create 1 second timeout.\n");
	};

	for (ubit8 i=0; i<((waitForTimeout) ? 0xFF : 3); i++)
	{
		MessageStream::sHeader	*iMessage;

		self->messageStream.pull(&iMessage);

		switch (iMessage->subsystem)
		{
		case MSGSTREAM_SUBSYSTEM_PROCESS:
			printf(NOTICE ORIENT"pulled %dth callback: err %d. "
				"New process' ID: 0x%x.\n",
				iMessage->privateData,
				iMessage->error,
				iMessage->sourceId);

			break;

		case MSGSTREAM_SUBSYSTEM_TIMER:
			TimerStream::sTimerMsg	*timerEvent;

			timerEvent = (TimerStream::sTimerMsg *)&iMessage;
			printf(NOTICE ORIENT"pulled timer timeout. "
				"Actual expiration: %d:%dns.\n",
				timerEvent->actualExpirationStamp.time.seconds,
				timerEvent->actualExpirationStamp.time.nseconds);

			printf(NOTICE ORIENT"Kernel should meet scheduler now, and be halted.\n");
			break;
		};
	};

	printf(NOTICE ORIENT"Halting unfavourably.\n");
	for (;FOREVER;) { asm volatile("hlt\n\t"); };
}

