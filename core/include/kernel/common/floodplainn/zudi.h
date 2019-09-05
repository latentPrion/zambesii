#ifndef _FLOODPLAINN_ZAMBESII_UDI_ENV_PUBLIC_API_H
	#define _FLOODPLAINN_ZAMBESII_UDI_ENV_PUBLIC_API_H

	#define UDI_VERSION 0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/floodplainn/fvfs.h>
	#include <kernel/common/floodplainn/channel.h>
	#include <__kclasses/resizeableArray.h>
	#include <__kclasses/slamCache.h>

#define ZUDI			"ZUDI: "

/**	EXPLANATION:
 * The Zambesii UDI environment public API.
 ******************************************************************************/
namespace fplainn
{
	class Zudi;
	class Driver;
	class Region;
	class Endpoint;
	class D2DChannel;
	class IncompleteD2DChannel;

namespace dma
{
	class Constraints;
	class ScatterGatherList;
}
}

struct sDriverInitEntry;
struct sMetaInitEntry;
class MemoryBmp;

class fplainn::Zudi
{
public:
	Zudi(void)
	:
	server(NULL)
	{}

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
	error_t spawnEndpoint(
		fplainn::Endpoint *channel_endp,
		utf8Char *metaName, udi_index_t spawn_idx,
		udi_ops_vector_t *ops_vector,
		udi_init_context_t *channel_context,
		fplainn::Endpoint **retendp);

	void anchorEndpoint(
		fplainn::Endpoint *endp, udi_ops_vector_t *ops_vector,
		void *endpPrivateData);

	error_t spawnInternalBindChannel(
		utf8Char *devicePath, utf8Char *metaName, ubit16 regionIndex,
		udi_ops_vector_t *opsVector0, udi_ops_vector_t *opsVector1,
		fplainn::Endpoint **retendp1);

	error_t spawnChildBindChannel(
		utf8Char *parentDevPath, utf8Char *selfDevPath,
		utf8Char *metaName,
		udi_ops_vector_t *opsVector,
		fplainn::Endpoint **retendp1);

	error_t getInternalBopInfo(
		ubit16 regionIndex, ubit16 *metaIndex,
		ubit16 *opsIndex0, ubit16 *opsIndex1, ubit16 *bindCbIndex);

	void setEndpointPrivateData(
		fplainn::Endpoint *endp, void *privateData);

	#define MSGSTREAM_ZUDI_CHANNEL_SEND		(2)
	error_t send(
		fplainn::Endpoint *endp,
		udi_cb_t *gcb, va_list args, udi_layout_t *layouts[3],
		utf8Char *metaName, udi_index_t meta_ops_num, udi_index_t op_idx,
		void *privateData);

	fplainn::Channel::bindChannelTypeE getBindChannelType(
		fplainn::Endpoint *endp);

	error_t getEndpointMetaName(fplainn::Endpoint *endp, utf8Char *mem);
	// TODO: Move this into libzudi. It shouldn't be in the kernel.
	fplainn::Endpoint *getMgmtEndpointForCurrentDeviceInstance(void);

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

private:
	static error_t createChannel(
		fplainn::IncompleteD2DChannel *blueprint,
		fplainn::D2DChannel **retchan);

	Thread				*server;

public:
	HeapList<fplainn::Driver>	driverList;
};

extern const sDriverInitEntry	driverInitInfo[];
extern const sMetaInitEntry	metaInitInfo[];

#endif
