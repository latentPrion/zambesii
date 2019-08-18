
#include <debug.h>
#include <arch/arch.h>
#include <arch/paging.h>
#include <arch/mathEmulation.h>
#include <chipset/memory.h>
#include <__kstdlib/__kmath.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/memBmp.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/floodplainn/dma.h>
#include <kernel/common/floodplainn/zudi.h>

#define MEMBMP_FULL_SLOT		(~((uarch_t)0))
#define MEMBMP_ALLOC_UNSUCCESSFUL	(~((uarch_t)0))

MemoryBmp::MemoryBmp(paddr_t baseAddr, paddr_t size)
{
	flags = 0;
	MemoryBmp::baseAddr = baseAddr;
	endAddr = baseAddr + (size - 1);

	/**	EXPLANATION:
	 * The reason its fine to use getLow() here is that we limit the number
	 * of supported PFN bits in the kernel to the amount that can fit into
	 * one uarch_t.
	 *
	 * This means that even if a bignum paddr arch supports memory whose
	 * PFN is greater than can fit in one uarch_t, the kernel will refuse to
	 * support more than that.
	 *
	 * So if we cut off the high bits of the result of the SHR calculation
	 * below (by only taking the low order bits of the result), it's fine.
	 **/
	basePfn = (baseAddr >> PAGING_BASE_SHIFT).getLow();
	endPfn = (endAddr >> PAGING_BASE_SHIFT).getLow();
	bmpNFrames = (endPfn - basePfn) + 1;

	nIndexes = __KMATH_NELEMENTS(bmpNFrames, __KBIT_NBITS_IN(*bmp.rsrc.bmp));
	bmpSize = nIndexes * sizeof(*bmp.rsrc.bmp);

	bmp.rsrc.lastAllocIndex = 0;
}

#define MEMBMP_DUMP_STRETCHTYPE_FREE		0
#define MEMBMP_DUMP_STRETCHTYPE_USED		1

void MemoryBmp::dump(void)
{
	ubit32		nFree=0, nUsed=0;
	sbit32		stretch=0, sc;
	ubit8		stretchType=MEMBMP_DUMP_STRETCHTYPE_FREE;

	bmp.lock.acquire();

	printf(NOTICE MEMBMP"v %p: PFN base %X, nframes %d,\n"
		"\tarray %p, nIndexes %d, lastAllocIndex %d.\n",
		this, basePfn, bmpNFrames, bmp.rsrc.bmp, nIndexes,
		bmp.rsrc.lastAllocIndex);

	for (ubit32 i=0; i<bmpNFrames; i++)
	{
		if (__KBIT_TEST(
			bmp.rsrc.bmp[i / __KBIT_NBITS_IN(*bmp.rsrc.bmp)],
			i % __KBIT_NBITS_IN(*bmp.rsrc.bmp)))
		{
			nUsed++;
		}
		else {
			nFree++;
		};
	};

	for (sc=0; sc<(signed)bmpNFrames; sc++)
	{
		if (__KBIT_TEST(
			bmp.rsrc.bmp[sc/__KBIT_NBITS_IN(*bmp.rsrc.bmp)],
			sc % __KBIT_NBITS_IN(*bmp.rsrc.bmp)))
		{
			if (stretchType == MEMBMP_DUMP_STRETCHTYPE_FREE)
			{
				stretchType = MEMBMP_DUMP_STRETCHTYPE_USED;
				printf(NOTICE MEMBMP"Free stretch %d bits "
					"starting at bit %d.\n",
					stretch, sc-stretch);

				stretch = 0;
			};
			stretch++;
		}
		else
		{
			if (stretchType == MEMBMP_DUMP_STRETCHTYPE_USED)
			{
				stretchType = MEMBMP_DUMP_STRETCHTYPE_FREE;
				printf(NOTICE MEMBMP"Used stretch %d bits "
					"starting at bit %d.\n",
					stretch, sc-stretch);

				stretch = 0;
			};
			stretch++;
		};
	};

	bmp.lock.release();
	printf(NOTICE MEMBMP"Final stretch %s, %d bits @ %d; Total %d free, "
		"%d used.\n",
		((stretchType == MEMBMP_DUMP_STRETCHTYPE_FREE)?"free":"used"),
		stretch, sc-stretch, nFree, nUsed);
}

