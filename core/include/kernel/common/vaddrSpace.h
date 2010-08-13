#ifndef _VADDR_SPACE_H
	#define _VADDR_SPACE_H

	#include <arch/paddr_t.h>
	#include <arch/paging.h>
	#include <kernel/common/recursiveLock.h>
	#include <kernel/common/sharedResourceGroup.h>

class vaddrSpaceC
{
public:
	vaddrSpaceC(void) {}

	void initialize(pagingLevel0S *level0Accessor, paddr_t paddr)
	{
		level0Accessor.rsrc = level0Accessor;
		level0Paddr = paddr;
	}

	vaddrSpaceC(pagingLevel0S *level0Accessor, paddr_t paddr)
	{
		initialize(level0Accessor, paddr);
	}

public:
	// Uses a recursive lock, take note.
	sharedResourceGroupC<recursiveLockC, pagingLevel0S *>	level0Accessor;
	paddr_t		level0Paddr;
};

#endif

