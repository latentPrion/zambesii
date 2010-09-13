
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

static struct chipsetMemConfigS *ibmPcBios_mi_getMemoryConfig(void)
{
	struct chipsetMemConfigS	*ret;
	uarch_t			ax, bx, cx, dx;

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

	return ret;
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

	ret = (void *)rivMalloc(sizeof(struct chipsetMemMapS));
	if (ret == __KNULL)
	{
		rivPrintf(ERROR"ibmPcBios_mi_getMemoryMap(): Failed to alloc "
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
		if (ibmPcBios_getEbx() == 0)
		{
			rivPrintf(NOTICE"EBX = 0, ending loop.\n");
			break;
		};

		ibmPcBios_setEax(0x0000E820);
		ibmPcBios_setEcx(24);
		ibmPcBios_setEdi(ibmPcBios_getEdi() + 24);
		ibmPcBios_executeInterrupt(0x15);
	};

	ibmPcBios_lock_release();

	ret->entries = (struct chipsetMemMapEntryS *)rivMalloc(
		nEntries * sizeof(struct chipsetMemMapEntryS));

	if (ret->entries == __KNULL) {
		rivPrintf(NOTICE"ibmPcBios_mi_getMemoryMap(): Failed to alloc "
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

	rivPrintf(NOTICE"ibmPcBios_mi_getMemoryMap(): %d entries in firmware "
		"map.\n", nEntries);

	ret->nEntries = i;
	return ret;
}

struct chipsetNumaMapS *ibmPcBios_mi_getNumaMap(void)
{
#if __SCALING__ < SCALING_CC_NUMA
	return __KNULL;
#endif
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

