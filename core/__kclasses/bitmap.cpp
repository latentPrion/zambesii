
#include <__kstdlib/__kmath.h>
#include <__kstdlib/__kbitManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/bitmap.h>

bitmapC::bitmapC(void)
{
	bmp.rsrc.bmp = __KNULL;
	bmp.rsrc.nBits = 0;
}

bitmapC::bitmapC(ubit32 nBits)
{
	initialize(nBits);
}

error_t bitmapC::initialize(ubit32 nBits)
{
	ubit32		nIndexes;

	nIndexes = __KMATH_NELEMENTS(
		nBits, (sizeof(*bmp.rsrc.bmp) * __BITS_PER_BYTE__));

	bmp.rsrc.bmp = new uarch_t[nIndexes];
	if (bmp.rsrc.bmp == __KNULL)
	{
		bmp.rsrc.nBits = 0;
		return ERROR_MEMORY_NOMEM;
	};

	bmp.rsrc.nBits = nBits;
	return ERROR_SUCCESS;
}	

bitmapC::~bitmapC(void)
{
	if (bmp.rsrc.bmp != __KNULL)
	{
		delete bmp.rsrc.bmp;
		bmp.rsrc.nBits = 0;
	};
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
	// Same here. Lock is not acquired.
	if (bit < bmp.rsrc.nBits)
	{
		__KBIT_UNSET(
			bmp.rsrc.bmp[BITMAP_INDEX(bit)],
			BITMAP_OFFSET(bit));
	};
}

sarch_t bitmapC::test(ubit32 bit)
{
	// And here too.
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
	ubit32		*tmp, currentBits, nIndexes;
	error_t		ret;


	if (nBits == 0)
	{
		bmp.lock.acquire();
		if (bmp.rsrc.bmp != __KNULL) {
			delete bmp.rsrc.bmp;
		}
		bmp.rsrc.nBits = 0;
		bmp.lock.release();
		return ERROR_SUCCESS;
	}

	bmp.lock.acquire();

	currentBits = bmp.rsrc.nBits;
	if (__KMATH_NELEMENTS(
		currentBits, (sizeof(*bmp.rsrc.bmp) * __BITS_PER_BYTE__))
		== __KMATH_NELEMENTS(
			nBits, (sizeof(*bmp.rsrc.bmp) * __BITS_PER_BYTE__)))
	{
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
		bmp.rsrc.bmp = tmp;
		bmp.rsrc.nBits = nBits;
		bmp.lock.release();
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
	bmp.lock.acquire();

	if (bit < bmp.rsrc.nBits)
	{
		return __KBIT_TEST(
			bmp.rsrc.bmp[BITMAP_INDEX(bit)],
			BITMAP_OFFSET(bit));
	};
	return 0;

	bmp.lock.release();
}

