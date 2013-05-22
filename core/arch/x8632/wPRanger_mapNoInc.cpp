
#include <arch/paging.h>
#include <arch/tlbControl.h>
#include <arch/walkerPageRanger.h>
#include <arch/x8632/wPRanger_accessors.h>
#include <arch/x8632/wPRanger_getLevelRanges.h>
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/process.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

/**	FIXME:
 * When you begin doing userspace stuff, remember that this
 * function maps frames as supervisor by default. So you have to pass user.
 *
 * Do something about that...
 **/

status_t walkerPageRanger::mapNoInc(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, paddr_t paddr,
	uarch_t nPages, uarch_t __kflags
	)
{
	// This function takes up a *lot* of room on the stack...
	uarch_t		l0Start, l0Current, l0End;
	uarch_t		l1Start, l1Current, l1Limit, l1End;
	paddr_t		l0Entry;
#ifdef CONFIG_ARCH_x86_32_PAE
	uarch_t		l2Start, l2Current, l2Limit, l2End;
	paddr_t		l1Entry, l2Entry;
#endif
	uarch_t		archFlags, localFlush;
	status_t	ret=0;
	sarch_t		l0WasModified=0;
	int		ztmp;

	if (nPages == 0) { return ret; };

	localFlush = __KFLAG_TEST(__kflags, PAGEATTRIB_LOCAL_FLUSH_ONLY);
	vaddr = reinterpret_cast<void *>(
		(uarch_t)vaddr & PAGING_BASE_MASK_HIGH );

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
	cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask()->parent
		->getVaddrSpaceStream()->vaddrSpace
		.level0Accessor.lock.acquire();

	for (l0Current = l0Start; l0Current <= l0End; l0Current++)
	{
		l0Entry = vaddrSpace->level0Accessor.rsrc->entries[l0Current];
		if (l0Entry == 0)
		{
			if (memoryTrib.pageTablePop(&l0Entry)
				!= ERROR_SUCCESS)
			{
				goto out;
			};

			// Top level address space modification...
			l0WasModified = 1;
			l0Entry |= PAGING_L0_PRESENT | PAGING_L0_WRITE;
			vaddrSpace->level0Accessor.rsrc->entries[l0Current] =
				l0Entry;

			*level1Modifier = vaddrSpace->level0Accessor.rsrc
				->entries[l0Current];

			// Flush the l1 accessor from the TLB.
			tlbControl::flushSingleEntry((void *)level1Accessor);

			// Zero out the new Page Table.
			for (ztmp=0; ztmp<PAGING_L1_NENTRIES; ztmp++) {
				level1Accessor->entries[ztmp] = 0;
			};
		}
		else
		{
			l0Entry >>= 12;
			*level1Modifier = (l0Entry << 12) | (*level1Modifier & 0xFFF);

			// Flush the l1 Accessor from the TLB.
			tlbControl::flushSingleEntry((void *)level1Accessor);
		};

		l1Current = ((l0Current == l0Start) ? l1Start : 0);
		l1Limit = ((l0Current == l0End)
			? l1End : (PAGING_L1_NENTRIES - 1));

		for (; l1Current <= l1Limit; l1Current++)
		{
#ifdef CONFIG_ARCH_x86_32_PAE
			l1Entry = level1Accessor->entries[l1Current];
			if (l1Entry == 0)
			{
				if (memoryTrib.pageTablePop(&l1Entry)
					!= ERROR_SUCCESS)
				{
					goto out;
				};

				l1Entry |= PAGING_L1_PRESENT | PAGING_L1_WRITE;
				level1Accessor->entries[l1Current] = l1Entry;

				*level2Modifier =
					level1Accessor->entries[l1Current];

				tlbControl::flushSingleEntry(
					(void *)level2Accessor);

				// Zero out the new table.
				for (ztmp=0; ztmp < PAGING_L2_NENTRIES; ztmp++)
				{
					level2Accessor->entries[ztmp] = 0;
				};
			}
			else
			{
				*level2Modifier = l1Entry;
				tlbControl::flushSingleEntry(
					(void *)level2Accessor);
			};

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
#ifndef CONFIG_ARCH_x86_32_PAE
	/* Check to see if this mapping call caused a level0 table change in
	 * the kernel's high address space range. If so, we need to propagate
	 * the change. Not required on PAE kernel.
	 **/
	if (l0Start >= ARCH_MEMORY___KLOAD_VADDR_BASE >> PAGING_L0_VADDR_SHIFT
		&& l0WasModified
		&& vaddrSpace != &processTrib.__kgetStream()
			->getVaddrSpaceStream()->vaddrSpace)
	{
		processTrib.__kgetStream()->getVaddrSpaceStream()
			->vaddrSpace.level0Accessor.lock.acquire();

		// Propagate the change to the kernel's vaddrspace object.
		for (uarch_t i=l0Start; i <= l0End; i++)
		{
			processTrib.__kgetStream()->getVaddrSpaceStream()
				->vaddrSpace.level0Accessor.rsrc->entries[i] =
				vaddrSpace->level0Accessor.rsrc->entries[i];
		};

		processTrib.__kgetStream()->getVaddrSpaceStream()
			->vaddrSpace.level0Accessor.lock.acquire();
	};
#endif

#if __SCALING__ > SCALING_SMP
	if (localFlush) {
		tlbControl::flushEntryRange(vaddr, ret);
	}
	else {
		tlbControl::smpFlushEntryRange(vaddr, ret);
	};
#else
	tlbControl::flushEntryRange(vaddr, ret);
#endif

	// Release both locks.
	vaddrSpace->level0Accessor.lock.release();
	cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask()->parent
		->getVaddrSpaceStream()->vaddrSpace
		.level0Accessor.lock.release();

	return ret;
}

