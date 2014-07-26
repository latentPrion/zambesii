#ifndef _HEAP_CACHE_H
	#define _HEAP_CACHE_H

class HeapCache
{
public:
	HeapCache(void)
	:
	sObjectize(0)
	{};

	HeapCache(uarch_t sObjectize)
	:
	sObjectize(sObjectize)
	{};

	virtual ~HeapCache(void) {}

public:
	//virtual void *allocate(void) { return NULL; };
	virtual void free(void *) {};

public:
	uarch_t sObjectize;
};

#endif

