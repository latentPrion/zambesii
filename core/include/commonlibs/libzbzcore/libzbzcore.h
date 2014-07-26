#ifndef ___K_RAW_LIB_ZBZCORE_H
	#define ___K_RAW_LIB_ZBZCORE_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/floodplainn/floodplainn.h>

#define LZBZCORE	"lzbzcore: "

class Thread;

namespace __klzbzcore
{
	void main(void);

	namespace distributary
	{
		error_t main(Thread *self);
	}

	namespace driver
	{
		const ubit32 	MAINTHREAD_U0_GET_THREAD_DEVICE_PATH_REQ=0,
				MAINTHREAD_U0_REGION_INIT_COMPLETE_IND=1,
				MAINTHREAD_U0_REGION_INIT_FAILURE_IND=2,
				MAINTHREAD_U0_SYNC=3;


		error_t main(Thread *self);
		void main_handleU0Request(
			MessageStream::sIterator *msgIt,
			fplainn::DriverInstance *drvInst,
			Thread *self);

		void main_handleKernelCall(
			Floodplainn::zudiKernelCallMsgS *msg);

		void main_handleMgmtCall(
			Floodplainn::zudiMgmtCallMsgS *msg);

		namespace __kcall
		{
			struct callerContextS
			{
				callerContextS(
					Floodplainn::zudiKernelCallMsgS *z)
				:
				sourceTid(z->header.sourceId),
				privateData(z->header.privateData),
				error(0)
				{
					strcpy8(devicePath, z->path);
				}

				processId_t		sourceTid;
				void			*privateData;
				error_t			error;
				utf8Char		devicePath[
					FVFS_PATH_MAXLEN];
			};

			error_t instantiateDevice(callerContextS *ctxt);
			error_t instantiateDevice1(callerContextS *ctxt);
			error_t instantiateDevice2(callerContextS *ctxt);

			error_t destroyDevice(callerContextS *ctxt);
		}

		namespace u0
		{
			error_t getThreadDevicePath(
				fplainn::DriverInstance *drvInst,
				processId_t tid,
				utf8Char *path);

			error_t regionInitInd(
				__klzbzcore::driver::__kcall::callerContextS *ctxt,
				ubit32 function);
		}

	}
}

#endif

