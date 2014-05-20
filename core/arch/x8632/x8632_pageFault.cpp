
#include <arch/paddr_t.h>
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <arch/tlbControl.h>
#include <arch/debug.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include "exceptions.h"


static inline void *getCr2(void)
{
	void *ret;
	asm volatile (
		"movl	%%cr2, %0\n\t"
		: "=r" (ret));

	return ret;
}

static sarch_t __kpropagateTopLevelVaddrSpaceChanges(void *faultAddr)
{
#ifndef CONFIG_ARCH_x86_32_PAE
	vaddrSpaceC		*__kvaddrSpace, *vaddrSpace;

	/* This function returns 1 if there was a need to propagate top-level
	 * changes, and 0 if not.
	 *
	 * First check to see if the page fault is caused by a kernel addrspace
	 * top-level change that hasn't yet been propagated to this process:
	 *
	 * (For PAE x86-32, this will never need to be checked, since the
	 * propagation issue does not exist with PAE.)
	 **/
	__kvaddrSpace = &processTrib.__kgetStream()->getVaddrSpaceStream()
		->vaddrSpace;

	vaddrSpace = &cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask()
		->parent->getVaddrSpaceStream()->vaddrSpace;

	if (vaddrSpace != __kvaddrSpace)
	{
		const uarch_t		topLevelEntry=
			((uintptr_t)faultAddr & PAGING_L0_VADDR_MASK)
				>> PAGING_L0_VADDR_SHIFT;

		vaddrSpace->level0Accessor.lock.acquire();
		__kvaddrSpace->level0Accessor.lock.acquire();

		if (__kvaddrSpace->level0Accessor.rsrc->entries[topLevelEntry]
			!= vaddrSpace->level0Accessor.rsrc
				->entries[topLevelEntry])
		{
			vaddrSpace->level0Accessor.rsrc->entries[topLevelEntry]
			= __kvaddrSpace->level0Accessor.rsrc
				->entries[topLevelEntry];

			__kvaddrSpace->level0Accessor.lock.release();
			vaddrSpace->level0Accessor.lock.release();

			tlbControl::flushSingleEntry(faultAddr);
			faultAddr = (void *)((uintptr_t)faultAddr
				>> PAGING_L0_VADDR_SHIFT);

			faultAddr = (void *)((uintptr_t)faultAddr
				<< PAGING_L0_VADDR_SHIFT);

			// Never assume the layout or behaviour of TLB caching.
			tlbControl::flushSingleEntry(faultAddr);

			return 1;
		};

		__kvaddrSpace->level0Accessor.lock.release();
		vaddrSpace->level0Accessor.lock.release();
	};
#endif

	// On PAE this will always return 0.
	return 0;
}

status_t x8632_page_fault(registerContextC *regs, ubit8)
{
	status_t		status;
	vaddrSpaceStreamC	*vaddrSpaceStream;
	void			*faultAddr = getCr2();
	paddr_t			pmap;
	uarch_t			__kflags;
	taskC			*currTask;
	threadC			*currThread=NULL;
	sbit8			panicWorthy=0, traceStack=0, printArgs=0;

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();
	if (currTask->getType() != task::PER_CPU)
		{ currThread = (threadC *)currTask; };

	vaddrSpaceStream = currTask->parent->getVaddrSpaceStream();

	if (faultAddr >= (void *)ARCH_MEMORY___KLOAD_VADDR_BASE)
		{ __kpropagateTopLevelVaddrSpaceChanges(faultAddr); };

	status = walkerPageRanger::lookup(
		&vaddrSpaceStream->vaddrSpace,
		faultAddr, &pmap, &__kflags);

	switch (status)
	{
	case WPRANGER_STATUS_FAKEMAPPED_DYNAMIC:
		uarch_t nTries;

		for (nTries=0, status = 0;
			nTries < 4 && status < 1;
			nTries++)
		{
			status = memoryTrib.fragmentedGetFrames(1, &pmap);
		};

		if (status < 1) { panic(FATAL"Out of pmem.\n");	};

		// Map new frame to fakemapped page.
		walkerPageRanger::remapInc(
			&vaddrSpaceStream->vaddrSpace,
			faultAddr, pmap, status, WPRANGER_OP_SET_PRESENT, 0);

		printf(NOTICE OPTS(NOLOG)
			"Page Fault: FAKE_DYN: addr 0x%p, "
			"EIP 0x%p\n\tWPRl map: stat %d, pmap 0x%P, "
			"__kf 0x%x.\n",
			faultAddr, regs->eip,
			walkerPageRanger::lookup(
				&vaddrSpaceStream->vaddrSpace,
				faultAddr, &pmap, &__kflags),
			pmap, __kflags);

		break;

	case WPRANGER_STATUS_HEAP_GUARDPAGE:
		panicWorthy = 1;
		traceStack = 1;

		printf(FATAL"Encountered a heap guardpage; heap corrupted\n"
			"\tVaddr: 0x%p, EIP 0x%p\n"
			"\tFault was caused by a %s, and %s an instruction "
			"fetch\n",
			faultAddr, regs->eip,
			(regs->errorCode & 0x2) ? "write" : "read",
			(regs->errorCode & 0x10) ? "was" : "was not");

		break;

	case WPRANGER_STATUS_UNMAPPED:
		uarch_t		esp;

		panicWorthy = 1;
		traceStack = 1;

		asm volatile(
			"movl %%esp, %0\n\t"
			: "=r" (esp));

		printf(FATAL"Encountered unmapped page at 0x%p, EIP: 0x%x, "
			"esp: 0x%x, schedstack end: 0x%p schedstack base 0x%p."
			"\n", faultAddr,
			regs->eip, esp,
			cpuTrib.getCurrentCpuStream()->schedStack,
			&cpuTrib.getCurrentCpuStream()->schedStack[
				sizeof(cpuTrib.getCurrentCpuStream()->schedStack)]);

		break;

	case WPRANGER_STATUS_BACKED:
		// Either COW, or simple access perms violation.
		panicWorthy = 1;
		//traceStack = 1;
		printf(FATAL"Kernel faulted on a backed page. Probably access "
			"perms violation, or unintentional COW\n"
			"Vaddr: 0x%p. EIP 0x%p.\n",
			faultAddr, regs->eip);

		break;

	case WPRANGER_STATUS_FAKEMAPPED_STATIC:
	case WPRANGER_STATUS_GUARDPAGE:
		panicWorthy = 1;
		traceStack = 1;
		break;

	case WPRANGER_STATUS_SWAPPED:
		panicWorthy = 1;
		traceStack = 1;
		// Not implemented.
		printf(FATAL"Encountered swapped page. at %X.\n", faultAddr);
		break;

	default:
		printf(FATAL"Encountered page with unknown status at %X.\n",
			faultAddr);

		panic(ERROR_UNKNOWN);
		break;
	};

	if (traceStack)
	{
		debug::stackDescriptorS		currStackDesc;

		debug::getCurrentStackInfo(&currStackDesc);
		if (currThread != NULL)
			{ printf(NOTICE"This is a normal thread.\n"); }
		else
			{ printf(NOTICE"This is a per-cpu thread.\n"); };

		debug::printStackTrace(
			(void *)regs->ebp, &currStackDesc);
	};

	if (printArgs)
	{
		// Doesn't work properly.
		debug::printStackArguments(
			(void *)regs->ebp, (void *)regs->dummyEsp);
	};

	if (panicWorthy) { panic(ERROR_FATAL); };
	return ERROR_SUCCESS;
}

