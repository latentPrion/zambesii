#ifndef _STD_LIB_H
	#define _STD_LIB_H

	#include <__kstdlib/__ktypes.h>

#ifdef __cplusplus
extern "C" {
#endif

void *malloc(uarch_t nBytes);
// void *realloc(void *oldMem, uarch_t newSize);
void *calloc(uarch_t objSize, uarch_t nObjs);
void free(void *mem);

#ifdef __cplusplus
inline
#else
static
#endif
error_t crudeRealloc(
	void *oldMem, uarch_t oldMemSize,
	void **newMem, uarch_t newMemSize
	)
{
	void		*_newMem;

	_newMem = new ubit8[newMemSize];
	if (_newMem == NULL) { return ERROR_MEMORY_NOMEM; };

	if (oldMem != NULL && oldMemSize > 0) {
		memcpy(_newMem, oldMem, oldMemSize);
	};

	*newMem = _newMem;

	::operator delete[](oldMem);
	return ERROR_SUCCESS;
}

#ifdef __cplusplus
}
#endif

#endif
