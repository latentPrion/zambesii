
#include <__ksymbols.h>
#include <arch/arch.h>
#include <chipset/zkcm/memoryDetection.h>
#define __ASM__
#include <platform/memory.h>
#undef __ASM__
#include <firmware/ibmPcBios/ibmPcBios_coreFuncs.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/libacpi.h>


// E820 definitions for ZkcmMemoryDetectionMod::getMemoryMap().
#define E820_USABLE		0x1
#define E820_RECLAIMABLE	0x3

struct sE820Entry
{
	ubit32	baseLow;
	ubit32	baseHigh;
	ubit32	lengthLow;
	ubit32	lengthHigh;
	ubit32	type;
	ubit32	acpiExt;
};

static sE820Entry		*e820Ptr;
static sZkcmMemoryMapS		*_mmap=NULL;
static sZkcmMemoryConfig		*_mcfg=NULL;

error_t ZkcmMemoryDetectionMod::initialize(void)
{
	return ibmPcBios::initialize();
}

error_t ZkcmMemoryDetectionMod::shutdown(void)
{
	return ibmPcBios::shutdown();
}

error_t ZkcmMemoryDetectionMod::suspend(void)
{
	return ERROR_SUCCESS;
}

error_t ZkcmMemoryDetectionMod::restore(void)
{
	return ERROR_SUCCESS;
}

static sZkcmNumaMap *ibmPc_mMod_gnm_rGnm(void)
{
	sZkcmNumaMap		*ret;
	acpi::sRsdt		*rsdt;
	acpiR::sSrat		*srat;
	acpiR::srat::sMem		*memEntry;
	void			*handle, *sratHandle, *context;
	ubit32			nEntries=0, currEntry=0;

	/**	EXPLANATION:
	 * Very simple: uses the kernel libACPI to get a list of NUMA memory
	 * nodes on the chipset, then transliterates them into a chipset-
	 * independent node map, and returns it to the caller.
	 *
	 * Only difference between this and ibmPc_mMod_gnm_xGnm() is that this
	 * function uses the RSDT and its accompanying tables, while the
	 * other function uses the XSDT and its accompanying tables.
	 **/

	// LibACPI has determined that there is an RSDT present.
	if (acpi::mapRsdt() != ERROR_SUCCESS) {	return NULL; };
	rsdt = acpi::getRsdt();

	// Now look for an SRAT entry in the RSDT.
	context = handle = NULL;
	srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);

	if (srat == NULL)
	{
		printf(NOTICE"rsdtGetNumaMap: No SRAT tables found.\n");
		return NULL;
	}

	// First run: find out how many entries exist.
	while (srat != NULL)
	{
		sratHandle = NULL;
		memEntry = acpiR::srat::getNextMemEntry(srat, &sratHandle);
		for (; memEntry != NULL;
			memEntry = acpiR::srat::getNextMemEntry(
				srat, &sratHandle))
		{
			nEntries++;
		};

		acpiRsdt::destroySdt((acpi::sSdt *)srat);
		srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);
	};

	if (nEntries < 1)
	{
		printf(NOTICE"rsdtGetNumaMap: No memory entries in SRAT.\n");
		return NULL;
	}

	ret = new sZkcmNumaMap;
	if (ret == NULL) { return NULL; };
	ret->nCpuEntries = 0;
	ret->cpuEntries = NULL;

	ret->memEntries = new sNumaMemoryMapEntry[nEntries];
	if (ret->memEntries == NULL)
	{
		delete ret;
		return NULL;
	};

	context = handle = NULL;
	srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);

	// Second run: Fill out kernel NUMA map.
	while (srat != NULL)
	{
		sratHandle = NULL;
		memEntry = acpiR::srat::getNextMemEntry(srat, &sratHandle);
		for (; memEntry != NULL && currEntry < nEntries;
			memEntry = acpiR::srat::getNextMemEntry(
				srat, &sratHandle))
		{
#ifdef __32_BIT__
			if (!(memEntry->baseHigh == 0
				&& memEntry->lengthHigh == 0))
			{
				continue;
			};
#endif
			ret->memEntries[currEntry].baseAddr
				= memEntry->baseLow
#ifndef __32_BIT__
				| (memEntry->baseHigh << 32)
#endif
				;
			ret->memEntries[currEntry].size
				= memEntry->lengthLow
#ifndef __32_BIT__
				| (memEntry->lengthHigh << 32)
#endif
				;

			ret->memEntries[currEntry].bankId =
				memEntry->domain0
				| (memEntry->domain1 << 16);

			if (FLAG_TEST(
				memEntry->flags,
				ACPI_SRAT_MEM_FLAGS_ENABLED))
			{
				FLAG_SET(
					ret->memEntries[currEntry].flags,
					NUMAMEMMAP_FLAGS_ONLINE);
			};

			if (FLAG_TEST(
				memEntry->flags,
				ACPI_SRAT_MEM_FLAGS_HOTPLUG))
			{
				FLAG_SET(
					ret->memEntries[currEntry].flags,
					NUMAMEMMAP_FLAGS_HOTPLUG);
			};

			currEntry++;
		};

		acpiRsdt::destroySdt((acpi::sSdt *)srat);
		srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);
	};

	ret->nMemEntries = currEntry;
	return ret;
}

