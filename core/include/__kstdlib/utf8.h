#ifndef ___KUTF_8_H
	#define ___KUTF_8_H

	#include <__kstdlib/__ktypes.h>

namespace utf8
{
	inline unicodePoint parse2(const utf8Char **str)
	{
		unicodePoint	c;

		c = (**str & 0x1F) << 6;
		*str++;
		c |= (**str & 0x3F);

		return c;
	};

	inline unicodePoint parse3(const utf8Char **str)
	{
		unicodePoint	c;

		c = (**str & 0xF) << 12;
		*str++;
		c |= (**str & 0x3F) << (12 - 6);
		*str++
		c |= (**str & 0x3F);

		return c;
	};

	inline unicodePoint parse4(const utf8Char **str)
	{
		unicodePoint	c;

		c = (**str & 0x7) << 18);
		*str++;
		c |= (**str & 0x3F) << (18 - 6);
		*str++;
		c |= (**str & 0x3F) << (18 - 12);
		*str++;
		c |= (**str & 0x3F);

		return c;
	};
}

#endif

