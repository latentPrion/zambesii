#ifndef _UTF_16_H
	#define _UTF_16_H

	#include <__kstdlib/__ktypes.h>

namespace utf16
{
	inline unicodePoint toCodepoint(utf16Char *str)
	{
		unicodePoint	c;

		c = (str[1] & 0x3FF);
		c |= (str[0] & ((1<<6) - 1)) << 10;
		c |= (((str[0] >> 6) & 0x1F) + 1) << 16;

		return c;
	}

	inline void toUtf16(unicodePoint c, utf16Char *h, utf16Char *l)
	{
		*l = (c & 0x3FF) | 0xDC00;
		*h = ((c & 0xFFFF) >> 10)
			| ((((c >> 16) & 0x1F) - 1) << 6)
			| 0xD800;
	}

}

#endif

