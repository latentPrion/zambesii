
#include <debug.h>
#define UDI_VERSION		0x101
#include <udi.h>
#undef UDI_VERSION
#include <__kstdlib/callback.h>
#include <__kstdlib/__kcxxlib/memory>
#include <kernel/common/thread.h>
#include <kernel/common/messageStream.h>
#include <kernel/common/zasyncStream.h>
#include <kernel/common/floodplainn/zum.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/floodplainn/movableMemory.h>
#include <kernel/common/floodplainn/initInfo.h>
#include <kernel/common/floodplainn/floodplainnStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <commonlibs/libzbzcore/libzbzcore.h>

/**	EXPLANATION:
 * Like the ZUI server, the ZUM server will also take commands over a ZAsync
 * Stream connection.
 *
 * This thread essentially implements all the logic required to allow the kernel
 * to call into a device on its udi_mgmt channel. It doesn't care about any
 * other channels.
 **/

udi_ops_vector_t		dummyMgmtMaOpsVector=0;

namespace zumServer
{
	void zasyncHandler(
		ZAsyncStream::sZAsyncMsg *msg,
		fplainn::Zum::sZAsyncMsg *request,
		Thread *self);

	// Reduces code duplication and increases readability.
	fplainn::Zum::sZumMsg *getNewZumMsg(
		utf8Char *funcName, utf8Char *devicePath,
		processId_t targetPid, ubit16 subsystem, ubit16 function,
		uarch_t size, uarch_t flags, void *privateData)
	{
		fplainn::Zum::sZumMsg		*ret;

		ret = new fplainn::Zum::sZumMsg(
			targetPid, subsystem, function,
			size, flags, privateData);

		if (ret == NULL)
		{
			printf(ERROR ZUM"%s %s: Failed to alloc async ctxt.\n"
				"\tCaller may be halted indefinitely.\n",
				funcName, devicePath);
		}

		return ret;
	}

	namespace start
	{
		void startDeviceReq(
			ZAsyncStream::sZAsyncMsg *msg,
			fplainn::Zum::sZAsyncMsg *request,
			Thread *self);

	// PRIVATE:
		class StartDeviceReqCb;
		typedef void (startDeviceReqCbFn)(
			MessageStream::sHeader *msg,
			fplainn::Zum::sZumMsg *ctxt,
			Thread *self,
			fplainn::Device *dev);

		startDeviceReqCbFn	startDeviceReq1;
		startDeviceReqCbFn	startDeviceReq2;
		startDeviceReqCbFn	startDeviceReq3;
	}

	namespace enumerateChildren
	{
		void enumerateChildrenReq(
			ZAsyncStream::sZAsyncMsg *msg,
			fplainn::Zum::sZAsyncMsg *request,
			Thread *self);

	// PRIVATE:
		typedef start::StartDeviceReqCb EnumerateChildrenReqCb;
		typedef start::startDeviceReqCbFn enumerateChildrenReqCbFn;

		enumerateChildrenReqCbFn	enumerateChildrenReq1;
		enumerateChildrenReqCbFn	enumerateChildrenReq2;
		enumerateChildrenReqCbFn	enumerateChildrenReq3;
	}

	namespace postManagementCb
	{
		void postManagementCbReq(
			ZAsyncStream::sZAsyncMsg *msg,
			fplainn::Zum::sZAsyncMsg *request,
			Thread *self);

	// PRIVATE:
		typedef start::StartDeviceReqCb PostManagementCbReqCb;
		typedef start::startDeviceReqCbFn postManagementCbReqCbFn;

		postManagementCbReqCbFn		postManagementCbReq1;
	}

	namespace mgmt
	{
		void usageInd(
			ZAsyncStream::sZAsyncMsg *msg,
			fplainn::Zum::sZAsyncMsg *request,
			Thread *self);

		void enumerateReq(
			ZAsyncStream::sZAsyncMsg *msg,
			fplainn::Zum::sZAsyncMsg *request,
			Thread *self);

		void deviceManagementReq(
			ZAsyncStream::sZAsyncMsg *msg,
			fplainn::Zum::sZAsyncMsg *request,
			Thread *self);

		void finalCleanupReq(
			ZAsyncStream::sZAsyncMsg *msg,
			fplainn::Zum::sZAsyncMsg *request,
			Thread *self);

		void channelEventInd(
			ZAsyncStream::sZAsyncMsg *msg,
			fplainn::Zum::sZAsyncMsg *request,
			Thread *self);

	// PRIVATE:
		typedef start::StartDeviceReqCb		MgmtReqCb;
		typedef start::startDeviceReqCbFn	mgmtReqCbFn;

		mgmtReqCbFn			usageRes;
		mgmtReqCbFn			enumerateAck;
		mgmtReqCbFn			deviceManagementAck;
		mgmtReqCbFn			finalCleanupAck;
		mgmtReqCbFn			channelEventComplete;

		// Reduces code duplication and improves readability.
		static error_t getDeviceHandleAndMgmtEndpoint(
			utf8Char *funcName,
			utf8Char *devicePath,
			fplainn::Device **retdev, fplainn::Endpoint **retendp)
		{
			error_t		ret;

			ret = floodplainn.getDevice(devicePath, retdev);
			if (ret != ERROR_SUCCESS)
			{
				printf(ERROR ZUM"%s %s: Invalid device name.\n",
					funcName, devicePath);

				return ret;
			};

			*retendp = (*retdev)->instance->mgmtEndpoint;
			if (*retendp == NULL)
			{
				printf(ERROR ZUM"%s %s: Not connected to "
					"device. Try startDeviceReq first.\n",
					funcName, devicePath);

				return ERROR_INVALID_STATE;
			};

			return ERROR_SUCCESS;
		}
	}
}

