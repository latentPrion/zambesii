#ifndef _RIVULET_LOCKING_API_H
	#define _RIVULET_LOCKING_API_H

/**	EXPLANATION:
 * Since the kernel is written in C++, and the rivulets are done in C, we need
 * to find a way to expose the locking API to rivulets. Enter the idea of
 * allowing rivulets to anonymously allocate locks and manipulate them via
 * pointers.
 *
 * The kernel takes the lock pointer as a handle and acquires, released, etc.
 * on behalf of the rivulet.
 **/

void *waitLock_create(void);
void waitLock_destroy(void *);

void waitLock_acquire(void *);
void waitLock_release(void *);

void *recursiveLock_create(void);
void recursiveLock_destroy(void *);

void recursiveLock_acquire(void *);
void recursiveLock_release(void *);

#endif

