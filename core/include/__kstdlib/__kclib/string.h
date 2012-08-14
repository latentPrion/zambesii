#ifndef _STRING_H
	#define _STRING_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/compiler/size_t.h>

/**	EXPLANATION:
 * Simple, C standard (mostly) compliant UCS-8 string manipulation library. Get
 * that solidly: NOT UTF-8: UCS-8.
 **/

#ifdef __cplusplus
extern "C" {
#endif

void *memset(void *ptr, int value, size_t count);
void *memcpy(void *dest, void *src, size_t count);
char *strcpy(char *dest, const char *src);
/*size_t strlen(const char *str);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, int count);*/

#ifdef __cplusplus
}
#endif

#endif

