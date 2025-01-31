#ifndef _ASM_ATOMIC_H
#define _ASM_ATOMIC_H

	#include <__kstdlib/__ktypes.h>

#define ARCH_HAS_ATOMIC_WAITLOCK_PRIMITIVE
#define ARCH_HAS_ATOMIC_SLEEPLOCK_PRIMITIVE

// In all of these, '__lock' is expected to be a pointer.
#define ARCH_ATOMIC_WAITLOCK_HEADER(__lock,__val,__prev)		\
	while (atomicAsm::exchange(__lock, __val) != __prev)

#define ARCH_ATOMIC_SLEEPLOCK_HEADER(__lock,__val,__prev,__count)	\
	while (__count-- && atomicAsm::exchange(__lock, __val) != __prev)

namespace atomicAsm
{
	/**	TODO:
	 * Implement the actual ASM for these functions correctly.
	 **/
	inline void memoryBarrier(void);
	inline void flushInstructionCache(void);
	inline void flushDataCache(void);
	inline void flushAllCaches(void);

	inline sarch_t exchange(volatile intptr_t *lock, sarch_t val);
	inline void set(volatile intptr_t *lock, sarch_t val);
	inline void set(volatile uintptr_t *lock, uarch_t val)
	{
		set((intptr_t *)lock, (sarch_t)val);
	}

	inline sarch_t exchangeAndAdd(
		volatile sarch_t *ptr1, sarch_t *ptr2, sarch_t val);

	inline sarch_t add(volatile sarch_t *lock, sarch_t val)
		{ *lock += val; return *lock; }

	inline ubit8 bitTestAndComplement(volatile sarch_t *lock, ubit8 bit);
	inline sarch_t read(volatile sarch_t *lock) { return *lock; }
	inline uarch_t read(volatile uarch_t *lock) { return *lock; }
	inline void increment(volatile sarch_t *lock) { *lock += 1; }
	inline void increment(volatile uarch_t *lock) { *lock += 1; }
	inline void decrement(volatile sarch_t *lock) { *lock -= 1; }
	inline void decrement(volatile uarch_t *lock) { *lock -= 1; }
}



/**	Inline Methods
 ***********************************************************************/

inline void atomicAsm::memoryBarrier(void)
{
	asm volatile (
		"pushl  %edx\n\t \
		movl    %cr2, %edx\n\t \
		movl    %edx, %cr2\n\t \
		popl    %edx\n\t");
}

inline sarch_t atomicAsm::exchange(volatile intptr_t *lock, sarch_t val)
{
	asm volatile (
		"xchgl %1,%0 \n\t"
		: "=r" (val)
		: "m" (*lock), "0" (val)
		: "memory"
	);
	return val;
}

inline void atomicAsm::set(volatile intptr_t *lock, sarch_t val)
{
	atomicAsm::exchange(lock, val);
}

#endif

