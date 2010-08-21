
#include <arch/tlbControl.h>
#include <arch/walkerPageRanger.h>
#include <arch/x8632/wPRanger_accessors.h>
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

/**	EXPLANATION:
 * Will peer into the mappings for the given vaddrspace and return the mapping
 * data for a particular page. One page only, of course.
 *
 * Returns a function specific value based on whether the entry was VALID,
 * SWAPPED, or FAKE MAPPED.
 *
 * The function considers an entry with a value of ZERO to be unmapped.
 **/
status_t walkerPageRanger::lookup(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, paddr_t *paddr, uarch_t *flags
	)
{
	uarch_t		l0Start, l1Start;
	paddr_t		l0Entry, l1Entry;
#ifdef CONFIG_ARCH_x86_32_PAE
	uarch_t		l2Start;
	paddr_t		l2Entry;
#endif
	status_t	ret;

	if (vaddrSpace == __KNULL || paddr == __KNULL || flags == __KNULL) {
		return WPRANGER_STATUS_UNMAPPED;
	};

	l0Start = (reinterpret_cast<uarch_t>( vaddr ) & PAGING_L0_VADDR_MASK)
		>> PAGING_L0_VADDR_SHIFT;
	l1Start = (reinterpret_cast<uarch_t>( vaddr ) & PAGING_L1_VADDR_MASK)
		>> PAGING_L1_VADDR_SHIFT;
#ifdef CONFIG_ARCH_x86_32_PAE
	l2Start = (reinterpret_cast<uarch_t>( vaddr ) & PAGING_L2_VADDR_MASK)
		>> PAGING_L2_VADDR_SHIFT;
#endif

	// Make ret default to WPRANGER_STATUS_UNMAPPED.
	ret = WPRANGER_STATUS_UNMAPPED;

	// They'll know we're SRS when we lock off the address spaces.
	vaddrSpace->level0Accessor.lock.acquire();
	cpuTrib.getCurrentCpuStream()->currentTask->parent->memoryStream
		->vaddrSpaceStream.vaddrSpace.level0Accessor.lock.acquire();

	l0Entry = vaddrSpace->level0Accessor.rsrc->entries[l0Start];
	if (l0Entry != 0)
	{
		*level1Modifier &= 0xFFF;
		l0Entry >>= 12;
		*level1Modifier |= (l0Entry << 12);

		tlbControl::flushSingleEntry((void *)level1Accessor);

		l1Entry = level1Accessor->entries[l1Start];
		if (l1Entry != 0)
		{
#ifdef CONFIG_ARCH_x86_32_PAE
			*level2Modifier &= 0xFFF;
			l1Entry >>= 12;
			*level2Modifier |= (l1Entry << 12);

			tlbControl::flushSingleEntry((void *)level2Accessor);
			l2Entry = level2Accessor->entries[l2Start];
			if (l2Entry != 0)
			{
				*paddr = l2Entry & 0xFFFFF000;
				*flags = decodeFlags(l2Entry & 0xFFF);
				if (__KFLAG_TEST(*flags, PAGEATTRIB_PRESENT)) {
					ret = WPRANGER_STATUS_BACKED;
				}
				else
				{
					switch ((*paddr
						>> PAGING_PAGESTATUS_SHIFT)
						& PAGESTATUS_MASK)
					{
					case PAGESTATUS_SWAPPED:
						ret = WPRANGER_STATUS_SWAPPED;
						break;

					case PAGESTATUS_GUARDPAGE:
						ret = WPRANGER_STATUS_GUARDPAGE;
						break;

					case PAGESTATUS_FAKEMAPPED_STATIC:
						ret = WPRANGER_STATUS_FAKEMAPPED_STATIC;
						break;

					case PAGESTATUS_FAKEMAPPED_DYNAMIC:
						ret = WPRANGER_STATUS_FAKEMAPPED_DYNAMIC;
						break;
					default: break;
					};
				};
			}
			else {
				ret = WPRANGER_STATUS_UNMAPPED;
			};
#else
			*paddr = l1Entry & 0xFFFFF000;
			*flags = decodeFlags(l1Entry & 0xFFF);
			if (__KFLAG_TEST(*flags, PAGEATTRIB_PRESENT)) {
				ret = WPRANGER_STATUS_BACKED;
			}
			else
			{
				switch ((*paddr >> PAGING_PAGESTATUS_SHIFT)
					& PAGESTATUS_MASK)
				{
				case PAGESTATUS_SWAPPED:
					ret = WPRANGER_STATUS_SWAPPED;
					break;

				case PAGESTATUS_GUARDPAGE:
					ret = WPRANGER_STATUS_GUARDPAGE;
					break;

				case PAGESTATUS_FAKEMAPPED_STATIC:
					ret = WPRANGER_STATUS_FAKEMAPPED_STATIC;
					break;

				case PAGESTATUS_FAKEMAPPED_DYNAMIC:
					ret = WPRANGER_STATUS_FAKEMAPPED_DYNAMIC;
					break;
				default: break;
				};
			};
#endif
		}
		else {
			ret = WPRANGER_STATUS_UNMAPPED;
		};
	}
	else {
		ret = WPRANGER_STATUS_UNMAPPED;
	};

	// Release both locks. Done with the SRS BSNS.
	cpuTrib.getCurrentCpuStream()->currentTask->parent->memoryStream
		->vaddrSpaceStream.vaddrSpace.level0Accessor.lock.release();

	vaddrSpace->level0Accessor.lock.release();

	return ret;
}