#define ZUM_ASYNC_MSG_EXTRA_NPAGES		2
void fplainn::Zum::main(void *)
{
	Thread				*self;
	MessageStream::sHeader		*iMsg;
	HeapObj<sZAsyncMsg>		requestData;
	const sMetaInitEntry		*mgmtInitEntry;

	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	requestData = getNewSZAsyncMsg(
		CC __func__, CC"NONE:MAIN",
		fplainn::Zum::sZAsyncMsg::OP_CHANNEL_EVENT_IND,
		PAGING_BASE_SIZE * ZUM_ASYNC_MSG_EXTRA_NPAGES);

//	requestData = new sZAsyncMsg(NULL, 0, PAGING_BASE_SIZE * 2);
	if (requestData.get() == NULL)
	{
		printf(ERROR ZUM"main: failed to alloc request data mem.\n");
		taskTrib.kill(self);
		return;
	};

	mgmtInitEntry = floodplainn.zudi.findMetaInitInfo(CC"udi_mgmt");
	if (mgmtInitEntry == NULL)
	{
		printf(FATAL ZUM"main: failed to get udi_mgmt meta_init.\n");
		taskTrib.kill(self);
		return;
	};

	fplainn::MetaInit	metaInitParser(mgmtInitEntry->udi_meta_info);

	// Construct the fast lookup arrays of visible and marshal layouts.
	for (uarch_t i=1; i<=4; i++)
	{
		udi_mei_op_template_t		*opTemplate;

		opTemplate = metaInitParser.getOpTemplate(UDI_MGMT_OPS_NUM, i);
		if (opTemplate == NULL)
		{
			printf(FATAL ZUM"main: failed to get udi_mgmt op "
				"template for ops_idx %d.\n",
				i);

			taskTrib.kill(self);
			return;
		};

		__klzbzcore::region::channel::mgmt::layouts::visible[i] =
			opTemplate->visible_layout;

		__klzbzcore::region::channel::mgmt::layouts::marshal[i] =
			opTemplate->marshal_layout;
	};

	printf(NOTICE ZUM"main: running, tid %x.\n", self->getFullId());

	for (;FOREVER;)
	{
		ZAsyncStream::sZAsyncMsg	*zAMsg;

		self->messageStream.pull(&iMsg);
		switch (iMsg->subsystem)
		{
		case MSGSTREAM_SUBSYSTEM_ZASYNC:
			zAMsg = (ZAsyncStream::sZAsyncMsg *)iMsg;

			zumServer::zasyncHandler(zAMsg, requestData.get(), self);
			break;

		default:
			Callback	*callback;

			callback = (Callback *)iMsg->privateData;
			if (callback == NULL)
			{
				printf(WARNING ZUM"main: message with no "
					"callback from %x.\n",
					iMsg->sourceId);

				continue;
			};

			(*callback)(iMsg);
			delete callback;
		};

		delete iMsg;
	};

	taskTrib.kill(self);
}

void zumServer::zasyncHandler(
	ZAsyncStream::sZAsyncMsg *msg,
	fplainn::Zum::sZAsyncMsg *request,
	Thread *self
	)
{
	error_t			err;

	if (msg->dataNBytes > sizeof(*request) + PAGING_BASE_SIZE * ZUM_ASYNC_MSG_EXTRA_NPAGES)
	{
		printf(WARNING ZUM"incoming request from %x has odd size. "
			"Rejected.\n",
			msg->header.sourceId);

		return;
	};

	err = self->parent->zasyncStream.receive(
		msg->dataHandle, request, 0);

	if (err != ERROR_SUCCESS)
	{
		printf(WARNING ZUM"receive() failed on request from %x.\n",
			msg->header.sourceId);

		return;
	};

	switch (request->opsIndex)
	{
	case fplainn::Zum::sZAsyncMsg::OP_USAGE_IND:
		mgmt::usageInd(msg, request, self);
		break;
	case fplainn::Zum::sZAsyncMsg::OP_ENUMERATE_REQ:
		mgmt::enumerateReq(msg, request, self);
		break;
	case fplainn::Zum::sZAsyncMsg::OP_DEVMGMT_REQ:
		mgmt::deviceManagementReq(msg, request, self);
		break;
	case fplainn::Zum::sZAsyncMsg::OP_FINAL_CLEANUP_REQ:
		mgmt::finalCleanupReq(msg, request, self);
		break;
	case fplainn::Zum::sZAsyncMsg::OP_CHANNEL_EVENT_IND:
		mgmt::channelEventInd(msg, request, self);
		break;
	case fplainn::Zum::sZAsyncMsg::OP_START_REQ:
		start::startDeviceReq(msg, request, self);
		break;
	case fplainn::Zum::sZAsyncMsg::OP_ENUMERATE_CHILDREN_REQ:
		enumerateChildren::enumerateChildrenReq(msg, request, self);
		break;
	case fplainn::Zum::sZAsyncMsg::OP_POST_MANAGEMENT_CB_REQ:
		postManagementCb::postManagementCbReq(msg, request, self);
		break;

	default:
		printf(WARNING ZUM"request from %x is for invalid ops_idx "
			"into udi_mgmt_ops_vector_t for device %s.\n",
			msg->header.sourceId,
			request->path);
	};
}

class zumServer::start::StartDeviceReqCb
: public _Callback<startDeviceReqCbFn>
{
	fplainn::Zum::sZumMsg *ctxt;
	Thread *self;
	fplainn::Device *dev;

public:
	StartDeviceReqCb(
		startDeviceReqCbFn *fn,
		fplainn::Zum::sZumMsg *ctxt, Thread *self, fplainn::Device *dev)
	: _Callback<startDeviceReqCbFn>(fn),
	ctxt(ctxt), self(self), dev(dev)
	{}

	virtual void operator()(MessageStream::sHeader *iMsg)
		{ function(iMsg, ctxt, self, dev); }
};

void zumServer::start::startDeviceReq(
	ZAsyncStream::sZAsyncMsg *msg,
	fplainn::Zum::sZAsyncMsg *request,
	Thread *self
	)
{
	fplainn::Zum::sZumMsg		*ctxt;
	error_t				err;
	fplainn::Endpoint		*endp;
	AsyncResponse			myResponse;
	fplainn::Device			*dev;

	ctxt = getNewZumMsg(
		CC __func__, request->path,
		msg->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUM, request->opsIndex,
		sizeof(*ctxt), msg->header.flags, msg->header.privateData);

	if (ctxt == NULL) { return; };
	// Copy the request data over.
	::new (&ctxt->info) fplainn::Zum::sZAsyncMsg(*request);
	ctxt->info.params.start.ibind.nChannels = -1;
	ctxt->info.params.start.pbind.nChannels = -1;

	myResponse(ctxt);

	// Does the requested device even exist?
	err = floodplainn.getDevice(ctxt->info.path, &dev);
	if (err != ERROR_SUCCESS) {
		myResponse(err); return;
	};

	/* First thing is to connect() to the device. We store the handle to
	 * our endpoint of the udi_mgmt channel to every device within its
	 * fplainn::DeviceInstance object.
	 *
	 * We can make sure that we haven't already connect()ed to the device by
	 * checking for it first before proceeding.
	 **/
	// Can't think of anything meaningful to use as the endpoint privdata.
	err = self->parent->floodplainnStream.connect(
		ctxt->info.path, CC"udi_mgmt",
		&dummyMgmtMaOpsVector, NULL, 0,
		&endp);

	if (err != ERROR_SUCCESS) {
		myResponse(err); return;
	};

	dev->instance->setMgmtEndpoint(
		static_cast<fplainn::FStreamEndpoint *>(endp));

	/* Now we have our connection. Start sending the initialization
	 * sequence (UDI Core Specification, section 24.2.1):
	 *	udi_usage_ind()
	 *	for (EACH; INTERNAL; BIND; CHILD; ENDPOINT) {
	 *		udi_channel_event_ind(UDI_CHANNEL_BOUND);
	 *	};
	 *	udi_channel_event_ind(parent_bind_channel, UDI_CHANNEL_BOUND);
	 *
	 * And that's it, friends. We'd have at that point, a running UDI
	 * driver, or as the spec describes it, a driver that is "open for
	 * business".
	 **/
	floodplainn.zum.usageInd(
		ctxt->info.path, ctxt->info.params.usage.resource_level,
		new StartDeviceReqCb(startDeviceReq1, ctxt, self, dev));

	myResponse(DONT_SEND_RESPONSE);
}

