
#include <tests.h>
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
#include <__kthreads/main.h>
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
static __kcbFn				__korientationMain3;
static __kcbFn				__korientationMain4;
static __kcbFn				__korientationMain5;

#include <commonlibs/libacpi/libacpi.h>
static void rDumpSrat(void)
{
	acpi::sRsdt	*rsdt;
	acpiR::sSrat	*srat;
	void		*context, *handle;
	uarch_t		nSrats;

	rsdt = acpi::getRsdt();
	printf(NOTICE"RSDT mapped to %p.\n", rsdt);

	context = handle = NULL;
	srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);
	for (nSrats=0; srat != NULL;
		srat = acpiRsdt::getNextSrat(rsdt, &context, &handle), nSrats++)
	{
		acpiR::srat::sCpu	*cpuEntry;
		void			*handle2;
		uarch_t			nCpuEntries;

		printf(NOTICE"Srat #%d, at vaddr %p. CPU entries:\n",
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
				"\tBase paddr %P_%P, length %P_%P.\n",
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
 *		a. Setup the __kspace memory bank (boot PMM).
 *		b. Setup the kernel process' Memory Stream (VMM).
 *		c. Setup the kernel heap.
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

extern "C" void main(ubit32, sMultibootData *)
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
	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread() ;

	/* Initialize exceptions, then move on to __kspace level physical
	 * memory management, then the kernel Memory Stream. Then when we have
	 * MM up, we initialize the debug pipe.
	 **/
	DO_OR_DIE(interruptTrib, initializeExceptions(), ret);
	DO_OR_DIE(memoryTrib, initialize(), ret);
	DO_OR_DIE(memoryTrib, __kspaceInitialize(), ret);

	DO_OR_DIE(zkcmCore, initialize(), ret);
	DO_OR_DIE(__kdebug, initialize(), ret);
	devMask = __kdebug.tieTo(DEBUGPIPE_DEVICE_BUFFER
		| DEBUGPIPE_DEVICE1 | DEBUGPIPE_DEVICE2);
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
#endif

	printf(NOTICE ORIENT"Entering message loop. Task ID %x (@%p).\n",
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
			if (callback == NULL)
			{
				printf(WARNING ORIENT"Unknown message with "
					"no callback.\n"
					"\tSrcTid %x, subsys %d, func %d.\n",
					iMessage->sourceId,
					iMessage->subsystem,
					iMessage->function);

				break;
			};

			(*callback)(iMessage);
			delete callback;
			break;
		};

		delete iMessage;
	};

	printf(NOTICE ORIENT"Main: Exited async loop. Killing.\n");
	taskTrib.kill(self->getFullId());
}

void __korientationMain1(void)
{
	error_t			ret;

	// Initialize the VFS roots.
	DO_OR_DIE(vfsTrib, initialize(), ret);
	DO_OR_DIE(vfsTrib, getFvfs()->initialize(), ret);
	DO_OR_DIE(vfsTrib, getDvfs()->initialize(), ret);

	/* Next block, we initialize the kernel's Floodplainn.
	 **/
	DO_OR_DIE(floodplainn, initialize(), ret);
	DO_OR_DIE(floodplainn.zui, initialize(), ret);
	DO_OR_DIE(floodplainn.zum, initialize(), ret);
	DO_OR_DIE(floodplainn.zudi, initialize(), ret);

	/* Start the chipset up.
	 **/
//	DO_OR_DIE(floodplainn, enumerateBaseDevices(), ret);
	floodplainn.zui.newDeviceInd(
		CC"by-id", fplainn::Zui::INDEX_KERNEL,
		new __kCallback(&__korientationMain2));
}

void __korientationMain2(MessageStream::sHeader *msgIt)
{
	Thread				*self;
	fplainn::Device			*chipsetDev; (void)chipsetDev;
	error_t				ret;
	fplainn::Zui::sIndexMsg		*msg;

	(void)self;
	(void)ret;
	msg = (fplainn::Zui::sIndexMsg *)msgIt;
	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	if (msg->info.action != fplainn::Zui::NDACTION_INSTANTIATE)
	{
		printf(FATAL ORIENT"Failed to instantiate "
			"root device.\n\tError is %s; got as far as %d.\n",
			strerror(msg->header.error), msg->info.action);
	};

	DIE_ON(msg->header.error);

	floodplainn.zum.startDeviceReq(
		CC"by-id", UDI_RESOURCES_NORMAL,
		new __kCallback(&__korientationMain3));
}

