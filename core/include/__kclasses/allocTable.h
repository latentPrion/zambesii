#ifndef _ALLOC_TABLE_H
	#define _ALLOC_TABLE_H

	#include <scaling.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/allocTableEntry.h>
	#include <__kclasses/__kequalizerList.h>
	#include <kernel/common/numaTypes.h>

/**	EXPLANATION:
 * Alloc Table is the kernel's assurance of recollection of memory from a
 * process when it is killed. Every process has an allocTableC, including the
 * kernel itself. AllocTableC not only monitors allocations, but also gives the
 * allocation's type information, along with flags for optimizations.
 *
 * Generally the kernel needs to know two main bits of information on an
 * allocation in a process's address space:
 *
 * 1. What type of allocation it is. Is it a heap range? Is it an executable
 *    range? These will aid the kernel in deciding which pages to swap out
 *    when we get to implementing swap. Generally we'll prefer swapping out
 *    data pages to swapping out executable or other types of pages.
 *
 * 2. Whether the whole range has one single physical corresponding mapping.
 *    This is useful mainly as an optimization to allow the kernel to look up
 *    physical mappings in the page tables less, and speed up frees. 
 **/

class allocTableC
{
public:
	error_t addEntry(void *vaddr, uarch_t nPages, ubit8 type, ubit8 flags);
	error_t lookup(void *vaddr, uarch_t *nPages, ubit8 *type, ubit8 *flags);
	void removeEntry(void *vaddr);

private:
	// allocTableC is nothing more than a wrapper around __kequalizerListC.
	__kequalizerListC<allocTableEntryC>	allocTable;
};

#endif

