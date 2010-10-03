
#include <arch/paddr_t.h>
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>
#include "exceptions.h"


static void *getCr2(void)
{
	void *ret;
	asm volatile (
		"movl	%%cr2, %0\n\t"
		: "=r" (ret));

	return ret;
}

status_t x8632_page_fault(taskContextS *regs)
{
	status_t	status;
	memoryStreamC	*memoryStream;
	void		*faultAddr = getCr2();
	paddr_t		pmap;
	uarch_t		__kflags;

	memoryStream = cpuTrib.getCurrentCpuStream()->currentTask->parent
		->memoryStream;

	status = walkerPageRanger::lookup(
		&memoryStream->vaddrSpaceStream.vaddrSpace,
		faultAddr, &pmap, &__kflags);

	switch (status)
	{
	case WPRANGER_STATUS_BACKED:
		// Either COW, or simple access perms violation.
		break;

	case WPRANGER_STATUS_FAKEMAPPED_DYNAMIC:
		for (status = 0; status < 1; ) {
			status = numaTrib.fragmentedGetFrames(1, &pmap);
		};

		// Map new frame to fakemapped page.
		walkerPageRanger::remapInc(
			&memoryStream->vaddrSpaceStream.vaddrSpace,
			faultAddr, pmap, status, WPRANGER_OP_SET_PRESENT, 0);

		__kprintf(NOTICE NOLOG"Page Fault: FAKE_DYN: addr 0x%p, "
			"EIP 0x%p\n\tWPRl map: stat %d, pmap 0x%P, "
			"__kf 0x%x.\n",
			faultAddr, regs->eip,
			walkerPageRanger::lookup(
				&memoryStream->vaddrSpaceStream.vaddrSpace,
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
		__kprintf(FATAL"Encountered unmapped page at %X.\n", faultAddr);
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

