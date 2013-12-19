#ifndef _FLOODPLAINN_H
	#define _FLOODPLAINN_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <__kclasses/ptrList.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/messageStream.h>
	#include <kernel/common/zudiIndexServer.h>
	#include <kernel/common/floodplainn/floodplainn.h>
	#include <kernel/common/floodplainn/device.h>
	#include <kernel/common/vfsTrib/commonOperations.h>

#define DRIVERINDEX_REQUEST_DEVNAME_MAXLEN		(128)

#define FPLAINN						"Fplainn: "
#define FPLAINNIDX					"FplainnIndex: "

#define FPLAINN_DETECTDRIVER_FLAGS_CPU_TARGET	MSGSTREAM_FLAGS_CPU_TARGET

class floodplainnC
:
public tributaryC//, public vfs::directoryOperationsC
{
public:
	floodplainnC(void) {}

	typedef void (initializeReqCallF)(error_t ret);
	error_t initializeReq(initializeReqCallF *callback);

	~floodplainnC(void) {}

public:
	struct zudiIndexMsgS
	:
	public zudiIndexServer::zudiIndexMsgS
	{
		zudiIndexMsgS(
			processId_t targetPid, ubit16 subsystem, ubit16 function,
			uarch_t size, uarch_t flags, void *privateData)
		:
		zudiIndexServer::zudiIndexMsgS(
			0, NULL, zudiIndexServer::INDEX_KERNEL),

		header(targetPid, subsystem, function, size, flags, privateData)
		{}

		void set(
			ubit8 command, utf8Char *path,
			zudiIndexServer::indexE index,
			zudiIndexServer::newDeviceActionE action=
				zudiIndexServer::NDACTION_NOTHING)
		{
			this->command = command;
			this->index = index;
			this->action = action;
			if (path != NULL)
			{
				strncpy8(
					this->path, path,
					ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN);

				this->path[
					ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN
						- 1] = '\0';
			};
		}

		messageStreamC::headerS		header;
	};

	/* Creates a child device under a given parent and returns it to the
	 * caller.
	 **/
	error_t createDevice(
		utf8Char *parentId, ubit16 childId, ubit32 flags,
		fplainn::deviceC **device);

	// Removes a given child from a given parent.
	error_t removeDevice(utf8Char *parentId, ubit32 childId, ubit32 flags);
	// Removes a tree of children from a given parent.
	error_t removeDeviceAndChildren(
		utf8Char *parentId, ubit32 childId, ubit32 flags);

	/* Retrieves a device by its path. This may be a by-id, by-name,
	 * by-class or by-path path.
	 **/
	error_t getDevice(utf8Char *path, fplainn::deviceC **device);

	/* FVFS listing functions. These allow the device tree to be treated
	 * like a VFS with "files" that can be listed.
	 *
	 * FVFS does not support open(), read(), write(), stat() etc on nodes
	 * however; only directory listing (node listing). To "read" the
	 * attributes of a device, use "getDeviceMetalanguageAttributes()" in
	 * conjunction with "getDeviceBaseAttributes()".
	 **/
	//error_t openDirectory(utf8Char *deviceId, void **handle);
	//error_t readDirectory(void *handle, void **tagInfo);
	//void closeDirectory(void *handle);

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

	typedef void (createRootDeviceReqCallF)(error_t);
	error_t createRootDeviceReq(createRootDeviceReqCallF *callback);

	static void __kdriverEntry(void);
	static void indexReaderEntry(void);

public:
	ptrListC<fplainn::driverC>	driverList;
	processId_t			zudiIndexServerTid;

private:
	static syscallbackFuncF initializeReq1;
};

extern floodplainnC		floodplainn;

#endif

