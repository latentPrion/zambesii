#ifndef _FLOODPLAINN_STREAM_H
	#define _FLOODPLAINN_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/heapList.h>
	#include <__kclasses/resizeableIdArray.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/messageStream.h>
	#include <kernel/common/floodplainn/channel.h>
	#include <kernel/common/floodplainn/device.h>
	#include <kernel/common/floodplainn/dma.h>

	#define FPSTREAM		"FplainnStream "
	#define FPSTREAM_ID		FPSTREAM "%x: "

/**	EXPLANATION:
 * The "Floodplainn" is the name for the hardware management layer of Zambesii.
 * Specifically, it is responsible for connecting processes to hardware.
 * Floodplainn negotiates connections between processes and driver instances
 * directly. Most processes will never initiate a connection to a driver,
 * however, so most processes will never use this API. In addition to
 * facilitating connections to drivers, Floodplain also maintains the kernel's
 * :device tree.
 *
 * There are two main sub-APIs provided by Floodplainn:
 *	1. ZKCM connection API:
 *	   This API negotiates connections between a process and all the ZKCM
 *	   drivers it connects to.
 *	2. UDI connection API:
 *	   This API negotiates UDI metalanguage connections to UDI drivers.
 * (It is likely that at some point, these APIs will be merged, as ZKCM
 * is unified into Floodplainn).
 *
 * Note well that Floodplainn is not responsible for forwarding or conducting
 * messages sent over a connection that it has approved -- it is only interested
 * in set-up and tear-down of connections. "Connections" are really just
 * FIFO queues to enable the kernel to queue requests from the connected process
 * to the driver.
 *
 * Each process has its own Floodplainn Stream, and thus its own per-process
 * driver connection metadata, enabling the kernel to quickly and easily cut any
 * process off from communicating with any device. The "big picture" is that a
 * process should be able to connect to any device in the kernel device tree
 * using the Floodplainn APIs, and the user should be able to control
 * connections to devices as well.
 *
 *	ZKCM:
 * ZKCM connections are hidden; there is no need to call Floodplainn to connect
 * to a ZKCM device (for now, at least).
 *
 *	UDI:
 * Metalanguage specific binds and connections are facilitated through
 * Floodplainn. One specifies a device in the device tree, and the metalanguage
 * one wishes to use, and the kernel will, if it deems the request to be sound,
 * create the channel queues and other necessary resources to enable
 * communication with the UDI driver.
 **/

/* This is the number of elements that the ResizeableArray that holds the
 * scatter gather lists will be grown by when it needs to be resized.
 */
#define FPLAINN_STREAM_SCGTH_LIST_GROWTH_STRIDE		(16)
// Max number of scgth lists per process.
#define FPLAINN_STREAM_MAX_N_SCGTH_LISTS		(128)

class ProcessStream;

