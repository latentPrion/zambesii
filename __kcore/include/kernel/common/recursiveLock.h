#ifndef _RECURSIVE_LOCK_H
	#define _RECURSIVE_LOCK_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/lock.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

/**	EXPLANATION:
 * Standard recursive non-sleeping lock. Use only where unavoidable. Introduces
 * a bit of overhead due to the contained WaitLock which must be acquired
 * before a read to the current task id.
 **/

class RecursiveLock
:
public Lock
{
public:
	RecursiveLock(void)
	{
		taskId.rsrc = PROCID_INVALID;
	}

public:
	void acquire(void);
	void release(void);

private:
	SharedResourceGroup<WaitLock, processId_t>	taskId;
};

#endif

