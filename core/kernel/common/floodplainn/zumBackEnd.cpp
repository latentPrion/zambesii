
#include <__kstdlib/callback.h>
#include <__kstdlib/__kcxxlib/memory.h>
#include <kernel/common/thread.h>
#include <kernel/common/messageStream.h>
#include <kernel/common/zasyncStream.h>
#include <kernel/common/floodplainn/zum.h>
#include <kernel/common/floodplainn/floodplainnStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


// Dat C++ inheritance.
struct sDummyMgmtMaOpsVector
: udi_ops_vector_t
{
} dummyMgmtMaOpsVector;

namespace zumServer
{
	void zasyncHandler(
		ZAsyncStream::sZAsyncMsg *msg,
		fplainn::Zum::sZAsyncMsg *request,
		Thread *self);

	namespace start
	{
		void startDeviceReq(
			ZAsyncStream::sZAsyncMsg *msg,
			fplainn::Zum::sZAsyncMsg *request,
			Thread *self);

		class StartDeviceReqCb;
		typedef void (startDeviceReqCbFn)(
			MessageStream::sHeader *msg,
			fplainn::Zum::sZumMsg *ctxt,
			Thread *self,
			fplainn::Device *dev);

//		startDeviceReqCbFn	startDeviceReq1;
//		startDeviceReqCbFn	startDeviceReq2;
	}
}

void fplainn::Zum::main(void *)
{
	Thread				*self;
	MessageStream::sHeader		*iMsg;
	HeapObj<sZAsyncMsg>		*requestData;

	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	requestData = new sZAsyncMsg(0);
	if (requestData.get() == NULL)
	{
		printf(ERROR ZUM"main: failed to alloc request data mem.\n");
		taskTrib.kill(self);
		return;
	};

	for (;FOREVER;)
	{
		self->messageStream.pull(&iMsg);

		switch (iMsg->subsystem)
		{
		case MSGSTREAM_SUBSYSTEM_ZASYNC:
			ZAsyncStream::sZAsyncMsg	*msg =
				(ZAsyncStream::sZAsyncMsg *)iMsg;

			zumServer::zasyncHandler(msg, requestData.get(), self);
			break;

		default:
			Callback	*callback;

			callback = (Callback *)iMsg->privateData;
			if (callback == NULL)
			{
				printf(WARNING ZUP"main: message with no "
					"callback from 0x%x.\n",
					iMsg->sourceId);

				continue;
			};

			(*callback)(iMsg);
			delete callback;
		};
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

	err = self->zasyncStream.receive(
		msg->dataHandle, request, 0);

	if (err != ERROR_SUCCESS)
	{
		printf(WARNING ZUM"receive() failed on request from 0x%x.\n",
			msg->header.sourceId);

		return;
	};

	switch (request->opsIndex)
	{
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
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
: public _Callback
{
	fplainn::Zum::sZumMsg *ctxt;
	Thread *self;
	fplainn::Device *dev;

public:
	StartDeviceReqCb(
		startDeviceReqCbFn *fn,
		fplainn::Zum::sZumMsg *ctxt, Thread *self, fplainn::Device *dev)
	: _Callback(fn),
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
	fplainn::FStreamEndpoint	*endp;
	AsyncResponse			myResponse;
	fplainn::Device			*dev;

	ctxt = new fplainn::Zum::sZumMsg(
		msg->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUM, request->opsIndex,
		sizeof(*ctxt), msg->header.flags, msg->header.privateData);

	if (ctxt == NULL)
	{
		printf(ERROR ZUM"startDevReq %s: Failed to alloc async ctxt.\n"
			"\tCaller may be halted indefinitely.\n"
			request->path);

		return;
	};

	myResponse(ctxt);
	// Copy the request data over.
	new (&ctxt->info) fplainn::Zum::sZAsyncMsg(*request);

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

//	dev->instance->setMgmtEndpoint(endp);
}
