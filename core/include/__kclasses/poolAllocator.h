#ifndef _POOL_ALLOCATOR_H
	#define _POOL_ALLOCATOR_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/allocClass.h>

class poolAllocatorC
:
public allocClassC
{
public:
	poolAllocatorC(uarch_t poolSize);
	error_t initialize(uarch_t poolSize);
	~poolAllocatorC(void);

public:
	void *allocate(uarch_t nBytes);
	void free(void *mem);

public:
	struct poolS
	{
		poolS	*next;
		uarch_t	refCount;
	};
	struct freeObjectS
	{
		freeObjectS	*next;
		uarch_t		nBytes;
	};

	uarch_t		poolSize;
};

#endif

