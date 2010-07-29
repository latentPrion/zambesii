
#include <kernel/common/firmwareTrib/firmwareStream.h>

// List all the rivulets that this chipset provides.
extern struct debugSupportRivS		ibmPc_terminal;

struct firmwareStreamS		chipsetFwStream =
{
	&ibmPc_terminal,
	__KNULL,
	__KNULL,
	__KNULL
};

