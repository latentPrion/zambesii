#ifndef _VADDR_SPACE_H
	#define _VADDR_SPACE_H

	#include <arch/paddr_t.h>
	#include <arch/paging.h>
	#include <kernel/common/recursiveLock.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/numaTypes.h>

#define VADDRSPACE		"vaddrSpace: "

class VaddrSpace
{
public:
	VaddrSpace(void)
	: level0Paddr(paddr_t(0))
	{
		VaddrSpace::level0Accessor.rsrc = NULL;
	}

	error_t initialize(numaBankId_t boundBankId);

	~VaddrSpace(void);

	void dumpAllNonEmpty(void);
	void dumpAllPresent(void);
	status_t getTopLevelAddrState(void *vaddr)
	{
		return getTopLevelAddrState(
			(uintptr_t)vaddr >> PAGING_L0_VADDR_SHIFT);
	}

	status_t getTopLevelAddrState(uarch_t entry);

public:
	// Uses a recursive lock, take note.
	SharedResourceGroup<RecursiveLock, sPagingLevel0 *>	level0Accessor;
#ifdef CONFIG_ARCH_x86_32_PAE
	sPagingLevel1	*extraPages;
#endif
	paddr_t		level0Paddr;
};

#endif

