
#include <__kstdlib/callback.h>
#include <__kstdlib/__kcxxlib/memory.h>
#include <kernel/common/thread.h>
#include <kernel/common/messageStream.h>
#include <kernel/common/zasyncStream.h>
#include <kernel/common/floodplainn/zum.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


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

void zumServer::start::startDeviceReq(
	ZAsyncStream::sZAsyncMsg *msg,
	fplainn::Zum::sZAsyncMsg *request,
	Thread *self
	)
{
}
