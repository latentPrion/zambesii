#ifndef _VADDR_SPACE_STREAM_H
	#define _VADDR_SPACE_STREAM_H

	#include <arch/paging.h>
	#include <__kclasses/stackCache.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/vaddrSpace.h>
	#include <kernel/common/vSwamp.h>

/**	EXPLANATION:
 * The vaddrSpaceStream is and abstraction of the idea of Virtual Address Space
 * Management. It 'monitors' the virtual address space therefore, of its
 * associated process.
 *
 * Each process's vaddrSpaceStream provides virtual address caching, 
 * and address space managment. The Virtual Address Space management is
 * provided by means of a helper class. The helper class is actually an
 * allocator for virtual addresses: it keeps a list of free virtual
 * addresses to be given out to the hosting memory stream when it requests
 * free virtual addresses from the process's address space.
 *
 * However, the swamp does not commit changes to the address space to the
 * hardware page tables. It just attempts to keep a coherent list of which
 * pages are _not_ occupied, and it is the kernel's responsibility to
 * ensure that those changes are reflected in the process's address space.
 *
 * See arch/walkerPageRanger.h, which is the kernel's page table walker,
 * responsible for committing changes to a virtual address space.
 **/

class vaddrSpaceStreamC
:
public streamC
{
public:
	// This constructor must be used to initialize userspace streams.
	vaddrSpaceStreamC(
		uarch_t id,
		void *swampStart, uarch_t swampSize,
		vSwampC::holeMapS *holeMap,
		pagingLevel0S *level0Accessor, paddr_t level0Paddr);

	// This constructor is used to initialize the kernel stream.
	vaddrSpaceStreamC(uarch_t id,
		pagingLevel0S *level0Accessor, paddr_t level0Paddr);

	// Provides the swamp with the right memory info to initialize.
	sarch_t initialize(
		void *swampStart, uarch_t swampSize,
		vSwampC::holeMapS *holeMap);

public:
	void *getPages(uarch_t nPages);
	void releasePages(void *vaddr, uarch_t nPages);

public:
	void cut(void);
	error_t bind(void);
	void dump(void);

public:
	vaddrSpaceC		vaddrSpace;
	stackCacheC<void *>	pageCache;
	vSwampC			vSwamp;
};

#endif

