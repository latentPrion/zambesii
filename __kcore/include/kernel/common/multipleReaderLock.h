#ifndef _MULTIPLE_READER_LOCK_H
	#define _MULTIPLE_READER_LOCK_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/lock.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

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
class MultipleReaderLock
:
public Lock
{
public:
	// Flag shift values for the 'flags' member, continuing from Lock::FLAGS_ENUM_END
	enum flagShiftE {
		MR_FLAGS_WRITE_REQUEST_SHIFT = Lock::FLAGS_ENUM_END,
		MR_FLAGS_ENUM_END
	};

	// Compile-time check that enum values fit within uarch_t
	typedef char __lock_flags_size_check2[(MR_FLAGS_ENUM_END <= __UARCH_T_NBITS__) ? 1 : -1];

	// Actual flag values derived from shifts
	enum flagValueE {
		MR_FLAGS_WRITE_REQUEST = (1 << MR_FLAGS_WRITE_REQUEST_SHIFT)
	};

	MultipleReaderLock(void)
	{}

public:
	void readAcquire(uarch_t *flags);
	void readRelease(uarch_t flags);

	void writeAcquire(void);
	void writeRelease(void);

	void readReleaseWriteAcquire(uarch_t flags);
};

#endif

