
#include <__kstdlib/__kclib/stdarg.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/firmwareTrib/rivDebugApi.h>

void rivPrintf(utf8Char *str, ...)
{
	va_list		args;

	va_start_forward(args, str);
	__kdebug.printf(str, args);
}

