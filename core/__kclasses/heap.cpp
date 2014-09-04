
#include <debug.h>
#include <arch/debug.h>
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/heap.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/process.h>


uintptr_t Heap::pageSize=0;

error_t Heap::initialize(void)
{
	error_t		ret;

#ifdef __ZAMBESII_KERNEL_SOURCE__
	pageSize = PAGING_BASE_SIZE;
#else
	pageSize = 4096;
#endif

	ret = allocationList.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
	return chunkList.initialize();
}

sbit8 Heap::checkGuardPage(void *)
{
	return 1;
}

error_t Heap::setGuardPage(void *vaddr)
{
#ifndef __ZAMBESII_KERNEL_SOURCE__
	// Map it unaccessible.
	if (mprotect(vaddr, pageSize, PROT_NONE) != 0)
		{ return ERROR_UNKNOWN; };

	return ERROR_SUCCESS;
#else
	/* We will do it like this:
	 *	1. WPRanger::lookup().
	 *	2a. If the page is FAKE_DYN, we map it HEAP_GUARDPAGE.
	 * 	2b. If the page is mapped, we free the frame and then
	 * 	    map it HEAP_GUARDPAGE.
	 **/
	status_t		status;
	paddr_t			p;
	uarch_t			f;

	status = walkerPageRanger::lookup(
		&memoryStream->parent->getVaddrSpaceStream()->vaddrSpace,
		vaddr, &p, &f);

	// Free the frame so we don't leak pmem all day.
	if (status == WPRANGER_STATUS_BACKED) {
		memoryTrib.releaseFrames(p, 1);
	};

	walkerPageRanger::remapInc(
		&memoryStream->parent->getVaddrSpaceStream()->vaddrSpace,
		vaddr,
		PAGESTATUS_HEAP_GUARDPAGE << PAGING_PAGESTATUS_SHIFT,
		1,
		// Just clear only the present bit.
		WPRANGER_OP_CLEAR,
		PAGEATTRIB_PRESENT);

	status = walkerPageRanger::lookup(
		&memoryStream->parent->getVaddrSpaceStream()->vaddrSpace,
		vaddr, &p, &f);

	if (status != WPRANGER_STATUS_HEAP_GUARDPAGE
		|| f & PAGEATTRIB_PRESENT)
	{
		printf(FATAL HEAP"Gpage: 0x%p; flags 0x%x, paddr bits 0x%P\n",
			vaddr, f, p);

		panic(FATAL HEAP"Failed to protect guardpage.\n");
	};

	return ERROR_SUCCESS;
#endif
}

error_t Heap::unsetGuardPage(void *vaddr)
{
#ifndef __ZAMBESII_KERNEL_SOURCE__
	// Unprotect the allocation.
	if (mprotect(vaddr, pageSize, PROT_READ | PROT_WRITE) != 0)
		{ return ERROR_UNKNOWN; };

	return ERROR_SUCCESS;
#else
	// Just unconditionally map the page FAKEMAPPED_DYNAMIC.
	walkerPageRanger::remapInc(
		&memoryStream->parent->getVaddrSpaceStream()->vaddrSpace,
		vaddr,
		PAGESTATUS_FAKEMAPPED_DYNAMIC << PAGING_PAGESTATUS_SHIFT,
		1,
		// Do nothing with the flag bits.
		0, 0);

	status_t	status;
	uarch_t		f;
	paddr_t		p;

	status = walkerPageRanger::lookup(
		&memoryStream->parent->getVaddrSpaceStream()->vaddrSpace,
		vaddr, &p, &f);

	// Verify that we unprotected the guardpage.
	if (status != WPRANGER_STATUS_FAKEMAPPED_DYNAMIC)
	{
		printf(FATAL HEAP"unsetGuardPage 0x%p: lookup didn't return "
			"FAKE_DYN status value. Status %d.\n",
			vaddr, status);

		panic(FATAL HEAP"Failed to unprotect heap guardpage\n");
	};

	return ERROR_SUCCESS;
#endif
}

