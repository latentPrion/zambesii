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
}

struct sDriverInitEntry;
struct sMetaInitEntry;

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
	fplainn::Endpoint *getMgmtEndpointForCurrentDeviceInstance(void);

	/**	EXPLANATION:
	 * This sub-API is concerned with the carrying out of UDI DMA
	 * transactions.
	 **********************************************************************/
	struct dma
	{
		/**	EXPLANATION:
		 * This class encapsulates the kernel's ability to read/write
		 * from/to a DMA SGlist. SGLists are in physical memory, so
		 * the kernel must map them before it can alter their contents.
		 *
		 * This class provides methods such as memset(), memcpy(),
		 * etc.
		 **/
		class MappedScatterGatherList
		/*: public SparseBuffer*/
		{
		protected:
			struct sListNode
			{
				List<sListNode>::sHeader	listHeader;
				ubit8				*vaddr;
				uarch_t				nBytes;
			};

		public:
			MappedScatterGatherList(void)
			:
			cache(sizeof(sListNode))
			{}

			error_t initialize()
			{
				error_t		ret;

				ret = cache.initialize();
				if (ret != ERROR_SUCCESS) { return ret; }
				ret = pages.initialize();
				if (ret != ERROR_SUCCESS) { return ret; }
			}

			~MappedScatterGatherList(void) {}

		public:
			virtual void memset8(
				uarch_t offset, ubit8 value, uarch_t nBytes);
			virtual void memset16(
				uarch_t offset, ubit8 value, uarch_t nBytes);
			virtual void memset32(
				uarch_t offset, ubit8 value, uarch_t nBytes);

			virtual void memcpy(
				uarch_t offset, void *mem, uarch_t nBytes);
			virtual void memcpy(
				void *mem, uarch_t offset, uarch_t nBytes);

			void memmove(
				uarch_t destOff, uarch_t srcOff,
				uarch_t nBytes);

			error_t addPages(void *vaddr, uarch_t nBytes);
			error_t removePages(void *vaddr, uarch_t nBytes);
			void compact(void);

		protected:
			SlamCache		cache;
			List<sListNode>		pages;
		};

		/**	EXPLANATION:
		 * This is an abstraction around the udi_scgth_t type, meant to
		 * make the building and manipulation of DMA elements easier.
		 *
		 * It also provides a map() method, which causes it to read all
		 * of its scatter-gather elements and return a virtual mapping
		 * to them. The purpose of this of course, is to enable the
		 * kernel to read/write the contents stored in a particular
		 * scatter-gather list's memory.
		 **/
		class ScatterGatherList
		{
		public:
			ScatterGatherList(void);
			error_t initialize(void) { return ERROR_SUCCESS; }
			~ScatterGatherList(void) {}

		public:
			error_t map(MappedScatterGatherList *retMapping);
			error_t remap(MappedScatterGatherList *mapping);
			void unmap(MappedScatterGatherList *mapping);

		public:
			ResizeableArray<udi_scgth_element_32_t>	elements32;
			ResizeableArray<udi_scgth_element_64_t>	elements64;
			udi_scgth_t				udiScgthList;
		};

		/**	EXPLANATION:
		 * This class represents a driver's request to perform a DMA
		 * transaction. It carries directs the entire operation, from
		 * allocating the DMA SGList, to copying data to and from DMA
		 * memory, and so on.
		 *
		 * This is what Zambesii's udi_dma_constraints_t handle points
		 * to.
		 **/
		class DmaConstraints
		{
		public:
			typedef ResizeableArray<udi_dma_constraints_attr_spec_t>
				AttrArray;

			DmaConstraints(void) {}
			error_t initialize(void)
			{
				error_t		ret;

				ret = attrs.initialize();
				return ret;
			}

		public:
			void dump(void);
			static utf8Char *getAttrTypeName(
				udi_dma_constraints_attr_t a);

			sbit8 attrAlreadySet(udi_dma_constraints_attr_t a)
			{
				for (
					AttrArray::Iterator it=attrs.begin();
					it != attrs.end();
					++it)
				{
					udi_dma_constraints_attr_spec_t	*s =
						&(*it);

					if (s->attr_type == a) { return 1; };
				};

				return 0;
			}

			/* We don't need to provide a "remove()" method since
			 * that is not allowed by UDI. Can only reset() single
			 * attributes.
			 **/
			error_t addOrModifyAttrs(
				udi_dma_constraints_attr_spec_t *attrs,
				uarch_t nAttrs);

		public:
			static utf8Char		*attrTypeNames[32];
			AttrArray		attrs;
		};

		class DmaRequest
		{
		public:
			DmaRequest(udi_dma_constraints_t *)
			{}

			error_t initialize(void) { return ERROR_SUCCESS; }
			~DmaRequest(void);

		protected:
			DmaConstraints			constraints;
			ScatterGatherList		sGList;
			MappedScatterGatherList		mapping;
		};
	};

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
