
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <platform/pageTables.h>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/vaddrSpace.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


error_t VaddrSpace::initialize(numaBankId_t boundBankId)
{
	VaddrSpaceStream	*boundVaddrSpaceStream;
#ifdef CONFIG_ARCH_x86_32_PAE
	const ubit16		startEntry=3, nEntries=1;
#else
	const ubit16		startEntry=768, nEntries=256;
#endif

	/* If it's the kernel vaddrSpace object, we just have to point it to
	 * the already-preallocated page table setup that we have for it in the
	 * kernel image.
	 **/
	if (this == &processTrib.__kgetStream()->getVaddrSpaceStream()
		->vaddrSpace)
	{
#ifdef CONFIG_ARCH_x86_32_PAE
		level0Accessor.rsrc = (sPagingLevel0 *)0xFFFFC000;
#else
		level0Accessor.rsrc = (sPagingLevel0 *)0xFFFFD000;
#endif
		level0Paddr = (uintptr_t)&__kpagingLevel0Tables;
		return ERROR_SUCCESS;
	};

	/* Else, for userspace processes we need to allocate the new page
	 * table top level table, etc.
	 **/
#ifdef CONFIG_ARCH_x86_32_PAE
#else
	level0Accessor.rsrc = (sPagingLevel0 *)processTrib.__kgetStream()
		->memoryStream.memAlloc(2, MEMALLOC_NO_FAKEMAP);
#endif

	if (level0Accessor.rsrc == NULL) { return ERROR_MEMORY_NOMEM; };
	memset(level0Accessor.rsrc, 0, sizeof(*level0Accessor.rsrc));

	/**	TODO:
	 * For now, we just get the kernel process' vaddrspace object and
	 * copy the kernel's page dir entries into the new process' new page
	 * dir.
	 *
	 * However, when we begin doing NUMA vaddrspace binding, we will need
	 * to get the /correct/ kernel vaddrspace object (from off its bank)
	 * and copy /that/ vaddrspace object into the new process' vaddrspace
	 * object.
	 **/
	boundVaddrSpaceStream = processTrib.__kgetStream()
		->getVaddrSpaceStream();

	// Clone the kernel vaddrspace top-level table into the new process.
	memcpy(
		const_cast<paddr_t *>(
			&level0Accessor.rsrc->entries[startEntry]),
		const_cast<paddr_t *>(
			&boundVaddrSpaceStream->vaddrSpace.level0Accessor.rsrc
			->entries[startEntry]),
		sizeof(level0Accessor.rsrc->entries[0]) * nEntries);

	uarch_t			__kflags;
	sPagingLevel1		*level1Table;
	paddr_t			level1TablePaddr;

	walkerPageRanger::lookup(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		level0Accessor.rsrc, &level0Paddr, &__kflags);

	// Now set up the jail-window mapping.
	level1Table = (sPagingLevel1 *)(
		(uarch_t)level0Accessor.rsrc + PAGING_BASE_SIZE);

	walkerPageRanger::lookup(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		level1Table, &level1TablePaddr, &__kflags);

	/**	EXPLANATION:
	 * See /core/platform/x8632-ibmPc/noPaeLevel1Tables.S (bottom of file).
	 *
	 * The jail-window mapping works like this:
	 *	1. PageDir[1023] -> level1TablePaddr | PRESENT | WRITE | SUPERV.
	 *	2. level1Table[1021] -> __kpagingLevel0Tables | PRES | WR | SUP.
	 *	3. level1Table[1022] -> level1TablePaddr | PRES | WRITE | SUPER.
	 *	4. level1Table[1023] -> 0 | PRESENT | WRITE | SUPERVISOR.
	 **/
	level0Accessor.rsrc->entries[1023] = level1TablePaddr
		| paddr_t(PAGING_L0_PRESENT | PAGING_L0_WRITE);

	// Map the kernel's top level table into the new addrspace.
	level1Table->entries[1021] = paddr_t((uintptr_t)&__kpagingLevel0Tables)
		| paddr_t(PAGING_L1_PRESENT | PAGING_L1_WRITE | PAGING_L1_CACHE_WRITE_THROUGH);

	// Map 0xFFFE000 to the level 1 table, allowing the kernel to modify it.
	level1Table->entries[1022] = level1TablePaddr
		| paddr_t(PAGING_L1_PRESENT | PAGING_L1_WRITE | PAGING_L1_CACHE_WRITE_THROUGH);

	// Map 0xFFFF000 to nothing, but set the access flags for it.
	level1Table->entries[1023] = paddr_t(0)
		| PAGING_L1_PRESENT | PAGING_L1_WRITE
		| PAGING_L1_CACHE_WRITE_THROUGH;

	printf(NOTICE VADDRSPACE"initialize: binding %d; lvl0: v %p, "
		"p %P.\n",
		boundBankId, level0Accessor.rsrc, &level0Paddr);

	return ERROR_SUCCESS;
}