Heap::Allocation *Heap::Chunk::malloc(
	Heap *heap, size_t sz, void *allocatedBy, utf8Char *desc
	)
{
	List<Block>::Iterator		it;
	Block					*currBlock;
	Allocation				*ret;
	uintptr_t				gapRequirement;

	/**	EXPLANATION:
	 * Iterate through all the blocks, and if any is able to satisfy
	 * the allocation, we will use it.
	 **/
	if (heap->options & Heap::OPT_GUARD_PAGED)
	{
		gapRequirement = sz & (heap->pageSize - 1);
		gapRequirement = heap->pageSize - gapRequirement;
	} else { gapRequirement = 0; };

	for (it=blocks.begin(); it != blocks.end(); ++it)
	{
		currBlock = *it;

		if (heap->options & Heap::OPT_GUARD_PAGED
			&& (uintptr_t)currBlock & (heap->pageSize - 1))
		{
			printf(ERROR HEAP"Chunk::malloc: found non-page "
				"aligned block, with GUARD_PAGED set\n");

			panic(ERROR_UNKNOWN);
		};

		if (currBlock->parent != this)
		{
			printf(ERROR HEAP"Chunk::malloc: corrupt block. Invalid parent.\n");
			panic(ERROR_UNKNOWN);
		};

		if (heap->options & Heap::OPT_CHECK_BLOCK_MAGIC_PASSIVELY
			&& !currBlock->magicIsValid())
		{
			printf(ERROR HEAP"Chunk::malloc: Corrupt block. Invalid magic.\n");
			panic(ERROR_UNKNOWN);
		};

		// If the block cannot satisfy the allocation, move on.
		if (gapRequirement + sz > currBlock->nBytes) { continue; };

		/* Construct a block header at the new block header
		 * location, and construct a new alloc header at the
		 * current block hdr loc'tn.
		 *
		 * If alloc request will consume the whole block, just
		 * give the whole block away.
		 **/

		if (currBlock->nBytes - sizeof(Block) <= gapRequirement + sz)
		{
			/* If the allocation can occupy this whole block,
			 * we just give away the whole block; add the
			 * excess bytes from the block header to the gap
			 * requirement.
			 **/
			gapRequirement +=
				currBlock->nBytes - (gapRequirement + sz);

			// May deadlock, not sure.
			blocks.remove(currBlock);
		}
		else
		{
			ubit8			*newBlockDest;
			Block			*newBlock;

			newBlockDest = (ubit8 *)currBlock
				+ (gapRequirement + sz);

			if (heap->options & OPT_GUARD_PAGED
				&& (uintptr_t)newBlockDest
					& (heap->pageSize - 1))
			{
				printf(ERROR HEAP"Chunk::malloc: GUARD_PAGED "
					"set, but for alloc of %dB, "
					"after alloc, new block destination "
					"0x%p is not page aligned\n",
					sz, (void*)newBlockDest);

				panic(ERROR_UNKNOWN);
			};

			newBlock = new (newBlockDest) Block(
				this, currBlock->nBytes - gapRequirement - sz,
				currBlock->freedBy);

			blocks.remove(currBlock);
			blocks.sortedInsert(newBlock);
		};

		ret = new ((ubit8 *)currBlock + gapRequirement)
			Allocation(
				this, gapRequirement, sz,
				allocatedBy, desc);

		nAllocations++;
		return ret;
	};

	return NULL;
}

void Heap::Chunk::appendAndCoalesce(Block *block, Allocation *alloc)
{
	ubit8			*blockEnd;
	Block			*nextBlock;

	// Just need to add the allocation's bytes to the target block's.
	block->nBytes += alloc->gapSize + alloc->nBytes;
	alloc->~Allocation();

	// Now coalesce, if possible.
	nextBlock = block->listHeader.getNextItem();
	if (nextBlock == NULL) { return; };

	blockEnd = (ubit8 *)block + (block->nBytes - 1);

	// Return early if no need to coalesce.
	if (blockEnd + 1 != (void *)nextBlock) { return; };

	block->nBytes += nextBlock->nBytes;
	blocks.remove(nextBlock);
	nextBlock->~Block();
}

