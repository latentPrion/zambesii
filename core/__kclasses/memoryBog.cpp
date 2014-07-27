
#include <debug.h>
#include <stddef.h>
#include <arch/paging.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/memoryBog.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/processTrib/processTrib.h>


#define MEMORYBOG_SIZE_ROUNDUP(__size)			\
	((__size + sizeof(uarch_t)) & (~(sizeof(uarch_t) - 1)))

MemoryBog::MemoryBog(uarch_t bogSize, MemoryStream *sourceStream)
:
sourceStream(sourceStream)
{
	blockSize = bogSize;
	head.rsrc = NULL;
}

error_t MemoryBog::initialize(void)
{
	return ERROR_SUCCESS;
}

MemoryBog::~MemoryBog(void)
{
	// Free all blocks.
}

// Transparent method of copying headers.
void MemoryBog::moveHeaderDown(void *hdr, uarch_t nBytes)
{
	MemoryBog::sAllocHeader	tmp;

	memcpy(&tmp, hdr, sizeof(MemoryBog::sAllocHeader));
	memcpy(
		reinterpret_cast<void *>(
			(uarch_t)hdr + nBytes ),
		&tmp,
		sizeof(MemoryBog::sAllocHeader));
}

void MemoryBog::moveHeaderUp(void *hdr, uarch_t nBytes)
{
	MemoryBog::sAllocHeader	tmp;

	memcpy(
		&tmp,
		reinterpret_cast<void *>( (uarch_t)hdr + nBytes ),
		sizeof(MemoryBog::sAllocHeader));

	memcpy(hdr, &tmp, sizeof(MemoryBog::sAllocHeader));
}

void MemoryBog::dump(void)
{
	sBogBlock	*block;
	sFreeObject	*obj;

	printf(NOTICE MEMBOG"Dumping.\n");

	head.lock.acquire();
	printf(NOTICE MEMBOG"Size: 0x%X, head pointer: 0x%X.\n",
		blockSize, head.rsrc);

	block = head.rsrc;
	for (; block != NULL; block = block->next)
	{
		printf((utf8Char *)"\tBlock: 0x%X, refCount %d 1stObj 0x%X."
			"\n", block, block->refCount, block->firstObject);

		obj = block->firstObject;
		for (; obj != NULL; obj = obj->next)
		{
			printf((utf8Char *)"\t\tFree object: "
				"Addr 0x%X, nBytes 0x%X.\n",
				obj, obj->nBytes);
		};
	};

	head.lock.release();
}

void *MemoryBog::allocate(uarch_t nBytes, uarch_t flags)
{
	sBogBlock	*blockTmp;
	sFreeObject	*ret=0, *objTmp, *objTmpPrev;

	if (nBytes == 0) {
		return NULL;
	};
	nBytes = MEMORYBOG_SIZE_ROUNDUP(nBytes);

	// Size of the alloc must accommodate the alloc header.
	nBytes += sizeof(sAllocHeader);

	head.lock.acquire();

	// If swamp is empty, allocate new block.
	if (head.rsrc == NULL)
	{
		head.rsrc = getNewBlock();
		if (head.rsrc == NULL)
		{
			head.lock.release();
			return NULL;
		};
	};

	// head.rsrc now points to a block of usable memory.
	blockTmp = head.rsrc;
	for (; blockTmp != NULL; blockTmp = blockTmp->next)
	{
		objTmpPrev = 0;
		objTmp = blockTmp->firstObject;
		for (; objTmp != NULL; )
		{
			// If our allocation can come from this node.
			if (objTmp->nBytes >= nBytes)
			{
				// Just give out the whole space in this case.
				if ((objTmp->nBytes - nBytes)
					<= sizeof(sFreeObject))
				{
					nBytes = objTmp->nBytes;
					if (objTmpPrev) {
						objTmpPrev->next = objTmp->next;
					};
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
					ret = R_CAST(
						sFreeObject *,
						R_CAST(uarch_t, objTmp)
							+ objTmp->nBytes );
				};

				blockTmp->refCount++;

				head.lock.release();

				strncpy8(
					R_CAST(sAllocHeader *, ret)->magic,
					MEMBOG_MAGIC,
					strlen8(MEMBOG_MAGIC));

				R_CAST(sAllocHeader *, ret)->nBytes = nBytes;
				R_CAST(sAllocHeader *, ret)->parent = blockTmp;
				R_CAST(sAllocHeader *, ret)->allocatedBy =
					__builtin_return_address(2);

				return R_CAST(
					void *,
					R_CAST(uarch_t, ret)
						+ sizeof(sAllocHeader) );
			};
			objTmpPrev = objTmp;
			objTmp = objTmp->next;
		};
	};

	// If we reach here, there isn't enough space. Get new pool, try again.
	if (!FLAG_TEST(flags, MEMBOG_NO_EXPAND_ON_FAIL))
	{
		blockTmp = getNewBlock();
		if (blockTmp == NULL)
		{
			head.lock.release();
			return NULL;
		};

		blockTmp->next = head.rsrc;
		head.rsrc = blockTmp;
		objTmp = head.rsrc->firstObject;

		if (objTmp->nBytes >= nBytes)
		{
			if (objTmp->nBytes - nBytes <= sizeof(sFreeObject))
			{
				nBytes = objTmp->nBytes;
				ret = objTmp;
				blockTmp->firstObject = NULL;
			}
			else
			{
				objTmp->nBytes -= nBytes;
				ret = R_CAST(
					sFreeObject *,
					R_CAST(uarch_t, objTmp)
						+ objTmp->nBytes );
			};
			blockTmp->refCount++;

			head.lock.release();

			strncpy8(
				((sAllocHeader *)ret)->magic,
				MEMBOG_MAGIC, strlen8(MEMBOG_MAGIC));

			((sAllocHeader *)ret)->nBytes = nBytes;
			((sAllocHeader *)ret)->parent = blockTmp;
			R_CAST(sAllocHeader *, ret)->allocatedBy =
				__builtin_return_address(2);

			return reinterpret_cast<void *>(
				R_CAST(uarch_t, ret) + sizeof(sAllocHeader) );
		};
	};

	head.lock.release();
	return NULL;
}

