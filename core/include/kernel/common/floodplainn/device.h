#ifndef _DEVICE_TYPES_H
	#define _DEVICE_TYPES_H

	#include <__kstdlib/__ktypes.h>

/**	deviceC:
 * Base type for a device in general. The type of driver used to instantiate
 * the device is independent of this class; both the driver instance and the
 * driver are simply pointed to by the device.
 *
 * Each device has a name and a unique ID.
 **/
class deviceC
:
public vfs::nodeC
{
public:
	deviceC(ubit16 id, utf8Char *longName);
	error_t initialize(void) { return vfs::nodeC::initialize(); }
	~deviceC(void);

public:
	driverC *getDriver(void) { return driver; }
	driverInstanceC *getDriverInstance(void) { return driverInstance; }

public:
	ubit16		id;
	utf8Char	shortName[],
			longName[],
			vendorName[],
			vendorContactInfo[],
				
};

#endif

