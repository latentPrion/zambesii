#ifndef _FLOODPLAINN_ZAMBESII_UDI_ENV_PUBLIC_API_H
	#define _FLOODPLAINN_ZAMBESII_UDI_ENV_PUBLIC_API_H

	#define UDI_VERSION 0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/floodplainn/zui.h>
	#include <kernel/common/floodplainn/device.h>
	#include <kernel/common/floodplainn/fvfs.h>

/**	EXPLANATION:
 * The Zambesii UDI environment public API.
 ******************************************************************************/
namespace fplainn
{
	class Zudi;
	class Driver;
}

struct sDriverInitEntry;
struct sMetaInitEntry;

class fplainn::Zudi
{
public:
	Zudi(void) {}
	error_t initialize(void);
	~Zudi(void) {}

public:
	error_t findDriver(utf8Char *fullName, fplainn::Driver **ret);
	const sDriverInitEntry *findDriverInitInfo(utf8Char *shortName);
	const sMetaInitEntry *findMetaInitInfo(utf8Char *shortName);

	// Create an instance of a device in its driver instance's addrspace.
	#define MSGSTREAM_ZUDI___KCALL		(0)
	error_t instantiateDeviceReq(utf8Char *path, void *privateData);
	// Called by driver processes to send response after instantiating.
	void instantiateDeviceAck(
		processId_t targetId, utf8Char *path, error_t err,
		void *privateData);

	#define MSGSTREAM_ZUDI_MGMT_CALL	(1)
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
	error_t udi_channel_spawn(
		udi_channel_spawn_call_t *cb, udi_cb_t *gcb,
		udi_channel_t channel, udi_index_t spawn_idx,
		udi_ops_vector_t *ops_vector,
		udi_init_context_t *channel_context);

	void udi_channel_anchor(void);

	error_t spawnInternalBindChannel(
		utf8Char *devicePath, ubit16 regionIndex,
		udi_ops_vector_t *opsVector0, udi_ops_vector_t *opsVector1);

	error_t spawnChildBindChannel(
		utf8Char *parentDevPath, utf8Char *selfDevPath,
		utf8Char *metaName,
		udi_ops_vector_t *opsVector);

	error_t getInternalBopVectorIndexes(
		ubit16 regionIndex, ubit16 *opsIndex0, ubit16 *opsIndex1);

public:
	struct sKernelCallMsg
	{
		enum commandE {
			CMD_INSTANTIATE_DEVICE, CMD_DESTROY_DEVICE,
			CMD_NEW_PARENT };

		sKernelCallMsg(
			processId_t targetPid,
			ubit16 subsystem, ubit16 function,
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
			strncpy8(
				this->path, devicePath,
				FVFS_PATH_MAXLEN);

			this->path[FVFS_PATH_MAXLEN - 1] = '\0';
			this->command = command;
		}

		MessageStream::sHeader	header;
		commandE		command;
		utf8Char		path[FVFS_PATH_MAXLEN];
	};

	struct sMgmtCallMsg
	{
		enum mgmtOperationE {
			MGMTOP_USAGE, MGMTOP_ENUMERATE, MGMTOP_DEVMGMT,
			MGMTOP_FINAL_CLEANUP };

		sMgmtCallMsg(
			utf8Char *path,
			processId_t targetPid,
			ubit16 subsystem, ubit16 function,
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

private:
	error_t spawnChannel(
		fplainn::Device *dev0,
		fplainn::DeviceInstance::sRegion *thread0,
		udi_ops_vector_t *opsVector0,
		udi_init_context_t *channelContext0,
		fplainn::Device *dev1,
		fplainn::DeviceInstance::sRegion *thread1,
		udi_ops_vector_t *opsVector1,
		udi_init_context_t *channelContext1);

public:
	PtrList<fplainn::Driver>	driverList;
};

extern const sDriverInitEntry	driverInitInfo[];
extern const sMetaInitEntry	metaInitInfo[];

#endif