void Heap::Chunk::prependAndCoalesce(
	Block *block, Block *prevBlock, Allocation *alloc,
	void *freedBy
	)
{
	Block		*newBlock;
	ubit8		*prevBlockEnd;

	// Construct new block header where allocation currently is.
	newBlock = new ((ubit8 *)alloc - alloc->gapSize) Block(
		this,
		alloc->gapSize + alloc->nBytes + block->nBytes,
		freedBy);

	blocks.remove(block);
	block->~Block();

	/* Return early if prepending to first block in chunk, because
	 * there is no previous block in that case. We do need to modify
	 * the block list head pointer state though.
	 **/
	if (prevBlock == NULL)
	{
		blocks.sortedInsert(newBlock);
		return;
	};

	/* If it is not the first block in the list, we need to check
	 * to see if we can coalesce the new block into its predecessor.
	 **/
	prevBlockEnd = (ubit8 *)prevBlock + (prevBlock->nBytes - 1);

	// Exit early if there is no need to coalesce.
	if (prevBlockEnd + 1 != (ubit8 *)newBlock)
	{
		blocks.sortedInsert(newBlock);
		return;
	};

	// Else, coalesce and exit.
	prevBlock->nBytes += newBlock->nBytes;
	newBlock->~Block();
}

void Heap::Chunk::free(Allocation *alloc, void *freedBy)
{
	List<Block>::Iterator		it, prev;
	ubit8					*allocEnd;

	/**	EXPLANATION:
	 * Iterate through all the blocks. If the allocation can be
	 * appended to another block, do that. Else, create a new block.
	 **/
	allocEnd = (ubit8 *)alloc + (alloc->nBytes - 1);

	for (it=blocks.begin(), prev=blocks.end();
		it != blocks.end();
		prev=it, ++it)
	{
		Block		*currBlock, *prevBlock;
		ubit8		*currBlockEnd, *prevBlockEnd=NULL;

		currBlock = *it;
		prevBlock = *prev;
		currBlockEnd = (ubit8 *)currBlock + (currBlock->nBytes - 1);
		if (prev != blocks.end())
		{
			prevBlockEnd = (ubit8 *)prevBlock
				+ (prevBlock->nBytes - 1);
		};

		// If the alloc can be prepended to the current block:
		if (allocEnd + 1 == (ubit8 *)currBlock)
		{
			prependAndCoalesce(
				currBlock, prevBlock, alloc, freedBy);

			nAllocations--;
			return;
		}
		// Else if the alloc can be appended to current block:
		else if (currBlockEnd + 1 == (ubit8 *)alloc - alloc->gapSize)
		{
			appendAndCoalesce(currBlock, alloc);
			nAllocations--;
			return;
		};

		/* Else, if the alloc can't be prepended/appended to
		 * this block, check to see if it can be fit between
		 * this block and its predecessor.
		 *
		 * i.e., if this is the correct insertion point for the
		 * free() operation.
		 **/
		if ((prev == blocks.end() && (void *)alloc < currBlock)
			|| ((void*)alloc > prevBlockEnd && (void*)alloc < currBlock))
			{ break; };
	};

	/* Else just construct a new block header at the alloc location,
	 * and make sure to set the preceding block to point to it.
	 **/
	Block			*newBlock;

	newBlock = new ((ubit8 *)alloc - alloc->gapSize)
		Block(this, alloc->gapSize + alloc->nBytes, freedBy);

	blocks.sortedInsert(newBlock);
	nAllocations--;
	return;
}

error_t Heap::getNewChunk(Chunk **retchunk) const
{
	error_t		ret;
	Block		*firstBlock;
	Chunk		*tmpchunk;

	if (memoryStream == NULL) { return ERROR_UNINITIALIZED; };

	tmpchunk = new (
#ifndef __ZAMBESII_KERNEL_SOURCE__
		mmap(
			NULL, chunkSize, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0))
#else
		memoryStream->memAlloc(
			PAGING_BYTES_TO_PAGES(
				chunkSize),
			MEMALLOC_PURE_VIRTUAL))
