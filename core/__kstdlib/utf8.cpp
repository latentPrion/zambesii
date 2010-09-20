
#include <__kstdlib/utf8.h>

utf16Char utf8::parse2(const utf8Char **str)
{
	unicodePoint	ret;

	ret = **str & 0x1F;
	*str += sizeof(**str);
	ret |= (**str & 0x3F) << 5;
	return ret;
}

utf16Char utf8::parse3(const utf8Char **str)
{
	unicodePoint	ret;

	ret = **str & 0x0F;
	*str += sizeof(**str);
	ret |= (**str & 0x3F) << 4;
	*str += sizeof(**str);
	ret |= (**str & 0x3F) << 10;
	return ret;
}

utf16Char utf8::parse4(const utf8Char **str)
{
	unicodePoint	ret;

	ret = **str & 0x07;
	*str += sizeof(**str);
	ret |= (**str & 0x3F) << 3;
	*str += sizeof(**str);
	ret |= (**str & 0x3F) << 9;
	*str += sizeof(**str);
	ret |= (**str & 0x3F) << 15;
	return ret;
}

