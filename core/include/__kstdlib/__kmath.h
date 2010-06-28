#ifndef ___KMATH_H
	#define ___KMATH_H

/* Returns the number of times that __block can fit into __raw, plus one extra
 * if there is any remainder.
 **/
#define __KMATH_NELEMENTS(__raw,__block)				\
	(((__raw) / (__block)) + (((__raw) % (__block)) ? 1: 0))

#endif

