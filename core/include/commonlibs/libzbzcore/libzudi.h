#ifndef ___K_LIB_ZAMBESII_UDI_H
	#define ___K_LIB_ZAMBESII_UDI_H

	#define UDI_VERSION	0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <zui.h>
	#include <extern.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <__kclasses/ptrList.h>
	#include <__kclasses/ptrlessList.h>

#define LZUDI			"lzudi: "

namespace fplainn
{
	class Endpoint;
	struct sChannelMsg;
}

namespace __klzbzcore
{
	namespace driver { class CachedInfo; }
}

namespace lzudi
{
	/* Used to encompass the data needed for a channel to work correctly.
	 *
	 * Calls to udi_channel_* will be intercepted and the channel_context
	 * argument will be replaced with one of these, and the actual
	 * channel_context argument will be pointed to by that sEndpointContext
	 * instance.
	 **/
	struct sEndpointContext
	{
		sEndpointContext(
			fplainn::Endpoint *__kendp,
			utf8Char *metaName,
			udi_mei_init_t *metaInfo, udi_index_t opsIdx,
			udi_index_t bindCbIndex,
			void *channel_context);

		void dump(void);

		void anchor(
			utf8Char *metaName, udi_mei_init_t *metaInfo,
			udi_index_t ops_idx);

		fplainn::Endpoint		*__kendpoint;
		utf8Char			metaName[
			ZUI_DRIVER_METALANGUAGE_MAXLEN];

		udi_mei_init_t			*metaInfo;
		udi_mei_ops_vec_template_t	*opsVectorTemplate;
		void				*channel_context;
		udi_index_t			bindCbIndex;
	};

	struct sRegion
	{
		error_t initialize(void) { return endpoints.initialize(); }

		sbit8 findEndpointContext(sEndpointContext *ec)
		{
			PtrList<sEndpointContext>::Iterator	it;

			for (it=endpoints.begin(0); it!=endpoints.end(); ++it) {
				if (*it == ec) { return 1; };
			};

			return 0;
		}

		sEndpointContext *getEndpointContextBy__kendpoint(
			fplainn::Endpoint *__kep)
		{
			PtrList<sEndpointContext>::Iterator	it;

			for (it=endpoints.begin(0); it!=endpoints.end(); ++it)
			{
				if ((*it)->__kendpoint == __kep)
					{ return *it; };
			};

			return NULL;
		}

		List<sRegion>::sHeader		listHeader;
		ubit16				index;
		udi_init_context_t		*rdata;
		PtrList<sEndpointContext>	endpoints;
	};

	struct sControlBlock
	{
		sControlBlock(void)
		:
		dtypedLayoutNElements(0), driverTypedLayout(NULL)
		{}

		void *operator new(size_t sz, uarch_t payloadSize);
		void operator delete(void *mem);

		uarch_t			dtypedLayoutNElements;
		udi_layout_t		*driverTypedLayout;
	};

	sControlBlock *calleeCloneCb(
		fplainn::sChannelMsg *msg,
		udi_mei_op_template_t *opTemplate, udi_index_t opsIndex);

	void *udi_mem_alloc_sync(udi_size_t size, udi_ubit8_t flags);

	error_t udi_cb_alloc_sync(
		__klzbzcore::driver::CachedInfo *drvInfoCache,
		udi_index_t cb_idx, udi_cb_t **retcb);
}

#endif
