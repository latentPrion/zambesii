
#define UDI_VERSION	0x101
#include <udi.h>
#include <__kstdlib/__kclib/stdarg.h>


void udi_mei_call(
	udi_cb_t *gcb,
	udi_mei_init_t *meta_info,
	udi_index_t meta_ops_num,
	udi_index_t vec_idx,
	...
	)
{
	va_list		args;

	va_start(args, vec_idx);
	va_end(args);
}
