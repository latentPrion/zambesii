#ifndef ___KUTF_8_H
	#define ___KUTF_8_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>

namespace utf8
{
	inline unicodePoint toCodepoint2(const utf8Char *str)
	{
		unicodePoint	c;

		c = (str[0] & 0x1F) << 6;
		c |= (str[1] & 0x3F);

		return c;
	}

	inline unicodePoint toCodepoint3(const utf8Char *str)
	{
		unicodePoint	c;

		c = (str[0] & 0xF) << 12;
		c |= (str[1] & 0x3F) << (12 - 6);
		c |= (str[2] & 0x3F);

		return c;
	}

	inline unicodePoint toCodepoint4(const utf8Char *str)
	{
		unicodePoint	c;

		c = (str[0] & 0x7) << 18;
		c |= (str[1] & 0x3F) << (18 - 6);
		c |= (str[2] & 0x3F) << (18 - 12);
		c |= (str[3] & 0x3F);

		return c;
	}
}

#endif

