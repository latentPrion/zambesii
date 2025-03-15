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

	inline sarch_t exchange(volatile uarch_t *lock, sarch_t val);
	inline void set(volatile sarch_t *lock, sarch_t val);
	inline void set(volatile uarch_t *lock, uarch_t val)
	{
		set((sarch_t *)lock, (sarch_t)val);
	}

	// CMPXCHG is available on IA-32 CPUs from i486 onwards.
	inline sarch_t compareAndExchange(
		volatile sarch_t *ptr,
		const sarch_t comparatorVal, const sarch_t loadNEqVal);
	inline uarch_t compareAndExchange(
		volatile uarch_t *ptr,
		const uarch_t comparatorVal, const uarch_t loadNEqVal);

	inline sarch_t exchangeAndAdd(volatile sarch_t *ptr, sarch_t val);
	inline sarch_t add(volatile sarch_t *lock, sarch_t val);
	inline sarch_t bitTestAndComplement(volatile uarch_t *lock, ubit8 bit);
	inline sarch_t bitTestAndSet(volatile uarch_t *lock, ubit8 bit);
	inline sarch_t bitTestAndClear(volatile uarch_t *lock, ubit8 bit);
	inline sarch_t read(volatile sarch_t *lock);
	inline uarch_t read(volatile uarch_t *lock);
	inline void increment(volatile sarch_t *lock);
	inline void increment(volatile uarch_t *lock);
	inline void decrement(volatile sarch_t *lock);
	inline void decrement(volatile uarch_t *lock);
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

inline sarch_t atomicAsm::exchange(volatile uarch_t *lock, sarch_t val)
{
	asm volatile (
		"xchgl %1,%0 \n\t"
		: "=r" (val)
		: "m" (*lock), "0" (val)
		: "memory"
	);
	return val;
}

inline void atomicAsm::set(volatile sarch_t *lock, sarch_t val)
{
	atomicAsm::exchange((uarch_t *)lock, val);
}

inline sarch_t atomicAsm::exchangeAndAdd(volatile sarch_t *ptr, sarch_t val)
{
	sarch_t result;
	asm volatile (
		"lock; xaddl %0, %1"
		: "=r" (result), "+m" (*ptr)
		: "0" (val)
		: "memory"
	);
	return result;
}

inline sarch_t atomicAsm::add(volatile sarch_t *lock, sarch_t val)
{
	sarch_t result;
	asm volatile (
		"lock; addl %2, %0\n\t"
		"movl %0, %1"
		: "+m" (*lock), "=r" (result)
		: "ri" (val)
		: "memory"
	);
	return result;
}

inline sarch_t atomicAsm::bitTestAndComplement(volatile uarch_t *lock, ubit8 bit)
{
	ubit8 byte_result;
	asm volatile (
		"lock; btcl %2, %0\n\t"
		"setc %b1"
		: "+m" (*lock), "=q" (byte_result)
		: "r" ((sarch_t)bit)
		: "memory", "cc"
	);
	return (sarch_t)byte_result;
}

inline sarch_t atomicAsm::bitTestAndSet(volatile uarch_t *lock, ubit8 bit)
{
	ubit8 byte_result;
	asm volatile (
		"lock; btsl %2, %0\n\t"
		"setc %b1"
		: "+m" (*lock), "=q" (byte_result)
		: "r" ((sarch_t)bit)
		: "memory", "cc"
	);
	return (sarch_t)byte_result;
}

inline sarch_t atomicAsm::bitTestAndClear(volatile uarch_t *lock, ubit8 bit)
{
	ubit8 byte_result;
	asm volatile (
		"lock; btrl %2, %0\n\t"
		"setc %b1"
		: "+m" (*lock), "=q" (byte_result)
		: "r" ((sarch_t)bit)
		: "memory", "cc"
	);
	return (sarch_t)byte_result;
}

inline sarch_t atomicAsm::read(volatile sarch_t *lock)
{
	sarch_t result;
	asm volatile (
		"movl %1, %0"
		: "=r" (result)
		: "m" (*lock)
		: "memory"
	);
	return result;
}

inline uarch_t atomicAsm::read(volatile uarch_t *lock)
{
	uarch_t result;
	asm volatile (
		"movl %1, %0"
		: "=r" (result)
		: "m" (*lock)
		: "memory"
	);
	return result;
}

inline void atomicAsm::increment(volatile sarch_t *lock)
{
	asm volatile (
		"lock; incl %0"
		: "+m" (*lock)
		:
		: "memory"
	);
}

inline void atomicAsm::increment(volatile uarch_t *lock)
{
	asm volatile (
		"lock; incl %0"
		: "+m" (*lock)
		:
		: "memory"
	);
}

inline void atomicAsm::decrement(volatile sarch_t *lock)
{
	asm volatile (
		"lock; decl %0"
		: "+m" (*lock)
		:
		: "memory"
	);
}

inline void atomicAsm::decrement(volatile uarch_t *lock)
{
	asm volatile (
		"lock; decl %0"
		: "+m" (*lock)
		:
		: "memory"
	);
}

inline sarch_t atomicAsm::compareAndExchange(
	volatile sarch_t *ptr,
	const sarch_t comparatorVal, const sarch_t loadNEqVal
	)
{
	return (sarch_t)compareAndExchange(
		(volatile uarch_t *)ptr,
		(uarch_t)comparatorVal,
		(uarch_t)loadNEqVal);
}

inline uarch_t atomicAsm::compareAndExchange(
	volatile uarch_t *ptr,
	const uarch_t comparatorVal, const uarch_t loadNEqVal
	)
{
	uarch_t result;
	asm volatile (
		"lock; cmpxchgl %2, %1"
		: "=a" (result), "+m" (*ptr)
		: "r" (loadNEqVal), "0" (comparatorVal)
		: "memory", "cc"
	);
	return result;
}

#endif