void __korientationMain4(MessageStream::sHeader *msgIt);

void __korientationMain3(MessageStream::sHeader *msgIt)
{
	Thread				*self;
	fplainn::Zum::sZumMsg		*msg=reinterpret_cast<
		fplainn::Zum::sZumMsg *>(msgIt);

	(void)self;
	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	fplainn::Device			*dev;

	floodplainn.getDevice(CC"by-id", &dev);

	if (msg->info.params.start.ibind.nFailures > 0)
	{
		printf(FATAL ORIENT"Failed to start Root device 'by-id'.\n"
			"\t%d of %d internal bind channels failed "
			"channel_event_ind(CHANNEL_BOUND).\n"
			"\tBoot halted.\n",
			msg->info.params.start.ibind.nFailures,
			msg->info.params.start.ibind.nChannels);

		return;
	}

	if (msg->info.params.start.pbind.nFailures > 0)
	{
		printf(FATAL ORIENT"Failed to start Root device 'by-id'.\n"
			"\t%d of %d parent bind channels failed "
			"channel_event_ind(PARENT_BOUND).\n"
			"\tBoot halted.\n",
			msg->info.params.start.pbind.nFailures,
			msg->info.params.start.pbind.nChannels);

		return;
	}

	udi_enumerate_cb_t		ecb;

	memset(&ecb, 0, sizeof(ecb));
	ecb.attr_list = NULL;
	ecb.filter_list = NULL;
	ecb.attr_valid_length = ecb.filter_list_length = 0;

	// Enumerate the children of the root device.
	floodplainn.zum.enumerateChildrenReq(
		CC"by-id", &ecb, 0,
		new __kCallback(&__korientationMain4));
}

void __korientationMain4(MessageStream::sHeader *msgIt)
{
	fplainn::Zum::sZumMsg		*msg = (fplainn::Zum::sZumMsg *)msgIt;
	status_t			stat;
	error_t				ret;

	if (msg->info.params.enumerateChildren.final_enumeration_result
		!= UDI_ENUMERATE_DONE)
	{
		printf(FATAL ORIENT"enumerateChildren() failed on root device "
			"'by-id'.\n");
		return;
	}

	if (msg->info.params.enumerateChildren.nDeviceIds < 1)
	{
		printf(WARNING ORIENT"enumerateChildren(): root device "
			"reported that it has no children.\n"
			"\tThis is highly unusual, but boot will proceed.\n");
	}

	printf(NOTICE ORIENT"Root dev reported %d children.\n",
		msg->info.params.enumerateChildren.nDeviceIds);

	for (uarch_t i=0; i<msg->info.params.enumerateChildren.nDeviceIds; i++)
	{
		utf8Char		tmp[FVFS_PATH_MAXLEN],
					tmp2[FVFS_PATH_MAXLEN];
		fvfs::Tag		*tag;
		error_t			err;

		snprintf(tmp, FVFS_PATH_MAXLEN, CC"%s/%d",
			msg->info.path,
			msg->info.params.enumerateChildren.deviceIdsHandle[i]);

		err = vfsTrib.getFvfs()->getPath(tmp, &tag);
		if (err != ERROR_SUCCESS)
		{
			printf(ERROR ORIENT"Failed to get new child dev %d\n",
				msg->info.params.enumerateChildren.deviceIdsHandle[i]);

			continue;
		};

		err = tag->getFullName(tmp2, FVFS_PATH_MAXLEN);
		if (err != ERROR_SUCCESS)
		{
			printf(ERROR ORIENT"Failed to get fullname of new "
				"child with ID %d.\n",
				msg->info.params.enumerateChildren.deviceIdsHandle[i]);

			continue;
		}

		printf(NOTICE ORIENT"Dev: name %s\n", tmp2);
		tag->getInode()->dumpEnumerationAttributes();
	}

	/* Initialize Interrupt Trib IRQ management (__kpin and __kvector),
	 * then load the chipset's bus-pin mappings and initialize timer
	 * services.
	 **/
	DO_OR_DIE(interruptTrib, initializeIrqs(), ret);
	DO_OR_DIE(zkcmCore.irqControl.bpm, loadBusPinMappings(CC"isa"), ret);
	DO_OR_DIE(zkcmCore.timerControl, initialize(), ret);
	DO_OR_DIE(timerTrib, initialize(), ret);

printf(NOTICE ORIENT"IRQs enabled? %d\n", cpuControl::interruptsEnabled());
printf(NOTICE ORIENT"Got here\n");
return;

	// Detect physical memory.
	DO_OR_DIE(zkcmCore.memoryDetection, initialize(), ret);
	DO_OR_DIE(memoryTrib, pmemInit(), ret);
	DO_OR_DIE(memoryTrib, memRegionInit(), ret);

	// Detect and wake all CPUs.
	DO_OR_DIE(cpuTrib, initializeAllCpus(), ret);

	uarch_t tot, succ, fail;
	struct {
		TESTS_FN_MAKE_PROTOTYPE_DEFVARS(runTests)
		{
			return ::runTests(nTotal, nSucceeded, nFailed);
		}
	} testobj;

	DO_OR_DIE(testobj, runTests(&tot, &succ, &fail), stat);
	printf(NOTICE"Tests: %d total, %d succ, %d fail\n", tot, succ, fail);
	printf(NOTICE"All is well in the universe.\n");
}


