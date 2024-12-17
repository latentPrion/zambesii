#ifndef _FLAG_MANIPULATION_H
	#define _FLAG_MANIPULATION_H

	#include <arch/arch.h>

#define FLAG_NBITS_IN(__field)				\
	(sizeof((__field)) * __BITS_PER_BYTE__)

#define FLAG_SET(__var,__flag)		((__var) |= (__flag))
#define FLAG_UNSET(__var,__flag)	((__var) &= ~((__flag)))
#define FLAG_TEST(__var,__flag)		((__var) & (__flag))

#endif
