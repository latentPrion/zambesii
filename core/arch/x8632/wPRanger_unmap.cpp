
#include <arch/tlbControl.h>
#include <arch/walkerPageRanger.h>
#include <arch/x8632/wPRanger_getLevelRanges.h>
#include <arch/x8632/wPRanger_accessors.h>
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

status_t walkerPageRanger::unmap(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, paddr_t *paddr,
	uarch_t nPages, uarch_t *flags
	)
{
	// This function takes up a *lot* of room on the stack...
	uarch_t		l0Start, l0Current, l0End;
	uarch_t		l1Start, l1Current, l1Limit, l1End;
	paddr_t		l0Entry, l1Entry;
#ifdef CONFIG_ARCH_x86_32_PAE
	uarch_t		l2Start, l2Current, l2Limit, l2End;
	paddr_t		l2Entry;
#endif
	status_t	ret;

	if (nPages == 0) { return WPRANGER_STATUS_UNMAPPED; };

	// Set the return to default to 'unmapped'.
	ret = WPRANGER_STATUS_UNMAPPED;

	getLevelRanges(
		vaddr, nPages,
		&l0Start, &l0End,
		&l1Start, &l1End
#ifdef CONFIG_ARCH_x86_32_PAE
		,&l2Start, &l2End
#endif
	);

	vaddrSpace->level0Accessor.lock.acquire();
	cpuTrib.getCurrentCpuStream()->currentTask->parent->memoryStream
		->vaddrSpaceStream.vaddrSpace.level0Accessor.lock.acquire();

	l0Current = l0Start;

	for (; l0Current <= l0End; l0Current++)
	{
		l0Entry = vaddrSpace->level0Accessor.rsrc->entries[l0Current];
		// If the table doesn't exist, move on.
		if (l0Entry == 0) {
			continue;
		};

		*level1Modifier = l0Entry;
		tlbControl::flushSingleEntry((void *)level1Accessor);

		l1Current = ((l0Current == l0Start) ? l1Start : 0);
		l1Limit = ((l0Current == l0End)
			? l1End : (PAGING_L1_NENTRIES - 1));

		for (; l1Current <= l1Limit; l1Current++)
		{
#ifdef CONFIG_ARCH_x86_32_PAE
			l1Entry = level1Accessor->entries[l1Current];
			// If the table for the entry doesn't exist, move on.
			if (l1Entry == 0) {
				continue;
			};

			*level2Modifier = l1Entry;
			tlbControl::flushSingleEntry((void *)level2Accessor);

			l2Current = (((l0Current == l0Start)
				&& (l1Current == l1Start)) ? l2Start : 0);

			l2Limit = (((l0Current == l0End)
				&& (l1Current == l1End))
				? l2End : (PAGING_L2_NENTRIES - 1));

			for (; l2Current <= l2Limit; l2Current++)
			{
				/* This function must return the status of the
				 * first page in the range. It will also return
				 * the flags (arch independent) and the
				 * translation for that first page in the range.
				 **/
				l2Entry = level2Accessor->entries[l2Current];

				// Unconditionally zero out the entry.
				level2Accessor->entries[l2Current] = 0;

				/* Only process the code below if this is the
				 * first page in the range to be unmapped.
				 **/
				if (l0Current != l0Start || l1Current != l1Start
					|| l2Current != l2Start)
				{
					continue;
				};

				if (l2Entry == 0) {
					ret = WPRANGER_STATUS_UNMAPPED;
				}
				else
				{
					*flags = walkerPageRanger::decodeFlags(
						l2Entry & 0xFFF);

					*paddr = (l2Entry >> 12);
					*paddr <<= 12;

					if (!__KFLAG_TEST(
						*flags, PAGEATTRIB_PRESENT))
					{
						if (__KFLAG_TEST(
							*paddr,
							PAGING_LEAF_SWAPPED))
						{
							ret = WPRANGER_STATUS_SWAPPED;
						};

						if (__KFLAG_TEST(
							*paddr,
							PAGING_LEAF_FAKEMAPPED))
						{
							ret = WPRANGER_STATUS_FAKEMAPPED;
						};

						if (__KFLAG_TEST(
							*paddr,
							PAGING_LEAF_GUARDPAGE))
						{
							ret = WPRANGER_STATUS_GUARDPAGE;
						};
					}
					else {
						ret = WPRANGER_STATUS_BACKED;
					};
				};
			};
#else
			l1Entry = level1Accessor->entries[l1Current];
			level1Accessor->entries[l1Current] = 0;

			if (l0Current != l0Start || l1Current != l1Start) {
				continue;
			};

			if (l1Entry == 0) {
				ret = WPRANGER_STATUS_UNMAPPED;
			}
			else
			{
				*flags = walkerPageRanger::decodeFlags(
					l1Entry & 0xFFF);

				*paddr = (l1Entry >> 12);
				*paddr <<= 12;
				
				if (!__KFLAG_TEST(*flags, PAGEATTRIB_PRESENT))
				{
					if (__KFLAG_TEST(
						*paddr, PAGING_LEAF_SWAPPED))
					{
						ret = WPRANGER_STATUS_SWAPPED;
					};
					if (__KFLAG_TEST(
						*paddr, PAGING_LEAF_FAKEMAPPED))
					{
						ret = WPRANGER_STATUS_FAKEMAPPED;
					};
					if (__KFLAG_TEST(
						*paddr, PAGING_LEAF_GUARDPAGE))
					{
						ret = WPRANGER_STATUS_GUARDPAGE;
					};
				}
				else {
					ret = WPRANGER_STATUS_BACKED;
				};
			};
#endif
		};
	};

	cpuTrib.getCurrentCpuStream()->currentTask->parent->memoryStream
		->vaddrSpaceStream.vaddrSpace.level0Accessor.lock.release();

	vaddrSpace->level0Accessor.lock.release();

	// Flush the entire range.
	tlbControl::flushEntryRange(vaddr, nPages);

	return ret;
}

