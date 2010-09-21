#ifndef _UTF_16_H
	#define _UTF_16_H

	#include <__kstdlib/__ktypes.h>

namespace utf16
{
	unicodePoint toCodepoint(utf16Char *str);
	ubit32 toUtf16(unicodePoint cp);
}

#endif

