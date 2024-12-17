#ifndef _PADDR_T_H
	#define _PADDR_T_H

	#include <__kstdlib/__ktypes.h>

#ifndef __ASM__

#ifdef CONFIG_ARCH_x86_32_PAE
class paddr_t
{
public:
	// Operators here.

private:
	ubit32		low, high;
};
#else
class paddr_t
{
public:
	paddr_t(const uarch_t &a=0) : low(a) {}

	paddr_t &operator =(const paddr_t &p) { low.n = p.low.n; return *this; }
	paddr_t &operator =(const volatile paddr_t &p)
		{ low.n = p.low.v; return *this; }
	// VOLATILES:
	paddr_t operator =(const paddr_t &p) volatile
		{ low.v = p.low.n; return getNv(); }
	paddr_t operator =(const volatile paddr_t &p) volatile
		{ low.v = p.low.v; return getNv(); }

public:
	int operator !(void) { return !low.n; }
	uarch_t getLow(void) { return low.n; }
	// VOLATILES:
	int operator !(void) volatile { return !low.v; }
	uarch_t getLow(void) volatile { return low.v; }


	int operator ==(paddr_t p) { return low.n == p.low.n; }
	int operator !=(paddr_t p) { return low.n != p.low.n; }
	int operator >=(paddr_t p) { return low.n >= p.low.n; }
	int operator <=(paddr_t p) { return low.n <= p.low.n; }
	int operator <(paddr_t p) { return low.n < p.low.n; }
	int operator >(paddr_t p) { return low.n > p.low.n; }
	// VOLATILES:
	int operator ==(paddr_t p) volatile { return getNv() == p; }
	int operator !=(paddr_t p) volatile { return getNv() != p; }
	int operator >=(paddr_t p) volatile { return getNv() >= p; }
	int operator <=(paddr_t p) volatile { return getNv() <= p; }
	int operator <(paddr_t p) volatile { return getNv() < p; }
	int operator >(paddr_t p) volatile { return getNv() > p; }

	paddr_t operator ~(void) { return ~low.n; }
	paddr_t operator >>(paddr_t p) { return low.n >> p.low.n; }
	paddr_t operator <<(paddr_t p) { return low.n << p.low.n; }
	paddr_t operator &(paddr_t p) { return low.n & p.low.n; }
	paddr_t operator |(paddr_t p) { return low.n | p.low.n; }
	paddr_t operator ^(paddr_t p) { return low.n ^ p.low.n; }
	// VOLATILES:
	paddr_t operator ~(void) volatile { return ~getNv(); }
	paddr_t operator >>(paddr_t p) volatile { return getNv() >> p; }
	paddr_t operator <<(paddr_t p) volatile { return getNv() << p; }
	paddr_t operator &(paddr_t p) volatile { return getNv() & p; }
	paddr_t operator |(paddr_t p) volatile { return getNv() | p; }
	paddr_t operator ^(paddr_t p) volatile { return getNv() ^ p; }

	paddr_t operator *(paddr_t p) { return low.n * p.low.n; }
	paddr_t operator /(paddr_t p) { return low.n / p.low.n; }
	paddr_t operator %(paddr_t p) { return low.n % p.low.n; }
	paddr_t operator +(paddr_t p) { return low.n + p.low.n; }
	paddr_t operator -(paddr_t p) { return low.n - p.low.n; }
	// VOLATILES:
	paddr_t operator *(paddr_t p) volatile { return getNv() * p; }
	paddr_t operator /(paddr_t p) volatile { return getNv() / p; }
	paddr_t operator %(paddr_t p) volatile { return getNv() % p; }
	paddr_t operator +(paddr_t p) volatile { return getNv() + p; }
	paddr_t operator -(paddr_t p) volatile { return getNv() - p; }

	paddr_t &operator >>=(int n) { low.n >>= n; return *this; }
	paddr_t &operator <<=(int n) { low.n <<= n; return *this; }
	paddr_t &operator &=(paddr_t p) { low.n &= p.low.n; return *this; }
	paddr_t &operator |=(paddr_t p) { low.n |= p.low.n; return *this; }
	paddr_t &operator ^=(paddr_t p) { low.n ^= p.low.n; return *this; }
	// VOLATILES:
	paddr_t operator >>=(int n) volatile { low.v >>= n; return getNv(); }
	paddr_t operator <<=(int n) volatile { low.v <<= n; return getNv(); }
	paddr_t operator &=(paddr_t p) volatile { low.v &= p.low.n; return getNv(); }
	paddr_t operator |=(paddr_t p) volatile { low.v |= p.low.n; return getNv(); }
	paddr_t operator ^=(paddr_t p) volatile { low.v ^= p.low.n; return getNv(); }

	paddr_t &operator *=(paddr_t p) { low.n *= p.low.n; return *this; }
	paddr_t &operator /=(paddr_t p) { low.n /= p.low.n; return *this; }
	paddr_t &operator %=(paddr_t p) { low.n %= p.low.n; return *this; }
	paddr_t &operator +=(paddr_t p) { low.n += p.low.n; return *this; }
	paddr_t &operator -=(paddr_t p) { low.n -= p.low.n; return *this; }
	// VOLATILES:
	paddr_t operator *=(paddr_t p) volatile { low.v *= p.low.n; return getNv(); }
	paddr_t operator /=(paddr_t p) volatile { low.v /= p.low.n; return getNv(); }
	paddr_t operator %=(paddr_t p) volatile { low.v %= p.low.n; return getNv(); }
	paddr_t operator +=(paddr_t p) volatile { low.v += p.low.n; return getNv(); }
	paddr_t operator -=(paddr_t p) volatile { low.v -= p.low.n; return getNv(); }

	// Returns a non-volatile Copy of "*this".
	paddr_t getNv(void) volatile { return (ubit32)(low.v); }

private:
	union semiVolatileU
	{
		semiVolatileU(ubit32 num)
		:
		n(num)
		{}

		volatile ubit32		v;	// Volatile.
		ubit32			n;	// Non-volatile.
	} low;
};
#endif

#endif /* ! defined (__ASM__) */

#endif
