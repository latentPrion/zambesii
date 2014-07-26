#ifndef _FLOODPLAINN_STREAM_H
	#define _FLOODPLAINN_STREAM_H

	#include <__kstdlib/__ktypes.h>
	//#include <__kclasses/singleWaiterQueue.h>
	#include <kernel/common/stream.h>

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
public Stream
{
public:
	FloodplainnStream(processId_t id, ProcessStream *parent)
	:
	Stream(id), parent(parent)
	{};

	error_t initialize(void) { return ERROR_SUCCESS; }
	// Release all connections.
	~FloodplainnStream(void) {};

private:
	// Allocate and deallocate queues.
	// error_t zkcmConnect(SingleWaiterQueue *queue);
	// error_t zkcmDisconnect(SingleWaiterQueue *queue);

private:
	ProcessStream		*parent;
};

#endif