void zumServer::start::startDeviceReq1(
	MessageStream::sHeader *msg,
	fplainn::Zum::sZumMsg *ctxt,
	Thread *self,
	fplainn::Device *dev
	)
{
	AsyncResponse				myResponse;
	fplainn::Zum::sZumMsg			*response;
	HeapList<fplainn::Channel>::Iterator	chanIt;

	(void)response;

	/**	EXPLANATION:
	 * We have to send a udi_channel_event_ind(UDI_CHANNEL_BOUND) to each
	 * internal bind channel on the child end of the channel.
	 **/
	response = (fplainn::Zum::sZumMsg *)msg;

	myResponse(ctxt);

	ctxt->info.params.start.ibind.nChannels = 0;
	ctxt->info.params.start.ibind.nSuccesses = 0;
	ctxt->info.params.start.ibind.nFailures = 0;
	for (chanIt=dev->instance->channels.begin(0);
		chanIt != dev->instance->channels.end(); ++chanIt)
	{
		fplainn::Channel	*currChan=*chanIt;

		if (currChan->bindChannelType
			!= fplainn::Channel::BIND_CHANNEL_TYPE_INTERNAL)
			{ continue; };

		ctxt->info.params.start.ibind.nChannels++;
		floodplainn.zum.internalChannelEventInd(
			ctxt->info.path, currChan->endpoints[0],
			UDI_CHANNEL_BOUND,
			new StartDeviceReqCb(startDeviceReq2, ctxt, self, dev));
	};

	myResponse(DONT_SEND_RESPONSE);
	if (ctxt->info.params.start.ibind.nChannels == 0)
	{
		/* If there were no secondary regions, (ergo no internal bind
		 * channels), we should just continue to the next sequence.
		 **/
		startDeviceReq2(msg, ctxt, self, dev);
	};
}

void zumServer::start::startDeviceReq2(
	MessageStream::sHeader *msg,
	fplainn::Zum::sZumMsg *ctxt,
	Thread *self,
	fplainn::Device *dev
	)
{
	fplainn::Zum::sZumMsg			*response;
	AsyncResponse				myResponse;
	HeapList<fplainn::Channel>::Iterator	chanIt;

	/**	EXPLANATION:
	 * In here we must tally both the failures and successes among the
	 * regions of the device.
	 *
	 * When all regions have responded, only if all have been successful
	 * will the MA proceed with allowing the startDeviceReq sequence to
	 * continue.
	 **/
	response = (fplainn::Zum::sZumMsg *)msg;

	myResponse(ctxt);

	if (ctxt->info.params.start.ibind.nChannels > 0)
	{
		if (response->info.params.channel_event.status == UDI_OK) {
			ctxt->info.params.start.ibind.nSuccesses++;
		} else {
			ctxt->info.params.start.ibind.nFailures++;
		};

		if (ctxt->info.params.start.ibind.nSuccesses
			+ ctxt->info.params.start.ibind.nFailures
			< ctxt->info.params.start.ibind.nChannels)
		{
			/* Don't let the response be sent until all secondary
			 * regions have responded. Just keep return()ing until
			 * the last one responds.
			 **/
			myResponse(DONT_SEND_RESPONSE);
			return;
		};
	};

	if (ctxt->info.params.start.ibind.nSuccesses
		< ctxt->info.params.start.ibind.nChannels)
	{
		myResponse(ERROR_INITIALIZATION_FAILURE);
		return;
	};

	/* Last step is to send a udi_channel_event_ind to the device's
	 * parent bind channels, and then we can officially say that the
	 * device is "open for business", to use the phrase from the spec.
	 **/
	ctxt->info.params.start.pbind.nChannels = 0;
	ctxt->info.params.start.pbind.nSuccesses = 0;
	ctxt->info.params.start.pbind.nFailures = 0;
	for (chanIt=dev->instance->channels.begin(0);
		chanIt != dev->instance->channels.end(); ++chanIt)
	{
		fplainn::Channel		*currChan=*chanIt;

		if (currChan->bindChannelType !=
			fplainn::Channel::BIND_CHANNEL_TYPE_CHILD)
			{ continue; };

		/* All child bind channels between drivers will be D2D, as
		 * opposed to child bind channels that connect() an application
		 * process to a driver, which will be D2S.
		 **/
		if (currChan->getType() != fplainn::Channel::TYPE_D2D)
			{ continue; };

		ctxt->info.params.start.pbind.nChannels++;
		floodplainn.zum.parentChannelEventInd(
			ctxt->info.path, currChan->endpoints[0],
			UDI_CHANNEL_BOUND,
			currChan->parentId,
			new StartDeviceReqCb(startDeviceReq3, ctxt, self, dev));
	};

	myResponse(DONT_SEND_RESPONSE);
	if (ctxt->info.params.start.pbind.nChannels == 0)
	{
		/* If there are no parent bind channels to work with, just
		 * immediately call the next sequence.
		 **/
		startDeviceReq3(msg, ctxt, self, dev);
	};
}

void zumServer::start::startDeviceReq3(
	MessageStream::sHeader *msg,
	fplainn::Zum::sZumMsg *ctxt,
	Thread *,
	fplainn::Device *
	)
{
	fplainn::Zum::sZumMsg			*response;
	AsyncResponse				myResponse;

	response = (fplainn::Zum::sZumMsg *)msg;

	myResponse(ctxt);
	myResponse(msg->error);

	if (ctxt->info.params.start.pbind.nChannels > 0)
	{
		if (response->info.params.channel_event.status == UDI_OK) {
			ctxt->info.params.start.pbind.nSuccesses++;
		} else {
			ctxt->info.params.start.pbind.nFailures++;
		};

		if (ctxt->info.params.start.pbind.nSuccesses
			+ ctxt->info.params.start.pbind.nFailures
			< ctxt->info.params.start.pbind.nChannels)
		{
			/* Don't let the response be sent until all secondary
			 * regions have responded. Just keep return()ing until
			 * the last one responds.
			 **/
			myResponse(DONT_SEND_RESPONSE);
			return;
		};
	};

	// Else, we now have a successfully initialized driver. Done.
	myResponse(ERROR_SUCCESS);
}

