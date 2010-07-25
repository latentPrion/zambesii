#ifndef __KBIT_MANIPULATION_H
	#define __KBIT_MANIPULATION_H

	#include <arch/arch.h>

#define __KBIT_ON		1
#define __KBIT_OFF		0

#define __KBIT_NBITS_IN(var)		(sizeof(var) * __BITS_PER_BYTE__ )

#define __KBIT_SET(var,bit)		(var |= (1 << bit))
#define __KBIT_UNSET(var,bit)		(var &= ~(1 << bit))
#define __KBIT_TEST(var,bit)		(var & (1 << bit))

//Only use these for bitmaps which are arrays of an integral type.
#define BITMAP_WHICH_INDEX(__b,__c)			\
	( __c / (sizeof(*__b) * __BITS_PER_BYTE__) )

#define BITMAP_WHICH_BIT(__b,__c)			\
	( __c % (sizeof(*__b) * __BITS_PER_BYTE__) )

#endif

