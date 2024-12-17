#ifndef ___K_RAW_LIB_ZBZCORE_H
	#define ___K_RAW_LIB_ZBZCORE_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kcxxlib/new>
	#include <__kclasses/list.h>
	#include <__kclasses/debugPipe.h>
	#include <kernel/common/floodplainn/floodplainn.h>

#define LZBZCORE	"lzbzcore: "

class Thread;
namespace lzudi
{
	struct sRegion;
	struct sEndpointContext;
}
namespace fplainn { class MetaInit; }

namespace __klzbzcore
{
	namespace distributary
	{
		typedef void (mainCbFn)(Thread *self, error_t);
		void main(Thread *self, mainCbFn *callback);
	}

	namespace driver
	{
		typedef void (mainCbFn)(Thread *self, error_t);

		void main(Thread *self, mainCbFn *callback);

		struct sMetaCbInfo
		{
			sMetaCbInfo(
				udi_index_t meta_idx,
				udi_index_t meta_cb_num,
				udi_size_t scratch_requirement)
			:
			metaIndex(meta_idx),
			metaCbNum(meta_cb_num),
			scratchRequirement(scratch_requirement)
			{}

			void dump(utf8Char *metaName);

			List<sMetaCbInfo>::sHeader	listHeader;
			udi_index_t			metaIndex, metaCbNum;
			udi_size_t			scratchRequirement;
			udi_layout_t			*visibleLayout;
			udi_size_t			visibleSize;
		};

		class CachedInfo
		{
		public:
			CachedInfo(udi_init_t *driver_init_info)
			:
			initInfo(driver_init_info)
			{}

			error_t initialize(void)
			{
				return metaInfos.initialize();
			}

			error_t generateMetaCbCache(void);
			error_t generateMetaInfoCache(void);

			~CachedInfo(void)
			{
				List<sMetaCbInfo>::Iterator	it;

				it = metaCbInfo.begin();
				for (; it != metaCbInfo.end(); ++it)
				{
					delete *it;
				};
			};

			void dump(void)
			{
				List<sMetaCbInfo>::Iterator	it;

				printf(NOTICE"cache @%p: dumping.\n");
				it = metaCbInfo.begin();
				for (; it != metaCbInfo.end(); ++it)
				{
					sMetaCbInfo	*tmp = *it;
					sMetaDescriptor	*metaDesc;

					metaDesc = getMetaDescriptor(
						tmp->metaIndex);

					if (metaDesc == NULL)
					{
						printf(ERROR"dump: Failed to "
							"get metaname for "
							"meta_idx %d.\n",
							tmp->metaIndex);
					};

					tmp->dump(metaDesc->name);
				};
			}

			struct sMetaDescriptor
			{
				sMetaDescriptor(
					utf8Char *metaName,
					udi_index_t metaIndex,
					udi_mei_init_t *udi_meta_info)
				:
				index(metaIndex), initInfo(udi_meta_info)
				{
					strncpy8(
						this->name, metaName,
						DRIVER_METALANGUAGE_MAXLEN);

					name[DRIVER_METALANGUAGE_MAXLEN - 1]
						= '\0';
				}

				udi_index_t		index;
				utf8Char		name[
					DRIVER_METALANGUAGE_MAXLEN];

				udi_mei_init_t		*initInfo;
			};

			sMetaDescriptor *getMetaDescriptor(udi_index_t idx)
			{
				HeapList<sMetaDescriptor>::Iterator	it;

				it = metaInfos.begin(0);
				for (; it != metaInfos.end(); ++it)
				{
					if ((*it)->index == idx) {
						return *it;
					};
				};

				return NULL;
			}

			sMetaDescriptor *getMetaDescriptor(utf8Char *metaName)
			{
				HeapList<sMetaDescriptor>::Iterator	it;

				it = metaInfos.begin(0);
				for (; it != metaInfos.end(); ++it)
				{
					sMetaDescriptor		*curr = *it;

					if (!strncmp8(
						curr->name, metaName,
						DRIVER_METALANGUAGE_MAXLEN))
					{
						return curr;
					};
				};

				return NULL;
			}

			status_t getScratchSizeFor(
				udi_index_t metaIndex, udi_index_t metaCbNum)
			{
				List<sMetaCbInfo>::Iterator	it;

				it = metaCbInfo.begin();
				for (; it != metaCbInfo.end(); ++it)
				{
					sMetaCbInfo	*curr = *it;

					if (curr->metaIndex != metaIndex
						|| curr->metaCbNum != metaCbNum)
						{ continue; };

					return curr->scratchRequirement;
				};

				return ERROR_NO_MATCH;
			}

			sMetaCbInfo *getMetaCbInfo(
				udi_index_t metaIndex, udi_index_t metaCbNum)
			{
				List<sMetaCbInfo>::Iterator	it;

				it = metaCbInfo.begin();
				for (; it != metaCbInfo.end(); ++it)
				{
					sMetaCbInfo	*curr = *it;

					if (curr->metaIndex != metaIndex
						|| curr->metaCbNum != metaCbNum)
						{ continue; };

					return curr;
				};

				return NULL;
			}

			const udi_init_t		*initInfo;
			List<sMetaCbInfo>		metaCbInfo;
			HeapList<sMetaDescriptor>	metaInfos;
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

			void regionInitInd(
				fplainn::Zudi::sKernelCallMsg *ctxt,
				error_t error);
		}
	}

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

		namespace channel
		{
			void handler(
				fplainn::sChannelMsg *msg,
				Thread *self,
				__klzbzcore::driver::CachedInfo *drvInfoCache,
				lzudi::sRegion *r);

			error_t allocateEndpointContext(
				fplainn::sChannelMsg *msg,
				Thread *self,
				__klzbzcore::driver::CachedInfo *drvInfoCache,
				lzudi::sRegion *r,
				lzudi::sEndpointContext **retctxt);

			void mgmtMeiCall(
				fplainn::sChannelMsg *msg,
				Thread *self,
				__klzbzcore::driver::CachedInfo *drvInfoCache,
				lzudi::sRegion *r,
				__klzbzcore::driver::CachedInfo::sMetaDescriptor
					*metaDesc,
				udi_mei_op_template_t *opTemplate,
				udi_cb_t *cb);

			void genericMeiCall(
				fplainn::sChannelMsg *msg,
				Thread *self,
				__klzbzcore::driver::CachedInfo *drvInfoCache,
				lzudi::sRegion *r,
				__klzbzcore::driver::CachedInfo::sMetaDescriptor
					*metaDesc,
				udi_mei_op_template_t *opTemplate,
				udi_cb_t *cb);

			void eventIndMeiCall(
				fplainn::sChannelMsg *msg,
				Thread *self,
				__klzbzcore::driver::CachedInfo *drvInfoCache,
				lzudi::sRegion *r,
				__klzbzcore::driver::CachedInfo::sMetaDescriptor
					*metaDesc,
				udi_mei_op_template_t *opTemplate,
				udi_cb_t *cb);

			namespace mgmt
			{
				namespace layouts
				{
					extern udi_layout_t	channel_event_cb[];
					extern udi_layout_t	*visible[];
					extern udi_layout_t	channel_event_ind[];
					extern udi_layout_t	*marshal[];
				}
			}
		}
	}

	void main(void);
	distributary::mainCbFn	distributaryMain1;
	driver::mainCbFn	driverMain1;
}

#endif

