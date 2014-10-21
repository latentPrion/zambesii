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

CPPEXTERN_START

void udi_mei_call(
	udi_cb_t *gcb,
	udi_mei_init_t *meta_info,
	udi_index_t meta_ops_num,
	udi_index_t vec_idx,
	...);

void udi_channel_spawn(
	udi_channel_spawn_call_t *callback,
	udi_cb_t *gcb,
	udi_channel_t channel,
	udi_index_t spawn_idx,
	udi_index_t ops_idx,
	void *channel_context);

void udi_channel_set_context(
	udi_channel_t target_channel,
	void *channel_context);

void udi_channel_anchor(
	udi_channel_anchor_call_t *callback,
	udi_cb_t *gcb,
	udi_channel_t channel,
	udi_index_t ops_idx,
	void *channel_context);

void udi_channel_close(udi_channel_t channel);

CPPEXTERN_END

namespace fplainn
{
	class Endpoint;
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
}

#endif
