#ifndef ___K_RAW_LIB_ZBZCORE_H
	#define ___K_RAW_LIB_ZBZCORE_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/floodplainn/floodplainn.h>

#define LZBZCORE	"lzbzcore: "

class threadC;

namespace __klzbzcore
{
	void main(void);

	namespace distributary
	{
		error_t main(threadC *self);
	}

	namespace driver
	{
		const ubit32 	MAINTHREAD_U0_GET_THREAD_DEVICE_PATH_REQ=0,
				MAINTHREAD_U0_REGION_INIT_COMPLETE_IND=1,
				MAINTHREAD_U0_REGION_INIT_FAILURE_IND=2;

		error_t main(threadC *self);
		void main_handleU0Request(
			messageStreamC::iteratorS *msgIt,
			fplainn::driverInstanceC *drvInst,
			threadC *self);

		void main_handleKernelCall(
			floodplainnC::zudiKernelCallMsgS *msg);

		namespace __kcall
		{
			struct callerContextS
			{
				callerContextS(
					floodplainnC::zudiKernelCallMsgS *z)
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
					ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN];
			};

			error_t instantiateDevice(callerContextS *ctxt);
			error_t destroyDevice(callerContextS *ctxt);
		}

		namespace u0
		{
			error_t getThreadDevicePath(
				fplainn::driverInstanceC *drvInst,
				processId_t tid,
				utf8Char *path);

			error_t regionInitInd(
				__klzbzcore::driver::__kcall::callerContextS *ctxt,
				ubit32 function);
		}

	}
}

#endif

