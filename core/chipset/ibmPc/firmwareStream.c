
#include <chipset/numaMap.h>
#include <__kstdlib/__ktypes.h>
#include <kernel/common/firmwareTrib/firmwareStream.h>
#include "pic.h"

// List all the rivulets that this chipset provides.
extern struct debugRivS		ibmPc_terminal;

static error_t ibmPc_nop_success(void)
{
	return ERROR_SUCCESS;
}

extern struct chipsetNumaMapS *ibmPc_mi_getNumaMap(void);

static error_t ibmPc_initialize(void)
{
	firmwareFwStream.memInfoRiv->getNumaMap = &ibmPc_mi_getNumaMap;
	return ERROR_SUCCESS;
}

struct firmwareStreamS		chipsetFwStream =
{
	// General control.
	&ibmPc_initialize,
	&ibmPc_nop_success,
	&ibmPc_nop_success,
	&ibmPc_nop_success,

	// Firmware support for debug output (1-4).
	&ibmPc_terminal,
	__KNULL,
	__KNULL,
	__KNULL,

	// memInfoRiv.
	__KNULL
};