void zumServer::enumerateChildren::enumerateChildrenReq(
	ZAsyncStream::sZAsyncMsg *msg,
	fplainn::Zum::sZAsyncMsg *request,
	Thread *self
	)
{
	fplainn::Zum::sZumMsg		*ctxt;
	error_t				err;
//	fplainn::Endpoint		*endp;
	AsyncResponse			myResponse;
	fplainn::Device			*dev;

	ctxt = getNewZumMsg(
		CC __func__, request->path,
		msg->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUM, request->opsIndex,
		sizeof(*ctxt), msg->header.flags, msg->header.privateData);

	if (ctxt == NULL) { return; };
	// Copy the request data over.
	::new (&ctxt->info) fplainn::Zum::sZAsyncMsg(*request);

	myResponse(ctxt);

	// Does the requested device even exist?
	err = floodplainn.getDevice(ctxt->info.path, &dev);
	if (err != ERROR_SUCCESS) {
		myResponse(err); return;
	};

	// Prepare the buffer page.
	ctxt->info.params.enumerateChildren.nDeviceIds = 0;
	ctxt->info.params.enumerateChildren.deviceIdsHandle =
		new (processTrib.__kgetStream()->memoryStream.memAlloc(1, 0)) ubit32;

	if (ctxt->info.params.enumerateChildren.deviceIdsHandle == NULL)
	{
		printf(ERROR ZUM"enumChildReq(%s):No memory for device ID "
			"buffer.\n",
			ctxt->info.path);

		myResponse(ERROR_MEMORY_NOMEM); return;
	};

	/**	EXPLANATION:
	 * Now we just cycle through, calling enumerateReq on the device until
	 * it tells us UDI_ENUMERATE_DONE.
	 **/
	floodplainn.zum.enumerateReq(
		ctxt->info.path,
		(FLAG_TEST(
			ctxt->info.params.enumerateChildren.flags,
			ZUM_ENUMCHILDREN_FLAGS_UNCACHED_SCAN)
			? UDI_ENUMERATE_START_RESCAN : UDI_ENUMERATE_START),
		&request->params.enumerateChildren.cb,
		new EnumerateChildrenReqCb(
			enumerateChildrenReq1, ctxt, self, dev));

	myResponse(DONT_SEND_RESPONSE);
}

void zumServer::enumerateChildren::enumerateChildrenReq1(
	MessageStream::sHeader *msg,
	fplainn::Zum::sZumMsg *ctxt,
	Thread *self,
	fplainn::Device *dev)
{
	error_t					err;
	fplainn::Driver::sMetalanguage		*enumeratingMeta;
	fplainn::Device				*newDevice;
//	fplainn::Endpoint			*endp;
	AsyncResponse				myResponse;
	fplainn::Zum::sZumMsg			*response;
	sbit8					loopAgain=0,
						clearBuffer=0, releaseBuffer=0;
	HeapArr<udi_instance_attr_list_t>	enumAttrs;
	const char 				*ueaStrings[] =
	{
		"OK", "LEAF", "DONE", "RESCAN", "REMOVED", "REMOVED_SELF",
		"RELEASED", "FAILED"
	};

	(void)ueaStrings;
	response = (fplainn::Zum::sZumMsg *)msg;
	myResponse(ctxt);

	/**	EXPLANATION:
	 * We're now going to construct a loop. The loop will process the result
	 * passed back from the device (udi_enumerate_ack()).
	 *
	 * For each new device detected, we will add the new device's ID string
	 * to a buffer of strings. Since we do not know how many child devices
	 * the target device will detect, we cannot predict in advance how large
	 * the buffer must be.
	 *
	 *	* For each new device, we add it to the device tree.
	 *	* For each already-known device, we modify its attributes based
	 *		on the information newly reported by the target device.
	 **/

	switch (response->info.params.enumerate.enumeration_result)
	{
	case UDI_ENUMERATE_OK:
		enumeratingMeta = dev->driverInstance->driver
			->lookupMetalanguageForDriverOpsIndex(
			response->info.params.enumerate.ops_idx);

		if (enumeratingMeta == NULL)
		{
			printf(ERROR ZUM"enumChildren %s: ops_idx %d in "
				"enumerating parent isn't known to kernel.\n",
				ctxt->info.path,
				response->info.params.enumerate.ops_idx);

			myResponse(ERROR_NOT_FOUND);
			break;
		}

		err = floodplainn.createDevice(
			ctxt->info.path, CHIPSET_NUMA_SHBANKID,
			response->info.params.enumerate.cb.child_ID,
			enumeratingMeta->name,
			0, &newDevice);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR ZUM"enumChildren %s: Failed to add child "
				"%d.\n",
				ctxt->info.path,
				response->info.params.enumerate.cb.child_ID);

			myResponse(err);
			// Let it gracefully break out. Don't "return".
			break;
		};

		// Add the new device's child_ID to the buffer.
		ctxt->info.params.enumerateChildren.deviceIdsHandle[
			ctxt->info.params.enumerateChildren.nDeviceIds++]
			= response->info.params.enumerate.cb.child_ID;

		enumAttrs = new udi_instance_attr_list_t[
			response->info.params.enumerate.cb.attr_valid_length];

		if (enumAttrs == NULL)
		{
			printf(ERROR ZUM"enumChildren %s: Failed to alloc mem "
				"for attrs for new device (childId %d).\n",
				ctxt->info.path,
				response->info.params.enumerate.cb.child_ID);

			myResponse(ERROR_MEMORY_NOMEM);
			break;
		}

		floodplainn.zum.getEnumerateReqAttrsAndFilters(
			&response->info.params.enumerate.cb,
			enumAttrs.get(), NULL);

		for (uarch_t i=0;
			i<response->info.params.enumerate.cb.attr_valid_length;
			i++)
		{
			err = newDevice->addEnumerationAttribute(&enumAttrs[i]);
			if (err != ERROR_SUCCESS)
			{
				printf(ERROR ZUM"enumChildren %s: Failed to "
					"add enum attr #%d of new dev childId "
					"%d.\n",
					ctxt->info.path,
					i,
					response->info.params.enumerate.cb.child_ID);

				myResponse(err);
				break;
			}
		}

		loopAgain = 1;
		break;

	case UDI_ENUMERATE_LEAF: releaseBuffer = clearBuffer = 1; break;
	case UDI_ENUMERATE_DONE: break;
	case UDI_ENUMERATE_REMOVED: break;
	case UDI_ENUMERATE_REMOVED_SELF:
		/**	TODO:
		 * Will need to do some analysis and design to determine how to
		 * handle this event.
		 **/
		break;
	// Should only be called in response to UDI_ENUMERATE_RELEASE
	case UDI_ENUMERATE_RELEASED: break;
	// We don't handle these. Let the caller handle them.
	case UDI_ENUMERATE_RESCAN:
	case UDI_ENUMERATE_FAILED:
		break;

	default:
		printf(WARNING ZUM"enumChildren %s: device responded with "
			"unknown enumeration_result %d.\n",
			ctxt->info.path,
			response->info.params.enumerate.enumeration_result);

		break;
	};

	if (clearBuffer) {
		ctxt->info.params.enumerateChildren.nDeviceIds = 0;
	};

	if (releaseBuffer)
	{
		processTrib.__kgetStream()->memoryStream.memFree(
			ctxt->info.params.enumerateChildren.deviceIdsHandle);

		ctxt->info.params.enumerateChildren.deviceIdsHandle = NULL;
	};

	if (loopAgain)
	{
		floodplainn.zum.enumerateReq(
			ctxt->info.path,
			UDI_ENUMERATE_NEXT,
			&ctxt->info.params.enumerateChildren.cb,
			new EnumerateChildrenReqCb(
				enumerateChildrenReq1, ctxt, self, dev));

		// Prevent response from being sent until the end of the loop.
		myResponse(DONT_SEND_RESPONSE);
	}
	else
	{
		// Allow the response to be sent. Nothing really to do here.
		ctxt->info.params.enumerateChildren.final_enumeration_result =
			response->info.params.enumerate.enumeration_result;
	};
}

