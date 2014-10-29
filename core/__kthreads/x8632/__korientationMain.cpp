
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
static __kcbFn				__korientationMain3;

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
			if (callback == NULL)
			{
				printf(WARNING ORIENT"Unknown message with "
					"no callback.\n"
					"\tSrcTid 0x%x, subsys %d, func %d.\n",
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
// 	DO_OR_DIE(floodplainn, enumerateBaseDevices(), ret);
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
	msg = (fplainn::Zui::sIndexMsg *)msgIt;
	self = static_cast<Thread *>( cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread() );

	if (msg->info.action != fplainn::Zui::NDACTION_INSTANTIATE)
	{
		printf(FATAL ORIENT"Failed to instantiate "
			"root device.\n\tError is %s; got as far as %d.\n",
			strerror(msg->header.error), msg->info.action);
	};

	DIE_ON(msg->header.error);

	floodplainn.zum.startDeviceReq(
		CC"by-id", UDI_RESOURCES_NORMAL,
//		NULL);
		new __kCallback(&__korientationMain3));

	printf(NOTICE ORIENT"About to dormant.\n");
}

#define UDI_PHYSIO_VERSION	0x101
#include <udi_physio.h>
#include <kernel/common/floodplainn/initInfo.h>
#include <kernel/common/floodplainn/movableMemoryHeader.h>
static udi_layout_t	sTLayout[] =
{
	UDI_DL_UBIT8_T, UDI_DL_UBIT8_T, UDI_DL_BUF,
		0, 0, 0,
	UDI_DL_UBIT16_T, UDI_DL_INLINE_UNTYPED,
	UDI_DL_END
};

struct sT
{

	void dump(void)
	{
		printf(NOTICE"m0 %d, m1 %d, m3 0x%p, m4 %d, m5 0x%p.\n",
			m0, m1, m2, m3, m4);
	}

	ubit8		m0;
	ubit8		m1;
	udi_buf_t	*m2;
	udi_ubit16_t	m3;
	void		*m4;
};

static udi_layout_t		trLayout[] =
{
	UDI_DL_UBIT32_T, UDI_DL_UBIT32_T,
	UDI_DL_END
};

ubit8		mem0[512];
ubit8		mem1[128];

struct sMMa
{
	sMMa()
	:
	h(sizeof(a))
	{}

	fplainn::sMovableMemory		h;
	udi_instance_attr_list_t	a;
};

struct sMMf
{
	sMMf()
	:
	h(sizeof(f))
	{}

	fplainn::sMovableMemory		h;
	udi_filter_element_t		f;
};

status_t func0(void *mem, udi_layout_t *lay, ...);

void __korientationMain3(MessageStream::sHeader *msgIt)
{
	Thread				*self;
	error_t				ret;

	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	const sMetaInitEntry		*mie;
	udi_mei_op_template_t		*opt;
	status_t			sz0, sz1, sz2, sz3, sz4, sz5, sz6;
	udi_enumerate_cb_t		uec, dst;

	mie = floodplainn.zudi.findMetaInitInfo(CC"udi_mgmt");
	fplainn::MetaInit		mp(mie->udi_meta_info);

	opt = mp.getOpTemplate(UDI_MGMT_OPS_NUM, 2);

printf(NOTICE"sizeof attr_list_t %d, filter_elem_t %d.\n",
	sizeof(udi_instance_attr_list_t),
	sizeof(udi_filter_element_t));

	uec.attr_list = &(new sMMa)->a;
	memset(uec.attr_list, 0x2B, sizeof(*uec.attr_list));
	uec.filter_list = &(new sMMf)->f;
	memset((void *)uec.filter_list, 0xB2, sizeof(*uec.filter_list));
	sz0 = fplainn::sChannelMsg::getTotalInlineLayoutSize(
		opt->visible_layout, NULL, &uec.gcb);

	sz1 = fplainn::sChannelMsg::marshalInlineObjects(
		mem0, &dst.gcb, &uec.gcb, opt->visible_layout, NULL);

	printf(NOTICE"total inline size %d.\n", sz0);
	printf(NOTICE"mem0[0] 0x%x, mem0[97] 0x%x\n", mem0[0], mem0[97]);
	printf(NOTICE"mem0[98] 0x%x, mem0[98+167] 0x%x\n", mem0[98], mem0[98+167]);
	printf(NOTICE"destcb pointers: %d, %d\n",
		(void *)dst.attr_list == mem0,
		(void *)dst.filter_list == mem0+98);
	return;

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

	printf(NOTICE ORIENT"Halting unfavourably.\n");
	for (;FOREVER;) { asm volatile("hlt\n\t"); };
}

status_t func0(void *mem, udi_layout_t *lay, ...)
{
	va_list		args;

	va_start(args, lay);
	return fplainn::sChannelMsg::marshalStackArguments((ubit8 *)mem, args, lay);
	va_end(args);
}
