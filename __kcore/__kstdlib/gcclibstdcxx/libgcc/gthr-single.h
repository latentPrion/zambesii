/* Minimal gthr-single.h for freestanding libstdc++ builds */

#ifndef _GLIBCXX_GTHR_SINGLE_H
#define _GLIBCXX_GTHR_SINGLE_H

/* Single-threaded implementation for freestanding environment */
#define __gthread_active_p() 0

/* All thread operations are no-ops in single-threaded mode */
#define __gthread_once(once_control, init_routine) \
  do { if (*(once_control) == 0) { *(once_control) = 1; init_routine(); } } while (0)

#define __gthread_key_create(key, dtor) 0
#define __gthread_key_delete(key) 0
#define __gthread_getspecific(key) 0
#define __gthread_setspecific(key, ptr) 0

#define __gthread_mutex_init(mutex, attr) 0
#define __gthread_mutex_destroy(mutex) 0
#define __gthread_mutex_lock(mutex) 0
#define __gthread_mutex_trylock(mutex) 0
#define __gthread_mutex_unlock(mutex) 0

#define __gthread_recursive_mutex_init(mutex, attr) 0
#define __gthread_recursive_mutex_destroy(mutex) 0
#define __gthread_recursive_mutex_lock(mutex) 0
#define __gthread_recursive_mutex_trylock(mutex) 0
#define __gthread_recursive_mutex_unlock(mutex) 0

#define __gthread_cond_init(cond, attr) 0
#define __gthread_cond_destroy(cond) 0
#define __gthread_cond_wait(cond, mutex) 0
#define __gthread_cond_timedwait(cond, mutex, abstime) 0
#define __gthread_cond_signal(cond) 0
#define __gthread_cond_broadcast(cond) 0

#define __gthread_rwlock_init(rwlock, attr) 0
#define __gthread_rwlock_destroy(rwlock) 0
#define __gthread_rwlock_rdlock(rwlock) 0
#define __gthread_rwlock_tryrdlock(rwlock) 0
#define __gthread_rwlock_wrlock(rwlock) 0
#define __gthread_rwlock_trywrlock(rwlock) 0
#define __gthread_rwlock_unlock(rwlock) 0

#endif /* _GLIBCXX_GTHR_SINGLE_H */
