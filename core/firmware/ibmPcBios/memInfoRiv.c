
#include <__kstdlib/__ktypes.h>
#include <kernel/common/firmwareTrib/rivDebugApi.h>
#include <kernel/common/firmwareTrib/memInfoRiv.h>
#include "ibmPcbios_coreFuncs.h"

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

struct memInfoRivS	ibmPcBios_memInfoRiv =
{
	&ibmPcBios_mi_initialize,
	&ibmPcBios_mi_shutdown,
	&ibmPcBios_mi_suspend,
	&ibmPcBios_mi_awake,

	&ibmPcBios_mi_getMemoryConfig
};

