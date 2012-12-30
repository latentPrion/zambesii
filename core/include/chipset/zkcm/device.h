#ifndef _ZKCM_DEVICE_H
	#define _ZKCM_DEVICE_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>

// Chipset device classes should instantiate one of these per device object.
class zkcmDeviceC
{
public:
	inline zkcmDeviceC(
		ubit32 childId,
		utf8Char *shortName, utf8Char *longName,
		utf8Char *vendorName, utf8Char *vendorContact);

public:
	// Uniquely identifies this child to its parent driver.
	ubit32		childId;
	ubit32		nChildren;

	utf8Char	shortName[64];
	utf8Char	longName[256];

	utf8Char	vendorName[256];
	utf8Char	vendorContact[256];
};

// Inherited by all kernel device API classes.
class zkcmDeviceBaseC
{
public:
	zkcmDeviceC *getBaseDevice(void) { return device; }

protected:
	zkcmDeviceBaseC(zkcmDeviceC *device)
	:
	device(device)
	{}

private:
	zkcmDeviceC	*device;
};

/**	Inline methods.
 ******************************************************************************/

zkcmDeviceC::zkcmDeviceC(
	ubit32 childId,
	utf8Char *shortName, utf8Char *longName,
	utf8Char *vendorName, utf8Char *vendorContact
	)
:
childId(childId)
{
	strcpy8(zkcmDeviceC::shortName, shortName);
	strcpy8(zkcmDeviceC::longName, longName);
	strcpy8(zkcmDeviceC::vendorName, vendorName);
	strcpy8(zkcmDeviceC::vendorContact, vendorContact);
}

#endif

