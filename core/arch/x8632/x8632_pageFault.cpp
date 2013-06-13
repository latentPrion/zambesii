
#include <arch/paddr_t.h>
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <arch/tlbControl.h>
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

static sarch_t __kpropagateTopLevelVaddrSpaceChanges(void)
{	
#ifndef CONFIG_ARCH_x86_32_PAE
	vaddrSpaceC		*__kvaddrSpace, *vaddrSpace;
	void			*faultAddr = getCr2();

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

	vaddrSpaceStream = cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentTask()->parent->getVaddrSpaceStream();

	if (faultAddr >= (void *)ARCH_MEMORY___KLOAD_VADDR_BASE)
		{ __kpropagateTopLevelVaddrSpaceChanges(); };

	status = walkerPageRanger::lookup(
		&vaddrSpaceStream->vaddrSpace,
		faultAddr, &pmap, &__kflags);

	switch (status)
	{
	case WPRANGER_STATUS_BACKED:
		// Either COW, or simple access perms violation.
		break;

	case WPRANGER_STATUS_FAKEMAPPED_DYNAMIC:
		for (status = 0; status < 1; ) {
			status = memoryTrib.fragmentedGetFrames(1, &pmap);
		};

		// Map new frame to fakemapped page.
		walkerPageRanger::remapInc(
			&vaddrSpaceStream->vaddrSpace,
			faultAddr, pmap, status, WPRANGER_OP_SET_PRESENT, 0);

		__kprintf(NOTICE NOLOG"Page Fault: FAKE_DYN: addr 0x%p, "
			"EIP 0x%p\n\tWPRl map: stat %d, pmap 0x%P, "
			"__kf 0x%x.\n",
			faultAddr, regs->eip,
			walkerPageRanger::lookup(
				&vaddrSpaceStream->vaddrSpace,
				faultAddr, &pmap, &__kflags),
			pmap, __kflags);

		break;

	case WPRANGER_STATUS_FAKEMAPPED_STATIC:
		break;

	case WPRANGER_STATUS_GUARDPAGE:
		break;

	case WPRANGER_STATUS_SWAPPED:
		// Not implemented.
		__kprintf(FATAL"Encountered swapped page. at %X.\n", faultAddr);
		panic(ERROR_GENERAL);
		break;

	case WPRANGER_STATUS_UNMAPPED:
		uarch_t esp;
		asm volatile(
			"movl %%esp, %0\n\t"
			: "=r" (esp));

		__kprintf(FATAL"Encountered unmapped page at 0x%p, EIP: 0x%x, "
			"esp: 0x%x, sleepstack end: 0x%p sleepstack base 0x%p."
			"\n", faultAddr,
			regs->eip, esp, cpuTrib.getCurrentCpuStream()->sleepStack,
			&cpuTrib.getCurrentCpuStream()->sleepStack[
				sizeof(cpuTrib.getCurrentCpuStream()->sleepStack)]);

		panic(ERROR_UNKNOWN);
		break;
	default:
		__kprintf(FATAL"Encountered page with unknown status at %X.\n",
			faultAddr);

		panic(ERROR_UNKNOWN);
		break;
	};

	return ERROR_SUCCESS;
}

