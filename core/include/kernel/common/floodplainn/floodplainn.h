#ifndef _FLOODPLAINN_H
	#define _FLOODPLAINN_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/callback.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <__kclasses/ptrList.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/zudiIndexServer.h>
	#include <kernel/common/floodplainn/floodplainn.h>
	#include <kernel/common/floodplainn/device.h>
	#include <kernel/common/vfsTrib/commonOperations.h>

#define FPLAINN						"Fplainn: "
#define FPLAINNIDX					"FplainnIndex: "

#define FPLAINN_DETECTDRIVER_FLAGS_CPU_TARGET	MSGSTREAM_FLAGS_CPU_TARGET

class Floodplainn
:
public Tributary//, public vfs::DirectoryOperations
{
public:
	Floodplainn(void)
	:
	zudiIndexServerTid(PROCID_INVALID)
	{}

	typedef void (initializeReqCbFn)(error_t ret);
	error_t initializeReq(initializeReqCbFn *callback);

	~Floodplainn(void) {}

public:
	struct sZudiKernelCallMsg
	{
		enum commandE {
			CMD_INSTANTIATE_DEVICE, CMD_DESTROY_DEVICE,
			CMD_NEW_PARENT };

		sZudiKernelCallMsg(
			processId_t targetPid, ubit16 subsystem, ubit16 function,
			uarch_t size, uarch_t flags, void *privateData)
		:
		header(
			targetPid, subsystem, function,
			size, flags, privateData),
		command(CMD_INSTANTIATE_DEVICE)
		{
			path[0] = '\0';
		}

		void set(utf8Char *devicePath, commandE command)
		{
			strncpy8(this->path, devicePath, FVFS_PATH_MAXLEN);
			this->path[FVFS_PATH_MAXLEN - 1] = '\0';
			this->command = command;
		}

		MessageStream::sHeader	header;
		commandE		command;
		utf8Char		path[FVFS_PATH_MAXLEN];
	};

	struct sZudiMgmtCallMsg
	{
		enum mgmtOperationE {
			MGMTOP_USAGE, MGMTOP_ENUMERATE, MGMTOP_DEVMGMT,
			MGMTOP_FINAL_CLEANUP };

		sZudiMgmtCallMsg(
			utf8Char *path,
			processId_t targetPid, ubit16 subsystem, ubit16 function,
			uarch_t size, uarch_t flags, void *privateData)
		:
		header(
			targetPid, subsystem, function,
			size, flags, privateData)
		{
			strncpy8(this->path, path, FVFS_PATH_MAXLEN);
			this->path[FVFS_PATH_MAXLEN - 1] = '\0';
			memset(&cb, 0, sizeof(cb));
		}

		void set_usage_ind(udi_ubit8_t usageLevel)
		{
			mgmtOp = MGMTOP_USAGE;
			this->usageLevel = usageLevel;
		}

		void set_devmgmt_req(
			udi_ubit8_t op, udi_ubit8_t parentId)
		{
			mgmtOp = MGMTOP_DEVMGMT;
			this->op = op;
			this->parentId = parentId;
		}

		void set_enumerate_req(udi_ubit8_t enumLvl)
		{
			mgmtOp = MGMTOP_ENUMERATE;
			this->enumerateLevel = enumLvl;
		}

		void set_final_cleanup_req(void)
			{ mgmtOp = MGMTOP_FINAL_CLEANUP; }

		MessageStream::sHeader	header;
		mgmtOperationE		mgmtOp;
		udi_ubit8_t		usageLevel;
		udi_ubit8_t		enumerateLevel;
		udi_ubit8_t		op, parentId;
		utf8Char		path[FVFS_PATH_MAXLEN];
		union
		{
			udi_mgmt_cb_t		mcb;
			udi_usage_cb_t		ucb;
			udi_enumerate_cb_t	ecb;
		} cb;
	};

	struct sZudiIndexMsg
	{
		sZudiIndexMsg(
			processId_t targetPid, ubit16 subsystem, ubit16 function,
			uarch_t size, uarch_t flags, void *privateData)
		:
		header(targetPid, subsystem, function, size, flags, privateData),
		info(0, NULL, zuiServer::INDEX_KERNEL)
		{}

		void set(
			ubit8 command, utf8Char *path,
			zuiServer::indexE index,
			zuiServer::newDeviceActionE action=
				zuiServer::NDACTION_NOTHING)
		{
			this->info.command = command;
			this->info.index = index;
			this->info.action = action;
			if (path != NULL)
			{
				strncpy8(
					this->info.path, path, FVFS_PATH_MAXLEN);

				this->info.path[FVFS_PATH_MAXLEN - 1] = '\0';
			};
		}

		MessageStream::sHeader		header;
		zuiServer::sIndexMsg		info;
	};

public:
	const sDriverInitEntry *findDriverInitInfo(utf8Char *shortName);
	const sMetaInitEntry *findMetaInitInfo(utf8Char *shortName);

	error_t findDriver(utf8Char *fullName, fplainn::Driver **ret);

	/* Creates a child device under a given parent and returns it to the
	 * caller.
	 **/
	error_t createDevice(
		utf8Char *parentId, numaBankId_t bid, ubit16 childId,
		ubit32 flags, fplainn::Device **device);

	// Removes a given child from a given parent.
	error_t removeDevice(utf8Char *parentId, ubit32 childId, ubit32 flags);
	// Removes a tree of children from a given parent.
	error_t removeDeviceAndChildren(
		utf8Char *parentId, ubit32 childId, ubit32 flags);

	/* Retrieves a device by its path. This may be a by-id, by-name,
	 * by-class or by-path path. Consider renaming this to getDeviceHandle.
	 **/
	error_t getDevice(utf8Char *path, fplainn::Device **device);
	error_t getDeviceFullName(fplainn::Device *dev, utf8Char *retname);

	// Create an instance of a device in its driver instance's addrspace.
	#define MSGSTREAM_FPLAINN_ZUDI___KCALL		(0)
	error_t instantiateDeviceReq(utf8Char *path, void *privateData);
	// Called by driver processes to send response after instantiating.
	void instantiateDeviceAck(
		processId_t targetId, utf8Char *path, error_t err,
		void *privateData);

	#define MSGSTREAM_FPLAINN_ZUDI_MGMT_CALL	(1)
	void udi_final_cleanup_req(utf8Char *devicePath, void *privateData);
	void udi_usage_ind(
		utf8Char *devicePath, udi_ubit8_t usageLevel,
		void *privateData);

	void udi_usage_res(
		utf8Char *devicePath,
		processId_t targetTid, void *privateData);

	void udi_enumerate_req(
		utf8Char *devicePath, udi_ubit8_t enumerateLevel,
		void *privateData);

	void udi_devmgmt_req(
		utf8Char *devicePath, udi_ubit8_t op, udi_ubit8_t parentId,
		void *privateData);

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

	error_t getInternalBopVectorIndexes(
		ubit16 regionIndex, ubit16 *opsIndex0, ubit16 *opsIndex1);

	error_t enumerateBaseDevices(void);
	error_t enumerateChipsets(void);

	/**	EXPLANATION:
	 * When spawning internal bind channels, all anchoring is done
	 * immediately.
	 *
	 *	Child-bind channels:
	 * The kernel does not understand the idea of parent-bind operations.
	 * Whenever a channel is to be constructed between parent and child,
	 * it is done as a child bind operation.
	 *
	 * Every device, upon instantiation, exports to the kernel the address
	 * of its parent bind ops vector. This way, the kernel can establish
	 * child bind channels without consulting the parent, thus reducing the
	 * number of address space switches involved.
	 *
	 * Effectively then, devices that do not have child bind ops don't
	 * need to expose any parent bind ops information to the kernel.
	 **/
	error_t spawnInternalBindChannel(
		utf8Char *devicePath, ubit16 regionIndex,
		udi_ops_vector_t *opsVector0, udi_ops_vector_t *opsVector1);

	error_t spawnChildBindChannel(
		utf8Char *parentDevPath, utf8Char *selfDevPath,
		utf8Char *metaName,
		udi_ops_vector_t *opsVector);

	static void indexReaderEntry(void);
	void setZudiIndexServerTid(processId_t tid)
		{ zudiIndexServerTid = tid; }

private:
	error_t spawnChannel(
		fplainn::Device *dev0, Thread *thread0,
		udi_ops_vector_t *opsVector0,
		udi_init_context_t *channelContext0,
		fplainn::Device *dev1, Thread *thread1,
		udi_ops_vector_t *opsVector1,
		udi_init_context_t *channelContext1);

public:
	PtrList<fplainn::Driver>	driverList;
	processId_t			zudiIndexServerTid;

private:
	class InitializeReqCb;
	typedef void (__kinitializeReqCbFn)(
		MessageStream::sHeader *m, initializeReqCbFn callerCb);

	static __kinitializeReqCbFn	initializeReq1;
};

extern Floodplainn		floodplainn;

extern const sDriverInitEntry	driverInitInfo[];
extern const sMetaInitEntry	metaInitInfo[];

#endif

