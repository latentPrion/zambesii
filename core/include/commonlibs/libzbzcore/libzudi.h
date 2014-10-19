#ifndef ___K_LIB_ZAMBESII_UDI_H
	#define ___K_LIB_ZAMBESII_UDI_H

	#define UDI_VERSION	0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <zui.h>
	#include <extern.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <__kclasses/ptrList.h>


CPPEXTERN_START

void udi_mei_call(
	udi_cb_t *gcb,
	udi_mei_init_t *meta_info,
	udi_index_t meta_ops_num,
	udi_index_t vec_idx,
	...);

CPPEXTERN_END

namespace fplainn { class Endpoint; }

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

		fplainn::Endpoint		*__kendpoint;
		utf8Char			metaName[
			ZUI_DRIVER_METALANGUAGE_MAXLEN];

		udi_mei_init_t			*metaInfo;
		udi_mei_ops_vec_template_t	*opsVectorTemplate;
		void				*channel_context;
	};

	struct sRegion
	{
		ubit16				index;
		udi_init_context_t		*rdata;
		PtrList<sEndpointContext>	endpoints;
	};

	udi_size_t _udi_get_layout_size(
		udi_layout_t *layout,
		udi_ubit16_t *inline_offset,
		udi_ubit16_t *chain_offset);

	void _udi_marshal_params(
		udi_layout_t *layout, void *marshal_space, va_list args);

	udi_boolean_t _udi_get_layout_offset(
		udi_layout_t *start, udi_layout_t **end, udi_size_t *offset,
		udi_layout_t key);
}

#endif
