
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/firmwareTrib/rivDebugApi.h>
#include <kernel/common/firmwareTrib/memInfoRiv.h>
#include "ibmPcbios_coreFuncs.h"

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
	uarch_t		ax, bx, cx, dx;

	ibmPcBios_lock_acquire();

	ibmPcBios_setEax(0x0000E801);
	ibmPcBios_executeInterrupt(0x15);

	ax = ibmPcBios_getEax();
	bx = ibmPcBios_getEbx();
	cx = ibmPcBios_getEcx();
	dx = ibmPcBios_getEdx();

	ibmPcBios_lock_release();

	rivPrintf(NOTICE"Regs returned from emu run: 0x%X, 0x%X, 0x%X, 0x%X.\n",
		ax, bx, cx, dx);

	return __KNULL;
}

static struct chipsetMemMapS *ibmPcBios_mi_getMemoryMap(void)
{
	struct chipsetMemMapS	*ret;
	ubit32		nEntries=0;

	ret = (void *)rivMalloc(sizeof(struct chipsetMemMapS));
	if (ret == __KNULL) {
		return __KNULL;
	};

	ibmPcBios_lock_acquire();

	// Buffer is placed into 0x1000 in lowmem.
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
		rivPrintf(NOTICE"Memory map: Base 0x%X_%X, length 0x%X_%X, "
			"type 0x%X, cont 0x%X.\n",
			*(ubit32 *)((M.mem_base + 0x1000) + 4),
			*(ubit32 *)((M.mem_base + 0x1000) + 0),
			*(ubit32 *)((M.mem_base + 0x1000) + 12),
			*(ubit32 *)((M.mem_base + 0x1000) + 8),
			*(ubit32 *)((M.mem_base + 0x1000) + 16),
			ibmPcBios_getEbx());

		if (ibmPcBios_getEbx() == 0)
		{
			rivPrintf(NOTICE"EBX = 0, ending loop.\n");
			break;
		};

		ibmPcBios_setEax(0x0000E820);
		ibmPcBios_setEcx(24);
		ibmPcBios_executeInterrupt(0x15);
	};

	ibmPcBios_lock_release();

	rivPrintf(NOTICE"%d entries in all.\n", nEntries);
}

struct memInfoRivS	ibmPcBios_memInfoRiv =
{
	&ibmPcBios_mi_initialize,
	&ibmPcBios_mi_shutdown,
	&ibmPcBios_mi_suspend,
	&ibmPcBios_mi_awake,

	&ibmPcBios_mi_getMemoryConfig,
	__KNULL,
	&ibmPcBios_mi_getMemoryMap
};

