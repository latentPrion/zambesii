
#include <arch/paddr_t.h>
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include "exceptions.h"


static void *getCr2(void)
{
	void *ret;
	asm volatile (
		"movl	%%cr2, %0\n\t"
		: "=r" (ret));

	return ret;
}

status_t x8632_page_fault(taskContextC *regs, ubit8)
{
	status_t		status;
	vaddrSpaceStreamC	*vaddrSpaceStream;
#ifndef CONFIG_ARCH_x86_32_PAE
	vaddrSpaceStreamC	*__kvaddrSpaceStream;
#endif
	void			*faultAddr = getCr2();
	paddr_t			pmap;
	uarch_t			__kflags;

	vaddrSpaceStream = cpuTrib.getCurrentCpuStream()
		->taskStream.currentTask->parent->getVaddrSpaceStream();

#ifndef CONFIG_ARCH_x86_32_PAE
	/* First check to see if the page fault is caused by a kernel addrspace
	 * top-level change that hasn't yet been propagated to this process:
	 *
	 * (For PAE x86-32, this will never need to be checked, since the
	 * propagation issue does not exist with PAE.)
	 **/
	__kvaddrSpaceStream = processTrib.__kgetStream()->getVaddrSpaceStream();
	if (vaddrSpaceStream != __kvaddrSpaceStream)
	{
		const uarch_t		topLevelEntry=
			((uarch_t)faultAddr & PAGING_L0_VADDR_MASK)
				>> PAGING_L0_VADDR_SHIFT;

		vaddrSpaceStream->vaddrSpace.level0Accessor.lock.acquire();
		__kvaddrSpaceStream->vaddrSpace.level0Accessor.lock.acquire();

		if (__kvaddrSpaceStream->vaddrSpace.level0Accessor.rsrc
			->entries[topLevelEntry]
			!= vaddrSpaceStream->vaddrSpace.level0Accessor.rsrc
				->entries[topLevelEntry])
		{
			vaddrSpaceStream->vaddrSpace.level0Accessor.rsrc
				->entries[topLevelEntry] = __kvaddrSpaceStream
					->vaddrSpace.level0Accessor.rsrc
					->entries[topLevelEntry];

			__kvaddrSpaceStream->vaddrSpace.level0Accessor.lock
				.release();

			vaddrSpaceStream->vaddrSpace.level0Accessor.lock
				.release();

			return ERROR_SUCCESS;
		};

		__kvaddrSpaceStream->vaddrSpace.level0Accessor.lock.release();
		vaddrSpaceStream->vaddrSpace.level0Accessor.lock.release();
	};
#endif

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

