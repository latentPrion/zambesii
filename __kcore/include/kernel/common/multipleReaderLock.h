#ifndef _MULTIPLE_READER_LOCK_H
	#define _MULTIPLE_READER_LOCK_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/assert.h>
	#include <kernel/common/lock.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/panic.h>
	

/**	EXPLANATION:
 * Multiple Reader Lock is a reader-writer lock which enforces concurrency
 * protection when it is likely that a resource is likely to be read often, but
 * updated seldom.
 *
 * The idea is to prevent concurrency issues even if it's likely that corrupt
 * reads won't occur in the case of a simultaneous write. As expected, writers
 * are prioritized on lock acquire.
 *
 * The readers are 
 **/

/* The writer asserts this flag to notify all current readers not to try for the
 * lock on readRelease().
 **/
#include <__kclasses/debugPipe.h>
class MultipleReaderLock
:
public Lock
{
public:
	friend class WaitLock;

	class ScopedWriteGuard
	: public Lock::ScopedGuard
	{
	public:
		ScopedWriteGuard(MultipleReaderLock* _lock)
		: Lock::ScopedGuard(_lock)
		{
			/**	EXPLANATION:
			 * The ScopedWriteGuard is specifically allowed to be
			 * initialized with a NULL lock pointer. This is to
			 * facilitate the use case where a dequeuing thread
			 * is allowed to manually manage the lock using
			 * CALLER_SCHEDLOCKED.
			 **/
			if (_lock != NULL)
				{ _lock->writeAcquire(); }
		}

		ScopedWriteGuard &move_assign(ScopedWriteGuard &other)
		{
			lock = other.lock;
			doAutoRelease = other.doAutoRelease;
			other.releaseManagement();
			return *this;
		}

		~ScopedWriteGuard()
			{ if (doAutoRelease) { unlock(); } }

		MultipleReaderLock *releaseManagement(void)
		{
			return static_cast<MultipleReaderLock*>(
				Lock::ScopedGuard::releaseManagement());
		}

		virtual void unlock(void)
		{
			if (lock == NULL) { return; }
			static_cast<MultipleReaderLock*>(lock)->writeRelease();
		}

		virtual void releaseManagementAndUnlock(void)
		{
			MultipleReaderLock	*tmp = releaseManagement();

			if (tmp != NULL) { tmp->writeRelease(); }
		}

	private:
		// Only allow TaskTrib::block() to call default constructor
		friend class TaskTrib;
		ScopedWriteGuard(void)
		: Lock::ScopedGuard(NULL)
		{
			Lock::ScopedGuard::doAutoRelease = 0;
		}
	};

	class ScopedReadGuard
	: public Lock::ScopedGuard
	{
	public:
		ScopedReadGuard(MultipleReaderLock* _lock)
		: Lock::ScopedGuard(_lock), flags(0)
		{
			assert_fatal(_lock != NULL);
			_lock->readAcquire(&flags);
		}

		ScopedReadGuard &move_assign(ScopedReadGuard &other)
		{
			lock = other.lock;
			flags = other.flags;
			doAutoRelease = other.doAutoRelease;
			other.releaseManagement();
			return *this;
		}

		~ScopedReadGuard()
			{ if (doAutoRelease) { unlock(); } }

		MultipleReaderLock *releaseManagement(void)
		{
			return static_cast<MultipleReaderLock*>(
				Lock::ScopedGuard::releaseManagement());
		}

		virtual void unlock(void)
		{
			if (lock == NULL) { return; }
			static_cast<MultipleReaderLock*>(lock)->readRelease(
				flags);
		}

		virtual void releaseManagementAndUnlock(void)
			{ releaseManagement()->readRelease(flags); }

	private:
		// Only allow TaskTrib::unblock() to call default constructor
		friend class TaskTrib;
		ScopedReadGuard(void)
		: Lock::ScopedGuard(NULL), flags(0)
		{
			Lock::ScopedGuard::doAutoRelease = 0;
		}

	private:
		// Only allow WaitLock to transfer IRQ ownership to this lock
		friend class SingleWaiterQueue;
		friend class MessageStream;
		uarch_t		flags;
	};

public:
	// Flag shift values for the 'flags' member, continuing from Lock::FLAGS_ENUM_END
	enum flagShiftE {
		MR_FLAGS_WRITE_REQUEST_SHIFT = Lock::FLAGS_ENUM_END,
		MR_FLAGS_INNER_WRITE_REQUEST_SHIFT,
		MR_FLAGS_ENUM_END
	};

	// Compile-time check that enum values fit within uarch_t
	typedef char __lock_flags_size_check2[(MR_FLAGS_ENUM_END <= __UARCH_T_NBITS__) ? 1 : -1];

	// Actual flag values derived from shifts
	enum flagValueE {
		MR_FLAGS_WRITE_REQUEST = (1 << MR_FLAGS_WRITE_REQUEST_SHIFT),
		MR_FLAGS_INNER_WRITE_REQUEST =
			(1 << MR_FLAGS_INNER_WRITE_REQUEST_SHIFT)
	};

public:
	MultipleReaderLock(const utf8Char *name)
	: Lock(name), postPrevOpState(name)
	{}

	void readAcquire(uarch_t *flags);
	void readRelease(uarch_t flags);

	void writeAcquire(void);
	void writeRelease(void);

	void readReleaseWriteAcquire(uarch_t flags);
	void dump(void);

	// Allow WaitLock to transfer IRQ ownership to this lock
	void setIrqFlags(uarch_t irqFlags) { flags |= irqFlags; }

private:
	/* This copy of the lock object is used to store the state of the lock
	 * after a previous operation has been executed.
	 * This is used to detect deadlocks.
	 *
	 * The idea is that we can see what state the previous operation left
	 * the lock in, and compare it to the current state of the lock so we
	 * can detect why the deadlock is occuring. Moreover, we can also see
	 * which function was the last to acquire the lock. So we can know which
	 * function acquired the lock, and which function is currently
	 * deadlocked on the lock.
	 **/
	Lock		postPrevOpState;
};

#endif

