
#include <debug.h>
#include <arch/paging.h>
#include <arch/tlbControl.h>
#include <arch/walkerPageRanger.h>
#include <arch/x8632/wPRanger_accessors.h>
#include <arch/x8632/wPRanger_getLevelRanges.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/process.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

status_t walkerPageRanger::mapNoInc(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, paddr_t paddr,
	uarch_t nPages, uarch_t __kflags
	)
{
	// This function takes up a *lot* of room on the stack...
	uarch_t		l0Start, l0Current, l0End;
	uarch_t		l1Start, l1Current, l1Limit, l1End;
	paddr_t		l0Entry, paddrTmp;
#ifdef CONFIG_ARCH_x86_32_PAE
	uarch_t		l2Start, l2Current, l2Limit, l2End;
	paddr_t		l1Entry, l2Entry;
#endif
	uarch_t		archFlags;
	status_t	ret=0;
	int		ztmp;

	if (nPages == 0) { return ret; };

	// Convert the generic kernel flags into arch specific flags.
	archFlags = walkerPageRanger::encodeFlags(__kflags);
	getLevelRanges(
		vaddr, nPages,
		&l0Start, &l0End,
		&l1Start, &l1End
#ifdef CONFIG_ARCH_x86_32_PAE
		,&l2Start, &l2End
#endif
	);

	// Lock off the address spaces to show we mean srs bsns.
	vaddrSpace->level0Accessor.lock.acquire();
	cpuTrib.getCurrentCpuStream()->currentTask->parent->memoryStream
		->vaddrSpaceStream.vaddrSpace.level0Accessor.lock.acquire();

	for (l0Current = l0Start; l0Current <= l0End; l0Current++)
	{
		l0Entry = vaddrSpace->level0Accessor.rsrc->entries[l0Current];
		if (l0Entry == 0)
		{
			if (memoryTrib.pageTablePop(&paddrTmp)
				!= ERROR_SUCCESS)
			{
				goto out;
			};

			// Top level address space modification...
			vaddrSpace->level0Accessor.rsrc->entries[l0Current] =
				paddrTmp | PAGING_L0_PRESENT | PAGING_L0_WRITE;

			*level1Modifier = paddrTmp | (*level1Modifier & 0xFFF);

			// Flush the l1 accessor from the TLB.
			tlbControl::flushSingleEntry((void *)level1Accessor);

			// Zero out the new Page Table.
			for (ztmp=0; ztmp<PAGING_L1_NENTRIES; ztmp++) {
				level1Accessor->entries[ztmp] = 0;
			};
			goto skipL1Flush;
		};

		l0Entry >>= 12;
		*level1Modifier = (l0Entry << 12) | (*level1Modifier & 0xFFF);

		// Flush the l1 Accessor from the TLB.
		tlbControl::flushSingleEntry((void *)level1Accessor);

skipL1Flush:
		l1Current = ((l0Current == l0Start) ? l1Start : 0);
		l1Limit = ((l0Current == l0End)
			? l1End : (PAGING_L1_NENTRIES - 1));

		for (; l1Current <= l1Limit; l1Current++)
		{
#ifdef CONFIG_ARCH_x86_32_PAE
			l1Entry = level1Accessor->entries[l1Current];
			if (l1Entry == 0)
			{
				if (memoryTrib.pageTablePop(&paddrTmp)
					!= ERROR_SUCCESS)
				{
					goto out;
				};

				level1Accessor->entries[l1Current] = paddrTmp
					| PAGING_L1_PRESENT | PAGING_L1_WRITE;

				*level2Modifier = paddrTmp
					| (*level2Modifier & 0xFFF);

				tlbControl::flushSingleEntry(
					(void *)level2Accessor);

				// Zero out the new table.
				for (ztmp=0; ztmp < PAGING_L2_NENTRIES; ztmp++)
				{
					level2Accessor->entries[ztmp] = 0;
				};
				goto skipL2Flush;
			};

			l1Entry >>= 12;
			*level2Modifier =
				(l1Entry << 12) | (*level2Modifier & 0xFFF);

			tlbControl::flushSingleEntry((void *)level2Accessor);

skipL2Flush:
			l2Current = (((l0Current == l0Start)
				&& (l1Current == l1Start)) ? l2Start : 0);

			l2Limit = (((l0Current == l0End)
				&& (l1Current == l1End)) 
				? l2End : (PAGING_L2_NENTRIES - 1));

			for (; l2Current <= l2Limit; l2Current++)
			{
				level2Accessor->entries[l2Current] =
					paddr | archFlags;

				ret++;
			};
#else
			level1Accessor->entries[l1Current] = paddr | archFlags;
			ret++;
#endif
		};
	};

out:
	// Release both locks.
	vaddrSpace->level0Accessor.lock.release();
	cpuTrib.getCurrentCpuStream()->currentTask->parent->memoryStream
		->vaddrSpaceStream.vaddrSpace.level0Accessor.lock.release();

	/* At this point, we need to flush all entries that were touched. Also:
	 * FIXME: Replace this with smpFlushEntryRange(), passing the cpuTrace
	 * bitmap from the task.
	 **/
	tlbControl::flushEntryRange(vaddr, ret);

	/*	FIXME:
	 * Code is needed here to examine our run just now, to see whether or
	 * not we need to propagate l0 changes in the kernel address space to
	 * all processes' address spaces.
	 **/
	return ret;
}

