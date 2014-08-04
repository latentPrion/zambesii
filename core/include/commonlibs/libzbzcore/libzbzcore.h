#ifndef ___K_RAW_LIB_ZBZCORE_H
	#define ___K_RAW_LIB_ZBZCORE_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/floodplainn/floodplainn.h>

#define LZBZCORE	"lzbzcore: "

class Thread;

namespace __klzbzcore
{
	namespace distributary
	{
		typedef void (mainCbFn)(Thread *self, error_t);
		void main(Thread *self, mainCbFn *callback);
	}

	namespace region
	{
		void main(void *);

	// PRIVATE:
		typedef void (__kmainCbFn)(
			Floodplainn::sZudiKernelCallMsg *ctxt,
			Thread *self, fplainn::Device *dev);

		__kmainCbFn	main1;

		namespace mei
		{
			void handler(void);
		}

		// Will probably be deprecated soon.
		namespace mgmt
		{
			error_t handler(
				Floodplainn::sZudiMgmtCallMsg *msg,
				fplainn::Device *dev, ubit16 regionIndex);
		}
	}

	namespace driver
	{
		typedef void (mainCbFn)(Thread *self, error_t);

		void main(Thread *self, mainCbFn *callback);

	// PRIVATE:
		class MainCb;
		typedef void (__kmainCbFn)(
			MessageStream::sHeader *msg, Thread *self,
			mainCbFn *callerCb);

		__kmainCbFn	main1;

		namespace __kcontrol
		{
			const ubit32	INSTANTIATE_DEVICE=0,
					DESTROY_DEVICE=1;

			typedef void (instantiateDeviceReqCbFn)(
				Floodplainn::sZudiKernelCallMsg *, error_t);
			typedef instantiateDeviceReqCbFn destroyDeviceReqCbFn;

			void handler(Floodplainn::sZudiKernelCallMsg *msg);

			void instantiateDeviceReq(
				Floodplainn::sZudiKernelCallMsg *ctxt);

			void destroyDeviceReq(
				Floodplainn::sZudiKernelCallMsg *ctxt);

		// PRIVATE:
			typedef void (__kinstantiateDeviceReqCbFn)(
				MessageStream::sHeader *msg,
				Floodplainn::sZudiKernelCallMsg *ctxt);
			typedef __kinstantiateDeviceReqCbFn
				__kdestroyDeviceReqCbFn;

			class InstantiateDeviceReqCb;
			typedef InstantiateDeviceReqCb DestroyDeviceReqCb;
			void instantiateDeviceReq1(Floodplainn::sZudiKernelCallMsg *);
			void instantiateDeviceReq2(Floodplainn::sZudiKernelCallMsg *);

			instantiateDeviceReqCbFn	instantiateDeviceAck;
			destroyDeviceReqCbFn		destroyDeviceAck;
		}

		namespace localService
		{
			const ubit32	USERQID=0;

			const ubit32	REGION_INIT_SYNC_REQ=0,
					REGION_INIT_IND=1,
					GET_THREAD_DEVICE_PATH_REQ=2;

			void handler(
				MessageStream::sHeader *msgIt,
				fplainn::DriverInstance *drvInst,
				Thread *self);

			// Deprecated.
			error_t getThreadDevicePathReq(
				fplainn::DriverInstance *drvInst,
				processId_t tid,
				utf8Char *path);

			void regionInitInd(
				Floodplainn::sZudiKernelCallMsg *ctxt,
				error_t error);
		}
	}

	void main(void);
	distributary::mainCbFn	distributaryMain1;
	driver::mainCbFn	driverMain1;
}

#endif

