#ifndef ___K_RAW_LIB_ZBZCORE_H
	#define ___K_RAW_LIB_ZBZCORE_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kcxxlib/new>
	#include <__kclasses/ptrlessList.h>
	#include <kernel/common/floodplainn/floodplainn.h>

#define LZBZCORE	"lzbzcore: "

class Thread;
namespace lzudi { struct sRegion; }

namespace __klzbzcore
{
	namespace distributary
	{
		typedef void (mainCbFn)(Thread *self, error_t);
		void main(Thread *self, mainCbFn *callback);
	}

	namespace driver { class CachedInfo; }
	namespace region
	{
		void main(void *);

	// PRIVATE:
		typedef void (__kmainCbFn)(
			MessageStream::sHeader *msg,
			fplainn::Zudi::sKernelCallMsg *ctxt,
			Thread *self,
			__klzbzcore::driver::CachedInfo **drvInfoCache,
			lzudi::sRegion *r,
			fplainn::Device *dev);

		class MainCb;
		__kmainCbFn	main1;
		__kmainCbFn	main2;

		namespace mei
		{
			void handler(void);
		}

		// Will probably be deprecated soon.
		namespace mgmt
		{
			error_t handler(
				fplainn::Zudi::sMgmtCallMsg *msg,
				fplainn::Device *dev, ubit16 regionIndex);
		}
	}

	namespace driver
	{
		typedef void (mainCbFn)(Thread *self, error_t);

		void main(Thread *self, mainCbFn *callback);

		struct sMetaCbScratchInfo
		{
			sMetaCbScratchInfo(
				udi_index_t meta_idx,
				udi_index_t meta_cb_num,
				udi_size_t scratch_requirement)
			:
			metaIndex(meta_idx),
			metaCbNum(meta_cb_num),
			scratchRequirement(scratch_requirement)
			{}

			List<sMetaCbScratchInfo>::sHeader	listHeader;
			udi_index_t	metaIndex, metaCbNum;
			udi_size_t	scratchRequirement;
		};

		class CachedInfo
		{
		public:
			CachedInfo(udi_init_t *driver_init_info)
			:
			initInfo(driver_init_info)
			{}

			error_t initialize(void) { return ERROR_SUCCESS; }

			error_t generateMetaCbScratchCache(void);

			~CachedInfo(void)
			{
				List<sMetaCbScratchInfo>::Iterator	it;

				it = metaCbScratchInfo.begin();
				for (; it != metaCbScratchInfo.end(); ++it)
				{
					delete *it;
				};
			};

			void dump(void)
			{
				List<sMetaCbScratchInfo>::Iterator	it;

				it = metaCbScratchInfo.begin();
				for (; it != metaCbScratchInfo.end(); ++it)
				{
					sMetaCbScratchInfo	*tmp = *it;

					printf(NOTICE"meta_idx %d, "
						"meta_cb_num %d, "
						"max scratch required %d.\n",
						tmp->metaIndex, tmp->metaCbNum,
						tmp->scratchRequirement);
				};
			}

			const udi_init_t		*initInfo;
			List<sMetaCbScratchInfo>	metaCbScratchInfo;
		};

	// PRIVATE:
		class MainCb;
		typedef void (__kmainCbFn)(
			MessageStream::sHeader *msg,
			CachedInfo *driverCachedInfo, Thread *self,
			mainCbFn *callerCb);

		__kmainCbFn	main1;

		namespace __kcontrol
		{
			const ubit32	INSTANTIATE_DEVICE=0,
					DESTROY_DEVICE=1;

			typedef void (instantiateDeviceReqCbFn)(
				fplainn::Zudi::sKernelCallMsg *, error_t);
			typedef instantiateDeviceReqCbFn destroyDeviceReqCbFn;

			void handler(
				fplainn::Zudi::sKernelCallMsg *msg,
				__klzbzcore::driver::CachedInfo *cache);

			void instantiateDeviceReq(
				fplainn::Zudi::sKernelCallMsg *ctxt);

			void destroyDeviceReq(
				fplainn::Zudi::sKernelCallMsg *ctxt);

		// PRIVATE:
			typedef void (__kinstantiateDeviceReqCbFn)(
				MessageStream::sHeader *msg,
				fplainn::Zudi::sKernelCallMsg *ctxt);
			typedef __kinstantiateDeviceReqCbFn
				__kdestroyDeviceReqCbFn;

			class InstantiateDeviceReqCb;
			typedef InstantiateDeviceReqCb DestroyDeviceReqCb;
			void instantiateDeviceReq1(fplainn::Zudi::sKernelCallMsg *);
			void instantiateDeviceReq2(fplainn::Zudi::sKernelCallMsg *);

			instantiateDeviceReqCbFn	instantiateDeviceAck;
			destroyDeviceReqCbFn		destroyDeviceAck;
		}

		namespace localService
		{
			const ubit32	USERQID=0;

			const ubit32	REGION_INIT_SYNC_REQ=0,
					REGION_INIT_IND=1,
					GET_THREAD_DEVICE_PATH_REQ=2,
					GET_DRIVER_CACHED_INFO=3;

			void handler(
				MessageStream::sPostMsg *msgIt,
				fplainn::DriverInstance *drvInst,
				__klzbzcore::driver::CachedInfo *cache,
				Thread *self);

			// Deprecated.
			error_t getThreadDevicePathReq(
				fplainn::DriverInstance *drvInst,
				processId_t tid,
				utf8Char *path);

			void regionInitInd(
				fplainn::Zudi::sKernelCallMsg *ctxt,
				error_t error);
		}
	}

	void main(void);
	distributary::mainCbFn	distributaryMain1;
	driver::mainCbFn	driverMain1;
}

#endif

