#ifndef _V_SWAMP_H
	#define _V_SWAMP_H

	#include <arch/paging.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/swampInfoNode.h>
	#include <__kclasses/slamCache.h>
	#include <__kclasses/allocClass.h>
	#include <kernel/common/recursiveLock.h>
	#include <kernel/common/sharedResourceGroup.h>

#define VSWAMP_NSWAMPS			1
#define VSWAMP_INSERT_BEFORE		-1
#define VSWAMP_INSERT_AFTER		-2

#define VSWAMP_RESERVE_FLAGS_FORCE	(1<<0)

class vSwampC
:
public allocClassC
{
public:
	vSwampC(void *swampStart, uarch_t swampSize)
	:
	baseAddr((void *)PAGING_BASE_ALIGN_FORWARD((uintptr_t)swampStart)),
	size(PAGING_BASE_ALIGN_TRUNCATED(swampSize)),
	flags(0),
	swampNodeList(sizeof(swampInfoNodeC), slamCacheC::RAW)
	{}

	error_t initialize(void)
	{
		// Ensure the size doesn't overflow the vaddrspace.
		if ((void *)((uarch_t)baseAddr + size) < baseAddr) {
			return ERROR_FATAL;
		};

		initSwampNode.baseAddr = baseAddr;
		initSwampNode.nPages = PAGING_BYTES_TO_PAGES(size);
		initSwampNode.prev = initSwampNode.next = __KNULL;

		state.rsrc.head = state.rsrc.tail = &initSwampNode;
		return swampNodeList.initialize();
	}

	~vSwampC(void) {}

public:
	// Use the algorithm to allocate currently free vmem.
	void *getPages(uarch_t nPages, ubit32 flags=0);
	// Manually placement-allocate virtual memory.
	error_t reservePages(void *baseAddr, uarch_t nPages, ubit32 flags);

	// Free memory that is in use.
	void releasePages(void *vaddr, uarch_t nPages);

	void dump(void);

private:
	swampInfoNodeC *getNewSwampNode(
		void *baseAddr, uarch_t size,
		swampInfoNodeC *prev, swampInfoNodeC *next);

	void deleteSwampNode(swampInfoNodeC *node);

	swampInfoNodeC *findInsertionNode(
		void *vaddr, status_t *status);

private:
	struct swampStateS
	{
		swampStateS(void)
		:
		head(__KNULL), tail(__KNULL)
		{}

		/* The tail pointer will become necessary when we implement
		 * stack allocation, where we traverse the swamp from the tail
		 * so as to get pages as high up in the address space as
		 * possible.
		 **/
		swampInfoNodeC	*head, *tail;
	};

	void		*baseAddr;
	uarch_t		size;
	uarch_t		flags;
	sharedResourceGroupC<recursiveLockC, swampStateS>	state;

	// Object allocator for swamp list nodes.
	// __kequalizerListC<swampInfoNodeC>		swampNodeList;
	slamCacheC	swampNodeList;

	/* This is a placeholder list node which is used when initializing the
	 * address space. If we try to dynamically allocate the first node, when
	 * initializing instances of this class, it will fail because address
	 * space management has not been initialized.
	 *
	 * We have to provide the first node ourselves, so this is how we do it.
	 **/
	swampInfoNodeC		initSwampNode;
};

#endif

