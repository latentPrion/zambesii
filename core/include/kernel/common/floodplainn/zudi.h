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
		class ScatterGatherList;

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
			MappedScatterGatherList(ScatterGatherList *_parent)
			:
			parent(_parent)
			{}

			error_t initialize()
			{
				//error_t		ret;

				return ERROR_SUCCESS;
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
			ScatterGatherList	*parent;
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
			enum eAddressSize { ADDR_SIZE_32, ADDR_SIZE_64 };

			ScatterGatherList(eAddressSize asize)
			:
			addressSize(asize)
			{
				memset(&udiScgthList, 0, sizeof(udiScgthList));
			}

			error_t initialize(void)
			{
				error_t		ret;

				ret = elements32.initialize();
				if (ret != ERROR_SUCCESS) { return ret; };

				ret = elements64.initialize();
				if (ret != ERROR_SUCCESS) { return ret; };

				return ERROR_SUCCESS;
			}

			~ScatterGatherList(void) {}

		public:
			status_t addFrames(paddr_t p, uarch_t nFrames)
			{
				status_t		ret;

				/**	EXPLANATION:
				 * Iterates through the list first to see if it
				 * can add the new frames to an existing
				 * element.
				 **/
				if (addressSize == ADDR_SIZE_32)
				{
					ret = addFrames(
						&elements32, p, nFrames);
				}
				else
				{
					ret = addFrames(
						&elements64, p, nFrames);
				};


				if (ret > ERROR_SUCCESS) {
					udiScgthList.scgth_num_elements++;
				};

				return ret;
			}

			/* Returns the number of elements that were discarded
			 * due to compaction.
			 **/
			uarch_t compact(void);

			// Returns a virtual mapping to this SGList.
			error_t map(MappedScatterGatherList *retMapping);
			// Returns a re-done mapping of this SGList.
			error_t remap(
				MappedScatterGatherList *oldMapping,
				MappedScatterGatherList *newMapping);

			// Destroys a mapping to this SGList.
			void unmap(MappedScatterGatherList *mapping);

		private:
			template <class scgth_elements_type>
			error_t addFrames(
				ResizeableArray<scgth_elements_type> *list,
				paddr_t p, uarch_t nFrames);

		public:
			typedef ResizeableArray<udi_scgth_element_32_t>
							SGList32;
			typedef ResizeableArray<udi_scgth_element_64_t>
							SGList64;

			eAddressSize			addressSize;
			SGList32			elements32;
			SGList64			elements64;
			udi_scgth_t			udiScgthList;
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
			:
			sGList(NULL), mapping(NULL)
			{}

			error_t initialize(void) { return ERROR_SUCCESS; }
			~DmaRequest(void);

		protected:
			DmaConstraints			constraints;
			ScatterGatherList		*sGList;
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


/**	Template definitions:
 ******************************************************************************/

inline int operator ==(paddr_t p, udi_scgth_element_32_t *s)
{
	return (p == s->block_busaddr);
}

inline int operator ==(paddr_t p, udi_scgth_element_64_t *s)
{
#if __VADDR_NBITS__ == 32 && !defined(CONFIG_ARCH_x86_32_PAE)
	struct s64BitInt
	{
		ubit32		low, high;
	} *e = (s64BitInt *)&s->block_busaddr;

	if (e->high != 0) { return 0; };
	return p == e->low;
#else
	paddr_t		*p2 = (paddr_t *)&s->block_busaddr;

	return p == *p2;
#endif
}

inline void assign_paddr_to_scgth_block_busaddr(udi_scgth_element_32_t *u32, paddr_t p)
{
#if __VADDR_NBITS__ == 32 && !defined(CONFIG_ARCH_x86_32_PAE)
	// No-pae-32bit-paddr being assigned to a 32-bit-block_busaddr.
	u32->block_busaddr = p.getLow();
#else
	/* Pae-64bit-paddr being assigned to a 32bit-block_busaddr.
	 *
	 * Requires us to cut off the high bits of the pae-64bit-paddr.
	 *
	 * In such a case, we would have clearly chosen to build a 32-bit
	 * SGList, so we should not be trying to add any frames with the high
	 * bits set.
	 **/
	if ((p >> 32).getLow() != 0)
	{
		panic("Trying to add a 64-bit paddr with high bits set, to a "
			"32-bit SGList.");
	};

	u32->block_busaddr = p.getLow();
#endif
}

inline void assign_paddr_to_scgth_block_busaddr(udi_scgth_element_64_t *u64, paddr_t p)
{
#if __VADDR_NBITS__ == 32 && !defined(CONFIG_ARCH_x86_32_PAE)
	/* No-pae-32bit-paddr being assigned to a 64-bit-block_busaddr.
	 *
	 * Just requires us to assign the paddr to the low 32 bits of the
	 * block_busaddr. We also clear the high bits to be pedantic.
	 **/
	struct s64BitInt
	{
		ubit32		low, high;
	} *e = (s64BitInt *)&u64->block_busaddr;

	e->high = 0;
	e->low = p.getLow();
#else
	// Pae-64bit-paddr being assigned to a 64-bit-block_busaddr.
	paddr_t		*p2 = (paddr_t *)&u64->block_busaddr;

	*p2 = p;
#endif
}

inline void assign_busaddr64_to_paddr(paddr_t &p, udi_busaddr64_t u64)
{
#if __VADDR_NBITS__ == 32 && !defined(CONFIG_ARCH_x86_32_PAE)
	struct s64BitInt
	{
		ubit32		low, high;
	} *s = (s64BitInt *)&u64;

	if (s->high != 0) {
		panic(CC"High bits set in udi_busaddr64_t on non-PAE build.\n");
	};

	p = s->low;
#else
	paddr_t			*p2 = (paddr_t *)&u64;

	p = *p2;
#endif
}

template <class scgth_elements_type>
error_t fplainn::Zudi::dma::ScatterGatherList::addFrames(
	ResizeableArray<scgth_elements_type> *list, paddr_t p, uarch_t nFrames
	)
{
	error_t			ret;

	/**	EXPLANATION:
	 * Returns ERROR_SUCCESS if there was no need to allocate a new
	 * SGList element.
	 *
	 * Returns 1 if a new element was created.
	 *
	 * Returns negative integer value on error.
	 **/
	for (
		typename ResizeableArray<scgth_elements_type>::Iterator it=
			list->begin();
		it != list->end();
		++it)
	{
		scgth_elements_type		*tmp=&*it;

		// Can the new paddr be prepended to this element?
		if (::operator==(p + PAGING_PAGES_TO_BYTES(nFrames), tmp))
		{
			assign_paddr_to_scgth_block_busaddr(tmp, p);
			tmp->block_length += PAGING_PAGES_TO_BYTES(nFrames);
			return ERROR_SUCCESS;
		}
		// Can the new paddr be appended to this element?
		else if (::operator==(p - tmp->block_length, tmp))
		{
			tmp->block_length += PAGING_PAGES_TO_BYTES(nFrames);
			return ERROR_SUCCESS;
		};
	};

	/* If we reached here, then we need to add a new element altogether.
	 **/
	uarch_t			prevNIndexes;
	scgth_elements_type	newElement;

	// Resize the list to hold the new SGList element.
	prevNIndexes = list->getNIndexes();
	ret = list->resizeToHoldIndex(prevNIndexes);
	if (ret != ERROR_SUCCESS) { return ret; };

	// Initialize the new element's values.
	memset(&newElement, 0, sizeof(newElement));
	assign_paddr_to_scgth_block_busaddr(&newElement, p);
	newElement.block_length = PAGING_PAGES_TO_BYTES(nFrames);

	// Finally add it to the list.
	(*list)[prevNIndexes] = newElement;
	return 1;
}

#endif

