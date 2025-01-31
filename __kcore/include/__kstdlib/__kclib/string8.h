#ifndef _UCS_8_STRING_H
	#define _UCS_8_STRING_H

	#include <stddef.h>
	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * Simple, C standard (mostly) compliant UCS-8 string manipulation library. Get
 * that solidly: NOT UTF-8: UCS-8.
 **/

#ifdef __cplusplus
extern "C" {
#endif

utf8Char *strstr8(const utf8Char *str1, const utf8Char *str2);
utf8Char *strcpy8(utf8Char *dest, const utf8Char *src);
utf8Char *strncpy8(utf8Char *dest, const utf8Char *src, size_t count);
size_t strlen8(const utf8Char *str);
int strcmp8(const utf8Char *str1, const utf8Char *str2);
int strncmp8(const utf8Char *str1, const utf8Char *str2, int count);
size_t strnlen8(const utf8Char *str1, size_t maxLen);
utf8Char *strnchr8(const utf8Char *str, size_t n, const utf8Char chr);
utf8Char *strcat8(utf8Char *dest, const utf8Char *src);

#ifdef __cplusplus
}
#endif

#endif

