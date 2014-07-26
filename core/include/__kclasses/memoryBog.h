#ifndef _MEMORY_BOG_H
	#define _MEMORY_BOG_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/allocClass.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

/**	EXPLANATION:
 * A memoryBog is essentially the equivalent of what is known as a heap. The
 * Zambesii memory bog is a large block partitioning allocator for the kernel
 * which does *not* cache object sizes. All it does is take a specific block
 * size, and allocate blocks of memory of that size. It keeps a linked list
 * of all of the blocks it has allocated for that bog size so far.
 *
 * From there, things are very simple: To allocate, the memoryBog class
 * will traverse a list of nodes of variable sizes (this is very similar to
 * VSwamp, and was inspired by VSwamp.) until a node of suitable size has been
 * found. This node's size will be decremented, and a pointer to the found
 * memory will be returned.
 *
 * To free, the list is traversed until the freed memory's rightful position is
 * found. The memory is placed into its slot, and linked back into a larger
 * node if necessary, and
 **/

#define MEMBOG_MAGIC			(CC"##Zambesii Memory Bog Allocation.##")

#define MEMBOG_NO_EXPAND_ON_FAIL	(1<<0)
#define MEMBOG				"Memory Bog: "

class MemReservoir;
class MemoryStream;

class MemoryBog
:
public AllocatorBase
{
friend class MemReservoir;
public:
	MemoryBog(uarch_t bogSize, MemoryStream *sourceStream);
	error_t initialize(void);
	~MemoryBog(void);

public:
	void *allocate(uarch_t nBytes, uarch_t flags=0);
	void free(void *mem);

	static void moveHeaderDown(void *hdr, uarch_t nBytes);
	static void moveHeaderUp(void *hdr, uarch_t nBytes);

	error_t checkAllocations(sarch_t nBytes);
	void dump(void);

public:
	uarch_t		blockSize;

private:
	struct freeObjectS
	{
		freeObjectS	*next;
		uarch_t		nBytes;
		// Address offset of the function that freed the block.
		void		*freedBy;
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
		utf8Char	magic[48];
		// Address offset of the function that allocated the block.
		void		*allocatedBy;
	};

	bogBlockS *getNewBlock(void);

	MemoryStream		*sourceStream;
	SharedResourceGroup<WaitLock, bogBlockS *>	head;
};

#endif