class FloodplainnStream
:
public Stream<ProcessStream>
{
	friend class Floodplainn;
public:
	FloodplainnStream(processId_t id, ProcessStream *parent)
	:
		Stream<ProcessStream>(parent, id),
	// MaxVal starts off at -1 because the array of scgth lists is empty.
	scatterGatherLists(-1)
	{};

	error_t initialize(void);
	// Release all connections.
	~FloodplainnStream(void) {};

public:
	class MetaConnection;
	class ZkcmConnection;

public:
	// Establishes a child-bind channel to a device on behalf of its parent.
	error_t connect(
		utf8Char *devName, utf8Char *metaName,
		udi_ops_vector_t *ops_vector, void *endpPrivateData1,
		uarch_t flags, fplainn::Endpoint **retendp);

	// Closes a connection to a device.
	sbit8 close(fplainn::Endpoint *endpoint, uarch_t flags);

	error_t spawnChannel(
		fplainn::Endpoint *endpoint,
		udi_index_t spawn_idx, udi_ops_vector_t *ops_vector,
		udi_init_context_t *channel_context,
		fplainn::Endpoint **retendp);

	void anchorChannel(
		fplainn::Endpoint *endpoint,
		udi_ops_vector_t *ops_vector,
		udi_init_context_t *channel_context);

	void setChannelContext(
		fplainn::Endpoint *endpoint,
		udi_init_context_t *channel_context);

	error_t send(
		fplainn::Endpoint *endp,
		udi_cb_t *gcb, udi_layout_t *layouts[3],
		utf8Char *metaName, udi_index_t meta_ops_num, udi_index_t op_idx,
		void *privateData,
		...);

	sbit8 closeChannel(fplainn::Endpoint *endpoint);

	// Should really be private...
	static error_t createChannel(
		fplainn::IncompleteD2SChannel *blueprint,
		fplainn::D2SChannel **retchan);

public:
	/**	Scatter Gather List related APIs.
	 *
	 *	EXPLANATION:
	 * This set of abstractions forms the base of DMA related functionality
	 * in Zambesii. This abstraction does not currently fully support the
	 * UDI notion of how sglists should work. In particular zbz sglists
	 * currently do not allow the owner to re-associate the sglist with a
	 * different set of allocation constraints.
	 *
	 * The constraints in zbz's sglists are set at allocation time and
	 * cannot be altered.
	 *
	 * Basically these sglist APIs return handles to sglist objects. SGList
	 * objects are transferrable between Floodplainn streams. This makes
	 * them a way of transferring frames between processes in addition to
	 * being a way to allocate memory with constraints that match a
	 * particular DMA engine's requirements.
	 **********************************************************************/
	status_t allocateScatterGatherList(
		fplainn::dma::ScatterGatherList **retlist);

	// Do not expose "justTransfer" to userspace in the syscall version.
	sbit8 releaseScatterGatherList(sarch_t id, sbit8 justTransfer);

	// Associates the list with the default "loose" constraints.
	error_t liberateScatterGatherList(sarch_t id)
		{ return constrainScatterGatherList(id, &defaultConstraints); }

	error_t constrainScatterGatherList(
		sarch_t id, fplainn::dma::constraints::Compiler *compiledCon)
	{
		fplainn::dma::ScatterGatherList		*list;

		list = getScatterGatherList(id);
		if (list == NULL)
		{
			printf(ERROR FPSTREAM_ID"constrain(%d): Failed to find "
				"requested list.\n",
				this->id, id);

			return ERROR_NOT_FOUND;
		}

		/**	FIXME:
		 * When re-constraining a previously constrained list, we may
		 * need to create an intermediate list whose frames meet the
		 * new constraints, then copy over the data currently in the
		 * list before returning.
		 *
		 * This would be the case if the new constraints are stricter
		 * than the old constraints.
		 */
		return list->constrain(compiledCon);
	}

	error_t resizeScatterGatherList(sarch_t id, uarch_t nFrames);
	status_t getNFramesInScatterGatherList(sarch_t id)
	{
		fplainn::dma::ScatterGatherList		*list;

		list = getScatterGatherList(id);
		if (list == NULL) { return ERROR_NOT_FOUND; }

		return list->getNFrames();
	}

	/**	EXPLANATION:
	 * Freezes a SGList. Freezing means that the frames list cannot
	 * be modified until the list is unfrozen. That includes resizing the
	 * list. This is effectively the implementation of
	 * udi_dma_buf_[un]map().
	 *
	 * When we DMA map the list, we don't want the list being modified while
	 * it's visible to the DMA engine.
	 *
	 * Even though it shouldn't be an issue for us to allow SGLists to be
	 * transferred between processes while they are frozen (DMA visible),
	 * we still disallow this because there's no good reason to allow it.
	 *
	 * The other thing that freezing does is IOMMU map the frames in the
	 * SGList such that they are accessible to the device.
	 **/
	status_t toggleFreezeScatterGatherList(
		sarch_t id, sbit8 freeze_or_unfreeze)
	{
		fplainn::dma::ScatterGatherList		*sgl;

		sgl = getScatterGatherList(id);
		if (sgl == NULL) {
			return ERROR_NOT_FOUND;
		}

		return sgl->toggleFreeze(freeze_or_unfreeze);
	}

	// Returns the ID of the slot in the target stream.
	enum transferScatterGatherListFlagsE {
		// Before transferring, unmap the list from the current owner
		XFER_SGLIST_FLAGS_UNMAP_FIRST = (1<<0),
		// Fakemap the list before transferring.
		XFER_SGLIST_FLAGS_FAKEMAP_FIRST = (1<<1)
	};
	error_t transferScatterGatherList(
		processId_t destStream, sarch_t srcListId,
		uarch_t flags);

	// Maps or remaps an SGList into the current process' vaddrspace.
	error_t remapScatterGatherList(sarch_t id, void **newvaddr)
	{
		fplainn::dma::ScatterGatherList		*sgl;
		error_t					ret;

		sgl = getScatterGatherList(id);
		if (sgl == NULL) {
			return ERROR_NOT_FOUND;
		}

		ret = sgl->remap();
		if (ret != ERROR_SUCCESS) {
			return ret;
		}

		*newvaddr = sgl->mapping.vaddr;
		return ERROR_SUCCESS;
	}

	/* Sets default constraints for a child device of a device instance.
	 * This is useful because the kernel requires that SGLists be
	 * constrained at the time of their allocation.
	 *
	 * This call will fail if invoked on a child that doesn't exist, or
	 * a child for which the calling parent hasn't yet at least received
	 * a UDI_CHANNEL_BOUND child bind message.
	 *
	 * The intended invocation context is for when the child has called
	 * both channel-bound to the parent and has initiated a metalanguage
	 * specific bind to that parent as well (specifically a UDI Bridge
	 * meta bind).
	 */
	error_t setChildConstraints(
		ubit32 childId, fplainn::dma::constraints::Compiler *ret);

	/* Retrieves the constraints for a parent by its ID.
	 * The kernel doesn't allow device instances to constrain SGLists
	 * in a more permissive manner than the constraints supplied for them
	 * by their most permissive parent.
	 *
	 * Therefore getParentConstraints() is the way for a device instance to
	 * obtain starting constraints from which they can derive acceptable
	 * constraints for use with their SGLists.
	 */
	error_t getParentConstraints(
		ubit16 parentId, fplainn::dma::constraints::Compiler *ret);

private:
	fplainn::dma::ScatterGatherList *getScatterGatherList(sarch_t id)
	{
		fplainn::dma::ScatterGatherList		*sgl;

		scatterGatherLists.lock();
		if (!scatterGatherLists.unlocked_indexIsValid(id))
		{
			scatterGatherLists.unlock();
			return NULL;
		}

		sgl = &scatterGatherLists[id];
		scatterGatherLists.unlock();

		if (*sgl == NULL) {
			printf(ERROR"getSGList(%d): Index refers to NULL "
				"object.\n",
				id);

			return NULL;
		}
		return sgl;
	}

private:
	friend class fplainn::FStreamEndpoint;

	error_t addEndpoint(fplainn::FStreamEndpoint *endp)
		{ return endpoints.insert(endp); }

	sbit8 removeEndpoint(fplainn::FStreamEndpoint *endp)
		{ return endpoints.remove(endp); }

	HeapList<fplainn::FStreamEndpoint>		endpoints;
	List<MetaConnection>				metaConnections;
	List<ZkcmConnection>				zkcmConnections;
	static fplainn::dma::constraints::Compiler	defaultConstraints;
	ResizeableIdArray<fplainn::dma::ScatterGatherList>
							scatterGatherLists;
};


