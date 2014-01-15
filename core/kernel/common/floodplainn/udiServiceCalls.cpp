
#define UDI_VERSION	0x101
#include <udi.h>
#include <stdarg.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/floodplainn/floodplainn.h>


void __udi_assert(const char *expr, const char *file, int line)
{
	printf(FATAL"udi_assert failure: line %d of %s; expr '%s'.\n",
		line, file, expr);

	assert_fatal(0);
}

error_t floodplainnC::zudi_sys_channel_spawn(
	processId_t regionId0, udi_ops_vector_t *opsVector0,
	void *channelContext0,
	processId_t regionId1, udi_ops_vector_t *opsVector1,
	void *channelContext1
	)
{
	(void)	regionId0;
	(void)	opsVector0;
	(void)	channelContext0;
	(void)	regionId1;
	(void)	opsVector1;
	(void)	channelContext1;

	return ERROR_SUCCESS;
}

void udi_mei_call(
	udi_cb_t *,
	udi_mei_init_t *,
	udi_index_t ,
	udi_index_t vec_idx,
	...
	)
{
	va_list		args;

	va_start(args, vec_idx);
	va_end(args);
}

