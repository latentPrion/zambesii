
#include <arch/arch.h>
#include <arch/paging.h>
#include <arch/mathEmulation.h>
#include <chipset/memory.h>
#include <chipset/__kmemory.h>
#include <lang/lang.h>
#include <__kstdlib/__kmath.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/memBmp.h>
#include <kernel/common/panic.h>
#include <kernel/common/memoryTrib/memoryTrib.h>

#define MEMBMP_FULL_SLOT		(~((uarch_t)0))
#define MEMBMP_ALLOC_UNSUCCESSFUL	(~((uarch_t)0))

memBmpC::memBmpC(void)
{
}

memBmpC::memBmpC(paddr_t baseAddr, paddr_t size, void *preAllocated)
{
	if (initialize(baseAddr, size, preAllocated) != ERROR_SUCCESS) {
		panic(ERROR_UNKNOWN, mmStr[0]);
	};
}

/**	EXPLANATION:
 * It isn't necessary to pass a page aligned address to this function. You 
 * can pass an unaligned address. If an unaligned address is passed, the bmp
 * will automatically map the unaligned frame as used.
 *
 * This is to prevent conflicts between BMPs.
 **/
error_t memBmpC::initialize(paddr_t baseAddr, paddr_t size, void *preAllocated)
{
	flags = 0;
	memBmpC::baseAddr = baseAddr;
	endAddr = baseAddr + (size - 1);

	basePfn = baseAddr / PAGING_BASE_SIZE;
	endPfn = endAddr / PAGING_BASE_SIZE;
	nFrames = (endPfn - basePfn) + 1;

	nIndexes = __KMATH_NELEMENTS(nFrames, __KBIT_NBITS_IN(*bmp.rsrc.bmp));

	bmpSize = nIndexes * sizeof(*bmp.rsrc.bmp);

	if (preAllocated) {
		bmp.rsrc.bmp = new (preAllocated) uarch_t[nIndexes];
	}
	else
	{
		//XXX: Note: This overallocates memory for the BMP.
		bmp.rsrc.bmp = new (
			memoryTrib.rawMemAlloc(
				PAGING_BYTES_TO_PAGES(bmpSize)))
			uarch_t[nIndexes];

		if (bmp.rsrc.bmp == __KNULL) {
			return ERROR_MEMORY_NOMEM;
		};

		__KFLAG_SET(flags, MEMBMP_FLAGS_DYNAMIC);
	};
	memset(bmp.rsrc.bmp, 0, bmpSize);
	bmp.rsrc.lastAllocIndex = 0;

	if (baseAddr & PAGING_BASE_MASK_LOW) {
		setFrame(basePfn);
	};
	if ((baseAddr + size) & PAGING_BASE_MASK_LOW) {
		setFrame(endPfn);
	};

	return ERROR_SUCCESS;
}

memBmpC::~memBmpC(void)
{
	if (__KFLAG_TEST(flags, MEMBMP_FLAGS_DYNAMIC))
	{
		memoryTrib.rawMemFree(bmp.rsrc.bmp,
			PAGING_BYTES_TO_PAGES(bmpSize));
	};
}

error_t memBmpC::contiguousGetFrames(uarch_t nFrames, paddr_t *paddr)
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
	uarch_t nFound = 0, startPfn = MEMBMP_ALLOC_UNSUCCESSFUL, _endPfn;

	if (nFrames == 0) { return ERROR_INVALID_ARG_VAL; };
	if (paddr == __KNULL) { return ERROR_INVALID_ARG; };

	bmp.lock.acquire();

	for (uarch_t i=bmp.rsrc.lastAllocIndex; i<nIndexes; i++)
	{
		if (bmp.rsrc.bmp[i] == MEMBMP_FULL_SLOT) { continue; };

		for (uarch_t j=0; j<__KBIT_NBITS_IN(*bmp.rsrc.bmp); j++)
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

					for (uarch_t k=((nFound == 0) ? j : 0);
						k < __KBIT_NBITS_IN(
							*bmp.rsrc.bmp);
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
						if (nFound >= nFrames) {
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
	_endPfn = startPfn + nFrames;
	for (uarch_t i=startPfn; i<_endPfn; i++) {
		setFrame(i);
	};
	
	bmp.rsrc.lastAllocIndex =
		(_endPfn - basePfn) / __KBIT_NBITS_IN(*bmp.rsrc.bmp);

	bmp.lock.release();
	*paddr = startPfn * PAGING_BASE_SIZE;
	return ERROR_SUCCESS;
}

status_t memBmpC::fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr)
{
	/* FIXME: bmp.rsrc.lastAllocIndex should be reset and the whole thing
	 * done over once more if it reaches the last bit and no memory was
	 * found.
	 *
	 * Remember to reset bitlimit to 0 when restarting.
	 **/
	uarch_t 	nFound = 0, _endPfn, startPfn;
	uarch_t		bitLimit=__KBIT_NBITS_IN(*bmp.rsrc.bmp);

	if (nFrames == 0) { return ERROR_INVALID_ARG_VAL; };
	if (paddr == __KNULL) { return ERROR_INVALID_ARG; };

	bmp.lock.acquire();

	for (uarch_t i=bmp.rsrc.lastAllocIndex; i<nIndexes; i++)
	{
		if (bmp.rsrc.bmp[i] == MEMBMP_FULL_SLOT) { continue; };

		if (i == nIndexes-1)
		{
			bitLimit = nFrames % __KBIT_NBITS_IN(*bmp.rsrc.bmp);
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
						if (nFound >= nFrames) {
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
	*paddr = startPfn * PAGING_BASE_SIZE;
	return nFound;
}	

void memBmpC::releaseFrames(paddr_t frameAddr, uarch_t nFrames)
{
	uarch_t		startPfn = frameAddr / PAGING_BASE_SIZE;
	uarch_t		_endPfn = startPfn + nFrames;

	if (nFrames == 0) { return; };

	bmp.lock.acquire();

	for (uarch_t i=startPfn; i<_endPfn; i++) {
		unsetFrame(i);
	};

	// Encourage possible cache hit by trying to reallocate same frames.
	bmp.rsrc.lastAllocIndex =
		(startPfn - basePfn) / __KBIT_NBITS_IN(*bmp.rsrc.bmp);

	bmp.lock.release();
}

void memBmpC::mapRangeUsed(paddr_t basePaddr, uarch_t nFrames)
{
	uarch_t		startPfn = basePaddr / PAGING_BASE_SIZE;
	uarch_t		_endPfn = startPfn + nFrames;

	if (nFrames == 0) { return; };

	bmp.lock.acquire();

	for (uarch_t i=startPfn; i<_endPfn; i++) {
		setFrame(i);
	};

	bmp.lock.release();
}

void memBmpC::mapRangeUnused(paddr_t basePaddr, uarch_t nFrames)
{
	uarch_t		startPfn = basePaddr / PAGING_BASE_SIZE;
	uarch_t		_endPfn = startPfn + nFrames;

	if (nFrames == 0) { return; };

	bmp.lock.acquire();

	for (uarch_t i=startPfn; i<_endPfn; i++) {
		unsetFrame(i);
	};

	bmp.lock.release();
}