void MemoryBog::free(void *_mem)
{
	sAllocHeader	*mem;
	sBogBlock	*block;
	sFreeObject	*objTmp, *prevObj;
	uarch_t		nBytesTmp;

	/**	EXPLANATION:
	 * We essentially want to first check to make sure the memory hasn't
	 * been corrupted, or the memory isn't a bad free, then as quickly as
	 * possible, locate the insertion point for the object and free.
	 **/
	mem = reinterpret_cast<sAllocHeader *>(
		reinterpret_cast<uarch_t>( _mem ) - sizeof(sAllocHeader) );

	if (mem == NULL) {
		return;
	};

	if (strncmp8(mem->magic, MEMBOG_MAGIC, strlen8(MEMBOG_MAGIC)) != 0)
	{
		mem->magic[sizeof(mem->magic) - 1] = '\0';
		printf(WARNING MEMBOG"Corrupt memory or bad free at 0x%X, "
			"magic was 0x%s.\n", mem, mem->magic);

		return;
	};

	// Erase the magic, so that freed blocks don't contain it any longer.
	memset(mem->magic, 0, strlen8(MEMBOG_MAGIC));

	if (mem->nBytes < 1)
	{
		printf(FATAL MEMBOG"free: Free'd block size is 0!\n"
			"\tAllocated by: 0x%p.\n",
			mem->allocatedBy);

//		dump();
//		DEBUG_ON(1);
		//panic(FATAL"Heap corruption.\n");
	};

	block = mem->parent;

	head.lock.acquire();

	prevObj = NULL;
	objTmp = block->firstObject;

	for (; objTmp != NULL; )
	{
		if ((void *)mem < (void *)objTmp)
		{
			nBytesTmp = mem->nBytes;
			((sFreeObject *)mem)->next = objTmp;
			((sFreeObject *)mem)->nBytes = nBytesTmp;

			// Concatenate forward.
			if ((R_CAST(uarch_t, mem)
				+ R_CAST(sFreeObject*, mem)->nBytes)
				== R_CAST(uarch_t, objTmp))
			{
				R_CAST(sFreeObject *, mem)->nBytes +=
					objTmp->nBytes;

				R_CAST(sFreeObject *, mem)->next = objTmp->next;
			};

			if (prevObj != NULL)
			{
				prevObj->next = R_CAST(sFreeObject *, mem);

				// Concatenate backward.
				if ((R_CAST(uarch_t, prevObj) + prevObj->nBytes)
					== R_CAST(uarch_t, mem))
				{
					prevObj->nBytes +=
						R_CAST(sFreeObject *, mem)
							->nBytes;

					prevObj->next =
						R_CAST(sFreeObject *, mem)
							->next;
				};
			}
			else {
				block->firstObject = R_CAST(sFreeObject *, mem);
			};
			block->refCount--;

			head.lock.release();
			return;
		};
		prevObj = objTmp;
		objTmp = objTmp->next;
	};

	/* The loop takes care of the case where we add at the front of the
	 * object list, and in the middle. This next bit of code will handle
	 * case where we add at the end of the list or the list is empty.
	 **/
	if (prevObj != NULL)
	{
		// Adding at the end of the list.
		nBytesTmp = mem->nBytes;
		prevObj->next = (sFreeObject *)mem;
		R_CAST(sFreeObject *, mem)->next = NULL;
		R_CAST(sFreeObject *, mem)->nBytes = nBytesTmp;

		if ((R_CAST(uarch_t, prevObj) + prevObj->nBytes)
			== R_CAST(uarch_t, mem))
		{
			prevObj->nBytes += R_CAST(sFreeObject *, mem)->nBytes;
			prevObj->next = R_CAST(sFreeObject *, mem)->next;
		}
		block->refCount--;

		head.lock.release();
		return;
	}
	else
	{
		// List is empty. Just add and terminal 'next' ptr.
		nBytesTmp = mem->nBytes;
		R_CAST(sFreeObject *, mem)->nBytes = nBytesTmp;
		R_CAST(sFreeObject *, mem)->next = NULL;
		block->firstObject = R_CAST(sFreeObject *, mem);
		block->refCount--;

		head.lock.release();
		return;
	};
}

