#include <arch/x8632/paging.h>
#include <arch/tlbControl.h>

void tlbControl::flushEntryRange(void *vaddr, uarch_t nPages)
{
	uarch_t		counter, endAddr;

	counter = reinterpret_cast<uarch_t>( vaddr );
	endAddr = counter + (nPages * PAGING_BASE_SIZE);
	for (; counter < endAddr; counter += PAGING_BASE_SIZE) {
		flushSingleEntry(reinterpret_cast<void *>( counter ));
	};
}