#endif
		Chunk;

	if (tmpchunk == NULL)
	{
		printf(WARNING HEAP"getNewChunk: Failed to get vmem chunk.\n");
		return ERROR_MEMORY_NOMEM;
	};

	ret = tmpchunk->initialize();
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR HEAP"getNewChunk: chunk::initialize() failed.\n");
		releaseChunk(tmpchunk);
		return ret;
	};

	firstBlock = (Block *)&tmpchunk[1];
	if (options & OPT_GUARD_PAGED)
	{
		uintptr_t		tmpBlock;

		tmpBlock = (uintptr_t)firstBlock;
		if (tmpBlock & (pageSize - 1))
		{
			tmpBlock += pageSize;
			tmpBlock &= ~(pageSize - 1);
		};

		firstBlock = (Block *)tmpBlock;
	};

	/* We have to subtract the size of the chunk header from the
	 * first block's header.
	 **/
	new (firstBlock) Block(
		tmpchunk,
		chunkSize - ((ubit8*)firstBlock - (ubit8*)tmpchunk),
		NULL);

	tmpchunk->blocks.sortedInsert(firstBlock);
	*retchunk = tmpchunk;
printf(NOTICE"getNewChunk: chunk 0x%p: first block @0x%p, %d bytes.\n",
	(void*)tmpchunk, (void*)firstBlock, firstBlock->nBytes);
	return ERROR_SUCCESS;
}

void Heap::releaseChunk(Chunk *chunk) const
{
	chunk->~Chunk();

#ifndef __ZAMBESII_KERNEL_SOURCE__
	munmap(chunk, chunkSize);
#else
	memoryStream->memFree(chunk);
#endif
}

void *Heap::malloc(size_t sz, void *allocatedBy, utf8Char *desc)
{
	size_t					origSz=sz;
	sbit8					alignmentRequired=0;
	error_t					err;
	List<Chunk>::Iterator			it;
	Chunk					*currChunk;
	Allocation				*ret;

	(void)origSz;
	(void)alignmentRequired;

	if (sz == 0)
	{
		printf(WARNING HEAP"malloc: 0 sized alloc from caller 0x%p.\n",
			allocatedBy);

		panic(ERROR_UNKNOWN);
	};

	sz += sizeof(Allocation);

	if (options & OPT_GUARD_PAGED)
	{
		/**	EXPLANATION:
		 * For GUARDPAGED mode, we add on an unaccessible page
		 * of memory at the end of each allocation. If the
		 * owner of the allocated memory overflows the
		 * boundaries, they will trigger an OS-enforced access
		 * violation.
		 **/
		sz += pageSize;
	};

	// If "sz" isn't native-word aligned, align it.
	if (sz & (sizeof(void *) - 1))
	{
		alignmentRequired = 1;
		sz += sizeof(void *);
		sz &= ~(sizeof(void *) - 1);
	};

	chunkList.lock();
	for (sbit8 pass=1; pass <= 2; pass++)
	{
		for (it=chunkList.begin(); it != chunkList.end(); ++it)
		{
			currChunk = *it;

			if (!currChunk->magicIsValid())
			{
				printf(FATAL HEAP"malloc: Found chunk with invalid magic.\n");
				chunkList.unlock();
				panic(ERROR_UNKNOWN);
			};

			ret = currChunk->malloc(
				this, sz, allocatedBy, desc);

			if (ret == NULL)
			{
				printf(NOTICE HEAP"malloc: chunk 0x%p: not enough for %d "
					"request\n",
					(void*)currChunk, sz);

				continue;
			};

			// Unlock early so the mapping call is unlocked.
			chunkList.unlock();

			if (options & OPT_GUARD_PAGED)
			{
				ubit8		*guardPageStart;

				guardPageStart = (ubit8 *)ret
					+ ret->nBytes - pageSize;

				if (setGuardPage(guardPageStart)
					!= ERROR_SUCCESS)
				{
					printf(ERROR HEAP"malloc: Failed to "
						"protect alloc 0x%p, %dB, "
						"gpage @0x%p!\n",
						(void*)ret, ret->nBytes,
						(void*)guardPageStart);

					return NULL;
				};
			};

			// Now add the new alloc to allocation list.
			allocationList.insert(ret);
			// Return the allocated memory.
			return &ret[1];
		};

		// If no chunks had enough memory, get a new chunk.
		err = getNewChunk(&currChunk);
		if (err != ERROR_SUCCESS)
		{
			chunkList.unlock();
			return NULL;
		};

		// Add the new chunk to the chunks list and retry.
		chunkList.insert(
			currChunk,
			List<Chunk>::OP_FLAGS_UNLOCKED);
	};

	// If both passes yielded nothing, return NULL.
	chunkList.unlock();
	return NULL;
}

