#ifndef _ALLOC_TABLE_H
	#define _ALLOC_TABLE_H

	#include <scaling.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/allocTableEntry.h>
	#include <__kclasses/__kequalizerList.h>
	#include <kernel/common/numaTypes.h>

/**	EXPLANATION:
 * Alloc Table is the kernel's assurance of recollection of dynamically
 * allocated pmem from a process when it is killed. Every process has an
 * AllocTable, including the kernel itself. AllocTableC ONLY tracks _dynamic_
 * allocations.
 *
 * Static executable sections such as .text, .data, .rodata etc do not require
 * an alloc table since they are not dynamic in nature. The kernel can easily
 * clean these up by checking the relocation information for the process in
 * question.
 *
 * Generally, only the virtual address of the allocation and its size in pages
 * are needed. There are 5 types of allocations which are dynamic in nature:
 *	1. Normal memory allocation:	 : SWAP, RW, FAKE
 *	2. Stack allocation.		 : NOSWAP, RW, FAKE-IF-NOT-KERNEL
 *	3. Shared memory allocation.	 : NOSWAP, RW, NOFAKE
 *	4. Memory mapped file allocation.: SWAP, RW, FAKE
 *	5. Memory Region allocation.	 : NOSWAP, RW, NOFAKE
 *
 * 1, 2: Handled by MemoryStream::memAlloc().
 * 3: Handled by MemoryStream::sharedMemoryAlloc().
 * 4: Handled by MemoryStream::memoryMappedFileAlloc().
 * 5: Handled by MemoryStream::memoryRegionAlloc().
 **/

#define ALLOCTABLE_ATTRIB_NOSWAP	(1<<0)
// 4 bits for up to 4 attribute flags.
#define ALLOCTABLE_ATTRIB_MASK		0xF

#define ALLOCTABLE_NPAGES_SHIFT		4

class AllocTable
{
public:
	AllocTable(void) {}
	error_t initialize(void) { return allocTable.initialize(); }

public:
	error_t addEntry(void *vaddr, uarch_t nPages, ubit8 attrib);
	error_t lookup(void *vaddr, uarch_t *nPages, ubit8 *attrib);
	void removeEntry(void *vaddr);

private:
	// AllocTable is nothing more than a wrapper around __kEqualizerList.
	__kEqualizerList<AllocTableEntry>	allocTable;
};

#endif

