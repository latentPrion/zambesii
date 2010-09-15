
#include <scaling.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/firmwareTrib/rivDebugApi.h>
#include <kernel/common/firmwareTrib/memInfoRiv.h>
#include "ibmPcbios_coreFuncs.h"


#define E820_USABLE		0x1
#define E820_RECLAIMABLE	0x3

struct e820EntryS
{
	ubit32	baseLow;
	ubit32	baseHigh;
	ubit32	lengthLow;
	ubit32	lengthHigh;
	ubit32	type;
	ubit32	acpiExt;
};

struct e820EntryS	*e820Ptr;

static struct chipsetMemMapS		*memMap;
static struct chipsetMemConfigS	*memConfig;

static error_t ibmPcBios_mi_initialize(void)
{
	/* TODO:
	 * Should only work if the main x86Emu thing is properly set up.
	 * Implement a function isInitialized() in the x86Emu wrapping layer
	 * to test, and return ERROR_SUCCESS only if that returns true.
	 **/
	return ERROR_SUCCESS;
}

static error_t ibmPcBios_mi_shutdown(void)
{
	return ERROR_SUCCESS;
}

static error_t ibmPcBios_mi_suspend(void)
{
	return ERROR_SUCCESS;
}

static error_t ibmPcBios_mi_awake(void)
{
	return ERROR_SUCCESS;
}

/* Will return an unsorted memory map with zero-length entries taken out, and
 * in the case of a 32-bit processor, all entries with baseaddress > 4GB taken
 * out as well.
 *
 * For 32-bit with PAE, will return all mem as if it's a 64-bit build, but
 * using paddr_t constructor.
 **/
static struct chipsetMemMapS *ibmPcBios_mi_getMemoryMap(void)
{
	struct chipsetMemMapS	*ret;
	ubit32		nEntries=0, i, j;

	if (memMap != __KNULL) {
		return memMap;
	};

	ret = (void *)rivMalloc(sizeof(struct chipsetMemMapS));
	if (ret == __KNULL)
	{
		rivPrintf(ERROR"Firmware: getMemoryMap(): Failed to alloc "
			"space for mem map descriptor.\n");

		return __KNULL;
	};

	ibmPcBios_lock_acquire();

	// Buffer is placed into 0x1000 in lowmem.
	e820Ptr = (struct e820EntryS *)(M.mem_base + 0x1000);
	ibmPcBios_setEdi(0x00001000);
	ibmPcBios_setEax(0x0000E820);
	ibmPcBios_setEbx(0);
	ibmPcBios_setEcx(24);
	ibmPcBios_setEdx(0x534D4150);

	ibmPcBios_executeInterrupt(0x15);

	while ((ibmPcBios_getEax() == 0x534D4150)
		&& !__KFLAG_TEST(ibmPcBios_getEflags(), (1<<0)))
	{
		nEntries++;
		if (ibmPcBios_getEbx() == 0) {
			break;
		};

		ibmPcBios_setEax(0x0000E820);
		ibmPcBios_setEcx(24);
		ibmPcBios_setEdi(ibmPcBios_getEdi() + 24);
		ibmPcBios_executeInterrupt(0x15);
	};

	ibmPcBios_lock_release();

	/**	EXPLANATION:
	 * On IBM-PC we want to allocate enough space for two extra static
	 * entries to map out the IVT+BDA and all mem from 0x80000 for 128
	 * frames as used.
	 **/
	ret->entries = (struct chipsetMemMapEntryS *)rivMalloc(
		(nEntries + 2) * sizeof(struct chipsetMemMapEntryS));

	if (ret->entries == __KNULL) {
		rivPrintf(NOTICE"Firmware: getMemoryMap(): Failed to alloc "
			"space for actual entries in mem map.\n");

		return __KNULL;
	};

