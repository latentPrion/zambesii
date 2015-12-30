
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

#include <commonlibs/libzbzcore/libzudi.h>
#include <commonlibs/libzbzcore/libzbzcore.h>
#include <kernel/common/floodplainn/initInfo.h>
#include <kernel/common/floodplainn/movableMemory.h>
template <class T, int N>
struct sMovableMemObject
{
	sMovableMemObject(void) : hdr(sizeof(T) * N) {}

	fplainn::sMovableMemory		hdr;
	T				a[N];
};
void __korientationMain3(MessageStream::sHeader *msgIt)
{
	Thread				*self;
	error_t				ret;

	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	fplainn::Device			*dev;

	floodplainn.getDevice(CC"by-id", &dev);
/*	struct sGxcb
	{
		sGxcb(void)
		{
			memset(&cb, 0, sizeof(cb));
			cb.tr_params = &tr_params;
		}

		lzudi::sControlBlock	cbHdr;
		udi_gio_xfer_cb_t	cb;
		struct _{
			_(void) { a[0] = a[1] = 0xDEaDBEEF; }
			ubit32		a[2];
		} tr_params;
	} cb;

extern udi_mei_init_t			udi_gio_meta_info;
fplainn::MetaInit			mParser(&udi_gio_meta_info);
udi_layout_t				tmpLay[] =
	{ UDI_DL_UBIT32_T, UDI_DL_UBIT32_T, UDI_DL_END };
//	{ UDI_DL_END };

	udi_layout_t		*l[3] = {
		mParser.getOpTemplate(1, 3)->visible_layout,
		mParser.getOpTemplate(1, 3)->marshal_layout,
		//__klzbzcore::region::channel::mgmt::layouts::channel_event_ind,
		tmpLay
	};

	error_t send(
		fplainn::Endpoint *endp,
		udi_cb_t *gcb, udi_layout_t *layouts[3],
		utf8Char *metaName, udi_index_t meta_ops_num, udi_index_t op_idx,
		void *privateData,
		...);

	self->parent->floodplainnStream.send(
		dev->instance->mgmtEndpoint, &cb.cb.gcb,
		l, CC"udi_gio",
		UDI_GIO_PROVIDER_OPS_NUM, 3, NULL,
		0xFF, 0xCaFEBaBE);*/

	struct sGecb {
		sGecb(void) {
			memset(&cb, 0, sizeof(cb));
		}

		lzudi::sControlBlock			hdr;
		udi_enumerate_cb_t		cb;
	} ecb;

	sMovableMemObject<udi_instance_attr_list_t, 2>	ta;
	sMovableMemObject<udi_filter_element_t, 2>	tf;

	{
		strcpy8(CC ta.a[0].attr_name, CC"attr0");
		strcpy8(CC ta.a[0].attr_value, CC"attr0-value");
		ta.a[0].attr_type = 1;
		strcpy8(CC ta.a[1].attr_name, CC"attr1");
		strcpy8(CC ta.a[1].attr_value, CC"attr1-value");
		ta.a[1].attr_type = 1;
	};
	{
		strcpy8(CC tf.a[0].attr_name, CC"filter0");
		strcpy8(CC tf.a[1].attr_name, CC"filter1");
	};
	{
		ecb.cb.attr_valid_length = 1;
		ecb.cb.filter_list_length = 2;
		ecb.cb.attr_list = ta.a;
		ecb.cb.filter_list = tf.a;
	};

	__kcbFn		__kotp;
	floodplainn.zum.enumerateReq(
		CC"by-id", UDI_ENUMERATE_NEXT, &ecb.cb,
		new __kCallback(&__kotp));

printf(NOTICE ORIENT"orient 3.\n");

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

void __kecrCb(MessageStream::sHeader *msgIt);

void __kotp(MessageStream::sHeader *msgIt)
{
	fplainn::Zum::sZumMsg		*msg = (fplainn::Zum::sZumMsg *)msgIt;
	udi_enumerate_cb_t		*ecb = &msg->info.params.enumerate.cb;
	udi_instance_attr_list_t	a[2];
	sMovableMemObject<udi_filter_element_t, 2>	tf;

printf(NOTICE"here, finished enum req.\n");
	printf(NOTICE ORIENT"msg 0x%p: %d attr @0x%p, %d filt @0x%p.\n",
		msg,
		ecb->attr_valid_length,
		ecb->attr_list,
		ecb->filter_list_length,
		ecb->filter_list);

	floodplainn.zum.getEnumerateReqAttrsAndFilters(ecb, a, tf.a);

	{
		ecb->filter_list_length = 2;
		ecb->filter_list = tf.a;
	};

	ecb->attr_valid_length = 0;
	ecb->attr_list = NULL;
printf(NOTICE ORIENT"About to call devmgmt.\n");
	floodplainn.zum.enumerateChildrenReq(
		CC"by-id", ecb, 0,
		new __kCallback(&__kecrCb));
/*	floodplainn.zum.finalCleanupReq(
		CC"by-id",
		new __kCallback(&__kecrCb));*/
}

void __kecrCb(MessageStream::sHeader *msgIt)
{
	fplainn::Zum::sZumMsg		*msg = (fplainn::Zum::sZumMsg *)msgIt;

	printf(NOTICE"Here, devmgmt done. %d new child IDs in buffer.\n",
		msg->info.params.enumerateChildren.nDeviceIds);

__kdebug.refresh();

	fplainn::Zudi::dma::DmaConstraints			c;
	udi_dma_constraints_attr_spec_t	a[4] =
	{
		{ UDI_DMA_ADDRESSABLE_BITS, 16 },
		{ UDI_DMA_NO_PARTIAL, 1 },
		{ UDI_DMA_SCGTH_FORMAT, UDI_SCGTH_32 | UDI_DMA_LITTLE_ENDIAN },
		{ UDI_DMA_SEQUENTIAL, 1 }
	};

	c.initialize();

	c.addOrModifyAttrs(a, 3);
	a[1].attr_value = 5;
	a[0].attr_type = UDI_DMA_SCGTH_MAX_EL_PER_SEG;
	c.addOrModifyAttrs(a, 4);
	c.dump();

/*	for (uarch_t i=0; i<msg->info.params.enumerateChildren.nDeviceIds; i++)
	{
		printf(NOTICE"New child: %s/%d.\n",
			msg->info.path,
			msg->info.params.enumerateChildren.deviceIdsHandle[i]);
	};*/
}
