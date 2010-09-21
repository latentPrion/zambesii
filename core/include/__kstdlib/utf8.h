#ifndef ___KUTF_8_H
	#define ___KUTF_8_H

	#include <__kstdlib/__ktypes.h>

namespace utf8
{
	unicodePoint parse2(const utf8Char **str);
	unicodePoint parse3(const utf8Char **str);
	unicodePoint parse4(const utf8Char **str);
}

#endif

