#include <stdarg.h>

#include <debug.h>
#include <__kclasses/debugPipe.h>


void PRINTFON(int cond, const utf8Char *str, ...)
{
	va_list args;

	va_start(args, str);
	if (cond)
		return vprintf(str, args);

	va_end(args);
}