sZkcmNumaMap *ZkcmMemoryDetectionMod::getNumaMap(void)
{
	error_t		err;
	sZkcmNumaMap	*ret=NULL;

	// Get NUMA map from ACPI.
	acpi::initializeCache();
	err = acpi::findRsdp();
	if (err != ERROR_SUCCESS) { return NULL; };

// Only try to use XSDT for 64-bit and upwards.
#if __VADDR_NBITS__ >= 64
	if (acpi::testForXsdt())
	{
		printf(NOTICE"getNumaMap: Using XSDT.\n");
		// Prefer XSDT to RSDT if found.
		ret = ibmPc_mMod_gnm_xGnm();
		if (ret != NULL) { return ret; };
	};
#endif
	// Else use RSDT.
	if (acpi::testForRsdt())
	{
		printf(NOTICE"getNumaMap: No XSDT; falling back to RSDT.\n");
		ret = ibmPc_mMod_gnm_rGnm();
	};

	return ret;
}

#ifdef __32_BIT__
	#ifdef CONFIG_ARCH_x86_32_PAE
		// For PAE, baseHigh is allowed to have up to 4 bits set.
		#define IBMPCMMAP_ADDRHIGH_BADMASK	0xFFFFFFF0
	#else
		// Else, no bits in baseHigh should be set (==0) for 32-bit.
		#define IBMPCMMAP_ADDRHIGH_BADMASK	0xFFFFFFFF
	#endif
#else
	// For 64-bit, of course, baseHigh is completely fair game.
	#define IBMPCMMAP_ADDRHIGH_BADMASK	0
#endif

sZkcmMemoryMapS *ZkcmMemoryDetectionMod::getMemoryMap(void)
{
	sZkcmMemoryMapS		*ret;
	ubit32			nEntries=0, nBadEntries, i, j;

	/**	EXPLANATION:
	 * Calls on the functions provided in the IBM-PC BIOS code to execute
	 * firmware interrupts on the board via the x86Emu library.
	 *
	 * This function executes INT 0x15(AH=0x0000E820).
	 *
	 * The function caches the result in a file-local variable. If the
	 * kernel asks for the memory map subsequently, it will get the old one
	 * back.
	 **/
	// If memory map was previously obtained, return the cached one.
	if (_mmap != NULL) { return _mmap; };

	ret = new sZkcmMemoryMapS;
	if (ret == NULL)
	{
		printf(ERROR"Failed to alloc memMap main structure.\n");
		return NULL;
	};

	// Find out how many E820 entries there are.
	ibmPcBios::acquireLock();

	// Buffer is placed into 0x1000 in lowmem.
	e820Ptr = (struct sE820Entry *)(M.mem_base + 0x1000);
	ibmPcBios_regs::setEdi(0x00001000);
	ibmPcBios_regs::setEax(0x0000E820);
	ibmPcBios_regs::setEbx(0);
	ibmPcBios_regs::setEcx(24);
	ibmPcBios_regs::setEdx(0x534D4150);

	ibmPcBios::executeInterrupt(0x15);

	while ((ibmPcBios_regs::getEax() == 0x534D4150)
		&& !FLAG_TEST(ibmPcBios_regs::getEflags(), (1<<0)))
	{
		nEntries++;
		if (ibmPcBios_regs::getEbx() == 0) {
			break;
		};

		ibmPcBios_regs::setEax(0x0000E820);
		ibmPcBios_regs::setEcx(24);
		ibmPcBios_regs::setEdi(ibmPcBios_regs::getEdi() + 24);
		ibmPcBios::executeInterrupt(0x15);
	};

	ibmPcBios::releaseLock();

	// Allocate enough space to hold them all, plus the extra 3.
	ret->entries = new sZkcmMemoryMapEntryS[nEntries + 3];
	if (ret->entries == NULL)
	{
		printf(ERROR"Failed to alloc space for mem map entries.\n");
		delete ret;
		return NULL;
	};

	// Generate the kernel's generic map from the E820.
	// 'i' indexes into the E820 map, and 'j' indexes into the generic map.
	nBadEntries = 0;
	for (i=0, j=0; i<nEntries; i++)
	{
		if (e820Ptr[i].baseHigh & IBMPCMMAP_ADDRHIGH_BADMASK)
		{
			nBadEntries++;
			continue;
		};

		ret->entries[j].baseAddr = e820Ptr[i].baseLow
// Only add the high 32 bits for 64-bit and above, and for PAE x86-32.
#if defined(CONFIG_ARCH_x86_32_PAE) || (!defined(__32_BIT__))
			| (e820Ptr[i].baseHigh << 32)
#endif
			;
		ret->entries[j].size = e820Ptr[i].lengthLow
#if defined(CONFIG_ARCH_x86_32_PAE) || (!defined(__32_BIT__))
			| (e820Ptr[i].lengthHigh << 32)
#endif
			;

		switch (e820Ptr[i].type)
		{
		case E820_USABLE:
			ret->entries[j].memType = ZKCM_MMAP_TYPE_USABLE;
			break;

		case E820_RECLAIMABLE:
			ret->entries[j].memType = ZKCM_MMAP_TYPE_RECLAIMABLE;
			break;

		default:
			ret->entries[j].memType = ZKCM_MMAP_TYPE_RESERVED;
			break;
		};
		j++;
	};

	printf(NOTICE"getMemoryMap(): %d firmware map entries; %d were "
		"skipped.\n",
		nEntries, nBadEntries);

	/* Hardcode in IVT + BDA.
	 * Additionally, the kernel force-uses several frames in lowmem, placing
	 * data and code at specific addresses. These locations should be marked
	 * in the memory map to avoid data trampling.
	 *
	 * They are:
	 *	* The kernel's x86 CPU wakeup trampoline code, which is placed
	 *	  at 0x9000, and spans about 4-8 frames.
	 *	* The E820 memory map, which is placed at 0x1000, as can be
	 *	  seen right above in this very function, which is where the
	 *	  memory map code is located.
	 *
	 * For safe measure, hardcode the size of the stretch of reserved mem as
	 * approx 0x20000 bytes in size.
	 **/
	ret->entries[j].baseAddr = 0x0;
	ret->entries[j].size = 0x20000;
	ret->entries[j].memType = ZKCM_MMAP_TYPE_RESERVED;

	// Hardcode entry for EBDA + BIOS + VGA framebuffer + anything else.
	ret->entries[j+1].baseAddr = 0x80000;
	ret->entries[j+1].size = 0x80000;
	ret->entries[j+1].memType = ZKCM_MMAP_TYPE_RESERVED;

	// This entry sets the kernel image in pmem as reserved.
	ret->entries[j+2].baseAddr = CHIPSET_MEMORY___KLOAD_PADDR_BASE;
	ret->entries[j+2].size = PLATFORM_MEMORY_GET___KSYMBOL_PADDR(__kend)
		- CHIPSET_MEMORY___KLOAD_PADDR_BASE;

	ret->entries[j+2].memType = ZKCM_MMAP_TYPE_RESERVED;

	printf(NOTICE"getMemoryMap(): Kernel phys: base %P, size %P.\n",
		&ret->entries[j+2].baseAddr,
		&ret->entries[j+2].size);

	ret->nEntries = j + 3;
	_mmap = ret;

	return ret;
}