void *Heap::realloc(void *p, size_t sz)
{
	Allocation			*alloc = (Allocation *)p;
	void				*newmem;

	/**	EXPLANATION:
	 * Simplest implementation is to check to see if the memory at "p" is
	 * too small, and if so, reallocate. Else do nothing and return "p".
	 *
	 * More complex implementation is to attempt to resize "p".
	 **/
	if (alloc != NULL)
	{
		alloc--;
		if (!alloc->magicIsValid())
		{
			printf(FATAL HEAP"realloc(0x%p, sz): magic invalid. Aborting."
				"\n", p, sz);

			panic(ERROR_FATAL);
		};

		if (options & OPT_CHECK_ALLOCS_ON_FREE)
		{
			// Is the alloc in the alloc list?
			if (!allocationList.find(alloc))
			{
				printf(NOTICE HEAP"realloc(0x%p, sz): mem pointer 0x%p "
					"(hdr 0x%p) not in alloc list!\n",
					p, sz, p, (void*)alloc);

				panic(ERROR_UNKNOWN);
			};
		};

		// If the alloc can handle the requested size, just return it.
		if (alloc->nBytes - sizeof(Allocation) - pageSize >= sz) {
			return p;
		};
	};

	/* If sz == 0, the memory at "p" is freed and the rest of the behaviour
	 * is implementation defined.
	 *
	 * We choose to return NULL, because allocating 0 bytes is always
	 * indicative of bad logic or a bug.
	 **/
	if (sz == 0) { free(p, __builtin_return_address(0)); return NULL; };

	// Allocate a new block of memory.
	newmem = malloc(sz, __builtin_return_address(0));
	if (newmem == NULL) { return NULL; };

	// Copy the contents of the old block into the new block.
	if (alloc != NULL)
	{
		memcpy(newmem, p, alloc->nBytes - sizeof(Allocation) - pageSize);
		free(p, __builtin_return_address(0));
	};

	return newmem;
}