void __kecrCb(MessageStream::sHeader *msgIt)
{
	fplainn::Zum::sZumMsg		*msg = (fplainn::Zum::sZumMsg *)msgIt;
	error_t err;
	status_t stat;

	printf(NOTICE"Here, devmgmt done. %d new child IDs in buffer.\n",
		msg->info.params.enumerateChildren.nDeviceIds);
//~ __kdebug.refresh();

	fplainn::dma::Constraints			c;
	fplainn::dma::constraints::Compiler		cmp;

	udi_dma_constraints_attr_spec_t a[] =
	{
		{ UDI_DMA_DATA_ADDRESSABLE_BITS, 64 },
		{ UDI_DMA_NO_PARTIAL, 0 },
		{ UDI_DMA_SCGTH_FORMAT, UDI_SCGTH_32 | UDI_SCGTH_64 | UDI_DMA_LITTLE_ENDIAN },
		{ UDI_DMA_SEQUENTIAL, 0 },
		{ UDI_DMA_ELEMENT_GRANULARITY_BITS, 15 },
		{ UDI_DMA_ELEMENT_ALIGNMENT_BITS, 14 },
		{ UDI_DMA_ADDR_FIXED_TYPE, UDI_DMA_FIXED_VALUE },
		{ UDI_DMA_ADDR_FIXED_VALUE_LO, 0x6 },
		{ UDI_DMA_ADDR_FIXED_BITS, 3 },
		{ UDI_DMA_ELEMENT_LENGTH_BITS, 17 },
	};

	cmp.initialize();
	c.initialize(a, 0);

	c.addOrModifyAttrs(a, 3);
	a[1].attr_value = 5;
	a[0].attr_type = UDI_DMA_SCGTH_MAX_EL_PER_SEG;
	c.addOrModifyAttrs(a, 10);
	c.dump();

	err = cmp.compile(&c);
	if (err != ERROR_SUCCESS) {
		printf(ERROR"Compilation failed.\n");
	};
	cmp.dump();

	MemoryBmp				tb(0xB0000000, 0x3F000000);
	fplainn::dma::ScatterGatherList	sgl;

	err = sgl.initialize();
	assert_fatal(err == ERROR_SUCCESS);
	err = tb.initialize();
	assert_fatal(err == ERROR_SUCCESS);
	tb.dump();
	stat = tb.constrainedGetFrames(&cmp, 2, &sgl, 0);
	printf(NOTICE"Ret is %d from constrainedGetFrames.\n", stat);
	stat = tb.constrainedGetFrames(&cmp, 7, &sgl, 0);
	printf(NOTICE"Ret is %d from constrainedGetFrames.\n", stat);
	stat = tb.constrainedGetFrames(&cmp, 1, &sgl, 0);
	printf(NOTICE"Ret is %d from constrainedGetFrames.\n", stat);
	stat = tb.constrainedGetFrames(&cmp, 99, &sgl, 0);
	printf(NOTICE"Ret is %d from constrainedGetFrames.\n", stat);

	sgl.dump();

/*	for (uarch_t i=0; i<msg->info.params.enumerateChildren.nDeviceIds; i++)
	{
		printf(NOTICE"New child: %s/%d.\n",
			msg->info.path,
			msg->info.params.enumerateChildren.deviceIdsHandle[i]);
	};*/
}
