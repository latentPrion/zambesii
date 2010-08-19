#ifndef ___KMATH_H
	#define ___KMATH_H

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

#endif

