
#include <__kstdlib/__kclib/stdarg.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/firmwareTrib/rivDebugApi.h>

void rivPrintf(utf8Char *str, ...)
{
	va_list		v;

	va_start_forward(v, str);
	__kprintf(str, v);
}

