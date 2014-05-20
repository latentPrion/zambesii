#ifndef _BITMAP_H
	#define _BITMAP_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

#define BITMAP			"Bitmap: "

#define BITMAP_INDEX(__bit)			\
	((__bit) / (sizeof(*bmp.rsrc.bmp) * __BITS_PER_BYTE__))
#define BITMAP_OFFSET(__bit)			\
	((__bit) % (sizeof(*bmp.rsrc.bmp) * __BITS_PER_BYTE__))

/* Args:
 * __pb = pointer to the bitmapC object to be checked.
 * __n = the bit number which the bitmap should be able to hold. For example,
 *	 if the bit number is 32, then the BMP will be checked for 33 bit
 *	 capacity or higher, since 0-31 = 32 bits, and bit 32 would be the 33rd
 *	 bit.
 * __ret = pointer to variable to return the error code from the operation in.
 * __fn = The name of the function this macro was called inside.
 * __bn = the human recognizible name of the bitmapC instance being checked.
 *
 * The latter two are used to print out a more useful error message should an
 * error occur.
 **/
#define CHECK_AND_RESIZE_BMP(__pb,__n,__ret,__fn,__bn)			\
	do { \
		*(__ret) = (__pb)->resizeTo((__n) + 1); \
		if (*(__ret) != ERROR_SUCCESS) \
		{ \
			printf(ERROR"%s: resize failed on %s with " \
				"required capacity = %d.\n", \
				__fn, __bn, __n); \
		}; \
	} while (0)

class messageStreamC;

class bitmapC
{
friend class messageStreamC;
public:
	/* Used to inialize BMPs which must be initialized in the absence of
	 * dynamic memory allocation.
	 **/
	struct preallocatedMemoryS
	{
		preallocatedMemoryS(void *ptr=NULL, ubit16 size=0)
		:
		vaddr(ptr), size(size)
		{}

		preallocatedMemoryS operator =(int)
			{ return *this; }

		void		*vaddr;
		ubit16		size;
	};

	bitmapC(void);

	error_t initialize(
		ubit32 nBits, preallocatedMemoryS preallocatedMemory=0);

	~bitmapC(void);

public:
	ubit32 getNBits(void) { return bmp.rsrc.nBits; };

	void merge(bitmapC *b);

	void lock(void);
	void unlock(void);

	// You must lock off the bmp with bitmapC::lock() before calling these.
	void set(ubit32 bit);
	void unset(ubit32 bit);
	sarch_t test(ubit32 bit);

	// These three acquire the lock on their own and shouldn't use lock().
	void setSingle(ubit32 bit);
	void unsetSingle(ubit32 bit);
	sarch_t testSingle(ubit32 bit);

	// XXX: passing 0 to this will deallocate the internal bmp, btw.
	error_t resizeTo(ubit32 nBits);

	void dump(void);

private:
	ubit8		preAllocated;
	ubit16		preAllocatedSize;

	struct bmpStateS
	{
		uarch_t		*bmp;
		ubit32		nBits;
	};
	sharedResourceGroupC<waitLockC, bmpStateS>	bmp;
};

#endif

