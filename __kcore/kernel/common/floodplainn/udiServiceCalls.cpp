
#define UDI_VERSION	0x101
#include <udi.h>
#include <stdarg.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/process.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>


void __udi_assert(const char *expr, const char *file, int line)
{
	printf(FATAL"udi_assert failure: line %d of %s; expr '%s'.\n",
		line, file, expr);

	assert_fatal(0);
}
