
#include <__kstdlib/__kmath.h>
#include <__kstdlib/__kbitManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/bitmap.h>

#include <debug.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

bitmapC::bitmapC(void)
{
	preAllocated = 0;
	preAllocatedSize = 0;
	bmp.rsrc.bmp = __KNULL;
	bmp.rsrc.nBits = 0;
}

bitmapC::bitmapC(ubit32 nBits)
{
	preAllocated = 0;
	preAllocatedSize = 0;
	initialize(nBits);
}

error_t bitmapC::initialize(
	ubit32 nBits,
	void *preAllocatedMemory, ubit16 preAllocatedMemorySize
	)
{
	ubit32		nIndexes;

	nIndexes = __KMATH_NELEMENTS(
		nBits, (sizeof(*bmp.rsrc.bmp) * __BITS_PER_BYTE__));

	if (preAllocatedMemory != __KNULL)
	{
		preAllocated = 1;
		preAllocatedSize = preAllocatedMemorySize;
	};

	bmp.rsrc.bmp = (preAllocatedMemory != __KNULL)
		? new (preAllocatedMemory) uarch_t[nIndexes]
		: new uarch_t[nIndexes];

	if (bmp.rsrc.bmp == __KNULL && nBits != 0)
	{
		bmp.rsrc.nBits = 0;
		return ERROR_MEMORY_NOMEM;
	};

	bmp.rsrc.nBits = nBits;
	memset(bmp.rsrc.bmp, 0, nIndexes * sizeof(*bmp.rsrc.bmp));
	return ERROR_SUCCESS;
}	

void bitmapC::merge(bitmapC *b)
{
	// ORs this bmp with the one passed as an argument.
	lock();
	b->lock();

	for (ubit32 i=0; i < b->getNBits() && i < getNBits(); i++)
	{
		if (b->test(i)) {
			set(i);
		};
	};

	b->unlock();
	unlock();
}

bitmapC::~bitmapC(void)
{
	if (!preAllocated && bmp.rsrc.bmp != __KNULL) {
		delete bmp.rsrc.bmp;
	};

	bmp.rsrc.nBits = 0;
	preAllocated = 0;
}

void bitmapC::lock(void)
{
	bmp.lock.acquire();
}

void bitmapC::unlock(void)
{
	bmp.lock.release();
}

void bitmapC::set(ubit32 bit)
{
	/* Don't acquire the lock. We expect the user to call lock() before 
	 * calling this method.
	 **/
	if (bit < bmp.rsrc.nBits)
	{
		__KBIT_SET(
			bmp.rsrc.bmp[BITMAP_INDEX(bit)],
			BITMAP_OFFSET(bit));
	};
}

void bitmapC::unset(ubit32 bit)
{
	/* Don't acquire the lock. We expect the user to call lock() before 
	 * calling this method.
	 **/
	if (bit < bmp.rsrc.nBits)
	{
		__KBIT_UNSET(
			bmp.rsrc.bmp[BITMAP_INDEX(bit)],
			BITMAP_OFFSET(bit));
	};
}

sarch_t bitmapC::test(ubit32 bit)
{
	/* Don't acquire the lock. We expect the user to call lock() before 
	 * calling this method.
	 **/
	if (bit < bmp.rsrc.nBits)
	{
		return __KBIT_TEST(
			bmp.rsrc.bmp[BITMAP_INDEX(bit)],
			BITMAP_OFFSET(bit));
	};
	return 0;
}

error_t bitmapC::resizeTo(ubit32 nBits)
{
	uarch_t		*tmp, *oldmem;
	ubit32		currentBits, nIndexes;
	error_t		ret;

	/* If the BMP was pre-allocated and the amount of memory that was
	 * assigned to it is enough to forego re-allocation, exit early.
	 **/
	if (preAllocated
		&& preAllocatedSize >= __KMATH_NELEMENTS(nBits, sizeof(ubit8)))
	{
		bmp.lock.acquire();
		bmp.rsrc.nBits = nBits;
		bmp.lock.release();
		return ERROR_SUCCESS;
	};

	// Passing nBits = 0 arg deallocates the BMP.
	if (nBits == 0)
	{
		bmp.lock.acquire();

		if (!preAllocated && bmp.rsrc.bmp != __KNULL) {
			delete bmp.rsrc.bmp;
		};

		preAllocated = 0;
		bmp.rsrc.bmp = __KNULL;
		bmp.rsrc.nBits = 0;

		bmp.lock.release();
		return ERROR_SUCCESS;
	}

	bmp.lock.acquire();

	currentBits = bmp.rsrc.nBits;

	// If the new nBits will fit in the currently allocated memory:
	if (__KMATH_NELEMENTS(
		currentBits, (sizeof(*bmp.rsrc.bmp) * __BITS_PER_BYTE__))
		== __KMATH_NELEMENTS(
			nBits, (sizeof(*bmp.rsrc.bmp) * __BITS_PER_BYTE__)))
	{
		bmp.rsrc.nBits = nBits;
		bmp.lock.release();
		return ERROR_SUCCESS;
	};

	bmp.lock.release();

	nIndexes = __KMATH_NELEMENTS(
		nBits, (sizeof(*bmp.rsrc.bmp) * __BITS_PER_BYTE__));

	tmp = new uarch_t[nIndexes];

	if (tmp == __KNULL) {
		ret = ERROR_MEMORY_NOMEM;
	}
	else
	{
		bmp.lock.acquire();
		// Copy the old array's state into the new.
		if (bmp.rsrc.bmp != __KNULL)
		{
			memcpy(
				tmp, bmp.rsrc.bmp,
				nIndexes * sizeof(*bmp.rsrc.bmp));
		};
		oldmem = bmp.rsrc.bmp;
		bmp.rsrc.bmp = tmp;
		bmp.rsrc.nBits = nBits;
		bmp.lock.release();

		if (!preAllocated && oldmem != __KNULL) { delete oldmem; };
		preAllocated = 0;

		ret = ERROR_SUCCESS;
	};

	return ret;
}

void bitmapC::setSingle(ubit32 bit)
{
	bmp.lock.acquire();

	if (bit < bmp.rsrc.nBits)
	{
		__KBIT_SET(
			bmp.rsrc.bmp[BITMAP_INDEX(bit)],
			BITMAP_OFFSET(bit));
	};

	bmp.lock.release();
}

void bitmapC::unsetSingle(ubit32 bit)
{
	bmp.lock.acquire();

	if (bit < bmp.rsrc.nBits)
	{
		__KBIT_UNSET(
			bmp.rsrc.bmp[BITMAP_INDEX(bit)],
			BITMAP_OFFSET(bit));
	};

	bmp.lock.release();
}

sarch_t bitmapC::testSingle(ubit32 bit)
{
	sarch_t		ret;

	bmp.lock.acquire();

	if (bit < bmp.rsrc.nBits)
	{
		ret = __KBIT_TEST(
			bmp.rsrc.bmp[BITMAP_INDEX(bit)],
			BITMAP_OFFSET(bit));

		bmp.lock.release();
		return ret;
	};

	bmp.lock.release();
	return 0;
}