/**	Class MetaConnection.
 * These are essentially our way of tracking the devices that a process
 * is connected to. UDI's "channel" and "child-binding" really lend
 * themselves to a very clean convergence of the host kernel and the
 * UDI environment.
 ******************************************************************************/

class FloodplainnStream::MetaConnection
{
friend class FloodplainnStream;
public:
	MetaConnection(
		fplainn::Driver::sChildBop *childBop,
		fplainn::Driver::sMetalanguage *meta,
		fplainn::FStreamEndpoint *channelEndpoint)
	:
	meta(meta), childBop(childBop), channelEndpoint(channelEndpoint)
	{}

	struct ChannelMsg
	{
		ChannelMsg(
			processId_t targetPid,
			ubit16 subsystem, ubit16 function,
	 		uarch_t size, uarch_t flags, void *privateData)
	 	:
	 	header(
			targetPid, subsystem, function,
			size, flags, privateData)
		{}

		MessageStream::sHeader		header;

	};

private:
	List<MetaConnection>::sHeader			listHeader;

	fplainn::Driver::sMetalanguage			*meta;
	fplainn::Driver::sChildBop			*childBop;
	fplainn::FStreamEndpoint			*channelEndpoint;
};

/**	Class ZkcmConnection.
 ******************************************************************************/

class FloodplainnStream::ZkcmConnection
{
};

/**	Inline methods:
 ******************************************************************************/

#endif
