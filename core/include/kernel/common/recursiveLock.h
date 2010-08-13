#ifndef _RECURSIVE_LOCK_H
	#define _RECURSIVE_LOCK_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/lock.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

/**	EXPLANATION:
 * Standard recursive non-sleeping lock. Use only where unavoidable. Introduces
 * a bit of overhead due to the contained waitLockC which must be acquired
 * before a read to the current task id.
 **/

class recursiveLockC
:
public lockC
{
public:
	recursiveLockC(void)
	{
		initialize()
	}

	void initialize(void)
	{
		taskId.rsrc = __KPROCESSID;
		lockC::initialize();
	}

public:
	void acquire(void);
	void release(void);

private:
	sharedResourceGroupC<waitLockC, uarch_t>	taskId;
};

#endif

