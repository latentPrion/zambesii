#ifndef _BITMAP_H
	#define _BITMAP_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

#define BITMAP_INDEX(__bit)	((__bit) / sizeof(*bmp.rsrc.bmp))
#define BITMAP_OFFSET(__bit)	((__bit) % sizeof(*bmp.rsrc.bmp))

class bitmapC
{
public:
	bitmapC(ubit32 nBits);
	bitmapC(void);

	/* Used to inialize BMPs which would have been constructed in the
	 * absence of dynamic memory allocation.
	 **/
	error_t initialize(ubit32 nBits);
	~bitmapC(void);

public:
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

private:
	struct bmpStateS
	{
		uarch_t		*bmp;
		ubit32		nBits;
	};
	sharedResourceGroupC<waitLockC, bmpStateS>	bmp;
};

#endif
