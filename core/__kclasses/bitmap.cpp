
#include <__kstdlib/__kmath.h>
#include <__kstdlib/__kbitManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kclasses/bitmap.h>


bitmapC::bitmapC(void)
{
	preAllocated = 0;
	preAllocatedSize = 0;
	bmp.rsrc.bmp = NULL;
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

	if (preAllocatedMemory != NULL)
	{
		preAllocated = 1;
		preAllocatedSize = preAllocatedMemorySize;
	};

	bmp.rsrc.bmp = (preAllocatedMemory != NULL)
		? new (preAllocatedMemory) uarch_t[nIndexes]
		: new uarch_t[nIndexes];

	if (bmp.rsrc.bmp == NULL && nBits != 0)
	{
		bmp.rsrc.nBits = 0;
		return ERROR_MEMORY_NOMEM;
	};

	bmp.rsrc.nBits = nBits;
	// Don't pass NULL to memset when BMP is initialized to 0 bits.
	if (bmp.rsrc.bmp != NULL)
	{
		if (!preAllocated)
		{
			memset(
				bmp.rsrc.bmp, 0,
				nIndexes * sizeof(*bmp.rsrc.bmp));
		}
		else {
			memset(bmp.rsrc.bmp, 0, preAllocatedSize);
		};
	};

	return ERROR_SUCCESS;
}	

void bitmapC::dump(void)
{
	__kprintf(NOTICE BITMAP"@0x%p: %d bits, %s (%dB), array @0x%p.\n",
		this,
		bmp.rsrc.nBits,
		(preAllocated) ? "pre-allocated" : "dyn-allocated",
		(preAllocated) ? preAllocatedSize : 0,
		bmp.rsrc.bmp);
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
	if (!preAllocated && bmp.rsrc.bmp != NULL) {
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
	if (preAllocated && (preAllocatedSize * __BITS_PER_BYTE__ >= nBits))
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

		if (!preAllocated && bmp.rsrc.bmp != NULL) {
			delete bmp.rsrc.bmp;
		};

		preAllocated = 0;
		bmp.rsrc.bmp = NULL;
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

	if (tmp == NULL) {
		ret = ERROR_MEMORY_NOMEM;
	}
	else
	{
		bmp.lock.acquire();
		// Copy the old array's state into the new.
		if (bmp.rsrc.bmp != NULL)
		{
			memcpy(
				tmp, bmp.rsrc.bmp,
				nIndexes * sizeof(*bmp.rsrc.bmp));
		};
		oldmem = bmp.rsrc.bmp;
		bmp.rsrc.bmp = tmp;
		bmp.rsrc.nBits = nBits;
		bmp.lock.release();

		if (!preAllocated && oldmem != NULL) { delete oldmem; };
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

