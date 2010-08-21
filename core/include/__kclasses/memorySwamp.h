#ifndef _MEMORY_SWAMP_H
	#define _MEMORY_SWAMP_H

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

class memorySwampC
:
public allocClassS
{
public:
	error_t initialize(uarch_t swampSize);

public:
	void *allocate(uarch_t nBytes);
	void free(void *mem);

public:
	uarch_t		blockSize;

private:
	struct swampBlockS
	{
		swampBlockS	*next;
		uarch_t	refCount;
	};
	struct freeObjectS
	{
		freeObjectS	*next;
		uarch_t		nBytes;
	};

	sharedResourceGroupC<waitLockC, swampBlockS *>	head;
};

#endif