void zumServer::postManagementCb::postManagementCbReq(
	ZAsyncStream::sZAsyncMsg *msg,
	fplainn::Zum::sZAsyncMsg *request,
	Thread *self
	)
{
	fplainn::Zum::sZumMsg		*ctxt;
	error_t				err;
//	fplainn::Endpoint		*endp;
	AsyncResponse			myResponse;
	fplainn::Device			*dev;

	ctxt = getNewZumMsg(
		CC __func__, request->path,
		msg->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUM, request->opsIndex,
		sizeof(*ctxt), msg->header.flags, msg->header.privateData);

	if (ctxt == NULL) { return; };
	// Copy the request data over.
	::new (&ctxt->info) fplainn::Zum::sZAsyncMsg(*request);

	myResponse(ctxt);

	// Does the requested device even exist?
	err = floodplainn.getDevice(ctxt->info.path, &dev);
	if (err != ERROR_SUCCESS) {
		myResponse(err); return;
	};

	/**	EXPLANATION:
	 * Now we just cycle through, calling enumerateReq on the device until
	 * it tells us UDI_ENUMERATE_DONE.
	 **/
	floodplainn.zum.enumerateReq(
		ctxt->info.path,
		UDI_ENUMERATE_NEW,
		&request->params.postManagementCb.cb,
		new PostManagementCbReqCb(
			&postManagementCbReq1, ctxt, self, dev));

	myResponse(DONT_SEND_RESPONSE);
}

void zumServer::postManagementCb::postManagementCbReq1(
	MessageStream::sHeader *msg,
	fplainn::Zum::sZumMsg *ctxt,
	Thread *self,
	fplainn::Device *dev)
{
	error_t					err;
	fplainn::Device				*newDevice;
	fplainn::Driver::sMetalanguage		*enumeratingMeta;
//	fplainn::Endpoint			*endp;
	AsyncResponse				myResponse;
	fplainn::Zum::sZumMsg			*response;
	HeapArr<udi_instance_attr_list_t>	enumAttrs;
	sbit8					loopAgain=0;
	const char 				*ueaStrings[] =
	{
		"OK", "LEAF", "DONE", "RESCAN", "REMOVED", "REMOVED_SELF",
		"RELEASED", "FAILED"
	};

	(void)ueaStrings;
	response = (fplainn::Zum::sZumMsg *)msg;
	myResponse(ctxt);

	/**	EXPLANATION:
	 * We now just sit waiting for the device to return our posted CB.
	 * Depending on the response we get, we will take one or more actions.
	 **/

	switch (response->info.params.enumerate.enumeration_result)
	{
	case UDI_ENUMERATE_OK:
		/* We need to know the metaName of the metalanguage which
		 * this device was enumerated with.
		 */
		enumeratingMeta = dev->driverInstance->driver
			->lookupMetalanguageForDriverOpsIndex(
			response->info.params.enumerate.ops_idx);

		if (enumeratingMeta == NULL)
		{
			printf(ERROR ZUM"enumChildren %s: ops_idx %d in "
				"enumerating parent isn't known to kernel.\n",
				ctxt->info.path,
				response->info.params.enumerate.ops_idx);

			myResponse(ERROR_NOT_FOUND);
			break;
		}

		err = floodplainn.createDevice(
			ctxt->info.path, CHIPSET_NUMA_SHBANKID,
			response->info.params.enumerate.cb.child_ID,
			enumeratingMeta->name,
			0, &newDevice);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR ZUM"postMgmtCb %s: Failed to add child "
				"%d.\n",
				ctxt->info.path,
				response->info.params.postManagementCb.cb.child_ID);

			myResponse(err);
			// Let it gracefully break out. Don't "return".
			break;
		};

		enumAttrs = new udi_instance_attr_list_t[
			response->info.params.enumerate.cb.attr_valid_length];

		if (enumAttrs == NULL)
		{
			printf(ERROR ZUM"enumChildren %s: Failed to alloc mem "
				"for attrs for new device (childId %d).\n",
				ctxt->info.path,
				response->info.params.enumerate.cb.child_ID);

			myResponse(ERROR_MEMORY_NOMEM);
			break;
		}

		floodplainn.zum.getEnumerateReqAttrsAndFilters(
			&response->info.params.enumerate.cb,
			enumAttrs.get(), NULL);

		for (uarch_t i=0;
			i<response->info.params.enumerate.cb.attr_valid_length;
			i++)
		{
			err = newDevice->addEnumerationAttribute(&enumAttrs[i]);
			if (err != ERROR_SUCCESS)
			{
				printf(ERROR ZUM"enumChildren %s: Failed to "
					"add enum attr #%d of new dev childId "
					"%d.\n",
					ctxt->info.path,
					i,
					response->info.params.enumerate.cb.child_ID);

				myResponse(err);
				break;
			}
		}

		loopAgain = 1;
		break;

	case UDI_ENUMERATE_LEAF: break;
	case UDI_ENUMERATE_DONE: break;
	case UDI_ENUMERATE_REMOVED:
		// This is more complicated.
		break;
	case UDI_ENUMERATE_REMOVED_SELF:
		/**	TODO:
		 * Will need to do some analysis and design to determine how to
		 * handle this event.
		 **/
		break;
	// Should only be called in response to UDI_ENUMERATE_RELEASE
	case UDI_ENUMERATE_RELEASED: break;
	// We don't handle these. Let the caller handle them.
	case UDI_ENUMERATE_RESCAN:
	case UDI_ENUMERATE_FAILED:
		break;

	default:
		printf(WARNING ZUM"enumChildren %s: device responded with "
			"unknown enumeration_result %d.\n",
			ctxt->info.path,
			response->info.params.enumerate.enumeration_result);

		break;
	};

	if (loopAgain)
	{
		floodplainn.zum.enumerateReq(
			ctxt->info.path,
			UDI_ENUMERATE_NEW,
			&ctxt->info.params.postManagementCb.cb,
			new PostManagementCbReqCb(
				postManagementCbReq1, ctxt, self, dev));

		// Prevent response from being sent until the end of the loop.
		myResponse(DONT_SEND_RESPONSE);
	}
	else
	{
		// Allow the response to be sent. Nothing really to do here.
		ctxt->info.params.postManagementCb.enumeration_result =
			response->info.params.enumerate.enumeration_result;
	};
}

