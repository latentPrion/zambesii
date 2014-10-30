#ifndef _ZUDI_MOVABLE_MEMORY_HEADER_H
	#define _ZUDI_MOVABLE_MEMORY_HEADER_H

	#include <__kstdlib/__ktypes.h>

namespace fplainn
{
	struct sMovableMemory
	{
		void *operator new(size_t sz, uarch_t objectSize);
		void operator delete(void *mem);
		sMovableMemory(uarch_t nBytes)
		:
		nBytes(nBytes)
		{}

		sMovableMemory(void)
		:
		nBytes(0)
		{}

		uarch_t		nBytes;
	};
}

#endif
