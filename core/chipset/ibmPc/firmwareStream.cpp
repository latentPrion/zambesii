
#include <kernel/common/firmwareTrib/firmwareStream.h>

// List all the rivulets that this chipset provides.
extern terminalFwRivS		chipsetTerminalFwRiv;

firmwareStreamS		chipsetFwStream =
{
	&chipsetTerminalFwRiv
};

