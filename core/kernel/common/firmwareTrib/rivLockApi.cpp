
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/waitLock.h>
#include <kernel/common/recursiveLock.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/firmwareTrib/rivLockApi.h>

void *waitLock_create(void)
{
	waitLockC	*ret;

	ret = new ((memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(1, 0)) waitLockC;

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

	memoryTrib.__kmemoryStream.memFree(wl);
}

void waitLock_acquire(void *wl)
{
	if (wl->magic == LOCK_MAGIC) {
		static_cast<waitLockC *>( wl )->acquire();
	};
}

void waitLock_release(void *wl)
{
	if (wl->magic == LOCK_MAGIC) {
		static_cast<waitLockC *>( wl )->release();
	};
}

void *recursiveLock_create(void)
{
	recursiveLockC	*ret;

	ret = new ((memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(1, 0)) recursiveLockC;

	if (ret == __KNULL) {
		return __KNULL;
	};

	return ret;
}

void recursiveLock_destroy(void *rl)
{
	if (rl == __KNULL) {
		return;
	}

	memoryTrib.__kmemoryStream.memFree(rl);
}

void recursiveLock_acquire(void *rl)
{
	if (rl->magic == LOCK_MAGIC) {
		static_cast<recursiveLockC *>( rl )->acquire();
	};
}

void recursiveLock_release(void *rl)
{
	if (rl->magic == LOCK_MAGIC) {
		static_cast<recursiveLockC *>( rl )->release();
	};
}

