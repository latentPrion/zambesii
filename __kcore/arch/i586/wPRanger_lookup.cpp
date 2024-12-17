
#include <arch/tlbControl.h>
#include <arch/walkerPageRanger.h>
#include <arch/x8632/wPRanger_accessors.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>

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
	VaddrSpace *vaddrSpace,
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

	if (vaddrSpace == NULL || paddr == NULL || flags == NULL) {
		return WPRANGER_STATUS_UNMAPPED;
	};

	vaddr = reinterpret_cast<void *>(
		(uarch_t)vaddr & PAGING_BASE_MASK_HIGH );

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
	cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()->parent
		->getVaddrSpaceStream()->vaddrSpace
		.level0Accessor.lock.acquire();

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
				*flags = decodeFlags(l2Entry & 0xFFF);
				*paddr = (l2Entry >> 12);
				*paddr <<= 12;

				if (FLAG_TEST(*flags, PAGEATTRIB_PRESENT)) {
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

					case PAGESTATUS_HEAP_GUARDPAGE:
						ret = WPRANGER_STATUS_HEAP_GUARDPAGE;
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
			*flags = decodeFlags((l1Entry & 0xFFF).getLow());
			*paddr = (l1Entry >> 12);
			*paddr <<= 12;

			if (FLAG_TEST(*flags, PAGEATTRIB_PRESENT)) {
				ret = WPRANGER_STATUS_BACKED;
			}
			else
			{
				paddr_t		switchable =
					(*paddr >> PAGING_PAGESTATUS_SHIFT)
						& PAGESTATUS_MASK;

				switch (switchable.getLow())
				{
				case PAGESTATUS_SWAPPED:
					ret = WPRANGER_STATUS_SWAPPED;
					break;

				case PAGESTATUS_GUARDPAGE:
					ret = WPRANGER_STATUS_GUARDPAGE;
					break;

				case PAGESTATUS_HEAP_GUARDPAGE:
					ret = WPRANGER_STATUS_HEAP_GUARDPAGE;
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
	cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()->parent
		->getVaddrSpaceStream()->vaddrSpace
		.level0Accessor.lock.release();

	vaddrSpace->level0Accessor.lock.release();

	return ret;
}

// This is actually a fully portable function, btw. Shouldn't really be in here.
void *walkerPageRanger::createMappingTo(
	paddr_t paddr, uarch_t nPages, uarch_t flags
	)
{
	void		*ret;
	status_t	nMapped;

	/*	WARNING:
	 * Returns a page aligned address which requires the use of
	 * WPRANGER_ADJUST_PADDR().
	 **/

	// Safely clear the lower bits of the paddr.
	paddr >>= PAGING_BASE_SHIFT;
	paddr <<= PAGING_BASE_SHIFT;

	// Get vmem.
	ret = processTrib.__kgetStream()->getVaddrSpaceStream()->getPages(
		nPages);

	if (ret == NULL)
	{
		printf(ERROR WPRANGER"createMappingTo(%P, %d): Failed to "
			"alloc vmem.\n",
			&paddr, nPages);

		return NULL;
	};

	// Map vmem to paddr.
	nMapped = mapInc(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		ret, paddr, nPages, flags);

	if (nMapped < (signed)nPages)
	{
		printf(ERROR WPRANGER"createMappingTo(%P, %d): mapInc "
			"failed.\n",
			&paddr, nPages);

		processTrib.__kgetStream()->getVaddrSpaceStream()->releasePages(
			ret, nPages);

		return NULL;
	};

	return ret;
}

void walkerPageRanger::destroyNonsharedMapping(void *vaddr, uarch_t nPages)
{
	VaddrSpaceStream	*vaddrSpaceStream;
	paddr_t			p;
	uarch_t			f;

	vaddrSpaceStream = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread()->parent->getVaddrSpaceStream();

	unmap(&vaddrSpaceStream->vaddrSpace, vaddr, &p, nPages, &f);
	vaddrSpaceStream->releasePages(vaddr, nPages);
}

void *walkerPageRanger::createSharedMappingTo(
	VaddrSpace *srcVaddrSpace, void *srcVaddr, uarch_t nPages,
	uarch_t flags, void *placementAddress
	)
{
	VaddrSpaceStream	*vaddrSpaceStream;
	void			*ret, *tmpVaddr;

	if (nPages == 0 || srcVaddr == NULL) { return NULL; };

	vaddrSpaceStream = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread()->parent->getVaddrSpaceStream();

	// If no placement addr, allocate loose pages.
	if (placementAddress != NULL) { ret = placementAddress; }
	else { ret = vaddrSpaceStream->getPages(nPages); };

	if (ret == NULL) { return NULL; };

	// Now map the loose pages to the addr range in the source addrspace.
	tmpVaddr = ret;
	for (uarch_t i=0; i<nPages;
		i++,
		srcVaddr = (void *)((uintptr_t)srcVaddr + PAGING_BASE_SIZE),
		tmpVaddr = (void *)((uintptr_t)tmpVaddr + PAGING_BASE_SIZE))
	{
		paddr_t			p;
		uarch_t			f;
		status_t		nMapped;

		lookup(srcVaddrSpace, srcVaddr, &p, &f);
		nMapped = mapInc(
			&vaddrSpaceStream->vaddrSpace, tmpVaddr, p, 1, flags);

		if (nMapped < 1)
		{
			/* If we were given a placement address, it means we
			 * didn't allocate the loose page mapping space
			 * ourselves; In that case, we also shouldn't free it
			 * either.
			 **/
			if (placementAddress != NULL) {
				vaddrSpaceStream->releasePages(ret, nPages);
			};

			return NULL;
		};
	};

	return ret;
}

