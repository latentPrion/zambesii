#ifndef ___K_LIB_ZAMBESII_UDI_H
	#define ___K_LIB_ZAMBESII_UDI_H

	#define UDI_VERSION		0x101
	#define UDI_PHYSIO_VERSION	0x101
	#include <udi.h>
	#include <udi_physio.h>
	#undef UDI_VERSION
	#undef UDI_PHYSIO_VERSION
	#include <zui.h>
	#include <extern.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <__kclasses/heapList.h>
	#include <__kclasses/list.h>

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
			HeapList<sEndpointContext>::Iterator	it;

			for (it=endpoints.begin(0); it!=endpoints.end(); ++it) {
				if (*it == ec) { return 1; };
			};

			return 0;
		}

		sEndpointContext *getEndpointContextBy__kendpoint(
			fplainn::Endpoint *__kep)
		{
			HeapList<sEndpointContext>::Iterator	it;

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
		HeapList<sEndpointContext>	endpoints;
	};

	struct sControlBlock
	{
		static const uarch_t	MAGIC=0xC01111D0;
		sControlBlock(void)
		:
		magic(MAGIC), dtypedLayoutNElements(0), driverTypedLayout(NULL)
		{}


		sbit8 magicIsValid(void) { return magic == MAGIC; }

		void *operator new(size_t sz, uarch_t payloadSize);
		void operator delete(void *mem);

		uarch_t			magic;
		uarch_t			dtypedLayoutNElements;
		udi_layout_t		*driverTypedLayout;
	};

	sControlBlock *calleeCloneCb(
		fplainn::sChannelMsg *msg,
		udi_mei_op_template_t *opTemplate, udi_index_t opsIndex);

	void *udi_mem_alloc_sync(udi_size_t size, udi_ubit8_t flags);
	void udi_mem_free_sync(void *mem, sbit8 dontPanicOnBadMagic=0);

	void udi_cb_free_sync(udi_cb_t *cb);
	error_t udi_cb_alloc_sync(
		__klzbzcore::driver::CachedInfo *drvInfoCache,
		udi_index_t cb_idx, udi_cb_t **retcb);

	namespace dma
	{
		void kek(void);
		void udi_dma_prepare(
			udi_dma_prepare_call_t *callback,
			udi_cb_t *gcb,
			udi_dma_constraints_t constraints,
			udi_ubit8_t flags);

		void udi_dma_free(udi_dma_handle_t dma_handle);

		void udi_dma_mem_alloc(
			udi_dma_mem_alloc_call_t *callback,
			udi_cb_t *gcb,
			udi_dma_constraints_t constraints,
			udi_ubit8_t flags,
			udi_ubit16_t nelements,
			udi_size_t element_size,
			udi_size_t max_gap);

		void udi_dma_mem_to_buf(
			udi_dma_mem_to_buf_call_t *callback,
			udi_cb_t *gcb,
			udi_dma_handle_t dma_handle,
			udi_size_t src_off,
			udi_size_t src_len,
			udi_buf_t *dst_buf);

		void udi_dma_buf_map(
			udi_dma_buf_map_call_t *callback,
			udi_cb_t *gcb,
			udi_dma_handle_t dma_handle,
			udi_buf_t *buf,
			udi_size_t offset,
			udi_size_t len,
			udi_ubit8_t flags);

		void udi_dma_mem_barrier(udi_dma_handle_t dma_handle);
		void udi_dma_sync(
			udi_dma_sync_call_t *callback,
			udi_cb_t *gcb,
			udi_dma_handle_t dma_handle,
			udi_size_t offset,
			udi_size_t length,
			udi_ubit8_t flags);
	}
}

#endif
