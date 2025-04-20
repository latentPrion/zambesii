#include <stdarg.h>

#include <debug.h>
#include <__kclasses/debugPipe.h>


sarch_t PRINTFON(int cond, const utf8Char *str, ...)
{
	va_list args;
	sarch_t n = 0;

	if (!cond) { return 0; }

	va_start(args, str);
	n = vprintf(str, args);
	va_end(args);

	return n;
}
