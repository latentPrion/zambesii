#ifndef _PADDR_T_H
	#define _PADDR_T_H

	#include <__kstdlib/__ktypes.h>

#ifndef __ASM__

#ifdef CONFIG_ARCH_x86_32_PAE
typedef
{
	ubit32		low;
	ubit32		high;
} paddr_t;

#define PADDR_INIT(high,low)		{low,high}

#define pp_eq(p,q)		do {p.low = q.low; p.high = q.high;} while (0)
#define pu_eq(p,n)		do {p.low = n; p.high = 0;} while (0)

#ifdef __cplusplus
extern "C" {
#endif

// Basic turing stuff.
inline void pu_add(paddr_t p, uarch_t n)
{
	if ((p.low + n) < p.low)
	{
		n -= (0xFFFFFFFF - p.low) + 1;
		p.low = 0;
		p.high += 1;
	};
	p.low += n;
}

inline void pu_sub(paddr_t p, uarch_t n)
{
	if (n > p.low)
	{
		n -= p.low;
		p.low = 0xFFFFFFFF;
		p.high -= 1;
	};
	p.low -= n;
}	

// Binary operations.
inline void pp_and(paddr_t p, paddr_t q)
{
	p.high &= q.high;
	p.low &= q.low;
}

inline void pp_or(paddr_t p, paddr_t q)
{
	p.high |= q.high;
	p.low |= q.low;
}

inline void pu_and(paddr_t p, uarch_t n)
{
	p.low &= n;
}

inline void pu_or(paddr_t p, uarch_t n)
{
	p.low |= n;
}

inline void p_shl(paddr_t p, ubit8_t n)
{
	uarch_t		tmp;

	if (n == 0) {
		return;
	};

	tmp = p.low >> (32 - n);
	p.low << n;
	p.high << n;
	p.high |= tmp;
}

inline void p_shr(paddr_t p, ubit8 n)
{
	uarch_t		tmp;

	if (n == 0) {
		return;
	};

	tmp = p.high << (32 - n);
	p.low >>= n;
	p.high >>= n;
	p.low |= tmp;
}	

#ifdef __cplusplus
}
#endif

#else
typedef uarch_t		paddr_t;

#define pp_eq(p,q)		do { p = q; } while (0)
#define pu_eq(p,u)		do { p = u; } while (0)

#define pu_add(p,u)		do { p+=u; } while (0)
#define pu_sub(p,u)		do { p-=u; } while (0)

#define pp_and(p,q)		do { p &= q; } while (0)
#define pp_or(p,q)		do { p |= q; } while (0)
#define pu_and(p,u)		do { p &= u; } while (0)
#define pu_or(p,u)		do { p |= u; } while (0)
#define p_shl(p,n)		do { p <<= n; } while (0)
#define p_shr(p,n)		do { p >>= n; } while (0)

#endif

#endif /* ! defined (__ASM__) */

#endif