void Heap::free(void *_p, void *freedBy)
{
	Allocation				*alloc=(Allocation *)_p;
	List<Chunk>::Iterator			chunkIt;
	sbit8					isWithinHeap=0;

	// Move backward in memory to get to the start of the header.
	alloc--;

	if (options & OPT_GUARD_PAGED)
	{
		uintptr_t		allocStart, allocEnd;

		allocStart = (uintptr_t)alloc - alloc->gapSize;
		if (allocStart & (pageSize - 1))
		{
			printf(WARNING HEAP"free: Alloc @0x%p, gapSize "
				"%d, true start 0x%p, caller 0x%p: not page aligned, "
				"but GUARD_PAGED is set\n",
				(void*)alloc, alloc->gapSize,
				(void*)allocStart, freedBy);

			panic(ERROR_UNKNOWN);
		};

		allocEnd = (uintptr_t)alloc + alloc->nBytes - 1;
		// If the alloc doesn't end just before a page boundary:
		if ((allocEnd + 1) & (pageSize - 1))
		{
			printf(ERROR HEAP"free: alloc @0x%p, size %dB "
				"doesn't end on a page boundary, but "
				"GUARD_PAGED set\n",
				(void*)alloc, alloc->nBytes);

			panic(ERROR_UNKNOWN);
		}
	};

	// First make sure allocation is within the heap's chunk ranges.
	if (options & OPT_CHECK_HEAP_RANGES_ON_FREE)
	{
		// Only need to lock the chunkList here.
		chunkList.lock();
		isWithinHeap = allocIsWithinHeap(alloc);
		chunkList.unlock();

		if (!isWithinHeap)
		{
			printf(WARNING HEAP"free: ptr 0x%p not within "
				"heap!\n", _p);

			panic(ERROR_UNKNOWN);
		};
	};

	if (!alloc->magicIsValid())
	{
		printf(NOTICE HEAP"free: Invalid alloc magic.\n");
		panic(ERROR_UNKNOWN);
	};

	if (alloc->nBytes == 0)
	{
		printf(NOTICE HEAP"free: 0 sized alloc!\n");
		panic(ERROR_UNKNOWN);
	};

	if (options & OPT_CHECK_ALLOCS_ON_FREE)
	{
		// Is the alloc in the alloc list?
		if (!allocationList.find(alloc))
		{
			void				*ebp;
			debug::sStackDescriptor		currStack;

			debug::getCurrentStackInfo(&currStack);
			asm volatile(
				"movl	%%ebp, %0\n\t"
				: "=r" (ebp));

			printf(NOTICE HEAP"free: mem pointer 0x%p "
				"(hdr 0x%p) not in alloc list!\n"
				"\tFree called for by 0x%p\n",
				_p, (void*)alloc, freedBy);

			//debug::printStackTrace(ebp, &currStack);
			panic(ERROR_UNKNOWN);
		};
	};

	if (!chunkList.find(alloc->parent))
	{
		printf(NOTICE HEAP"free: mem ptr 0x%p (hdr 0x%p) has invalid "
			"parent chunk 0x%p!\n",
			_p, (void*)alloc, (void*)alloc->parent);

		panic(ERROR_UNKNOWN);
	};

	allocationList.remove(alloc);

	if (options & OPT_GUARD_PAGED)
	{
		ubit8		*guardPageStart;

		guardPageStart = (ubit8 *)alloc
			+ alloc->nBytes - pageSize;

		if (unsetGuardPage(guardPageStart) != ERROR_SUCCESS)
		{
			printf(ERROR HEAP"free: failed to unprotect alloc: "
				"0x%p, %dB, gpage @0x%p!\n",
				(void*)alloc, alloc->nBytes,
				(void*)guardPageStart);

			return;
		};
	};

	chunkList.lock();
	alloc->parent->free(alloc, freedBy);
	chunkList.unlock();
}

void Heap::dumpChunks(ubit32 flags)
{
	List<Chunk>::Iterator		it;

	printf(NOTICE"dumpChunks: %u chunks, chunk size %u, dumping chunks.\n",
		chunkList.getNItems(List<Chunk>::OP_FLAGS_UNLOCKED),
		chunkSize);

	if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { chunkList.lock(); };

	for (it = chunkList.begin(); it != chunkList.end(); ++it)
	{
		Chunk		*currChunk;

		currChunk = *it;
		currChunk->dump();
	};

	if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { chunkList.unlock(); };
}

void Heap::dumpAllocations(ubit32 flags)
{
	List<Allocation>::Iterator	it;

	printf(NOTICE"dumpAllocs: %u allocs, spanning %u chunks\n",
		allocationList.getNItems(List<Allocation>::OP_FLAGS_UNLOCKED),
		chunkList.getNItems(List<Chunk>::OP_FLAGS_UNLOCKED));

	if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { allocationList.lock(); };

	for (it=allocationList.begin(); it != allocationList.end(); ++it)
	{
		Allocation		*currAlloc;

		currAlloc = *it;
		currAlloc->dump();
	};

	if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { allocationList.unlock(); };
}

