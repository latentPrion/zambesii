#ifndef _WAIT_LOCK_H
	#define _WAIT_LOCK_H

	#include <arch/atomic.h>
	#include <kernel/common/lock.h>

/**	EXPLANATION:
 * Standard non-sleeping lock. Clears local logical CPU's IRQs and spins until
 * the lock is acquired.
 **/

class RecursiveLock;
class MultipleReaderLock;

class WaitLock
:
public Lock
{
friend class RecursiveLock;
friend class MultipleReaderLock;

public:
	enum flagShiftE {
		WL_FLAGS_ENUM_END = Lock::FLAGS_ENUM_END
	};

	// Compile-time check that enum values fit within uarch_t
	typedef char __lock_flags_size_check2[(WL_FLAGS_ENUM_END <= __UARCH_T_NBITS__) ? 1 : -1];

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

