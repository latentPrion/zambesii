
#include <arch/paging.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/memorySwamp.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


error_t memorySwampC::initialize(uarch_t swampSize)
{
	blockSize = swampSize;
};

void *memorySwampC::allocate(uarch_t nBytes, uarch_t flags)
{
	swampBlockS	*blockTmp;
	freeObjectS	*ret=0, *objTmp, *objTmpPrev;

	if (nBytes == 0) {
		return __KNULL;
	};

	// Size of the alloc must accommodate the alloc header.
	nBytes += sizeof(allocHeaderS);

	head.lock.acquire();

	// If swamp is empty, allocate new block.
	if (head.rsrc == __KNULL)
	{
		head.rsrc = getNewBlock();
		if (head.rsrc == __KNULL)
		{
			head.lock.release();
			return __KNULL;
		};
	};

	// head.rsrc now points to a block of usable memory.
	blockTmp = head.rsrc;

	for (; blockTmp != __KNULL; blockTmp = blockTmp->next)
	{
		objTmpPrev = 0;
		objTmp = blockTmp->firstObject;
		for (; objTmp != __KNULL; )
		{
			// If our allocation can come from this node.
			if (objTmp->nBytes >= nBytes)
			{
				// Just give out the whole space in this case.
				if ((objTmp->nBytes - nBytes)
					<= sizeof(freeObjectS))
				{
					nBytes = objTmp->nBytes;
					objTmpPrev->next = objTmp->next;
					ret = objTmp;
					// Make sure to keep head valid.
					if (objTmp == blockTmp->firstObject)
					{
						blockTmp->firstObject =
							objTmp->next;
					};
				}
				else
				{
					objTmp->nBytes -= nBytes;
					ret = reinterpret_cast<freeObjectS *>(
						(uarch_t)objTmp
							+ objTmp->nBytes );
				};
				blockTmp->refCount++;

				head.lock.release();

				static_cast<allocHeaderS *>( ret )->magic =
					MEMSWAMP_MAGIC;

				static_cast<allocHeaderS *>( ret )->nBytes =
					nBytes;

				static_cast<allocHeaderS *>( ret )->parent =
					blockTmp;

				return reinterpret_cast<void *>(
					(uarch_t)ret + sizeof(allocHeaderS) );
			};
			objTmpPrev = objTmp;
			objTmp = objTmp->next;
		};
	};

	// If we reach here, there isn't enough space. Get new pool, try again.
	if (!__KFLAG_TEST(flags, MEMSWAMP_NO_EXPAND_ON_FAIL))
	{
		blockTmp = getNewBlock();
		if (blockTmp == __KNULL)
		{
			head.lock.release();
			return __KNULL;
		};

		blockTmp->next = head.rsrc;
		head.rsrc = blockTmp;
		objTmp = head.rsrc->firstObject;

		if (objTmp->nBytes >= nBytes)
		{
			if (objTmp->nBytes - nBytes <= sizeof(freeObjectS))
			{
				nBytes = objTmp->nBytes;
				ret = objTmp;
			}
			else
			{
				objTmp->nBytes -= nBytes;
				ret = reinterpret_cast<freeObjectS *>(
					(uarch_t)objTmp + objTmp->nBytes );
			};
			blockTmp->refCount++;

			head.lock.release();

			static_cast<allocHeaderS *>( ret )->magic =
				MEMSWAMP_MAGIC;

			static_cast<allocHeaderS *>( ret )->nBytes = nBytes;
			return reinterpret_cast<void *>(
				(uarch_t)ret + sizeof(allocHeaderS );

			static_cast<allocHeaderS *>( ret )->parent = blockTmp;
		};
	};

	return __KNULL;
}

void memorySwampC::free(void *_mem)
{
	allocHeaderS	*mem;
	swampBlockS	*block;
	freeObjectS	*objTmp, *prevObj;
	uarch_t		nBytesTmp;

	/**	EXPLANATION:
	 * We essentially want to first check to make sure the memory hasn't
	 * been corrupted, or the memory isn't a bad free, then as quickly as
	 * possible, locate the insertion point for the object and free.
	 **/

	mem = static_cast<allocHeaderS *>( _mem );
	mem = reinterpret_cast<allocHeaderS *>(
		(uarch_t)mem - sizeof(allocHeaderS) );

	if (mem == __KNULL) {
		return __KNULL;
	};

	if (mem->magic != MEMSWAMP_MAGIC)
	{
		__kprintf(WARNING MEMSWAMP"Corrupt memory or bad free at %X, "
			"magic was %X.\n", mem, mem->magic);

		return;
	};

	head.lock.acquire();

	block = mem->parent;
	// If the mem to be freed is logically before the first object pointer.
	if (mem < block->firstObject || block->firstObject == __KNULL)
	{
		nBytesTmp = mem->nBytes;
		static_cast<freeObjectS *>( mem )->next = block->firstObject;
		block->firstObject = static_cast<freeObjectS *>( mem );
		static_cast<freeObjectS *>( mem )->nBytes = nBytesTmp;

		// Check to see if we can concatenate two objects.
		if (block->firstObject->next != __KNULL)
		{
			if (((uarch_t)block->firstObject
				+ block->firstObject->nBytes)
				== (uarch_t)block->firstObject->next)
			{
				block->firstObject->nBytes +=
					block->firstObject->next->nBytes;

				block->firstObject->next =
					block->firstObject->next->next;
			};
		};
		return;
	};

	objTmp = block->firstObject;
	for (; objTmp != __KNULL; )
	{
		
memorySwampC::swampBlockS *memorySwampC::getNewBlock(void)
{
	swampBlockS		*ret;

	ret = new ((memoryTrib.__kmemoryStream.*
		memoryTrib.__kmemoryStream.memAlloc)(
			PAGING_BYTES_TO_PAGES(
				blockSize + sizeof(swampBlockS)), 0))
		swampBlockS;

	if (ret == __KNULL) {
		return __KNULL;
	};

	ret->next = __KNULL;
	ret->refCount = 0;
	ret->firstObject = reinterpret_cast<freeObjectS *>(
		reinterpret_cast<uarch_t>( ret )
			+ sizeof(swampBlockS) );

	ret->firstObject->size = blockSize;
	ret->firstObject->next = __KNULL;

	return ret;
}

