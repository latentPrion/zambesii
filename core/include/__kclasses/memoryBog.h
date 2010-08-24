#ifndef _MEMORY_BOG_H
	#define _MEMORY_BOG_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/allocClass.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

/**	EXPLANATION:
 * A memoryBog is essentially the equivalent of what is known as a heap. The
 * Zambezii memory bog is a large block partitioning allocator for the kernel
 * which does *not* cache object sizes. All it does is take a specific block
 * size, and allocate blocks of memory of that size. It keeps a linked list
 * of all of the blocks it has allocated for that bog size so far.
 *
 * From there, things are very simple: To allocate, the memoryBog class
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
	#define MEMBOG_MAGIC			(0x5111A111)
#elif defined (__64_BIT__)
	// SWAMPALLOCS
	#define MEMBOG_MAGIC			(0x5111A1111DA110C5)
#endif

#define MEMBOG_NO_EXPAND_ON_FAIL	(1<<0)
#define MEMBOG				"Memory Bog: "

class memoryBogC
:
public allocClassC
{
public:
	memoryBogC(uarch_t bogSize);
	error_t initialize(void);
	~memoryBogC(void);

public:
	void *allocate(uarch_t nBytes, uarch_t flags=0);
	void free(void *mem);

	void dump(void);

public:
	uarch_t		blockSize;

private:
	struct freeObjectS
	{
		freeObjectS	*next;
		uarch_t		nBytes;
	};
	struct bogBlockS
	{
		bogBlockS	*next;
		uarch_t		refCount;
		freeObjectS	*firstObject;
	};
	struct allocHeaderS
	{
		uarch_t		nBytes;
		bogBlockS	*parent;
		uarch_t		magic;
	};

	bogBlockS *getNewBlock(void);

	sharedResourceGroupC<waitLockC, bogBlockS *>	head;
};

#endif

