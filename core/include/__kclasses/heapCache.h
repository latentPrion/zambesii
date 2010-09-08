#ifndef _HEAP_CACHE_H
	#define _HEAP_CACHE_H

class heapCacheC
{
public:
	heapCacheC(void)
	:
	objectSize(0)
	{};

	heapCacheC(uarch_t objectSize)
	:
	objectSize(objectSize)
	{};

	virtual ~heapCacheC(void) {}

public:
	virtual void *allocate(void) { return __KNULL; };
	virtual void free(void *) {};

public:
	uarch_t objectSize;
};

#endif

