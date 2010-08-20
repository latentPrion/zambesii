
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/poolAllocator.h>


void *operator new (size_t nBytes)
{
	return poolAllocator.allocate(nBytes);
}
void *operator new[](size_t nBytes)
{
	return poolAllocator.allocate(nBytes);
}

void operator delete (void *mem)
{
	poolAllocator.free(mem);
}
void operator delete[](void *mem)
{
	poolAllocator.free(mem);
}

