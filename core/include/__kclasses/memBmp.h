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

#define MEMBMP_FLAGS_DYNAMIC		(1<<0)

#define MEMBMP_OFFSET(__pfn,__basePfn)	(__pfn - __basePfn)

#define MEMBMP_INDEX(__pfn,__basePfn,__bmp)		\
	(MEMBMP_OFFSET(__pfn,__basePfn) / __KBIT_NBITS_IN(__bmp))

#define MEMBMP_BIT(__pfn,__basePfn,__bmp)			\
	(MEMBMP_OFFSET(__pfn,__basePfn) % __KBIT_NBITS_IN(__bmp))

class numaMemoryBankC;

class memBmpC
:
public allocClassC
{
friend class numaMemoryBankC;

public:
	memBmpC(paddr_t baseAddr, paddr_t size);
	error_t initialize(void *preAllocated=__KNULL);

	~memBmpC(void);

public:
	// The frame address is returned on the stack.
	error_t contiguousGetFrames(uarch_t nFrames, paddr_t *paddr);
	status_t fragmentedGetFrames(uarch_t nFrames, paddr_t *paddr);
	void releaseFrames(paddr_t frameAddr, uarch_t nFrames);

	void mapMemUsed(paddr_t basePaddr, uarch_t nFrames);
	void mapMemUnused(paddr_t basePaddr, uarch_t nFrames);

public:
	inline void setFrame(uarch_t pfn);
	inline void unsetFrame(uarch_t pfn);
	inline sarch_t testFrame(uarch_t pfn);

public:
	uarch_t		basePfn, endPfn, bmpNFrames, flags, nIndexes;
	uarch_t		bmpSize;
	paddr_t		baseAddr, endAddr;

	struct bmpPtrsS
	{
		uarch_t		*bmp, lastAllocIndex;
	};
	sharedResourceGroupC<waitLockC, bmpPtrsS>	bmp;
};


/**	Inline Methods
 ******************************************************************************/

inline void memBmpC::setFrame(uarch_t pfn)
{
	__KBIT_SET(
		bmp.rsrc.bmp[ MEMBMP_INDEX(pfn, basePfn, *bmp.rsrc.bmp) ],
		MEMBMP_BIT(pfn, basePfn, *bmp.rsrc.bmp));
}

inline void memBmpC::unsetFrame(uarch_t pfn)
{	
	__KBIT_UNSET(
		bmp.rsrc.bmp[ MEMBMP_INDEX(pfn, basePfn, *bmp.rsrc.bmp) ],
		MEMBMP_BIT(pfn, basePfn, *bmp.rsrc.bmp));
}

inline sarch_t memBmpC::testFrame(uarch_t pfn)
{
	return __KBIT_TEST(
		bmp.rsrc.bmp[ MEMBMP_INDEX(pfn, basePfn, *bmp.rsrc.bmp) ],
		MEMBMP_BIT(pfn, basePfn, *bmp.rsrc.bmp));
}

#endif

