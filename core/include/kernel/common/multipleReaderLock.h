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
#define MRLOCK_FLAGS_WRITE_REQUEST	(1<<5)

class multipleReaderLockC
:
public lockC
{
public:
	void readAcquire(uarch_t *flags);
	void readRelease(uarch_t flags);

	void writeAcquire(void);
	void writeRelease(void);

	void readReleaseWriteAcquire(uarch_t flags);

private:
	// An atomic counter for the number of current readers.
	sharedResourceGroupC<waitLockC, volatile sarch_t>	readerCount;
};

#endif

