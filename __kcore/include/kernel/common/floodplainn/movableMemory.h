#ifndef _ZUDI_MOVABLE_MEMORY_HEADER_H
	#define _ZUDI_MOVABLE_MEMORY_HEADER_H

	#include <__kstdlib/__ktypes.h>

	#define UDI_MEM_MAGIC			(0x11D1111E)
	#define UDI_MOVABLE_MEM_MAGIC		(0x111011AB)

namespace fplainn
{
	struct sMovableMemory
	{
		void *operator new(size_t sz, uarch_t objectSize)
		{
			return ::operator new(sz + objectSize);
		}

		void operator delete(void *mem, uarch_t)
		{
			::operator delete(mem);
		}

		void operator delete(void *mem)
		{
			operator delete(mem, 0);
		}

		sMovableMemory(uarch_t objectNBytes)
		:
		magic(0), objectNBytes(objectNBytes)
		{}

		sMovableMemory(void)
		:
		magic(0), objectNBytes(0)
		{}

		void setMagic(uarch_t _magic) { magic = _magic; }
		sbit8 magicIsValid(void)
		{
			if (magic == UDI_MEM_MAGIC
				|| magic == UDI_MOVABLE_MEM_MAGIC)
				{ return 1; };

			return 0;
		}

		uarch_t		magic;
		uarch_t		objectNBytes;
	};
}

#endif