VaddrSpace::~VaddrSpace(void)
{
	// Only try to free if it's not the kernel vaddrspace.
	if (this != &processTrib.__kgetStream()->getVaddrSpaceStream()
		->vaddrSpace)
	{
		processTrib.__kgetStream()->memoryStream.memFree(
			level0Accessor.rsrc);
	};

	level0Accessor.rsrc = NULL;
}

status_t VaddrSpace::getTopLevelAddrState(uarch_t entry)
{
	paddr_t			raw, paddr;
//	uarch_t			flags;

	level0Accessor.lock.acquire();
	raw = level0Accessor.rsrc->entries[entry];
	level0Accessor.lock.release();

//	flags = walkerPageRanger::decodeFlags(raw & 0xFFF);
	paddr = (raw >> PAGING_BASE_SHIFT) << PAGING_BASE_SHIFT;

	if (raw != 0)
	{
		paddr_t		switchable;

		switchable = (paddr >> PAGING_PAGESTATUS_SHIFT)
			& PAGESTATUS_MASK;

		switch (switchable.getLow())
		{
		case PAGESTATUS_SWAPPED:
			return WPRANGER_STATUS_SWAPPED;

		case PAGESTATUS_GUARDPAGE:
			return WPRANGER_STATUS_GUARDPAGE;

		case PAGESTATUS_HEAP_GUARDPAGE:
			return WPRANGER_STATUS_HEAP_GUARDPAGE;

		case PAGESTATUS_FAKEMAPPED_STATIC:
			return WPRANGER_STATUS_FAKEMAPPED_STATIC;

		case PAGESTATUS_FAKEMAPPED_DYNAMIC:
			return WPRANGER_STATUS_FAKEMAPPED_DYNAMIC;
		default: return ERROR_INVALID_STATE;
		};
	}
	else {
		return WPRANGER_STATUS_UNMAPPED;
	};
}

void VaddrSpace::dumpAllNonEmpty(void)
{
	uarch_t			flipFlop=0;

	printf(NOTICE VADDRSPACE"@%p (P%P): dumping all nonzero PDEs.\n\t",
		level0Accessor.rsrc, &level0Paddr);

	for (uarch_t i=0; i<PAGING_L0_NENTRIES; i++)
	{
		if (flipFlop == 0) { printf(CC"\t"); };
		if (level0Accessor.rsrc->entries[i] != 0)
		{
			printf(CC"%d ", i);
			flipFlop++;
		};

		if (flipFlop == 12) {
			printf(CC"\n");
			flipFlop = 0;
		};
	};
}

void VaddrSpace::dumpAllPresent(void)
{
	uarch_t			flipFlop=0;

	printf(NOTICE VADDRSPACE"@%p (P%P): dumping all present PDEs.\n",
		level0Accessor.rsrc, &level0Paddr);

	for (uarch_t i=0; i<PAGING_L0_NENTRIES; i++)
	{
		paddr_t		comparable;

		comparable = level0Accessor.rsrc->entries[i] & PAGING_L0_PRESENT;
		if (flipFlop == 0) { printf(CC"\t"); };
		if (comparable.getLow())
		{
			printf(CC"%d ", i);
			flipFlop++;
		};

		if (flipFlop == 12) {
			printf(CC"\n");
			flipFlop = 0;
		};
	};

	if (flipFlop != 0) { printf(CC"\n"); };

}
