#ifndef _FLOODPLAINN_H
	#define _FLOODPLAINN_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/floodplainn/floodplainn.h>
	#include <kernel/common/floodplainn/device.h>
	#include <kernel/common/vfsTrib/commonOperations.h>

class floodplainnC
:
public tributaryC, public vfs::directoryOperationsC
{
public:
	floodplainnC(void) {}
	error_t initialize(void) { return ERROR_SUCCESS; }
	~floodplainnC(void) {}

public:
	error_t createDevice(
		utf8Char *parentId, fplainn::deviceC *device,
		ubit32 flags);

	error_t createDevice(
		fplainn::deviceC *parentDev, fplainn::deviceC *device,
		ubit32 flags);

	error_t removeDevice(utf8Char *parentId, ubit32 childId,
		ubit32 flags);

	error_t removeDevice(
		fplainn::deviceC *parentDev, ubit32 childId,
		ubit32 flags);

	error_t removeDeviceAndChildren(
		utf8Char *parentId, ubit32 childId, ubit32 flags);

	error_t removeDeviceAndChildren(
		fplainn::deviceC *parentDev, ubit32 childId,
		ubit32 flags);

	fplainn::deviceC *getDevice(utf8Char *path);

	/* FVFS listing functions. These allow the device tree to be treated
	 * like a VFS with "files" that can be listed.
	 *
	 * FVFS does not support open(), read(), write(), stat() etc on nodes
	 * however; only directory listing (node listing). To "read" the
	 * attributes of a device, use "getDeviceMetalanguageAttributes()" in
	 * conjunction with "getDeviceBaseAttributes()".
	 **/
	error_t openDirectory(utf8Char *deviceId, void **handle);
	error_t readDirectory(void *handle, void **tagInfo);
	void closeDirectory(void *handle);

	/* These aren't really used by the kernel, and are mostly provided for
	 * userspace's benefit. The kernel will generally directly use
	 * "getDevice()" and just read the attributes it desires directly from
	 * the device's metadata object.
	 *
	 * getDeviceBaseAttributes():
	 *	Fills out a structure with the most basic information common
	 *	to every device: its ID, name info, vendor info, the driver
	 *	that has been chosen to instantiate the device (if any),
	 *	the instantiation status of the device, the device's
	 *	enumeration attributes (key-value pairs), the device's
	 *	registered device-categories (if any), and the device's
	 *	metalanguages, as well as the number of metalanguages it has
	 *	(if known).
	 *
	 * getDeviceMetalanguageAttributes():
	 *	This is used to get further information about the device. The
	 *	base information doesn't state anything about the class/category
	 *	of the device, its class-specific properties (e.g.: for an NIC,
	 *	these might include link speed, media type, connection status,
	 *	etc.).
	 *
	 *	Using the information about the device's metalanguages (given by
	 *	getDeviceBaseAttributes()), the caller can then query each of
	 *	the metalanguage-specific attributes for the device.
	 *
	 *	E.g.: If a device exports a "printer" metalanguage interface and
	 *	a "scanner" metalanguage interface, each of those metalanguages
	 *	would specify different attributes which describe properties of
	 *	that sub-function of the device.
	 *
	 *	By asking for the per-metalanguage attributes of the device
	 *	for its "printer" and "scanner" metalanguage-specific attributes
	 *	one can get all of the attributes the device exports.
	 **/
	error_t getDeviceBaseAttributes(utf8Char *deviceName, void *outputMem);
	error_t getDeviceMetalanguageAttributes(
		utf8Char *deviceName, utf8Char *metalanguageName,
		void *outputMem);
};

#endif

