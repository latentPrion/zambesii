
#include <__kstdlib/__ktypes.h>
#include <kernel/common/firmwareTrib/firmwareStream.h>
#include "pic.h"

// List all the rivulets that this chipset provides.
extern struct debugRivS		ibmPc_terminal;

static error_t ibmPc_nop_success(void)
{
	return ERROR_SUCCESS;
}

struct firmwareStreamS		chipsetFwStream =
{
	// General control.
	&ibmPc_nop_success,
	&ibmPc_nop_success,
	&ibmPc_nop_success,
	&ibmPc_nop_success,

	// Interrupt vector control.
	&ibmPc_pic_maskSingle,
	&ibmPc_pic_maskAll,
	&ibmPc_pic_unmaskSingle,
	&ibmPc_pic_unmaskAll,

	/**	Devices:
	 * Watchdog, debug 1-4.
	 **/
	__KNULL,
	&ibmPc_terminal,
	__KNULL,
	__KNULL,
	__KNULL
};