void zumServer::mgmt::usageInd(
	ZAsyncStream::sZAsyncMsg *msg,
	fplainn::Zum::sZAsyncMsg *request,
	Thread *self
	)
{
	fplainn::Endpoint		*endp;
	fplainn::Device			*dev;
	error_t				err;
	fplainn::Zum::sZumMsg		*ctxt;
	AsyncResponse			myResponse;

	/**	EXPLANATION:
	 * Basically:
	 *	* Get the device, if it exists.
	 *	* Ensure that we are connected to the device first.
	 *	* Get the kernel's mgmt endpoint.
	 *	* Then fill out a udi_usage_cb_t control block.
	 *	* Call FloodplainnStream::send():
	 *		* metaName="udi_mgmt", meta_ops_num=1, ops_idx=1.
	 **/
	ctxt = getNewZumMsg(
		CC __func__, request->path,
		msg->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUM, request->opsIndex,
		sizeof(*ctxt), msg->header.flags, msg->header.privateData);

	if (ctxt == NULL) { return; };
	::new (&ctxt->info) fplainn::Zum::sZAsyncMsg(*request);
	myResponse(ctxt);

	err = getDeviceHandleAndMgmtEndpoint(
		CC __func__, ctxt->info.path, &dev, &endp);

	if (err != ERROR_SUCCESS) {
		myResponse(err); return;
	};

	udi_layout_t		*layouts[3] =
	{
		__klzbzcore::region::channel::mgmt::layouts::visible[ctxt->info.opsIndex],
		__klzbzcore::region::channel::mgmt::layouts::marshal[ctxt->info.opsIndex],
		NULL
	};

	err = self->parent->floodplainnStream.send(
		endp, &ctxt->info.params.usage.cb.gcb, layouts,
		CC "udi_mgmt", UDI_MGMT_OPS_NUM, ctxt->info.opsIndex,
		new MgmtReqCb(usageRes, ctxt, self, dev),
		ctxt->info.params.usage.resource_level);

	if (err != ERROR_SUCCESS) {
		myResponse(err); return;
	};

	myResponse(DONT_SEND_RESPONSE);
}

void zumServer::mgmt::usageRes(
	MessageStream::sHeader *msg,
	fplainn::Zum::sZumMsg *ctxt,
	Thread *,
	fplainn::Device *
	)
{
	fplainn::sChannelMsg	*response = (fplainn::sChannelMsg *)msg;
	AsyncResponse		myResponse;
	udi_usage_cb_t		*cb;

	myResponse(ctxt);

	cb = (udi_usage_cb_t *)response->getPayloadAddr();
	ctxt->info.params.usage.cb = *cb;

	/* This is the status for the usageInd call itself, and not for the
	 * driver's return value in its usage_res call (there is none actually),
	 * or anything like that.
	 **/
	myResponse(response->header.error);
}

void zumServer::mgmt::enumerateReq(
	ZAsyncStream::sZAsyncMsg *msg,
	fplainn::Zum::sZAsyncMsg *request,
	Thread *self
	)
{
	fplainn::Endpoint		*endp;
	fplainn::Device			*dev;
	error_t				err;
	fplainn::Zum::sZumMsg		*ctxt;
	AsyncResponse			myResponse;

	/**	EXPLANATION:
	 * Basically:
	 *	* Get the device, if it exists.
	 *	* Ensure that we are connected to the device first.
	 *	* Get the kernel's mgmt endpoint.
	 *	* Then fill out a udi_usage_cb_t control block.
	 *	* Call FloodplainnStream::send():
	 *		* metaName="udi_mgmt", meta_ops_num=1, ops_idx=2.
	 **/
	ctxt = getNewZumMsg(
		CC __func__, request->path,
		msg->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUM, request->opsIndex,
		sizeof(*ctxt), msg->header.flags, msg->header.privateData);

	if (ctxt == NULL) { return; };
	::new (&ctxt->info) fplainn::Zum::sZAsyncMsg(*request);

	myResponse(ctxt);

	err = getDeviceHandleAndMgmtEndpoint(
		CC __func__, ctxt->info.path, &dev, &endp);

	if (err != ERROR_SUCCESS) {
		myResponse(err); return;
	};

	udi_layout_t		*layouts[] =
	{
		__klzbzcore::region::channel::mgmt::layouts::visible[ctxt->info.opsIndex],
		__klzbzcore::region::channel::mgmt::layouts::marshal[ctxt->info.opsIndex],
		NULL
	};

	err = self->parent->floodplainnStream.send(
		endp, &ctxt->info.params.enumerate.cb.gcb, layouts,
		CC "udi_mgmt", UDI_MGMT_OPS_NUM, ctxt->info.opsIndex,
		new MgmtReqCb(enumerateAck, ctxt, self, dev),
		ctxt->info.params.enumerate.enumeration_level);

	if (err != ERROR_SUCCESS) {
		myResponse(err); return;
	};

	myResponse(DONT_SEND_RESPONSE);
}

