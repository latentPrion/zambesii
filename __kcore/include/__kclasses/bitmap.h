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

#define BITMAP_NELEMENTS_FOR_BITS(_nbits_desired) \
	(__KMATH_NELEMENTS( \
		((_nbits_desired)), \
		sizeof(Bitmap::element_t) * __BITS_PER_BYTE__) \
	)

#define BITMAP_PREALLOCATED_NELEMENTS_FOR_BITS(_nbits_desired) \
	BITMAP_NELEMENTS_FOR_BITS(_nbits_desired)

#define BITMAP_DEFINE_PREALLOCATED_MEM(_varname,_nbits_desired) \
	Bitmap::element_t	(_varname)[ \
		BITMAP_PREALLOCATED_NELEMENTS_FOR_BITS((_nbits_desired))]

/* Args:
 * __pb = pointer to the Bitmap object to be checked.
 * __n = the bit number which the bitmap should be able to hold. For example,
 *	 if the bit number is 32, then the BMP will be checked for 33 bit
 *	 capacity or higher, since 0-31 = 32 bits, and bit 32 would be the 33rd
 *	 bit.
 * __ret = pointer to variable to return the error code from the operation in.
 * __fn = The name of the function this macro was called inside.
 * __bn = the human recognizible name of the Bitmap instance being checked.
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

class MessageStream;

class Bitmap
{
friend class MessageStream;
public:
	typedef ubit32 element_t;

	/* Used to inialize BMPs which must be initialized in the absence of
	 * dynamic memory allocation.
	 **/
	struct sPreallocatedMemory
	{
		sPreallocatedMemory(element_t *ptr=NULL, ubit16 size=0)
		:
		vaddr(ptr), size(size)
		{}

		element_t	*vaddr;
		ubit16		size;
	};

	Bitmap(void)
	: bmp(CC"Bitmap bmp")
	{
		bmp.rsrc.bmp = NULL;
		bmp.rsrc.nBits = 0;
	}

	error_t initialize(
		ubit32 nBits,
		sPreallocatedMemory _preallocatedMemory=sPreallocatedMemory(NULL, 0));

	~Bitmap(void);

public:
	ubit32 getNBits(void) { return bmp.rsrc.nBits; };

	void merge(Bitmap *b);

	void lock(void);
	void unlock(void);

	// You must lock off the bmp with Bitmap::lock() before calling these.
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
	sPreallocatedMemory preallocatedMemory;

	uarch_t unlocked_getBitCapacity(void) const;

	struct sBmpState
	{
		element_t	*bmp;
		ubit32		nBits;
	};
	SharedResourceGroup<WaitLock, sBmpState>	bmp;
};

#endif

