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

#define LZUDI_CHECK_GCB_AND_CALLBACK_VALID(gcb,cb,...) do { \
	if ((gcb) == NULL || (cb) == NULL) { \
		printf(ERROR"%s: gcb(%p) or cb(%p) arg is NULL.\n", \
			__FUNCTION__, (gcb), (cb)); \
		if ((cb) != NULL) { \
			cb(__VA_ARGS__); \
		} \
	} \
} while (0)

namespace fplainn
{
	class Endpoint;
	struct sChannelMsg;
	namespace dma
	{
		class Constraints;
		class ScatterGatherList;
		class MappedScatterGatherList;
	}
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

	namespace buf
	{
		class MappedScatterGatherList
		:	public udi_buf_t
		{
		public:
			MappedScatterGatherList(void)
			:
			vaddr(NULL), nBytes(0), nFrames(0), sGListIndex(-1)
			{}

			error_t initialize() { return ERROR_SUCCESS; }

			~MappedScatterGatherList(void) {}

		public:
			virtual void memset8(
				uarch_t offset, ubit8 value, uarch_t nBytes);
			virtual void memset16(
				uarch_t offset, ubit8 value, uarch_t nBytes);
			virtual void memset32(
				uarch_t offset, ubit8 value, uarch_t nBytes);

			virtual sbit8 hasEnoughMemoryForWrite(
				uarch_t dest_off, uarch_t dest_len,
				uarch_t src_len);

			virtual sbit8 hasEnoughMemoryForRead(
				uarch_t off, uarch_t len);

			virtual sarch_t write(
				const void *inbuff,
				uarch_t offset, uarch_t nBytes);

			virtual sarch_t read(
				void *const mem,
				uarch_t offset, uarch_t nBytes);

			void memmove(
				uarch_t destOff, uarch_t srcOff,
				uarch_t nBytes);

		protected:
			void			*vaddr;
			uarch_t			nBytes, nFrames;
			sarch_t			sGListIndex;
		};
	}

	namespace dma
	{
		struct sHandle
		{
			sarch_t					sGListIndex;
			fplainn::dma::Constraints 		*constraints;
			fplainn::dma::ScatterGatherList		*sGList;
			uarch_t					flags;
		};

		sHandle *udi_dma_prepare_sync(
			udi_dma_constraints_t constraints,
			udi_ubit8_t flags);
	}
}

#endif
