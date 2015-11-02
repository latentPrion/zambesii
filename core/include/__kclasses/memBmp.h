#ifndef _MEM_BMP_H
	#define _MEM_BMP_H

	#include <__kstdlib/__kbitManipulation.h>
	#include <__kclasses/allocClass.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/sharedResourceGroup.h>

/**	EXPLANATION:
 * Simple bitmap, 1/0 for used/free. Will soon be changed into some sort of
 * highly efficient doubled layered setup. This combined with the frame caching
 * that accompanies each bitmap in its NUMA Bank wrapper will make most frame
 * allocations *very* fast.
 **/

#define MEMBMP				"MemBMP: "

#define MEMBMP_FLAGS_DYNAMIC		(1<<0)

inline uarch_t MEMBMP_OFFSET(uarch_t _pfn, uarch_t _basePfn)
	{ return _pfn - _basePfn; }

template <class T>
inline uarch_t MEMBMP_INDEX(uarch_t _pfn, uarch_t _basePfn, T _bmp)
	{ return MEMBMP_OFFSET(_pfn, _basePfn) / __KBIT_NBITS_IN(_bmp); }

/*#define MEMBMP_INDEX(__pfn,__basePfn,__bmp)		\
	(MEMBMP_OFFSET((__pfn),(__basePfn)) / __KBIT_NBITS_IN((__bmp)))*/

template <class T>
inline uarch_t MEMBMP_BIT(uarch_t _pfn, uarch_t _basePfn, T _bmp)
	{ return MEMBMP_OFFSET(_pfn, _basePfn) % __KBIT_NBITS_IN(_bmp); }

/*#define MEMBMP_BIT(__pfn,__basePfn,__bmp)			\
	(MEMBMP_OFFSET((__pfn),(__basePfn)) % __KBIT_NBITS_IN((__bmp))) */

class NumaMemoryBank;

class MemoryBmp
:
public AllocatorBase
{
friend class NumaMemoryBank;

public:
	MemoryBmp(paddr_t baseAddr, paddr_t size);
	error_t initialize(void *preAllocated=NULL);

	~MemoryBmp(void);

	void dump(void);

public:
	// The frame address is returned on the stack.
	error_t contiguousGetFrames(uarch_t nFrames, paddr_t *paddr);
	status_t fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr);
	void releaseFrames(paddr_t frameAddr, uarch_t nFrames);

	void mapMemUsed(paddr_t basePaddr, uarch_t nFrames);
	void mapMemUnused(paddr_t basePaddr, uarch_t nFrames);

	/* Does a big OR operation on this bmp, such that "this" |= bmp.
	 * Useful for the PMM, when transitioning from __kspace into NUMA Trib
	 * detected memory.
	 *
	 * Returns the number of bits that were set in "this" bmp due to the OR
	 * with the other one.
	 **/
	status_t merge(MemoryBmp *bmp);

public:
	inline void setFrame(uarch_t pfn);
	inline void unsetFrame(uarch_t pfn);
	inline sbit8 testFrame(uarch_t pfn);

public:
	uarch_t		basePfn, endPfn, bmpNFrames, flags, nIndexes;
	uarch_t		bmpSize;
	paddr_t		baseAddr, endAddr;

	struct sBmpState
	{
		uarch_t		*bmp, lastAllocIndex;
	};
	SharedResourceGroup<WaitLock, sBmpState>	bmp;
};


/**	Inline Methods
 ******************************************************************************/

inline void MemoryBmp::setFrame(uarch_t pfn)
{
	__KBIT_SET(
		bmp.rsrc.bmp[ MEMBMP_INDEX(pfn, basePfn, *bmp.rsrc.bmp) ],
		MEMBMP_BIT(pfn, basePfn, *bmp.rsrc.bmp));
}

inline void MemoryBmp::unsetFrame(uarch_t pfn)
{
	__KBIT_UNSET(
		bmp.rsrc.bmp[ MEMBMP_INDEX(pfn, basePfn, *bmp.rsrc.bmp) ],
		MEMBMP_BIT(pfn, basePfn, *bmp.rsrc.bmp));
}

inline sbit8 MemoryBmp::testFrame(uarch_t pfn)
{
	return __KBIT_TEST(
		bmp.rsrc.bmp[ MEMBMP_INDEX(pfn, basePfn, *bmp.rsrc.bmp) ],
		MEMBMP_BIT(pfn, basePfn, *bmp.rsrc.bmp));
}

#endif

