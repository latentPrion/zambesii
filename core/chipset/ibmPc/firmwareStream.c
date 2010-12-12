
#include <chipset/numaMap.h>
#include <__kstdlib/__ktypes.h>
#include <kernel/common/firmwareTrib/firmwareStream.h>
#include "pic.h"

// List all the rivulets that this chipset provides.
extern struct debugRivS		ibmPc_terminal;
extern struct debugRivS		ibmPc_rs232;

static error_t ibmPc_nop_success(void)
{
	return ERROR_SUCCESS;
}

static error_t ibmPc_initialize(void)
{
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
	&ibmPc_rs232,
	__KNULL,
	__KNULL,

	// memInfoRiv.
	__KNULL
};

