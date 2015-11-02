
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

	printf(NOTICE MEMBMP"v 0x%p: PFN base 0x%X, nframes %d,\n"
		"\tarray 0x%p, nIndexes %d, lastAllocIndex %d.\n",
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

error_t MemoryBmp::contiguousGetFrames(uarch_t _nFrames, paddr_t *paddr)
{
	/* FIXME: bmp.rsrc.lastAllocIndex should be reset and the whole thing
	 * done over once more if it reaches the last bit and no memory was
	 * found.
	 *
	 * FIXME: Right now, the bmp measures its limit by the number of
	 * indexes, and not the number of bits within the range. Therefore, if
	 * the last index is such that there should only be 2 relevant bits in
	 * the last index, and the other 30 should not be searched since they
	 * are just garbage, right now as things are, the bmp will assume all
	 * 32 bits are valid to be searched, and possibly find memory in the
	 * irrelevant bits.
	 **/
	uarch_t		nFound=0, startPfn=MEMBMP_ALLOC_UNSUCCESSFUL, _endPfn;
	uarch_t	bitLimit=__KBIT_NBITS_IN(*bmp.rsrc.bmp);

	if (_nFrames == 0) { return ERROR_INVALID_ARG_VAL; };
	if (paddr == NULL) { return ERROR_INVALID_ARG; };

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
				startPfn = basePfn
					+ (i * __KBIT_NBITS_IN(
						*bmp.rsrc.bmp))
					+ j;

				for(; i<nIndexes; i++)
				{
					if (bmp.rsrc.bmp[i] == MEMBMP_FULL_SLOT)
					{
						nFound = 0;
						break;
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
							startPfn = MEMBMP_ALLOC_UNSUCCESSFUL;
							nFound = 0;
							j = k;
							break;
						};
						nFound++;
						if (nFound >= _nFrames) {
							goto success;
						};
					};
					if (nFound == 0) {
						break;
					};
				};
			};
		};
	};

	/* If we end up here then it means that not enough contiguous frames
	 * were found. Release all locks and exit.
	 **/
	bmp.lock.release();
	return ERROR_MEMORY_NOMEM_PHYSICAL;

success:
	_endPfn = startPfn + _nFrames;
	for (uarch_t i=startPfn; i<_endPfn; i++) {
		setFrame(i);
	};

	bmp.rsrc.lastAllocIndex =
		(_endPfn - basePfn) / __KBIT_NBITS_IN(*bmp.rsrc.bmp);

	bmp.lock.release();
	*paddr = startPfn << PAGING_BASE_SHIFT;
	return ERROR_SUCCESS;
}

status_t MemoryBmp::fragmentedGetFrames(uarch_t _nFrames, paddr_t *paddr)
{
	/* FIXME: bmp.rsrc.lastAllocIndex should be reset and the whole thing
	 * done over once more if it reaches the last bit and no memory was
	 * found.
	 *
	 * Remember to reset bitlimit to 0 when restarting.
	 **/
	uarch_t 	nFound = 0, _endPfn, startPfn;
	uarch_t		bitLimit=__KBIT_NBITS_IN(*bmp.rsrc.bmp);

	if (_nFrames == 0) { return ERROR_INVALID_ARG_VAL; };
	if (paddr == NULL) { return ERROR_INVALID_ARG; };

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

void MemoryBmp::releaseFrames(paddr_t frameAddr, uarch_t _nFrames)
{
	uarch_t		rangeStartPfn;
	uarch_t		rangeEndPfn;;

	if (_nFrames == 0) { return; };

	rangeStartPfn = (frameAddr >> PAGING_BASE_SHIFT).getLow();
	rangeEndPfn = rangeStartPfn + _nFrames;

	bmp.lock.acquire();

	for (uarch_t i=rangeStartPfn; i<rangeEndPfn; i++) {
		unsetFrame(i);
	};

	// Encourage possible cache hit by trying to reallocate same frames.
	bmp.rsrc.lastAllocIndex =
		(rangeStartPfn - basePfn) / __KBIT_NBITS_IN(*bmp.rsrc.bmp);

	bmp.lock.release();
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