sZkcmMemoryConfig *ZkcmMemoryDetectionMod::getMemoryConfig(void)
{
	sZkcmMemoryConfig		*ret;
	uarch_t			ax, bx, cx, dx;
	ubit32			highest=0;

	/**	EXPLANATION:
	 * Prefer to derive the size of memory from the E820 map. If you can't
	 * get an E820 map, then use the firmware's INT 0x15(AH=0x0000E801).
	 **/
	if (_mcfg != NULL) { return _mcfg; };
	ret = new sZkcmMemoryConfig;
	if (ret == NULL)
	{
		printf(ERROR"getMemoryConfig: Failed to alloc config.\n");
		return NULL;
	};

	if (_mmap == NULL)
	{
		getMemoryMap();
		if (_mmap == NULL) { goto useE801; };
	};

	for (ubit32 i=0; i<_mmap->nEntries; i++)
	{
		if ((_mmap->entries[i].baseAddr
			> _mmap->entries[highest].baseAddr)
			&& _mmap->entries[i].memType
				== ZKCM_MMAP_TYPE_USABLE)
		{
			highest = i;
		};
	};

	if (_mmap->entries[highest].memType != ZKCM_MMAP_TYPE_USABLE) {
		goto useE801;
	};

	ret->memBase = 0x0;
	ret->memSize = _mmap->entries[highest].baseAddr
		+ _mmap->entries[highest].size;

	return ret;

useE801:

	ibmPcBios::acquireLock();

	ibmPcBios_regs::setEax(0x0000E801);
	ibmPcBios::executeInterrupt(0x15);

	ax = ibmPcBios_regs::getEax();
	bx = ibmPcBios_regs::getEbx();
	cx = ibmPcBios_regs::getEcx();
	dx = ibmPcBios_regs::getEdx();

	ibmPcBios::releaseLock();

	ret->memBase = 0x0;
	if (ax == 0)
	{
		ret->memSize = 0x100000 + (cx << 10);
		ret->memSize += dx << 16;
	}
	else
	{
		ret->memSize = 0x100000 + (ax << 10);
		ret->memSize += bx << 16;
	}

	return ret;
}

