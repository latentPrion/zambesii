#ifndef ___KFLAG_MANIPULATION_H
	#define ___KFLAG_MANIPULATION_H

	#include <arch/arch.h>

#define __KFLAG_NBITS_IN(__field)				\
	(sizeof((__field)) * __BITS_PER_BYTE__)

#define __KFLAG_SET(__var,__flag)		((__var) |= (__flag))
#define __KFLAG_UNSET(__var,__flag)	((__var) &= ~((__flag)))
#define __KFLAG_TEST(__var,__flag)		((__var) & (__flag))

#endif
