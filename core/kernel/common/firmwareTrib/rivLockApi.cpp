
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/waitLock.h>
#include <kernel/common/recursiveLock.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/firmwareTrib/rivLockApi.h>

void *waitLock_create(void)
{
	waitLockC	*ret;

	ret = new waitLockC;

	if (ret == __KNULL) {
		return __KNULL;
	};

	return ret;
}

void waitLock_destroy(void *wl)
{
	if (wl == __KNULL) {
		return;
	};

	delete static_cast<waitLockC *>( wl );
}

void waitLock_acquire(void *wl)
{
	if (static_cast<waitLockC *>( wl )->magic == LOCK_MAGIC) {
		static_cast<waitLockC *>( wl )->acquire();
	};
}

void waitLock_release(void *wl)
{
	if (static_cast<waitLockC *>( wl )->magic == LOCK_MAGIC) {
		static_cast<waitLockC *>( wl )->release();
	};
}

void *recursiveLock_create(void)
{
	recursiveLockC	*ret;

	ret = new recursiveLockC;

	if (ret == __KNULL) {
		return __KNULL;
	};

	return ret;
}

void recursiveLock_destroy(void *rl)
{
	if (static_cast<recursiveLockC *>( rl ) == __KNULL) {
		return;
	}

	delete static_cast<recursiveLockC *>( rl );
}

void recursiveLock_acquire(void *rl)
{
	if (static_cast<recursiveLockC *>( rl )->magic == LOCK_MAGIC) {
		static_cast<recursiveLockC *>( rl )->acquire();
	};
}

void recursiveLock_release(void *rl)
{
	if (static_cast<recursiveLockC *>( rl )->magic == LOCK_MAGIC) {
		static_cast<recursiveLockC *>( rl )->release();
	};
}

