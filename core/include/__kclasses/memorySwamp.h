#ifndef _MEMORY_SWAMP_H
	#define _MEMORY_SWAMP_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/allocClass.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

/**	EXPLANATION:
 * A memorySwamp is essentially the equivalent of what is known as a heap. The
 * Zambezii memory swamp is a large block partitioning allocator for the kernel
 * which does *not* cache object sizes. All it does is take a specific block
 * size, and allocate blocks of memory of that size. It keeps a linked list
 * of all of the blocks it has allocated for that swamp size so far.
 *
 * From there, things are very simple: To allocate, the memorySwamp class
 * will traverse a list of nodes of variable sizes (this is very similar to
 * vSwampC, and was inspired by vSwampC.) until a node of suitable size has been
 * found. This node's size will be decremented, and a pointer to the found
 * memory will be returned.
 *
 * To free, the list is traversed until the freed memory's rightful position is
 * found. The memory is placed into its slot, and linked back into a larger
 * node if necessary, and 
 **/

#ifdef __32_BIT__
	// SWAM ('111' is both a W and an M).
	#define MEMSWAMP_MAGIC			(0x5111A111)
#elif defined (__64_BIT__)
	// SWAMPALLOCS
	#define MEMSWAMP_MAGIC			(0x5111A1111DA110C5)
#endif

#define MEMSWAMP_NO_EXPAND_ON_FAIL		(1<<0)
#define MEMSWAMP				(utf8Char *)"Memory Swamp "

class memorySwampC
:
public allocClassS
{
public:
	error_t initialize(uarch_t swampSize);

public:
	void *allocate(uarch_t nBytes, uarch_t flags=0);
	void free(void *mem);

public:
	uarch_t		blockSize;

private:
	struct freeObjectS
	{
		freeObjectS	*next;
		uarch_t		nBytes;
	};
	struct swampBlockS
	{
		swampBlockS	*next;
		uarch_t		refCount;
		freeObjectS	*firstObject;
	};
	struct allocHeaderS
	{
		uarch_t		nBytes;
		freeObjectS	*parent;
		uarch_t		magic;
	};

	sharedResourceGroupC<waitLockC, swampBlockS *>	head;
};

#endif