/**	EXPLANATION:
 * It isn't necessary to pass a page aligned address to this function. You
 * can pass an unaligned address. If an unaligned address is passed, the bmp
 * will automatically map the unaligned frame as used.
 *
 * This is to prevent conflicts between BMPs.
 **/
error_t MemoryBmp::initialize(void *preAllocated)
{
	if (preAllocated != NULL) {
		bmp.rsrc.bmp = new (preAllocated) uarch_t[nIndexes];
	}
	else
	{
		bmp.rsrc.bmp = new (
			processTrib.__kgetStream()->memoryStream
				.memAlloc(
					PAGING_BYTES_TO_PAGES(bmpSize),
					MEMALLOC_NO_FAKEMAP))
			uarch_t[nIndexes];

		if (bmp.rsrc.bmp == NULL) {
			return ERROR_MEMORY_NOMEM;
		};

		FLAG_SET(flags, MEMBMP_FLAGS_DYNAMIC);
	};
	memset(bmp.rsrc.bmp, 0, bmpSize);

	if (!!(baseAddr & PAGING_BASE_MASK_LOW)) {
		setFrame(basePfn);
	};
	// Set last frame if end address is on the exact bound of a new frame.
	if (!(endAddr & PAGING_BASE_MASK_LOW)) {
		setFrame(endPfn);
	};

	return ERROR_SUCCESS;
}

MemoryBmp::~MemoryBmp(void)
{
	if (FLAG_TEST(flags, MEMBMP_FLAGS_DYNAMIC))
	{
		processTrib.__kgetStream()->memoryStream.memFree(bmp.rsrc.bmp);
	};
}

status_t MemoryBmp::merge(MemoryBmp *b)
{
	uarch_t		loopMaxPfn, loopStartPfn;
	status_t	ret=0;

	// Make sure we're not wasting on time on two that don't intersect.
	if (b->basePfn > endPfn || b->endPfn < basePfn) {
		return 0;
	};

	if (b->basePfn < basePfn) {
		loopStartPfn = basePfn;
	}
	else {
		loopStartPfn = b->basePfn;
	};

	if (b->endPfn > endPfn) {
		loopMaxPfn = endPfn;
	}
	else {
		loopMaxPfn = b->endPfn;
	};

	bmp.lock.acquire();

	for (uarch_t pfn=loopStartPfn; pfn < loopMaxPfn; pfn++)
	{
		if (b->testFrame(pfn))
		{
			setFrame(pfn);
			ret++;
		};
	};

	bmp.lock.release();

	return ret;
}

