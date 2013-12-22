
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/memReservoir.h>


void *operator new (size_t nBytes)
{
	return memReservoir.allocate(nBytes);
}

void operator delete (void *mem)
{
	if (mem == NULL) { return; };
	memReservoir.free(mem);
}

