#ifndef ___KMATH_H
	#define ___KMATH_H

	#include <__kstdlib/__ktypes.h>

/* Returns the number of times that __block can fit into __raw, plus one extra
 * if there is any remainder.
 **/
#define __KMATH_NELEMENTS(__raw,__block)				\
	(((__raw) / (__block)) + (((__raw) % (__block)) ? 1: 0))

// Round a number up to a power of two.
#define __KMATH_P2ROUND_UP_BY(__item,__boundary)			\
	if ((__item) & ((__boundary) - 1)) \
	{ \
		__item &= ~((__boundary) - 1); \
		__item += (__boundary); \
	}

#define __KMATH_ROUND_UP_BY(__item,__boundary)				\
	if ((__item) % (__boundary)) \
	{ \
		__item -= ((__item) % (__boundary)); \
		__item += (__boundary); \
	}

/**	EXPLANATION:
 * This function can theoretically save us a lot of multiplication/division.
 * If there is some power-of-two number which can only be known at runtime,
 * which you will be dividing/multiplying by repeatedly, and you can cache this
 * number and re-use it, you can instead just use this function to get the
 * number of bit-shifts that will yield a DIV or MUL by that number, and cache
 * the number of bit-shifts so you can SHL/SHR instead of MUL/DIV-ing.
 **/
ubit16 getShiftFor(uarch_t num);

#endif

