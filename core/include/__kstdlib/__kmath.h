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
	if (__item & (__boundary - 1)) \
	{ \
		__item &= ~(__boundary - 1); \
		__item += __boundary; \
	}

#define __KMATH_ROUND_UP_BY(__item,__boundary)				\
	if (__item % __boundary) \
	{ \
		__item -= (__item % __boundary); \
		__item += __boundary; \
	}

/**	EXPLANATION:
 * This function helps us avoid division and multiplication a LOT.
 * Probably one of the most absolutely useful functions in the whole kernel.
 *
 * Whenever you have a number to multiply by, and you know that that number is
 * definitely always going to be a multiple of 2, you should use this function:
 *
 * Say, you have a line of code that needs to divide by sizeof(uarch_t).
 * On some builds, uarch_t may be 4 bytes large, others 8, others 16, maybe
 * even (for some masochist who decides to port to a 16 bit platform) even 2
 * bytes.
 *
 * But ALL of these numbers are powers of two. So shifting would be much faster.
 * This function takes our power of two, and returns the number of places you'd
 * have to shift by to get a MUL or DIV by that number.
 *
 * Best way to use it is not on every MUL or DIV, but once globally, and store
 * the value in some variable. Then used that stored value from then on.
 **/
ubit16 getShiftFor(uarch_t num);

#endif