status_t MemoryBmp::fragmentedGetFrames(uarch_t _nFrames, paddr_t *paddr)
{
	/* FIXME: bmp.rsrc.lastAllocIndex should be reset and the whole thing
	 * done over once more if it reaches the last bit and no memory was
	 * found.
	 *
	 * Remember to reset bitlimit to 0 when restarting.
	 **/
	uarch_t		nFound = 0, _endPfn, startPfn;
	uarch_t		bitLimit=__KBIT_NBITS_IN(*bmp.rsrc.bmp);

	if (paddr == NULL) { return ERROR_INVALID_ARG; };
	if (_nFrames == 0)
	{
		// If 0 frames requested, return success early.
		printf(WARNING MEMBMP"fragmentedGetFrames called with 0.\n");
		return ERROR_SUCCESS;
	};

	bmp.lock.acquire();

	for (uarch_t i=bmp.rsrc.lastAllocIndex; i<nIndexes; i++)
	{
		if (bmp.rsrc.bmp[i] == MEMBMP_FULL_SLOT) { continue; };

		if (i == nIndexes-1)
		{
			bitLimit = bmpNFrames % __KBIT_NBITS_IN(*bmp.rsrc.bmp);
			if (bitLimit == 0) {
				bitLimit = __KBIT_NBITS_IN(*bmp.rsrc.bmp);
			};
		};

		for (uarch_t j=0; j<bitLimit; j++)
		{
			if (!__KBIT_TEST(bmp.rsrc.bmp[i], j))
			{
				startPfn = basePfn + (i *
					__KBIT_NBITS_IN(*bmp.rsrc.bmp)) + j;

				for(; i<nIndexes; i++)
				{
					if (bmp.rsrc.bmp[i] == MEMBMP_FULL_SLOT)
					{
						goto out;
					};

					if (i == nIndexes-1)
					{
						bitLimit = bmpNFrames
							% __KBIT_NBITS_IN(*bmp.rsrc.bmp);

						if (bitLimit == 0)
						{
							bitLimit =
								__KBIT_NBITS_IN(*bmp.rsrc.bmp);
						};
					};

					for (uarch_t k=((nFound == 0) ? j : 0);
						k < bitLimit;
						k++)
					{
						if (__KBIT_TEST(
							bmp.rsrc.bmp[i], k))
						{
							goto out;
						};
						nFound++;
						if (nFound >= _nFrames) {
							goto out;
						};
					};
				};
			};
		};
	};

	/* If we end up here then not enough contiguous frames were
	 * found. Release all locks and exit.
	 **/
	bmp.lock.release();
	return ERROR_MEMORY_NOMEM_PHYSICAL;

out:
	_endPfn = startPfn + nFound;
	for (uarch_t i=startPfn; i<_endPfn; i++) {
		setFrame(i);
	};

	bmp.rsrc.lastAllocIndex =
		(_endPfn - basePfn) / __KBIT_NBITS_IN(*bmp.rsrc.bmp);

	bmp.lock.release();
	*paddr = startPfn << PAGING_BASE_SHIFT;
	return nFound;
}

sbit8 MemoryBmp::releaseFrames(paddr_t frameAddr, uarch_t _nFrames)
{
	uarch_t		rangeStartPfn;
	uarch_t		rangeEndPfn;;

	if (_nFrames == 0) { return 0; };

	rangeStartPfn = (frameAddr >> PAGING_BASE_SHIFT).getLow();
	rangeEndPfn = rangeStartPfn + _nFrames;

	/* If the frames to be released are not in range of this BMP, return
	 * false.
	 **/
	if (!((frameAddr >= rangeStartPfn && frameAddr <= endPfn)
		|| (rangeEndPfn > rangeStartPfn && rangeEndPfn <= endPfn)))
	{
		return 0;
	};

	bmp.lock.acquire();

	for (uarch_t i=rangeStartPfn; i<rangeEndPfn; i++) {
		unsetFrame(i);
	};

	// Encourage possible cache hit by trying to reallocate same frames.
	bmp.rsrc.lastAllocIndex =
		(rangeStartPfn - basePfn) / __KBIT_NBITS_IN(*bmp.rsrc.bmp);

	bmp.lock.release();
	return 1;
}

void MemoryBmp::mapMemUsed(paddr_t rangeBaseAddr, uarch_t rangeNFrames)
{
	uarch_t		rangeStartPfn=basePfn;
	uarch_t		rangeEndPfn;

	if (rangeBaseAddr >= baseAddr) {
		rangeStartPfn = (rangeBaseAddr >> PAGING_BASE_SHIFT).getLow();
	};

	rangeEndPfn = (rangeBaseAddr >> PAGING_BASE_SHIFT).getLow()
		+ rangeNFrames;

	if (rangeEndPfn > endPfn) {
		rangeEndPfn = endPfn + 1;
	};

	bmp.lock.acquire();

	for (uarch_t i=rangeStartPfn; i<rangeEndPfn; i++) {
		setFrame(i);
	};

	bmp.lock.release();
}

