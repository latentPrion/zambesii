#ifndef ___K_LIB_ZAMBESII_UDI_H
	#define ___K_LIB_ZAMBESII_UDI_H

	#define UDI_VERSION	0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <extern.h>

CPPEXTERN_START

void udi_mei_call(
	udi_cb_t *gcb,
	udi_mei_init_t *meta_info,
	udi_index_t meta_ops_num,
	udi_index_t vec_idx,
	...);

CPPEXTERN_END

#endif
