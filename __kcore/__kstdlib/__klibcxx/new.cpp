
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/memReservoir.h>


void *operator new (size_t nBytes)
{
	return memReservoir.allocate(nBytes);
}

void *operator new[](size_t nBytes)
{
	/**	EXPLANATION:
	 * Internally, the compiler inserts the following code here:
	 *
	 * nBytes += sizeof(void *);
	 **/
	return ::operator new(nBytes);
	/**	EXPLANATION:
	 * And after the call, it takes the pointer that is to be returned,
	 * (represented as 'ret') and inserts this code here:
	 *
	 * *(void **)ret = NUM_OBJECTS_ALLOCATED;
	 * ret = (void *)((uintptr_t)ret + sizeof(void *));
	 *
	 * // Construct all objects.
	 * for (int i=0; i<NUM_OBJECTS_ALLOCATED; i++) {
	 *	new (&ret[i]) OBJECT_TYPE;
	 * };
	 * return ret;
	 **/
}

void operator delete (void *mem)
{
	if (mem == NULL) { return; };
	memReservoir.free(mem);
}

void operator delete[](void *mem)
{
	/**	EXPLANATION:
	 * Internally, the compiler inserts the following code here:
	 *
	 * void		**tmp = mem;
	 * tmp--;
	 * int		numObjects = *(int *)tmp;
	 *
	 * // Call destructor for each object.
	 * for (int i=0; i<numObjects; i++) {
	 *	mem[i].~OBJECT_TYPE_DESTRUCTOR();
	 * };
	 *
	 * mem = (void *)tmp;
	 **/
	::operator delete(mem);
}


void *operator new (size_t nBytes, MemReservoir *heap)
{
	return heap->allocate(nBytes);
}

void operator delete(void *mem, MemReservoir *heap)
{
	if (mem == NULL) { return; };
	heap->free(mem);
}