error_t MemoryBog::checkAllocations(sarch_t nBytes)
{
	error_t		ret=ERROR_SUCCESS;

	head.lock.acquire();

	for (sBogBlock *currBlock=head.rsrc;
		currBlock != NULL;
		currBlock = currBlock->next)
	{
		ubit8			*start, *end;
		union
		{
			ubit8		*byte;
			sAllocHeader	*allocation;
		} cursor;

		/**	EXPLANATION:
		 * We actually go through the heap, byte by byte, looking for
		 * the magic number.
		 *
		 * We're looking through current allocations, trying to find
		 * any that look suspicious. We have to locate all alloc headers
		 * and check them.
		 **/
		start = &((ubit8 *)currBlock)[blockSize - sizeof(sAllocHeader)];
		end = start - (nBytes - sizeof(sAllocHeader));

		// Remember we're actually progressing backwards in memory.
		for (cursor.byte=start;
			cursor.byte >= end;
			cursor.byte--)
		{
			if (strncmp8(
				cursor.byte,
				MEMBOG_MAGIC, strlen8(MEMBOG_MAGIC)) != 0)
				{ continue; };

			cursor.byte -= offsetof(sAllocHeader, magic);
			if (strncmp8(cursor.byte, MEMBOG_MAGIC, strlen8(MEMBOG_MAGIC))==0)
			{
				printf(NOTICE"Problem!\n");
			};

			cursor.allocation->magic[sizeof(cursor.allocation->magic) - 1] = '\0';
			printf(NOTICE"Found alloc: %d bytes @ 0x%p, %s.\n",
				cursor.allocation->nBytes, cursor.allocation, cursor.allocation->magic);

			if (cursor.allocation->parent != currBlock)
			{
				printf(FATAL"Parent block is not currently "
					"scrutinized block!. Val 0x%p, should "
					"be 0x%p.\n",
					cursor.allocation->parent,
					currBlock);

				ret = ERROR_UNKNOWN;
			};
		};
	};

	head.lock.release();
	return ret;
}

MemoryBog::sBogBlock *MemoryBog::getNewBlock(void)
{
	sBogBlock	*ret;

	ret = new (sourceStream->memAlloc(
		PAGING_BYTES_TO_PAGES(
			blockSize + sizeof(sBogBlock)), MEMALLOC_PURE_VIRTUAL))
		sBogBlock;

	if (ret == NULL) {
		return NULL;
	};

	ret->next = NULL;
	ret->refCount = 0;
	ret->firstObject = R_CAST(sFreeObject *,
		( R_CAST(uarch_t, ret) + sizeof(sBogBlock)) );

	ret->firstObject->nBytes = blockSize;
	ret->firstObject->next = NULL;

	printf(NOTICE MEMBOG"New bog block @v 0x%p, 1stObj 0x%p, 1stObj "
		"nBytes 0x%x.\n",
		ret, ret->firstObject, ret->firstObject->nBytes);

	return ret;
}

