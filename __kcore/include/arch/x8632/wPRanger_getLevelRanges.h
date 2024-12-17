#ifndef _WALKER_PAGE_RANGER_GET_LEVEL_RANGES_H
	#define _WALKER_PAGE_RANGER_GET_LEVEL_RANGES_H

/**	EXPLANATION:
 * This function is used to quickly set the ranges for each level of the paging
 * structure walk. Pass it the start virtual address and the number of pages to
 * be mapped, and it will calculate the and address, and the number of entries
 * in each table which must be walked.
 **/

void getLevelRanges(
	void *_vaddr, uarch_t nPages,
	uarch_t *l0Start, uarch_t *l0End,
	uarch_t *l1Start, uarch_t *l1End
#ifdef CONFIG_ARCH_x86_32_PAE
	,uarch_t *l2Start, uarch_t *l2End
#endif
	);

#endif

