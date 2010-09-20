#ifndef ___KUTF_8_H
	#define ___KUTF_8_H

	#include <__kstdlib/__ktypes.h>

namespace utf8
{
	utf16Char parse2(const utf8Char **str);
	utf16Char parse3(const utf8Char **str);
	utf16Char parse4(const utf8Char **str);
}

#endif

