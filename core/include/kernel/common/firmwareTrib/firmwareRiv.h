#ifndef _FIRMWARE_RIVULET_H
	#define _FIRMWARE_RIVULET_H

	#include <kernel/common/rivulet.h>

/**	EXPLANATION:
 * The Firmware Rivulet is the Zambezii equivalent of a "kernel driver module"
 * since they are essentially base classes for drivers, or firmware access
 * services.
 *
 * Each Firmware stream holds a bunch of these, it presents them to the
 * Firmware Tributary. The firmware Tributary makes a mirror stream, and copies
 * the addresses of each chosen rivulet into that stream, and returns that
 * pointer whenever the kernel asks for a certain rivulet.
 *
 * The common API for all Firmware Rivulets is simple: the kernel mandates the
 * following operations:
 *	1. intialize();
 *	2. shutdown();
 *	3. suspend();
 *	4. awake();
 *
 * However, they are not polymorphic since the arguments to a terminalFwRivC
 * would not be the same as those which would pertain to a memoryFwRivC.
 *
 * When constructed, a firmware rivulet is to do nothing. When destroyed, a
 * firmware rivulet is to do nothing.
 **/

class firmwareRivC
:
public rivuletC
{
public:
	firmwareRivC(void) {};
	virtual ~firmwareRivC(void) {};

public:
	virtual error_t initialize(void);
	virtual error_t shutdown(void);
	virtual error_t suspend(void);
	virtual error_t shutdown(void);
};

#endif

