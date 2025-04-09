#ifndef _RECURSIVE_LOCK_H
	#define _RECURSIVE_LOCK_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/lock.h>
	#include <kernel/common/processId.h>
	#include <arch/x8632/atomic.h>

/**	EXPLANATION:
 * Standard recursive non-sleeping lock. Uses atomic compareAndExchange
 * to efficiently manage lock ownership without requiring a separate WaitLock.
 * The lock member of the base Lock class is used to store the thread ID of
 * the owner, or PROCID_INVALID if the lock is free.
 *
 * Includes deadlock detection when CONFIG_DEBUG_LOCKS is defined.
 **/

class RecursiveLock
:
public Lock
{
public:
	class ScopedGuard
	: public Lock::ScopedGuard
	{
	public:
		ScopedGuard(RecursiveLock* _lock)
		: Lock::ScopedGuard(_lock)
		{
			_lock->acquire();
		}

		~ScopedGuard()
			{ if (doAutoRelease) { unlock(); } }

		RecursiveLock *releaseManagement(void)
		{
			return static_cast<RecursiveLock *>(
				Lock::ScopedGuard::releaseManagement());
		}

		virtual void unlock(void)
		{
			if (lock == NULL) { return; }
			static_cast<RecursiveLock *>(lock)->release();
		}

		virtual void releaseManagementAndUnlock(void)
			{ releaseManagement()->release(); }
	};

public:
	enum flagShiftE {
		RL_FLAGS_ENUM_END = Lock::FLAGS_ENUM_END
	};

	// Compile-time check that enum values fit within uarch_t
	typedef char __lock_flags_size_check2[(RL_FLAGS_ENUM_END <= __UARCH_T_NBITS__) ? 1 : -1];

	RecursiveLock(void)
	: Lock()
	{
		// Initialize lock to PROCID_INVALID to indicate it's free
		lock = PROCID_INVALID;
		recursionCount = 0;
	}

public:
	void acquire(void);
	void release(void);

private:
	// Count of how many times the lock has been acquired recursively
	uarch_t recursionCount;
};

#endif

