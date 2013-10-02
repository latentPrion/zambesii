#ifndef _STRING_H
	#define _STRING_H

	#include <stddef.h>
	#include <__kstdlib/__ktypes.h>

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
int memcmp(const void *ptr1, const void *ptr2, size_t n);

#ifdef __cplusplus
}
#endif

#endif

