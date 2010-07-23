#ifndef _DEBUG_RIVULET_H
	#define _DEBUG_RIVULET_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/rivulet.h>

#define DEBUGRIV_DEV_VIDEO	(1<<0)
#define DEBUGRIV_DEV_SERIAL	(1<<1)
#define DEBUGRIV_DEV_PARALLEL	(1<<2)
#define DEBUGRIV_DEV_NIC	(1<<3)

class debugRivC
:
public rivuletC
{
public:
	void printf();

public:
	// Refreshes the ouput on all currently tied devices.
	void refresh(void);
	// Clears the output on the specified device.
	errot_t clear(uarch_t dev);

	// Ties debug output to the devices passed as a bitfield.
	error_t tieTo(uarch_t dev);
	// Causes the debug output to stop being sent to a specified device.
	void untieFrom(uarch_t dev);
};

#endif

