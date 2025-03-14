
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/lock.h>
#include <kernel/common/waitLock.h>
#include <kernel/common/recursiveLock.h>
#include <kernel/common/multipleReaderLock.h>
#include <kernel/common/deadlock.h>


#ifdef CONFIG_DEBUG_LOCKS
sDeadlockBuff deadlockBuffers[CONFIG_MAX_NCPUS];
#endif

void Lock::sOperationDescriptor::execute()
{
	if (lock == NULL) { panic(FATAL"execute: lock is NULL.\n"); };

	switch (type)
	{
	case WAIT:
		static_cast<WaitLock *>( lock )->release();
		break;

	case RECURSIVE:
		static_cast<RecursiveLock *>( lock )->release();
		break;

	case MULTIPLE_READER:
		switch (operation)
		{
		case READ:
			static_cast<MultipleReaderLock *>( lock )
				->readRelease(rwFlags);

			break;

		case WRITE:
			static_cast<MultipleReaderLock *>( lock )
				->writeRelease();

			break;

		default:
			panic(FATAL"execute: Invalid unlock operation.\n");
		};
		break;

	default: panic(FATAL"execute: Invalid lock type.\n");
	};
}
