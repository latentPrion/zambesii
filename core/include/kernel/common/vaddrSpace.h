#ifndef _VADDR_SPACE_H
	#define _VADDR_SPACE_H

	#include <arch/paddr_t.h>
	#include <arch/paging.h>
	#include <kernel/common/recursiveLock.h>
	#include <kernel/common/sharedResourceGroup.h>

class vaddrSpaceC
{
public:
	vaddrSpaceC(pagingLevel0S *level0Accessor, paddr_t paddr)
	: level0Paddr(paddr)
	{
		vaddrSpaceC::level0Accessor.rsrc = level0Accessor;
	};

public:
	// Uses a recursive lock, take note.
	sharedResourceGroupC<recursiveLockC, pagingLevel0S *>	level0Accessor;
	paddr_t		level0Paddr;
};

#endif

