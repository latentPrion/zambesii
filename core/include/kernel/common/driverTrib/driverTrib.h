#ifndef _DRIVER_TRIBUTARY_H
	#define _DRIVER_TRIBUTARY_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>


#define DRIVERTRIB_TYPE_BUS		0x1
#define DRIVERTRIB_TYPE_VIDEO		0x2
#define DRIVERTRIB_TYPE_AUDIO		0x3
#define DRIVERTRIB_TYPE_STORAGE		0x4
#define DRIVERTRIB_TYPE_CHARINPUT	0x5
#define DRIVERTRIB_TYPE_COORDINPUT	0x6
#define DRIVERTRIB_TYPE_NETWORK		0x7
#define DRIVERTRIB_TYPE_FILESYSTEM	0x8

class driverTribC
:
public tributaryC
{
public:
	addDriver(ubit16 driverType, driverC *driver);

private:
	// Device classes.
	driverC		*bus,
			*video,
			*audio,
			*storage,
			*network,
			*charInput,
			*coordInput,
			*fileSystem;
};

#endif

