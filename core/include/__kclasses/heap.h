#ifndef _HEAP_H
	#define _HEAP_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <__kstdlib/__kclib/string.h>
	#include <__kclasses/ptrlessList.h>

#define HEAP_CHUNK_MAGIC	(utf8Char *)"##Zambesii heap chunk##"
#define HEAP_BLOCK_MAGIC	(utf8Char *)"##Zambesii heap block##"
#define HEAP_ALLOC_MAGIC	(utf8Char *)"##Zambesii heap alloc##"

#define HEAP			"Heap: "

class MemoryStream;
class MemReservoir;
class Heap
{
public:
	friend class MemReservoir;
	Heap(
		uintptr_t chunkSize, MemoryStream *sourceStream,
		uint32_t options=0)
	:
	chunkSize(chunkSize), options(options),
	memoryStream(sourceStream)
	{}

	error_t initialize(void);

	~Heap(void) {};

public:
	void *malloc(size_t sz, void *allocatedBy, utf8Char *desc=NULL);
	void free(void *p, void *freedBy);
	void *realloc(void *p, size_t sz);

	error_t spawnIdleThread(void);

	error_t checkAllocations(void);
	error_t checkBlocks(void);

	void dump(void)
	{
		printf(NOTICE HEAP"dump: typesizes: chunk %dB, block %dB "
			"alloc %dB\n",
			sizeof(Chunk), sizeof(Block), sizeof(Allocation));

		dumpChunks();
		dumpBlocks();
		dumpAllocations();
	}

	void dumpAllocations(ubit32 flags=0);
	void dumpChunks(ubit32 flags=0);
	void dumpBlocks(ubit32 flags=0);

	uintptr_t getChunkSize(void) { return chunkSize; }

public:
	static const uint32_t	OPT_GUARD_PAGED=(1<<0),
				OPT_CHECK_ALLOCS_ON_FREE=(1<<1),
				OPT_CHECK_HEAP_RANGES_ON_FREE=(1<<2),
				OPT_CHECK_BLOCK_MAGIC_PASSIVELY=(1<<3);

	static const uint32_t	OP_FLAGS_UNLOCKED=(1<<0);

private:
	// Size of each chunk, including the chunk header.
	uintptr_t		chunkSize;
	static uintptr_t	pageSize;
	uint32_t		options;

	static const uint32_t	MAGIC_MAXLEN=24;

	class Block;
	class Allocation;

	class Chunk
	{
	public:
		Chunk(void)
		:
		nAllocations(0)
		{
			strcpy8(magic, HEAP_CHUNK_MAGIC);
		}

		error_t initialize(void)
			{ return blocks.initialize(); }

		~Chunk(void)
		{
			memset(magic, 0, MAGIC_MAXLEN);
		}

		sbit8 magicIsValid(void) const
		{
			if (!strcmp8(magic, HEAP_CHUNK_MAGIC)) { return 1; };
			return 0;
		}

		void free(Allocation *alloc, void *freedBy);
		Allocation *malloc(
			Heap *heap, size_t sz, void *allocatedBy,
			utf8Char *desc=NULL);

		void dump(void)
		{
			printf(CC"\tchunk: 0x%p, %d allocations, magic %s.\n",
				(void *)this, nAllocations,
				strcmp8(magic, HEAP_CHUNK_MAGIC) == 0
					? "valid"
					: "invalid");
		}

	private:
		void appendAndCoalesce(Block *block, Allocation *alloc);
		void prependAndCoalesce(
			Block *block, Block *prevBlock,
			Allocation *alloc, void *freedBy);

	public:
		List<Chunk>::sHeader	listHeader;
		uarch_t				nAllocations;
		List<Block>		blocks;
		utf8Char			magic[MAGIC_MAXLEN];
	};

	class Block
	{
	public:
		Block(Chunk *parent, uarch_t nBytes, void *freedBy)
		:
		parent(parent), freedBy(freedBy), nBytes(nBytes)
		{
			strcpy8(magic, HEAP_BLOCK_MAGIC);
		};

		~Block(void)
		{
			memset(magic, 0, MAGIC_MAXLEN);
		}

		sbit8 magicIsValid(void) const
		{
			if (!strcmp8(magic, HEAP_BLOCK_MAGIC)) { return 1; };
			return 0;
		}

		void dump(void)
		{
			printf(CC"\tblock: 0x%p, next 0x%p, %dB. chunk 0x%p, "
				"magic %s; freed by 0x%p\n",
				(void*)this, (void*)listHeader.getNextItem(),
				nBytes, (void*)parent,
				(strcmp8(magic, HEAP_BLOCK_MAGIC) == 0)
					? "valid"
					: "invalid",
				(void*)freedBy);
		}

	public:
		List<Block>::sHeader	listHeader;
		Chunk				*parent;
		void				*freedBy;
		// Size of the block, including its block header.
		uarch_t				nBytes;
		utf8Char			magic[MAGIC_MAXLEN];
	};

	class Allocation
	{
	public:
		Allocation(
			Chunk *parent, uarch_t gapSize, uarch_t nBytes,
			void *allocatedBy, utf8Char *desc=NULL)
		:
		allocatedBy(allocatedBy), parent(parent),
		nBytes(nBytes), gapSize(gapSize)
		{
			strcpy8(magic, HEAP_ALLOC_MAGIC);
			if (desc != NULL)
			{
				strncpy8(
					description, desc,
					descriptionMaxlen);

				description[descriptionMaxlen - 1] =
					'\0';
			}
			else {
				description[0] = '\0';
			};
		}

		~Allocation(void)
		{
			memset(magic, 0, MAGIC_MAXLEN);
		}

		sbit8 magicIsValid(void) const
		{
			if (!strcmp8(magic, HEAP_ALLOC_MAGIC)) { return 1; };
			return 0;
		}

		void dump(void)
		{
			printf(CC"\talloc[1]: 0x%p, alloc'd by 0x%p, chunk 0x%p, "
				"%d(%d)B, %s magic\n\tDesc: %s\n",
				(void*)&this[1], (void*)allocatedBy,
				(void*)parent,
				nBytes,
				nBytes - sizeof(Allocation),
				strcmp8(magic, HEAP_ALLOC_MAGIC) == 0
					? "valid"
					: "invalid",
				description);
		}

	public:
		static const ubit8			descriptionMaxlen=32;
		List<Allocation>::sHeader		listHeader;
		void					*allocatedBy;
		utf8Char				description[
			descriptionMaxlen];
		Chunk					*parent;
		// Size of the allocation, including its allocation header.
		uintptr_t				nBytes;
		uintptr_t				gapSize;
		utf8Char				magic[MAGIC_MAXLEN];
	};

	error_t getNewChunk(Chunk **retchunk) const;
	void releaseChunk(Chunk *chunk) const;
	error_t setGuardPage(void *vaddr);
	error_t unsetGuardPage(void *vaddr);
	sbit8 checkGuardPage(void *vaddr);

	sbit8 allocIsWithinHeap(void *alloc, Chunk **parentChunk=NULL);

	MemoryStream			*memoryStream;
	List<Allocation>	allocationList;
	List<Chunk>		chunkList;
};

#endif
