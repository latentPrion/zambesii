#ifndef _VADDR_SPACE_H
	#define _VADDR_SPACE_H

	#include <arch/paddr_t.h>
	#include <arch/paging.h>
	#include <kernel/common/recursiveLock.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/numaTypes.h>

#define VADDRSPACE		"vaddrSpace: "

class vaddrSpaceC
{
public:
	vaddrSpaceC(void)
	: level0Paddr((paddr_t)__KNULL)
	{
		vaddrSpaceC::level0Accessor.rsrc = __KNULL;
	}

	error_t initialize(numaBankId_t boundBankId);

	~vaddrSpaceC(void);

public:
	// Uses a recursive lock, take note.
	sharedResourceGroupC<recursiveLockC, pagingLevel0S *>	level0Accessor;
#ifdef CONFIG_ARCH_x86_32_PAE
	pagingLevel1S	*extraPages;
#endif
	paddr_t		level0Paddr;
};

#endif

