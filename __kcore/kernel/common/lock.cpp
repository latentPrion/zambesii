#include <stdarg.h>
#include <arch/cpuControl.h>
#include <arch/debug.h>
#include <__kstdlib/__ktypes.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/lock.h>
#include <kernel/common/waitLock.h>
#include <kernel/common/recursiveLock.h>
#include <kernel/common/multipleReaderLock.h>
#include <kernel/common/deadlock.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


#ifdef CONFIG_DEBUG_LOCKS
sDeadlockBuff deadlockBuffers[CONFIG_MAX_NCPUS];

/**
 * Common function to handle deadlock detection for all lock types
 *
 * This function centralizes the deadlock detection logic that was previously
 * duplicated across different lock implementations. It handles:
 * 1. Getting the current CPU ID
 * 2. Checking for recursive deadlock detection
 * 3. Formatting and printing the deadlock message
 * 4. Halting the CPU
 *
 * The format string should include placeholders for all arguments, including
 * the lock type, operation, lock object, CPU ID, caller address, and any additional information.
 */
void reportDeadlock(utf8Char *formatStr, ...)
{
	va_list args;
	cpu_t cid;

	// Get the current CPU ID
	cid = cpuTrib.getCurrentCpuStream()->cpuId;
	if (cid == CPUID_INVALID) { cid = 0; }

	/**	EXPLANATION:
	 * This "inUse" feature allows us to detect infinite recursion
	 * deadlock loops. This can occur when the kernel somehow
	 * manages to deadlock in printf() AND also then deadlock
	 * in the deadlock debugger printf().
	 **/
	if (deadlockBuffers[cid].inUse == 1) {
		panic();
	}

	deadlockBuffers[cid].inUse = 1;

	// Format the entire message using the varargs, ensuring it wraps at 80 chars
	va_start(args, formatStr);

	// Use the debugPipe's built-in wrapping capability
	vnprintf(
		&deadlockBuffers[cid].buffer,
		DEADLOCK_BUFF_MAX_NBYTES,
		formatStr,
		args);
	va_end(args);

	debug::sStackDescriptor stack;
	debug::getCurrentStackInfo(&stack);

	debug::printStackTraceToBuffer(
		&deadlockBuffers[cid].buffer,
		DEADLOCK_BUFF_MAX_NBYTES,
		debug::getBasePointer(),
		&stack);

	deadlockBuffers[cid].inUse = 0;
	cpuControl::halt();
}
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
