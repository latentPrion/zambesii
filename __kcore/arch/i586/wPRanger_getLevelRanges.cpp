
#include <arch/paging.h>
#include <arch/x8632/wPRanger_getLevelRanges.h>
#include <__kstdlib/__ktypes.h>

void getLevelRanges(
	void *_vaddr, uarch_t nPages,
	uarch_t *l0Start, uarch_t *l0End,
	uarch_t *l1Start, uarch_t *l1End
#ifdef CONFIG_ARCH_x86_32_PAE
	,uarch_t *l2Start, uarch_t *l2End
#endif
	)
{
	uarch_t		vaddr, endAddr;

	vaddr = reinterpret_cast<uarch_t>( _vaddr );
	endAddr = vaddr + ((nPages - 1) * PAGING_BASE_SIZE);

	*l0Start = (vaddr & PAGING_L0_VADDR_MASK) >> PAGING_L0_VADDR_SHIFT;
	*l0End = (endAddr & PAGING_L0_VADDR_MASK) >> PAGING_L0_VADDR_SHIFT;

	*l1Start = (vaddr & PAGING_L1_VADDR_MASK) >> PAGING_L1_VADDR_SHIFT;
	*l1End = (endAddr & PAGING_L1_VADDR_MASK) >> PAGING_L1_VADDR_SHIFT;

#ifdef CONFIG_ARCH_x86_32_PAE
	*l2Start = (vaddr & PAGING_L2_VADDR_MASK) >> PAGING_L2_VADDR_SHIFT;
	*l2End = (endAddr & PAGING_L2_VADDR_MASK) >> PAGING_L2_VADDR_SHIFT;
#endif
}
