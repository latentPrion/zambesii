
#include <__kstdlib/__ktypes.h>
#include <kernel/common/firmwareTrib/firmwareStream.h>

// List all the rivulets that this chipset provides.
extern struct terminalFwRivS		chipsetTerminalFwRiv;

struct firmwareStreamS		chipsetFwStream =
{
	// No watchdog.
	&chipsetTerminalFwRiv,
	__KNULL
};

