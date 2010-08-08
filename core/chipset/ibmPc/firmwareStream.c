
#include <__kstdlib/__ktypes.h>
#include <kernel/common/firmwareTrib/firmwareStream.h>

// List all the rivulets that this chipset provides.
extern struct debugSupportRivS		ibmPc_terminal;

error_t ibmPc_nop(void)
{
	return ERROR_SUCCESS;
}

struct firmwareStreamS		chipsetFwStream =
{
	&ibmPc_nop,
	&ibmPc_nop,
	&ibmPc_nop,
	&ibmPc_nop,
	// No watchdog.
	__KNULL,
	&ibmPc_terminal,
	__KNULL,
	__KNULL,
	__KNULL
};

