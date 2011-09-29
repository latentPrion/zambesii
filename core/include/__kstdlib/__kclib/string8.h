#ifndef _UCS_8_STRING_H
	#define _UCS_8_STRING_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/size_t.h>

/**	EXPLANATION:
 * Simple, C standard (mostly) compliant UCS-8 string manipulation library. Get
 * that solidly: NOT UTF-8: UCS-8.
 **/

#ifdef __cplusplus
extern "C" {
#endif

void *memset8(void *ptr, int value, size_t count);
void *memcpy8(void *dest, void *src, size_t count);
utf8Char *strcpy8(utf8Char *dest, const utf8Char *src);
utf8Char *strncpy8(utf8Char *dest, const utf8Char *src, size_t count);
size_t strlen8(const utf8Char *str);
int strcmp8(const utf8Char *str1, const utf8Char *str2);
int strncmp8(const utf8Char *str1, const utf8Char *str2, int count);

#ifdef __cplusplus
}
#endif

#endif
