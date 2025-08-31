/* Minimal gthr.h for freestanding libstdc++ builds */

#ifndef _GLIBCXX_GTHR_H
#define _GLIBCXX_GTHR_H

/* Thread types for freestanding environment */
typedef int __gthread_key_t;
typedef int __gthread_once_t;
typedef int __gthread_mutex_t;
typedef int __gthread_recursive_mutex_t;
typedef int __gthread_cond_t;
typedef int __gthread_rwlock_t;

/* Thread functions - stubbed out for freestanding */
#define __gthread_active_p() 0
#define __gthread_create(thread, attr, func, arg) 0
#define __gthread_join(thread, value_ptr) 0
#define __gthread_detach(thread) 0
#define __gthread_equal(t1, t2) 0
#define __gthread_self() 0
#define __gthread_yield() 0
#define __gthread_once(once_control, init_routine) 0
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

#endif /* _GLIBCXX_GTHR_H */