void Heap::dumpBlocks(ubit32 flags)
{
	List<Chunk>::Iterator		it;

	printf(NOTICE"dumpBlocks: spanning %u chunks\n",
		chunkList.getNItems(List<Chunk>::OP_FLAGS_UNLOCKED));

	if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { chunkList.lock(); };

	for (it=chunkList.begin(); it != chunkList.end(); ++it)
	{
		Chunk					*currChunk;
		List<Block>::Iterator		it2;

		currChunk = *it;
		for (it2=currChunk->blocks.begin();
			it2 != currChunk->blocks.end(); ++it2)
		{
			Block		*currBlock;

			currBlock = *it2;
			currBlock->dump();
		};
	};

	if (!FLAG_TEST(flags, OP_FLAGS_UNLOCKED)) { chunkList.unlock(); };
}

sbit8 Heap::allocIsWithinHeap(void *alloc, Chunk **parentChunk)
{
	List<Chunk>::Iterator		iChunks;

	iChunks = chunkList.begin();
	for (; iChunks != chunkList.end(); ++iChunks)
	{
		Chunk		*chunk;
		ubit8		*chunkEof;

		chunk = *iChunks;

		chunkEof = (ubit8 *)chunk + chunkSize;
		if (alloc < chunk || alloc >= chunkEof) { continue; };
		if (parentChunk != NULL) { *parentChunk = chunk; };
		return 1;
	};

	return 0;
}

error_t Heap::checkBlocks(void)
{
	List<Chunk>::Iterator		iChunks;

	class LocalAutoUnlocker
	{
		List<Chunk>		*list;

	public:
		~LocalAutoUnlocker(void)
		{
			list->unlock();
		}
		LocalAutoUnlocker(List<Chunk> *list)
		:
		list(list)
		{}
	} localAutoUnlocker(&chunkList);

	/**	EXPLANATION:
	 * Check for:
	 *	1. Blocks that overlap one another.
	 *	2. Blocks that are not sorted.
	 *	3. Blocks that extend beyond their containing chunk.
	 **/

	chunkList.lock();

	iChunks = chunkList.begin();
	for (; iChunks != chunkList.end(); ++iChunks)
	{
		List<Block>::Iterator	iBlocks, iBlocksPrev;
		Chunk				*currChunk;
		ubit8				*currChunkEnd;

		currChunk = *iChunks;
		currChunkEnd = (ubit8 *)currChunk + chunkSize;

		iBlocks = currChunk->blocks.begin();
		iBlocksPrev = currChunk->blocks.end();

		for (; iBlocks != currChunk->blocks.end();
			iBlocksPrev=iBlocks, ++iBlocks)
		{
			Block		*currBlock, *prevBlock;
			ubit8		*currBlockEnd, *prevBlockEnd;

			currBlock = *iBlocks;
			prevBlock = *iBlocksPrev;

			if (!currBlock->magicIsValid())
			{
				printf(ERROR HEAP"checkBlocks: Block 0x%p: "
					"invalid magic!\n",
					currBlock);

				return ERROR_UNKNOWN;
			};

			if (currBlock->parent != currChunk)
			{
				printf(ERROR HEAP"checkBlocks: Block 0x%p: "
					"Incorrect parent chunk!\n",
					currBlock);

				return ERROR_UNKNOWN;
			};

			// If block goes beyond chunk:
			currBlockEnd = (ubit8 *)currBlock + currBlock->nBytes;
			if (currBlockEnd > currChunkEnd)
			{
				printf(ERROR HEAP"checkBlocks: Block 0x%p, "
					"%dB: Extends beyond parent chunk "
					"0x%p, %dB!\n",
					currBlock, currBlock->nBytes,
					currChunk, chunkSize);

				return ERROR_LIMIT_OVERFLOWED;
			};

			// Check for unsorted blocks.
			if (prevBlock != NULL && currBlock < prevBlock)
			{
				printf(ERROR HEAP"checkBlocks: Block 0x%p, "
					"%dB: out of order! "
					"Prevblock 0x%p, %dB\n",
					currBlock, currBlock->nBytes,
					prevBlock, prevBlock->nBytes);

				return ERROR_GENERAL;
			};

			// Check for duplicate blocks.
			if (prevBlock != NULL && currBlock == prevBlock)
			{
				printf(ERROR HEAP"checkBlocks: Block 0x%p, "
					"%dB: duplicate block! "
					"Prevblock 0x%p, %dB\n",
					currBlock, currBlock->nBytes,
					prevBlock, prevBlock->nBytes);

				return ERROR_DUPLICATE;
			};

			if (prevBlock != NULL)
			{
				prevBlockEnd = (ubit8 *)prevBlock
					+ prevBlock->nBytes;
			};

			// Check for blocks that overlap each other.
			if (prevBlock != NULL && prevBlockEnd >= (void *)currBlock)
			{
				printf(ERROR HEAP"checkBlocks: Block 0x%p, "
					"%dB: overlapping block! "
					"Prevblock 0x%p, %dB\n",
					currBlock, currBlock->nBytes,
					prevBlock, prevBlock->nBytes);

				return ERROR_UNKNOWN;
			};
		};
	};

	return ERROR_SUCCESS;
}

