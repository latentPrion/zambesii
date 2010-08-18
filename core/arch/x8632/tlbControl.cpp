
#include <arch/tlbControl.h>
#include <arch/x8632/paging.h>

void tlbControl::flushSingleEntry(void *vaddr)
{
	asm volatile (
		"invlpg %0 \n\t"
		:
		: "m" (vaddr)
		: "memory"
	);
}

void tlbControl::flushEntryRange(void *vaddr, uarch_t nPages)
{

	for (; nPages > 0;
		vaddr = reinterpret_cast<void *>(
			reinterpret_cast<uarch_t>( vaddr ) + PAGING_BASE_SIZE ),
			nPages--)
	{
		asm volatile (
			"invlpg (%0) \n\t"
			: "=r" (reinterpret_cast<uarch_t>( vaddr ))
		);
	};
}