	for (i=0, j=0; j<nEntries; j++)
	{
		// Skip entries with length = 0.
		if (e820Ptr[j].lengthLow == 0 && e820Ptr[j].lengthHigh == 0) {
			continue;
		};
#if defined(__32_BIT__)
	#ifdef CONFIG_ARCH_x86_32_PAE
		ret->entries[i].baseAddr = paddr_t(
			e820Ptr[j].baseHigh, e820Ptr[j].baseLow);

		ret->entries[i].size = paddr_t(
			e820Ptr[j].lengthHigh, e820Ptr[j].lengthLow);
	#else
		// 32bit with no PAE, so ignore all e820 entries > 4GB.
		if (e820Ptr[j].baseHigh != 0) {
			continue;
		};

		ret->entries[i].baseAddr = e820Ptr[j].baseLow;
		ret->entries[i].size = e820Ptr[j].lengthLow;
	#endif

		switch (e820Ptr[j].type)
		{
		case E820_USABLE:
			ret->entries[i].memType = CHIPSETMMAP_TYPE_USABLE;
			break;

		case E820_RECLAIMABLE:
			ret->entries[i].memType = CHIPSETMMAP_TYPE_RECLAIMABLE;
			break;

		default:
			ret->entries[i].memType = CHIPSETMMAP_TYPE_RESERVED;
			break;
		};

		i++;
#else
		ret->entries[i].baseAddr = (e820Ptr[i].baseHigh << 32)
			| e820Ptr[j].baseLow);

		ret->entries[i].size = (e820Ptr[i].lengthHigh << 32)
			| e820Ptr[j].lengthLow);

		switch (e820Ptr[j].type)
		{
		case E820_USABLE:
			ret->entries[i].memType = CHIPSETMMAP_TYPE_USABLE;
			break;

		case E820_RECLAIMABLE:
			ret->entries[i].memType = CHIPSETMMAP_TYPE_RECLAIMABLE;
			break;

		default:
			ret->entries[i].memType = CHIPSETMMAP_TYPE_RESERVED;
			break;
		};

		i++;
#endif
	};

	rivPrintf(NOTICE"Firmware: getMemoryMap(): %d entries in firmware "
		"mem map.\n", nEntries);

	// Hardcode in the extra two entries for all chipsets.
	ret->entries[i].baseAddr = 0x0;
	ret->entries[i].size = 0x4FF;
	ret->entries[i].memType = CHIPSETMMAP_TYPE_RESERVED;

	ret->entries[i+1].baseAddr = 0x80000;
	ret->entries[i+1].size = 0x80000;
	ret->entries[i+1].memType = CHIPSETMMAP_TYPE_RESERVED;

	ret->nEntries = i + 2;
	memMap = ret;

	return ret;
}

static struct chipsetMemConfigS *ibmPcBios_mi_getMemoryConfig(void)
{
	struct chipsetMemConfigS	*ret;
	uarch_t				ax, bx, cx, dx;
	sarch_t				highest=0;
	uarch_t				i;

	if (memMap == __KNULL)
	{
		// Try to get a memory map.
		ibmPcBios_mi_getMemoryMap();
		if (memMap != __KNULL)
		{
			// Derive mem size from mem map instead of E801 below.
			for (i=0; i<memMap->nEntries; i++)
			{
				if ((memMap->entries[i].baseAddr
					> memMap->entries[highest].baseAddr)
					&& (memMap->entries[i].memType
						== CHIPSETMMAP_TYPE_USABLE))
				{
					highest = i;
				};
			};

			if (memMap->entries[highest].memType
				!= CHIPSETMMAP_TYPE_USABLE)
			{
				goto useE801;
			};

			ret = (struct chipsetMemConfigS *)rivMalloc(
				sizeof(struct chipsetMemConfigS));

			ret->memSize = memMap->entries[highest].baseAddr
				+ memMap->entries[highest].size;

			memConfig = ret;
			return ret;
		};
	}
	else {
		return memConfig;
	};

useE801:
	ibmPcBios_lock_acquire();

	ibmPcBios_setEax(0x0000E801);
	ibmPcBios_executeInterrupt(0x15);

	ax = ibmPcBios_getEax();
	bx = ibmPcBios_getEbx();
	cx = ibmPcBios_getEcx();
	dx = ibmPcBios_getEdx();

	ibmPcBios_lock_release();

	ret = (struct chipsetMemConfigS *)rivMalloc(
		sizeof(struct chipsetMemConfigS));

	if (ret == __KNULL) {
		return __KNULL;
	};

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

	// Don't set the static var memConfig above when we don't use the e820.
	return ret;
}

struct chipsetNumaMapS *ibmPcBios_mi_getNumaMap(void)
{
	return __KNULL;
}

struct memInfoRivS	ibmPcBios_memInfoRiv =
{
	&ibmPcBios_mi_initialize,
	&ibmPcBios_mi_shutdown,
	&ibmPcBios_mi_suspend,
	&ibmPcBios_mi_awake,

	&ibmPcBios_mi_getMemoryConfig,
	&ibmPcBios_mi_getNumaMap,
	&ibmPcBios_mi_getMemoryMap
};