error_t Heap::checkAllocations(void)
{
	uarch_t					totalAllocs, nAllocsInList;
	List<Chunk>::Iterator		iChunks;
	List<Allocation>::Iterator	iAllocs;

	/**	EXPLANATION:
	 * First, get the total number of allocations across all chunks.
	 * All the allocations in the list must meet this total.
	 *
	 * Furthermore, for each allocation:
	 *	* Check to ensure that it is within the heap.
	 *	* Check its magic.
	 *	* Check its parent block pointer.
	 *	* If the heap is in guard-paged mode, check to ensure that the
	 *	  guard page is correctly mapped.
	 **/
	nAllocsInList = 0;
	chunkList.lock();

	iChunks = chunkList.begin();
	for (totalAllocs=0; iChunks != chunkList.end(); ++iChunks)
	{
		Chunk		*currChunk;

		currChunk = *iChunks;

		if (!currChunk->magicIsValid())
		{
			printf(FATAL HEAP"checkAllocs: chunk 0x%p, invalid "
				"magic\n",
				currChunk);

			chunkList.unlock();
			return ERROR_FATAL;
		};

		totalAllocs += currChunk->nAllocations;
	};

	allocationList.lock();

	iAllocs = allocationList.begin();
	for (; iAllocs != allocationList.end(); ++iAllocs, nAllocsInList++)
	{
		Allocation		*alloc;
		Chunk			*detectedParent;

		alloc = *iAllocs;

		if (!allocIsWithinHeap(alloc, &detectedParent))
		{
			printf(FATAL HEAP"checkAllocs: alloc 0x%p not within "
				"heap!\n",
				alloc);

			allocationList.unlock();
			chunkList.unlock();
			return ERROR_FATAL;
		};

		if (!alloc->magicIsValid())
		{
			printf(FATAL HEAP"checkAllocs: Alloc 0x%p: invalid "
				"magic, %dth alloc of %d\n",
				alloc, nAllocsInList, totalAllocs);

			allocationList.unlock();
			chunkList.unlock();
			return ERROR_FATAL;
		};

		if (!chunkList.find(
			alloc->parent, List<Chunk>::OP_FLAGS_UNLOCKED)
			|| alloc->parent != detectedParent)
		{
			printf(FATAL HEAP"checkAllocs: alloc 0x%p: invalid "
				"parent chunk 0x%p!\n\t%s\n",
				alloc, alloc->parent,
				(alloc->parent != detectedParent)
					? "Parent chunk != detected parent!"
					: "Parent chunk not in chunk list!");

			allocationList.unlock();
			chunkList.unlock();
			return ERROR_FATAL;
		};
	};

	allocationList.unlock();
	chunkList.unlock();

	if (nAllocsInList != totalAllocs)
	{
		printf(FATAL HEAP"checkAllocs: Total allocations across "
			"chunks: %d, but %d allocs in alloc list\n",
			totalAllocs, nAllocsInList);

			return ERROR_FATAL;
	};

	return ERROR_SUCCESS;
}
