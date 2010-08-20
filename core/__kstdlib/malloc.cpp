
#include <__kstdlib/__kclib/stdlib.h>
#include <__kclasses/poolAllocator.h>


void *malloc(uarch_t nBytes)
{
	return poolAllocator.allocate(nBytes);
}

void *realloc(void *, uarch_t)
{
	return __KNULL;
}

void *calloc(uarch_t objSize, uarch_t nObjs)
{
	return poolAllocator.allocate(objSize * nObjs);
}

void free(void *mem)
{
	poolAllocator.free(mem);
}