void MemoryBmp::mapMemUnused(paddr_t rangeBaseAddr, uarch_t rangeNFrames)
{
	uarch_t		rangeStartPfn=basePfn;
	uarch_t		rangeEndPfn;

	if (rangeBaseAddr >= baseAddr) {
		rangeStartPfn = (rangeBaseAddr >> PAGING_BASE_SHIFT).getLow();
	};

	rangeEndPfn = (rangeBaseAddr >> PAGING_BASE_SHIFT).getLow()
		+ rangeNFrames;

	if (rangeEndPfn > endPfn) {
		rangeEndPfn = endPfn + 1;
	};

	bmp.lock.acquire();

	for (uarch_t i=rangeStartPfn; i<rangeEndPfn; i++) {
		unsetFrame(i);
	};

	bmp.lock.release();
}

status_t MemoryBmp::constrainedGetFrames(
	fplainn::dma::constraints::Compiler *c,
	uarch_t nFrames,
	fplainn::dma::ScatterGatherList *retlist,
	ubit32 flags
	)
{
	status_t			ret=0;
	uarch_t 			nextBoundarySkip, alignmentStartBit,
					searchEndBit=bmpNFrames,
					nScgthElements=0, currScgthElement=0,
					scgthPreviousNElements=0;
	error_t 			error;
	sbit8				dontLockRetlist;

	/* This flag tells us whether or not the "retlist" object was locked
	 * before being passed to us. If the flag is set, then it means that the
	 * object is already locked, and we should operate it unlocked.
	 */
	dontLockRetlist = FLAG_TEST(
		flags, MemoryTrib::CGF_SGLIST_UNLOCKED)
		? 1 : 0;

	/**	EXPLANATION:
	 * Two pass algorithm for searching a bitmap for pmem, and then
	 * allocating memory to store it as a scatter gather list.
	 *
	 * On the first pass we just check to see how many bits we can get.
	 * Then if we can get all the memory we need, we allocate the memory
	 * for the metadata needed to track the pmem. Finally, we do a second
	 * pass to both allocate and track the pmem.
	 */
	(void)flags;

	if (c == NULL || retlist == NULL) { return ERROR_INVALID_ARG; };
	if (!c->isValid()) { return ERROR_INVALID_STATE; }

	if (nFrames == 0) {
		return 0;
	};

	/**	EXPLANATION:
	 * In order to preserve the alignment constraint required by the caller,
	 * we have to ensure that we start attempting to allocate on an aligned
	 * bit in the first place.
	 **/
	alignmentStartBit = (c->i.startPfn.getLow() >= basePfn)
		? c->i.startPfn.getLow()
		: basePfn;

	if (alignmentStartBit & c->i.pfnSkipStride)
	{
		alignmentStartBit += c->i.pfnSkipStride + 1;
		alignmentStartBit &= ~c->i.pfnSkipStride;
	};

	if (alignmentStartBit >= endPfn)
	{
		/* If attempting to find a starting bit that is aligned to the
		 * caller's constraints causes us to hit the end of the bitmap's
		 * bits, then it's impossible for us to satisfy the allocation
		 * from this bitmap. Return error.
		 **/
		return ERROR_LIMIT_OVERFLOWED;
	}

	// The algorithm deals with bit positions; not PFNs.
	alignmentStartBit -= basePfn;

	// Decide the upper bit bound of our search.
	if (c->i.beyondEndPfn.getLow() - basePfn < bmpNFrames) {
		searchEndBit = c->i.beyondEndPfn.getLow() - basePfn;
	}

	bmp.lock.acquire();

	for (sbit8 isSecondPass = 0; isSecondPass <= 1; isSecondPass++)
	{
		/**	EXPLANATION:
		 * From here, we're just allocating. Run through the bitmaps and try to
		 * get nFrames in accordance with the compiled constraints specification
		 * we've been asked to satisfy.
		 *
		 * When searching for DMA memory, we don't use the "lastIndex"
		 * optimization. We need memory that satisfies the constraints, and we
		 * don't need to seriously optimize this allocation because we accept
		 * that we are already performing a costly allocation that must satisfy
		 * particular constraints.
		 **/
		for (
			uarch_t i=alignmentStartBit;
			i<searchEndBit && ret < (signed)nFrames;
			i+=(1 + c->i.pfnSkipStride + nextBoundarySkip))
		{
			uarch_t nFound = 0;
			status_t foundStartBit;
			paddr_t foundBaseAddr;

			nextBoundarySkip = 0;
			foundStartBit = checkForContiguousBitsAt(
				bmp.rsrc.bmp, i,
				c->i.minElementGranularityNFrames.getLow(),
				c->i.maxNContiguousFrames.getLow(),
				bmpNFrames - i, &nFound);

			if (foundStartBit < 1) {
				break; // Cannot satisfy the allocation.
			};

			/* Minimize memory waste on the final element of the
			 * allocation.
			 **/
			if (ret + nFound >= nFrames) {
				nFound = nFrames - ret;
				if (nFound < c->i.minElementGranularityNFrames.getLow()) {
					nFound = c->i.minElementGranularityNFrames.getLow();
				}
			}

			/* If the number of frames we found means that the next search
			 * will begin at an unaligned bit (alignment is a multiple of
			 * pfnSkipStride), we have to pad until the next alignment
			 * boundary.
			 **/
			while (c->i.pfnSkipStride + 1 + nextBoundarySkip < nFound) {
				nextBoundarySkip += c->i.pfnSkipStride + 1;
			};

			if (isSecondPass)
			{
				/* Add the stretch to the retlist. This really shouldn't fail
				 * because we preallocated the room for it, but still check.
				 **/
				foundBaseAddr = foundStartBit + basePfn;
				foundBaseAddr <<= PAGING_BASE_SHIFT;

				error = retlist->addFrames(
					foundBaseAddr, nFound,
					scgthPreviousNElements
						+ currScgthElement,
					((dontLockRetlist)
						? fplainn::dma::ScatterGatherList::AF_FLAGS_UNLOCKED : 0));
				if (error != ERROR_SUCCESS)
				{
					bmp.lock.release();
					printf(ERROR MEMBMP"Failed to add %d "
						"frames to SGList, starting at "
						"%P.\n",
						nFound, &foundBaseAddr);
					return error;
				}

//printf(NOTICE MEMBMP"Added element @index %d: baseAddr %P, %d frames, ret is %d\n",
//	currScgthElement, &foundBaseAddr, nFound, error);
				// Set bits in the BMP.
				for (uarch_t j=0; j<nFound; j++) {
					setFrame(basePfn + foundStartBit + j);
				};
			};

			if (!isSecondPass) {
				nScgthElements++;
			} else {
				currScgthElement++;
			}
			ret += nFound;
		};

		if (ret < (signed)nFrames)
		{
			// Free all the frames that were found and allocated.
			bmp.lock.release();
			if (isSecondPass)
			{
				panic(ERROR_UNKNOWN, CC"The second pass of "
					"constrainedGetFrames should be "
					"failsafe.\n");
			}
			return ERROR_MEMORY_NOMEM_PHYSICAL;
		}

		/* Resizing the array resizes it from the end of the previous
		 * internal memory.
		 *
		 *	FIXME: LOCKING:
		 * We need to lock the getNIndexes() and resizeToHoldIndex()
		 * operations together.
		 */
		if (!isSecondPass)
		{
			if (!dontLockRetlist) { retlist->lock(); }

			scgthPreviousNElements = retlist->elements.unlocked_getNIndexes();

			error = retlist->preallocateEntries(
				scgthPreviousNElements + nScgthElements,
				fplainn::dma::ScatterGatherList::PE_FLAGS_UNLOCKED);

			if (!dontLockRetlist) { retlist->unlock(); }

			if (error != ERROR_SUCCESS)
			{
				bmp.lock.release();
				printf(ERROR MEMBMP"Failed to prealloc %d SGList entries for "
					"constrained alloc.\n", nScgthElements);
				return error;
			};
//printf(NOTICE"PReallocated room for %d list entries, to describe %d frames total.\n",
//	nScgthElements, nFrames);
		};

		if (!isSecondPass) { ret = 0; };
	}

	bmp.lock.release();
	return ret;
}
