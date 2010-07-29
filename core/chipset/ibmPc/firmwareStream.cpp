
#include <kernel/common/firmwareTrib/firmwareStream.h>

// List all the rivulets that this chipset provides.
extern debugSupportRivS		ibmPc_terminal;

firmwareStreamS		chipsetFwStream =
{
	&ibmPc_terminal,
	__KNULL,
	__KNULL,
	__KNULL
};

