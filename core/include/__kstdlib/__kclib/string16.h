#ifndef ___KCLIB_STRING_UCS16_H
	#define ___KCLIB_STRING_UCS16_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/compiler/size_t.h>

/**	EXPLANATION:
 * This is not code to manipulate UTF-16 strings. It manipulates UCS-16. *NOT*
 * UTF-16. Get that VERY clearly through your head before you use these.
 **/

#ifdef __cplusplus
extern "C" {
#endif

void *memset16(void *ptr, int value, size_t count);
void *memcpy16(void *dest, void *src, size_t count);
utf16Char *strcpy16(utf16Char *dest, const utf16Char *src);
size_t strlen16(const utf16Char *str);
int strcmp16(const utf16Char *str1, const utf16Char *str2);
int strncmp16(const utf16Char *str1, const utf16Char *str2, int count);
size_t strnlen16(const utf16Char *str1, size_t maxLen);

#ifdef __cplusplus
}
#endif

#endif

