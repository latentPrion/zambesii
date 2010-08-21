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
 * kernel itself. AllocTableC monitors dynamic allocations in a process.
 *
 * Generally, all that is needed is the virtual address of the allocation,
 * and its size in pages. However, since I intend for the page swapping code
 * to essentially prefer to swap out heap/dynamically allocated memory over
 * swapping out, for example, executable sections (though swapping executable
 * sections is a good second choice), the alloc tables will also hold
 * "attributes", to tell the swapper which pages it is not safe to swap out.
 *
 * So for example, a page with the ALLOCTABLE_ATTRIB_NOSWAP attribute set is
 * meant to be left in memory all the time, and never swapped out.
 **/

#define ALLOCTABLE_ATTRIB_NOSWAP	(1<<0)

// 2 bits for up to 3 values (0 is unusable).
#define ALLOCTABLE_ATTRIB_MASK		0x3

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

