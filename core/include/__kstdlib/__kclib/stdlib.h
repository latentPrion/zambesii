#ifndef _STD_LIB_H
	#define _STD_LIB_H

	#include <__kstdlib/__ktypes.h>

#ifdef __cplusplus
extern "C" {
#endif

void *malloc(uarch_t nBytes);
void *realloc(void *oldMem, uarch_t newSize);
void *calloc(uarch_t objSize, uarch_t nObjs);
void free(void *mem);

#ifdef __cplusplus
}
#endif

#endif

