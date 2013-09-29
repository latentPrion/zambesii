
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <platform/pageTables.h>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/vaddrSpace.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


error_t vaddrSpaceC::initialize(numaBankId_t boundBankId)
{
	vaddrSpaceStreamC	*boundVaddrSpaceStream;
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
		level0Accessor.rsrc = (pagingLevel0S *)0xFFFFC000;
#else
		level0Accessor.rsrc = (pagingLevel0S *)0xFFFFD000;
#endif
		level0Paddr = paddr_t(&__kpagingLevel0Tables);
		return ERROR_SUCCESS;
	};

	/* Else, for userspace processes we need to allocate the new page
	 * table top level table, etc.
	 **/
#ifdef CONFIG_ARCH_x86_32_PAE
#else
	level0Accessor.rsrc = (pagingLevel0S *)processTrib.__kgetStream()
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
		&level0Accessor.rsrc->entries[startEntry],
		&boundVaddrSpaceStream->vaddrSpace.level0Accessor.rsrc
			->entries[startEntry],
		sizeof(level0Accessor.rsrc->entries[0]) * nEntries);

	uarch_t			__kflags;
	pagingLevel1S		*level1Table;
	paddr_t			level1TablePaddr;

	walkerPageRanger::lookup(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		level0Accessor.rsrc, &level0Paddr, &__kflags);

	// Now set up the jail-window mapping.
	level1Table = (pagingLevel1S *)(
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
	level1Table->entries[1021] = paddr_t(&__kpagingLevel0Tables)
		| paddr_t(PAGING_L1_PRESENT | PAGING_L1_WRITE);

	// Map 0xFFFE000 to the level 1 table, allowing the kernel to modify it.
	level1Table->entries[1022] = level1TablePaddr
		| paddr_t(PAGING_L1_PRESENT | PAGING_L1_WRITE);

	// Map 0xFFFF000 to nothing, but set the access flags for it.
	level1Table->entries[1023] = 0 
		| paddr_t(PAGING_L1_PRESENT | PAGING_L1_WRITE);

	printf(NOTICE VADDRSPACE"initialize: binding %d; lvl0: v 0x%p, "
		"p 0x%P.\n",
		boundBankId, level0Accessor.rsrc, level0Paddr);

	return ERROR_SUCCESS;
}

vaddrSpaceC::~vaddrSpaceC(void)
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
