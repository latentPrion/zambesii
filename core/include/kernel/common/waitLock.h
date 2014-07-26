#ifndef _WAIT_LOCK_H
	#define _WAIT_LOCK_H

	#include <arch/atomic.h>
	#include <kernel/common/lock.h>

/**	EXPLANATION:
 * Standard non-sleeping lock. Clears local logical CPU's IRQs and spins until
 * the lock is acquired.
 **/

class recursiveLockC;
class multipleReaderLockC;

class waitLockC
:
public Lock
{
friend class recursiveLockC;
friend class multipleReaderLockC;

public:
	void acquire();
	void release();
	// Release without re-enabling IRQs even if they were on before acquire.
	void releaseNoIrqs();

#ifdef ARCH_HAS_ATOMIC_WAITLOCK_PRIMITIVE
};
#else
} __attribute__(( section(".__katomicData") ));
#endif

#endif

