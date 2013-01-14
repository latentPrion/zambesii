
#include <__kstdlib/__kclib/stdlib.h>
#include <__kclasses/memReservoir.h>


void *malloc(uarch_t nBytes)
{
	return memReservoir.allocate(nBytes);
}

void *realloc(void *mem, uarch_t nBytes)
{
	memReservoir.free(mem);
	return memReservoir.allocate(nBytes);
}

void *calloc(uarch_t objSize, uarch_t nObjs)
{
	return memReservoir.allocate(objSize * nObjs);
}

void free(void *mem)
{
	if (mem == __KNULL) { return; };
	memReservoir.free(mem);
}

