
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

void fplainn::Zum::main(void *)
{
	Thread				*self;
	MessageStream::sHeader		*iMsg;
	HeapObj<sZAsyncMsg>		requestData;
	const sMetaInitEntry		*mgmtInitEntry;

	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	requestData = new sZAsyncMsg(NULL, 0);
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

	printf(NOTICE ZUM"main: running, tid 0x%x.\n", self->getFullId());

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
					"callback from 0x%x.\n",
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

	if (msg->dataNBytes != sizeof(*request))
	{
		printf(WARNING ZUM"incoming request from 0x%x has odd size. "
			"Rejected.\n",
			msg->header.sourceId);

		return;
	};

	err = self->parent->zasyncStream.receive(
		msg->dataHandle, request, 0);

	if (err != ERROR_SUCCESS)
	{
		printf(WARNING ZUM"receive() failed on request from 0x%x.\n",
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
		break;
	case fplainn::Zum::sZAsyncMsg::OP_FINAL_CLEANUP_REQ:
		break;
	case fplainn::Zum::sZAsyncMsg::OP_CHANNEL_EVENT_IND:
		mgmt::channelEventInd(msg, request, self);
		break;
	case fplainn::Zum::sZAsyncMsg::OP_START_REQ:
		start::startDeviceReq(msg, request, self);
		break;

	default:
		printf(WARNING ZUM"request from 0x% is for invalid ops_idx "
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
	new (&ctxt->info) fplainn::Zum::sZAsyncMsg(*request);
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
	PtrList<fplainn::Channel>::Iterator	chanIt;

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
	PtrList<fplainn::Channel>::Iterator	chanIt;

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
			// For "parent_ID", we'll have to do more work later.
			1,
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
	new (&ctxt->info) fplainn::Zum::sZAsyncMsg(*request);
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
	new (&ctxt->info) fplainn::Zum::sZAsyncMsg(*request);

	ctxt->info.params.enumerate.filter_list = NULL;
	ctxt->info.params.enumerate.attr_list = NULL;
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

	/* Cleanup the heap objects we used to transmit the attr and filter lists
	 * now. They have been marshalled, and we no longer need to keep those
	 * floating heap objects.
	 **/
	fplainn::sMovableMemory			*a, *f;

	a = (fplainn::sMovableMemory *)request->params.enumerate.cb.attr_list;
	f = (fplainn::sMovableMemory *)request->params.enumerate.cb.filter_list;
	if (a != NULL) { a--; };
	if (f != NULL) { f--; };

	if (request->params.enumerate.cb.attr_valid_length > 0) { delete a; };
	if (request->params.enumerate.cb.filter_list_length > 0) { delete f; };

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

	myResponse(ctxt);

	cb = (udi_enumerate_cb_t *)response->getPayloadAddr();

	ctxt->info.params.enumerate.cb = *cb;

	// Copy the attr and filter lists into some kernel memory area.
	if (cb->attr_valid_length > 0)
	{
		ctxt->info.params.enumerate.cb.attr_valid_length =
			cb->attr_valid_length;

		ctxt->info.params.enumerate.attr_list =
			ctxt->info.params.enumerate.cb.attr_list =
			new udi_instance_attr_list_t[cb->attr_valid_length];

		if (ctxt->info.params.enumerate.attr_list == NULL) {
			myResponse(ERROR_MEMORY_NOMEM); return;
		};

		memcpy(
			ctxt->info.params.enumerate.attr_list,
			cb->attr_list,
			sizeof(*cb->attr_list) * cb->attr_valid_length);
	};

	if (cb->filter_list_length > 0)
	{
		ctxt->info.params.enumerate.cb.filter_list_length =
			cb->filter_list_length;

		ctxt->info.params.enumerate.filter_list =
			*const_cast<udi_filter_element_t **>(&ctxt->info.params.enumerate.cb.filter_list) =
			new udi_filter_element_t[cb->filter_list_length];

		if (ctxt->info.params.enumerate.filter_list == NULL)
		{
			delete[] ctxt->info.params.enumerate.attr_list;
			myResponse(ERROR_MEMORY_NOMEM); return;
		};

		memcpy(
			ctxt->info.params.enumerate.filter_list,
			cb->filter_list,
			sizeof(*cb->filter_list) * cb->filter_list_length);
	};

	/* This is the status for the enumerate_req call itself, and not for the
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
	new (&ctxt->info) fplainn::Zum::sZAsyncMsg(*request);
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
