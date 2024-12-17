
#include <debug.h>
#include <__kstdlib/__kmath.h>
#include <__kstdlib/__kbitManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kclasses/bitmap.h>
#include <kernel/common/panic.h>


error_t Bitmap::initialize(
	ubit32 nBits, sPreallocatedMemory _preAllocatedMemory
	)
{
	const uarch_t		nIndexes=BITMAP_NELEMENTS_FOR_BITS(nBits);

	if (_preAllocatedMemory.vaddr != NULL)
	{
		if (nBits > _preAllocatedMemory.size * __BITS_PER_BYTE__)
			{ return ERROR_LIMIT_OVERFLOWED; };

		preallocatedMemory = _preAllocatedMemory;
		bmp.rsrc.bmp = preallocatedMemory.vaddr;
	}
	else
	{
		if (nIndexes == 0) { return ERROR_SUCCESS; }

		bmp.rsrc.bmp = new element_t[nIndexes];
		if (bmp.rsrc.bmp == NULL)
			{ return ERROR_MEMORY_NOMEM; };
	}

	bmp.rsrc.nBits = nBits;
	memset(
		bmp.rsrc.bmp, 0,
		(nIndexes > 0)
			? nIndexes * sizeof(*bmp.rsrc.bmp)
			: preallocatedMemory.size);

	return ERROR_SUCCESS;
}

void Bitmap::dump(void)
{
	printf(NOTICE BITMAP"@%p: %d bits, (%s %dB), array @%p.\n",
		this,
		bmp.rsrc.nBits,
		(!!preallocatedMemory.vaddr) ? "pre-allocated" : "dyn-allocated",
		(!!preallocatedMemory.vaddr) ? preallocatedMemory.size : 0,
		bmp.rsrc.bmp);
}

void Bitmap::merge(Bitmap *b)
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

Bitmap::~Bitmap(void)
{
	if (preallocatedMemory.vaddr == NULL && bmp.rsrc.bmp != NULL) {
		delete[] bmp.rsrc.bmp;
	};

	bmp.rsrc.bmp = NULL;
	bmp.rsrc.nBits = 0;
	preallocatedMemory = sPreallocatedMemory(NULL, 0);
}

void Bitmap::lock(void)
{
	bmp.lock.acquire();
}

void Bitmap::unlock(void)
{
	bmp.lock.release();
}

void Bitmap::set(ubit32 bit)
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

void Bitmap::unset(ubit32 bit)
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

sarch_t Bitmap::test(ubit32 bit)
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

error_t Bitmap::resizeTo(ubit32 _nBits)
{
	uarch_t		bitCapacity;
	element_t	*tmp, *old;
	sbit8		wasPreAllocated;

	bmp.lock.acquire();

	// If _nBits == 0, deallocate all mem.
	if (_nBits == 0)
	{
		bmp.rsrc.nBits = 0;
		old = bmp.rsrc.bmp;
		bmp.rsrc.bmp = NULL;
		wasPreAllocated = !!preallocatedMemory.vaddr;
		preallocatedMemory = sPreallocatedMemory(NULL, 0);

		bmp.lock.release();

		if (!wasPreAllocated) { delete[] old; };
		printf(WARNING"BMP resized to 0. Caller is %p.\n",
			__builtin_return_address(0));

		return ERROR_SUCCESS;;
	};

	if (!!preallocatedMemory.vaddr) {
		bitCapacity = preallocatedMemory.size * __BITS_PER_BYTE__;
	}
	else
	{
		bitCapacity = BITMAP_NELEMENTS_FOR_BITS(bmp.rsrc.nBits)
			* sizeof(*bmp.rsrc.bmp) * __BITS_PER_BYTE__;

		assert_fatal(bitCapacity >= bmp.rsrc.nBits);
		// Not precise, but within ballpark -- still a useful check.
		assert_fatal(bitCapacity < bmp.rsrc.nBits
			+ sizeof(*bmp.rsrc.bmp) * __BITS_PER_BYTE__);
	};

	// If we don't need to resize upwards:
	if (_nBits < bitCapacity)
	{
		bmp.rsrc.nBits = _nBits;
		bmp.lock.release();
		return ERROR_SUCCESS;
	};

	// Else:
	tmp = new element_t[BITMAP_NELEMENTS_FOR_BITS(_nBits)];
	if (tmp == NULL)
	{
		bmp.lock.release();
		return ERROR_MEMORY_NOMEM;
	};

	memset(tmp, 0, __KMATH_NELEMENTS(_nBits, __BITS_PER_BYTE__));
	old = bmp.rsrc.bmp;
	if (old != NULL)
	{
		memcpy(
			tmp, old,
			__KMATH_NELEMENTS(bmp.rsrc.nBits, __BITS_PER_BYTE__));
	};

	bmp.rsrc.nBits = _nBits;
	bmp.rsrc.bmp = tmp;
	wasPreAllocated = !!preallocatedMemory.vaddr;
	preallocatedMemory = sPreallocatedMemory(NULL, 0);

	bmp.lock.release();

	if (!wasPreAllocated) { delete[] old; };
	return ERROR_SUCCESS;
}

void Bitmap::setSingle(ubit32 bit)
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

void Bitmap::unsetSingle(ubit32 bit)
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

sarch_t Bitmap::testSingle(ubit32 bit)
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