void zumServer::mgmt::enumerateAck(
	MessageStream::sHeader *msg,
	fplainn::Zum::sZumMsg *ctxt,
	Thread *,
	fplainn::Device *
	)
{
	fplainn::sChannelMsg	*response = (fplainn::sChannelMsg *)msg;
	AsyncResponse		myResponse;
	udi_enumerate_cb_t	*cb;
	fplainn::Zum::EnumerateReqMovableObjects
				*movableMem=NULL;
	uarch_t			movableMemRequirement;

	myResponse(ctxt);

	cb = (udi_enumerate_cb_t *)response->getPayloadAddr();

	ctxt->info.params.enumerate.cb = *cb;

	movableMemRequirement = fplainn::Zum::EnumerateReqMovableObjects
		::calcMemRequirementsFor(
			cb->attr_valid_length, cb->filter_list_length);

	if (movableMemRequirement > 0)
	{
		movableMem = new (new ubit8[movableMemRequirement])
			fplainn::Zum::EnumerateReqMovableObjects(
			cb->attr_valid_length, cb->filter_list_length);

		if (movableMem == NULL) {
			myResponse(ERROR_MEMORY_NOMEM); return;
		};
	}


	// Copy the attr and filter lists into some kernel memory area.
	if (cb->attr_valid_length > 0 && cb->attr_list != NULL)
	{
		ctxt->info.params.enumerate.cb.attr_list =
			movableMem->calcAttrList(cb->attr_valid_length);

		memcpy(
			ctxt->info.params.enumerate.cb.attr_list,
			cb->attr_list,
			sizeof(*cb->attr_list) * cb->attr_valid_length);
	}
	else {
		ctxt->info.params.enumerate.cb.attr_list = NULL;
	};

	if (cb->filter_list_length > 0 && cb->filter_list != NULL)
	{
		*const_cast<udi_filter_element_t **>(&ctxt->info.params.enumerate.cb.filter_list) =
			movableMem->calcFilterList(
				cb->attr_valid_length, cb->filter_list_length);

		memcpy(
			const_cast<udi_filter_element_t *>(ctxt->info.params.enumerate.cb.filter_list),
			cb->filter_list,
			sizeof(*cb->filter_list) * cb->filter_list_length);
	}
	else {
		*const_cast<udi_filter_element_t **>(&ctxt->info.params.enumerate.cb.filter_list) = NULL;
	};

	// Finally, capture the stack arguments.
	udi_size_t		enumCbSize;

#if 0
	/* Stack arguments are after the visible layout of the CB. We'll need to
	 * get the visible layout. Then we can calculate its size to determine
	 * how many bytes past the visible_layout we have to skip, in order to
	 * get to the stack arguments.
	 **/
	const sMetaInitEntry		*mgmt;

	mgmt = floodplainn.zudi.findMetaInitInfo(CC"udi_mgmt");
	if (mgmt == NULL)
	{
		printf(ERROR ZUM"enumerateAck: failed to get mgmt meta info.\n");
		myResponse(ERROR_NOT_FOUND);
		return;
	};

	fplainn::MetaInit	parser(mgmt->udi_meta_info);
	udi_mei_op_template_t	*enumReqTmplt;
	udi_layout_t		*enumCbLay;

	enumReqTmplt = parser.getOpTemplate((udi_index_t)1, 2);
	enumCbLay = enumReqTmplt->visible_layout;
	enumCbSize = fplainn::sChannelMsg::zudi_layout_get_size(enumCbLay, 1);
#else // Both paths give the same result, but this one is dramatically faster.
	enumCbSize = sizeof(udi_enumerate_cb_t) - sizeof(udi_cb_t);
#endif

	ubit8			*args8 = &((ubit8 *)(&cb->gcb + 1))[enumCbSize];
	struct			sEnumReqArgs
	{
		udi_ubit8_t	enumeration_result;
		udi_index_t	ops_idx;
	} *args = (sEnumReqArgs *)args8;

	ctxt->info.params.enumerate.ops_idx = args->ops_idx;
	ctxt->info.params.enumerate.enumeration_result =
		args->enumeration_result;

	/* This is the status for the enumerate_req call itself, and not for the
	 * driver's return value in its usage_res call (there is none actually),
	 * or anything like that.
	 **/
	myResponse(response->header.error);
}

void zumServer::mgmt::deviceManagementReq(
	ZAsyncStream::sZAsyncMsg *msg,
	fplainn::Zum::sZAsyncMsg *request,
	Thread *self
	)
{
	fplainn::Endpoint		*endp;
	fplainn::Device			*dev;
	error_t				err;
	fplainn::Zum::sZumMsg		*ctxt;
	AsyncResponse			myResponse;

	/**	EXPLANATION:
	 * Basically:
	 *	* Get the device, if it exists.
	 *	* Ensure that we are connected to the device first.
	 *	* Get the kernel's mgmt endpoint.
	 *	* Then fill out a udi_usage_cb_t control block.
	 *	* Call FloodplainnStream::send():
	 *		* metaName="udi_mgmt", meta_ops_num=1, ops_idx=1.
	 **/
	ctxt = getNewZumMsg(
		CC __func__, request->path,
		msg->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUM, request->opsIndex,
		sizeof(*ctxt), msg->header.flags, msg->header.privateData);

	if (ctxt == NULL) { return; };
	::new (&ctxt->info) fplainn::Zum::sZAsyncMsg(*request);
	myResponse(ctxt);

	err = getDeviceHandleAndMgmtEndpoint(
		CC __func__, ctxt->info.path, &dev, &endp);

	if (err != ERROR_SUCCESS) {
		myResponse(err); return;
	};

	udi_layout_t		*layouts[3] =
	{
		__klzbzcore::region::channel::mgmt::layouts::visible[ctxt->info.opsIndex],
		__klzbzcore::region::channel::mgmt::layouts::marshal[ctxt->info.opsIndex],
		NULL
	};

	err = self->parent->floodplainnStream.send(
		endp, &ctxt->info.params.devmgmt.cb.gcb, layouts,
		CC "udi_mgmt", UDI_MGMT_OPS_NUM, ctxt->info.opsIndex,
		new MgmtReqCb(&deviceManagementAck, ctxt, self, dev),
		ctxt->info.params.devmgmt.mgmt_op,
		ctxt->info.params.devmgmt.parent_ID);

	if (err != ERROR_SUCCESS) {
		myResponse(err); return;
	};

	myResponse(DONT_SEND_RESPONSE);
}

void zumServer::mgmt::deviceManagementAck(
	MessageStream::sHeader *msg,
	fplainn::Zum::sZumMsg *ctxt,
	Thread *,
	fplainn::Device *
	)
{
	fplainn::sChannelMsg	*response = (fplainn::sChannelMsg *)msg;
	AsyncResponse		myResponse;
	udi_mgmt_cb_t		*cb;
	uarch_t			devmgmtCbSize;

	myResponse(ctxt);

	cb = (udi_mgmt_cb_t *)response->getPayloadAddr();
	ctxt->info.params.devmgmt.cb = *cb;

	// Extract the stack arguments from the marshal space.
#if 0
	/* Stack arguments are after the visible layout of the CB. We'll need to
	 * get the visible layout. Then we can calculate its size to determine
	 * how many bytes past the visible_layout we have to skip, in order to
	 * get to the stack arguments.
	 **/
	const sMetaInitEntry		*mgmt;

	mgmt = floodplainn.zudi.findMetaInitInfo(CC"udi_mgmt");
	if (mgmt == NULL)
	{
		printf(ERROR ZUM"enumerateAck: failed to get mgmt meta info.\n");
		myResponse(ERROR_NOT_FOUND);
		return;
	};

	fplainn::MetaInit	parser(mgmt->udi_meta_info);
	udi_mei_op_template_t	*devmgmtReqTmplt;
	udi_layout_t		*devmgmtCbLay;

	devmgmtReqTmplt = parser.getOpTemplate((udi_index_t)1, 3);
	devmgmtCbLay = devmgmtReqTmplt->visible_layout;
	devmgmtCbSize = fplainn::sChannelMsg::zudi_layout_get_size(
		devmgmtCbLay, 1);
#else // Both paths give the same result, but this one is dramatically faster.
	devmgmtCbSize = sizeof(udi_mgmt_cb_t) - sizeof(udi_cb_t);
#endif

	ubit8			*args8 = &((ubit8 *)(&cb->gcb + 1))[devmgmtCbSize];
	struct			sDevmgmtReqArgs
	{
		udi_ubit8_t	flags;
		udi_status_t	status;
	} *args = (sDevmgmtReqArgs *)args8;

	ctxt->info.params.devmgmt.flags = args->flags;
	ctxt->info.params.devmgmt.status = args->status;

	/* This is the status for the usageInd call itself, and not for the
	 * driver's return value in its usage_res call (there is none actually),
	 * or anything like that.
	 **/
	myResponse(response->header.error);
}

