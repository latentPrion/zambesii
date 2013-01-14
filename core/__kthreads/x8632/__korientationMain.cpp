
#include <__ksymbols.h>
#include <arch/memory.h>
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
#include <kernel/common/execTrib/execTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>

#include <arch/cpuControl.h>
#include "../chipset/ibmPc/i8254.h"
#include <__kclasses/prioQueue.h>

int oo=0, pp=0, qq=0, rr=0;

int ghfoo(void)
{
	__kprintf(NOTICE"This is a thread.\n");
	return 0;
}


extern "C" void __korientationMain(ubit32, multibootDataS *)
{
	error_t		ret;
	uarch_t		devMask;

	__koptimizationHacks();

	// Prepare the kernel by zeroing .BSS and calling constructors.
	memset(&__kbssStart, 0, &__kbssEnd - &__kbssStart);

	// processTrib initializes __kprocess & __korientation.
	DO_OR_DIE(processTrib, initialize(), ret);
	DO_OR_DIE(cpuTrib, initialize(), ret);

	cxxrtl::callGlobalConstructors();
	// Initialize the chipset's module package.
	DO_OR_DIE(zkcmCore, initialize(), ret);
	DO_OR_DIE(interruptTrib, initialize(), ret);

	// Initialize the kernel swamp.
	DO_OR_DIE(
		memoryTrib.__kmemoryStream,
		initialize(
			reinterpret_cast<void *>(
				ARCH_MEMORY___KLOAD_VADDR_BASE + 0x400000 ),
			0x3FB00000, __KNULL),
		ret);

	DO_OR_DIE(memoryTrib, __kspaceInit(), ret);
	DO_OR_DIE(__kdebug, initialize(), ret);

	devMask = __kdebug.tieTo(DEBUGPIPE_DEVICE_BUFFER | DEBUGPIPE_DEVICE1);
	if (!__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE_BUFFER)) {
		__kprintf(WARNING ORIENT"No debug buffer allocated.\n");
	};

	__kdebug.refresh();
	__kprintf(NOTICE ORIENT"Kernel debug output tied to devices BUFFER and "
		"DEVICE1.\n");

	// Initialize the kernel Memory Reservoir (heap) and object cache pool.
	DO_OR_DIE(memReservoir, initialize(), ret);
	DO_OR_DIE(cachePool, initialize(), ret);

	// Enable the Task Stream on the BSP CPU for boot time scheduling.
	DO_OR_DIE(cpuTrib, initializeBspTaskStream(), ret);
	//cpuTrib.getCurrentCpuStream()->taskStream.dump();
for (__kprintf(NOTICE ORIENT"Reached HLT.\n");;) { asm volatile("hlt\n\t"); };
	// Initialize IRQ Control and chipset bus-pin mapping management.
	DO_OR_DIE(interruptTrib, initialize2(), ret);
	DO_OR_DIE(zkcmCore.irqControl.bpm, loadBusPinMappings(CC"isa"), ret);
	// Start the Timer Trib timer services.
	DO_OR_DIE(zkcmCore.timerControl, initialize(), ret);
	DO_OR_DIE(timerTrib, initialize(), ret);

	// Detect physical memory.
	DO_OR_DIE(memoryTrib, pmemInit(), ret);

	DO_OR_DIE(processTrib, initialize2(), ret);
	DO_OR_DIE(cpuTrib, initialize2(), ret);

	DO_OR_DIE(execTrib, initialize(), ret);
	DO_OR_DIE(vfsTrib, initialize(), ret);

	ubit8		t;
	void		*r;
	status_t	st;
	processStreamC	*newProc;

	vfsTrib.createTree(CC":ekfs");
	vfsTrib.setDefaultTree(CC":ekfs");
	vfsTrib.createTree(CC":sapphire");
	vfsTrib.createTree(CC":ftp");
	__kprintf(NOTICE"Created trees :ekfs, :sapphire, :ftp, :ekfs is the "
		"default tree.\n");

	st = vfsTrib.getPath(CC":ekfs", &t, &r);
	__kprintf(NOTICE"GetPath on :ekfs: %d.\n", st);

	__kdebug.refresh();
	__kprintf(NOTICE ORIENT"Result of createFolder: %d.\n",
		vfsTrib.createFolder(static_cast<vfsDirC *>( r ), CC"zambezii"));

	__kprintf(NOTICE ORIENT"Result of createFile: %d.\n",
		vfsTrib.createFile(static_cast<vfsDirC *>( r ), CC"file1"));

	__kprintf(NOTICE ORIENT"result of getPath on :ekfs/zambezii: %d.\n",
		vfsTrib.getPath(CC":ekfs/zambezii", &t, &r));

	__kprintf(NOTICE ORIENT"result of getPath on :ekfs/file1: %d.\n",
		vfsTrib.getPath(CC":ekfs/file1", &t, &r));

	vfsTrib.getDefaultTree()->desc->dumpSubDirs();
	vfsTrib.getDefaultTree()->desc->dumpFiles();

	__kprintf(NOTICE"About to test process spawning.\n");

	for (uarch_t i=0; i<32; i++)
	{
		newProc = processTrib.spawn(
			(const utf8Char *)":ekfs/file1",
			__KNULL,
			__KNULL,
			PROCESS_EXECDOMAIN_KERNEL,
			0,
			taskC::ROUND_ROBIN,
			SCHEDPRIO_DEFAULT,
			0,
			&ret);

		__kprintf(NOTICE"Results: new process at 0x%p, ret from call: %d, procId: 0x%x, absName: %s.\n",
			newProc, ret, newProc->id, newProc->commandLine);
	};

	__kdebug.refresh();

	__kprintf(NOTICE ORIENT"Successful!\n");
}

