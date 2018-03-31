#ifndef _FLOODPLAINN_STREAM_H
	#define _FLOODPLAINN_STREAM_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/heapList.h>
	#include <__kclasses/hardwareIdList.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/messageStream.h>
	#include <kernel/common/floodplainn/channel.h>
	#include <kernel/common/floodplainn/device.h>
	#include <kernel/common/floodplainn/dma.h>

	#define FPSTREAM		"FplainnStream "

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

class ProcessStream;

class FloodplainnStream
:
public Stream<ProcessStream>
{
public:
	FloodplainnStream(processId_t id, ProcessStream *parent)
	:
	Stream<ProcessStream>(parent, id)
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
	status_t allocateScatterGatherList(fplainn::dma::ScatterGatherList *retlist);
	sbit8 releaseScatterGatherList(fplainn::dma::ScatterGatherList *retlist);
	// Associate an SGList with a set of constraints.
	error_t constrainScatterGatherList(
		fplainn::dma::ScatterGatherList *list,
		fplainn::dma::Constraints *constraints);
	error_t liberateScatterGatherList(fplainn::dma::ScatterGatherList *list)
	{
		// Passing NULL associates the list with a default "loose" list.
		return constrainScatterGatherList(list, &defaultConstraints);
	}

	enum transferScatterGatherListFlagsE {
		// Before transferring, unmap the list from the current owner
		XFER_SGLIST_FLAGS_UNMAP_FIRST = (1<<0),
		// Fakemap the list before transferring.
		XFER_SGLIST_FLAGS_FAKEMAP_FIRST = (1<<1)
	};
	error_t transferScatterGatherList(
		processId_t destStream, fplainn::dma::ScatterGatherList *srcList,
		uarch_t flags);

private:
	friend class fplainn::FStreamEndpoint;

	error_t addEndpoint(fplainn::FStreamEndpoint *endp)
		{ return endpoints.insert(endp); }

	sbit8 removeEndpoint(fplainn::FStreamEndpoint *endp)
		{ return endpoints.remove(endp); }

	HeapList<fplainn::FStreamEndpoint>	endpoints;
	List<MetaConnection>			metaConnections;
	List<ZkcmConnection>			zkcmConnections;
	fplainn::dma::Constraints		defaultConstraints;
	HardwareIdList				scatterGatherLists;
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
