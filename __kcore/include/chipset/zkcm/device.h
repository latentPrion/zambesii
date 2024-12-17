#ifndef _ZKCM_DEVICE_H
	#define _ZKCM_DEVICE_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>

// Chipset device classes should instantiate one of these per device object.
class ZkcmDevice
{
public:
	inline ZkcmDevice(
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
class ZkcmDeviceBase
{
public:
	ZkcmDevice *getBaseDevice(void) { return device; }

protected:
	ZkcmDeviceBase(ZkcmDevice *device)
	:
	device(device)
	{}

private:
	ZkcmDevice	*device;
};

/**	Inline methods.
 ******************************************************************************/

ZkcmDevice::ZkcmDevice(
	ubit32 childId,
	utf8Char *shortName, utf8Char *longName,
	utf8Char *vendorName, utf8Char *vendorContact
	)
:
childId(childId)
{
	strcpy8(ZkcmDevice::shortName, shortName);
	strcpy8(ZkcmDevice::longName, longName);
	strcpy8(ZkcmDevice::vendorName, vendorName);
	strcpy8(ZkcmDevice::vendorContact, vendorContact);
}

#endif