void zumServer::mgmt::finalCleanupReq(
	ZAsyncStream::sZAsyncMsg *msg,
	fplainn::Zum::sZAsyncMsg *request,
	Thread *self
	)
{
	fplainn::Endpoint		*endp;
	fplainn::Device			*dev;
	error_t				err;
	fplainn::Zum::sZumMsg		*ctxt;
	AsyncResponse			myResponse;

	/**	EXPLANATION:
	 * Basically:
	 *	* Get the device, if it exists.
	 *	* Ensure that we are connected to the device first.
	 *	* Get the kernel's mgmt endpoint.
	 *	* Then fill out a udi_mgmt_cb_t control block.
	 *	* Call FloodplainnStream::send():
	 *		* metaName="udi_mgmt", meta_ops_num=1, ops_idx=1.
	 **/
	ctxt = getNewZumMsg(
		CC __func__, request->path,
		msg->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUM, request->opsIndex,
		sizeof(*ctxt), msg->header.flags, msg->header.privateData);

	if (ctxt == NULL) { return; };
	::new (&ctxt->info) fplainn::Zum::sZAsyncMsg(*request);
	myResponse(ctxt);

	err = getDeviceHandleAndMgmtEndpoint(
		CC __func__, ctxt->info.path, &dev, &endp);

	if (err != ERROR_SUCCESS) {
		myResponse(err); return;
	};

	udi_layout_t		*layouts[3] =
	{
		__klzbzcore::region::channel::mgmt::layouts::visible[ctxt->info.opsIndex],
		__klzbzcore::region::channel::mgmt::layouts::marshal[ctxt->info.opsIndex],
		NULL
	};

	err = self->parent->floodplainnStream.send(
		endp, &ctxt->info.params.final_cleanup.cb.gcb, layouts,
		CC "udi_mgmt", UDI_MGMT_OPS_NUM, ctxt->info.opsIndex,
		new MgmtReqCb(finalCleanupAck, ctxt, self, dev));

	if (err != ERROR_SUCCESS) {
		myResponse(err); return;
	};

	myResponse(DONT_SEND_RESPONSE);
}

void zumServer::mgmt::finalCleanupAck(
	MessageStream::sHeader *msg,
	fplainn::Zum::sZumMsg *ctxt,
	Thread *,
	fplainn::Device *
	)
{
	fplainn::sChannelMsg	*response = (fplainn::sChannelMsg *)msg;
	AsyncResponse		myResponse;
	udi_mgmt_cb_t		*cb;

	myResponse(ctxt);

	cb = (udi_mgmt_cb_t *)response->getPayloadAddr();
	ctxt->info.params.final_cleanup.cb = *cb;

	/* This is the status for the usageInd call itself, and not for the
	 * driver's return value in its usage_res call (there is none actually),
	 * or anything like that.
	 **/
	myResponse(response->header.error);
}

void zumServer::mgmt::channelEventInd(
	ZAsyncStream::sZAsyncMsg *msg,
	fplainn::Zum::sZAsyncMsg *request,
	Thread *self
	)
{
	fplainn::Device			*dev;
	error_t				err;
	fplainn::Zum::sZumMsg		*ctxt;
	AsyncResponse			myResponse;

	/**	EXPLANATION:
	 * Basically:
	 *	* Get the device, if it exists.
	 *	* Ensure that we are connected to the device first.
	 *	* Get the kernel's mgmt endpoint.
	 *	* Then fill out a udi_channel_event_cb_t control block.
	 *	* Call FloodplainnStream::send():
	 *		* metaName="udi_mgmt", meta_ops_num=1, ops_idx=0.
	 **/
	ctxt = getNewZumMsg(
		CC __func__, request->path,
		msg->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUM, request->opsIndex,
		sizeof(*ctxt), msg->header.flags, msg->header.privateData);

	if (ctxt == NULL) { return; };
	::new (&ctxt->info) fplainn::Zum::sZAsyncMsg(*request);
	myResponse(ctxt);

	err = floodplainn.getDevice(ctxt->info.path, &dev);
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR ZUM"%s %s: Device doesn't exist.\n",
			__func__, ctxt->info.path);

		myResponse(err); return;
	};

	udi_layout_t		*layouts[3] =
	{
		__klzbzcore::region::channel::mgmt::layouts::visible[ctxt->info.opsIndex],
		__klzbzcore::region::channel::mgmt::layouts::marshal[ctxt->info.opsIndex],
		NULL
	};

	err = fplainn::sChannelMsg::send(
		ctxt->info.params.channel_event.endpoint,
		&ctxt->info.params.channel_event.cb.gcb,
		(va_list)NULL, layouts,
		CC"udi_mgmt", UDI_MGMT_OPS_NUM, ctxt->info.opsIndex,
		new MgmtReqCb(channelEventComplete, ctxt, self, dev));

	if (err != ERROR_SUCCESS) {
		myResponse(err); return;
	};

	myResponse(DONT_SEND_RESPONSE);
}

void zumServer::mgmt::channelEventComplete(
	MessageStream::sHeader *msg,
	fplainn::Zum::sZumMsg *ctxt,
	Thread *,
	fplainn::Device *
	)
{
	fplainn::sChannelMsg	*response = (fplainn::sChannelMsg *)msg;
	AsyncResponse		myResponse;
	udi_size_t		visibleSize;
	ubit8			*cb8;

	cb8 = (ubit8 *)response->getPayloadAddr();

	myResponse(ctxt);
	myResponse(msg->error);

	/**	EXPLANATION:
	 * Take the marshalled argument out and place it into the params
	 * space in ctxt->info.params.channel_event.
	 **/
	visibleSize = fplainn::sChannelMsg::zudi_layout_get_size(
		__klzbzcore::region::channel::mgmt::layouts::channel_event_cb, 1);

	ctxt->info.params.channel_event.status =
		*(udi_index_t *)(cb8 + sizeof(udi_cb_t) + visibleSize);
}
