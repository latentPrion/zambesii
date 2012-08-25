#ifndef _TIMER_TRIBUTARY_TIME_TYPES_H
	#define _TIMER_TRIBUTARY_TIME_TYPES_H

	#include <__kstdlib/__ktypes.h>

	/**	EXPLANATION:
	 * Timer Tributary time types: Zambesii types used to represent dates
	 * and times.
	 *
	 *	DATE:
	 * Dates are represented as gregorian dates packed into a 32-bit
	 * integer. Any locale which does not use gregorian dates can easily
	 * convert to and from them, and most locales today do use them, so I
	 * feel that this is a sensible and natural choice, as opposed to using
	 * seconds elapsed since January X XXXX.
	 *
	 *	TIME:
	 * Time is represented as 24-hour clock value packed into a 32-bit
	 * integer. A locale which prefers some other format may easily convert
	 * to and from the 24-hour clock format, as it is widely known. I prefer
	 * this to using seconds elapsed since January X XXXX.
	 *
	 * Time is accurate within this abstract type up to millisecond
	 * precision.
	 *
	 *	MACRO USAGE:
	 * date_t	myBirthday;
	 * myBirthday = TIMERTRIB_DATE_SET_YEAR(1991)
	 *	| TIMERTRIB_DATE_SET_MONTH(11)
	 *	| TIMERTRIB_DATE_SET_DAY(8);
	 *
	 * printf("My birthday is the %dth of the %dth month, %d.\n",
	 *	TIMERTRIB_DATE_GET_DAY(myBirthday),
	 *	TIMERTRIB_DATE_GET_MONTH(myBirthday),
	 *	TIMERTRIB_DATE_GET_YEAR(myBirthday));
	 **/

#define TIMERTRIB_DATE_DAY_SHIFT
#define TIMERTRIB_DATE_DAY_MASK

#define TIMERTRIB_DATE_MONTH_SHIFT
#define TIMERTRIB_DATE_MONTH_MASK

/**	EXPLANATION:
 * Year is given 18 bits to represent a set of dates ranging from 100,000 B.C.E
 * to 100,000 C.E. I think that is adequate for any program having been written
 * according to the C standard, and more than adequate for any program which
 * does normal date manipulation. Programs which do more than that generally
 * have their own date format, and won't be asking the kernel for date/time
 * keeping services beyond what they absolutely need to.
 **/
// These are "intrinsics" macros -- use the two below for manipulating years.
#define TIMERTRIB_DATE_YEAR_SHIFT		(0)
#define TIMERTRIB_DATE_YEAR_MASK		(0x3FFFF)

// Use these to set and get date values.
#define TIMERTRIB_DATE_GET_YEAR(__year)		\
	(((__year) >> TIMERTRIB_DATE_YEAR_SHIFT) & TIMERTRIB_DATE_YEAR_MASK)

#define TIMERTRIB_DATE_ENCODE_YEAR(__val)\
	(((__val) & TIMERTRIB_DATE_YEAR_MASK) << TIMERTRIB_DATE_YEAR_SHIFT)

/**	EXPLANATION:
 * Month is given 4 bits so that it can represent values from 0-15. Valid
 * values range from 1-12. Programs using this data type are responsible for
 * ensuring that dates they use internally are sane.
 **/
// These are "intrinsics" macros -- use the two below for manipulating years.
#define TIMERTRIB_DATE_MONTH_SHIFT		(18)
#define TIMERTRIB_DATE_MONTH_MASK		(0xF)

// Use these to set and get date values.
#define TIMERTRIB_DATE_GET_MONTH(__mon)		\
	(((__mon) >> TIMERTRIB_DATE_MONTH_SHIFT) & TIMERTRIB_DATE_MONTH_MASK)

#define TIMERTRIB_DATE_ENCODE_MONTH(__val)\
	(((__val) & TIMERTRIB_DATE_MONTH_MASK) << TIMERTRIB_DATE_MONTH_SHIFT)

/**	EXPLANATION:
 * Day is given 5 bits so that it can represent values from 0-31. Valid
 * values range from 1-31. Programs using this data type are responsible for
 * ensuring that dates they use internally are sane.
 **/
// These are "intrinsics" macros -- use the two below for manipulating years.
#define TIMERTRIB_DATE_DAY_SHIFT		(22)
#define TIMERTRIB_DATE_DAY_MASK			(0x1F)

// Use these to set and get date values.
#define TIMERTRIB_DATE_GET_DAY(__day)		\
	(((__day) >> TIMERTRIB_DATE_DAY_SHIFT) & TIMERTRIB_DATE_DAY_MASK)

#define TIMERTRIB_DATE_ENCODE_DAY(__val)\
	(((__val) & TIMERTRIB_DATE_DAY_MASK) << TIMERTRIB_DATE_DAY_SHIFT)

typedef ubit32		date_t;

// Hour set/get macros.
#define TIMERTRIB_TIME_HOUR_SHIFT		(0)
#define TIMERTRIB_TIME_HOUR_MASK		(0x1F)
#define TIMERTRIB_TIME_GET_HOUR(__t)		\
	(((__t) >> TIMERTRIB_TIME_HOUR_SHIFT) & TIMERTRIB_TIME_HOUR_MASK)

#define TIMERTRIB_TIME_SET_HOUR(__h)		\
	(((__h) & TIMERTRIB_TIME_HOUR_MASK) << TIMERTRIB_TIME_HOUR_SHIFT)

// Minute set/get macros.
#define TIMERTRIB_TIME_MINUTE_SHIFT		(5)
#define TIMERTRIB_TIME_MINUTE_MASK		(0x3F)
#define TIMERTRIB_TIME_GET_MINUTE(__t)		\
	(((__t) >> TIMERTRIB_TIME_MINUTE_SHIFT) & TIMERTRIB_TIME_MINUTE_MASK)

#define TIMERTRIB_TIME_SET_MINUTE(__m)		\
	(((__m) & TIMERTRIB_TIME_MINUTE_MASK) << TIMERTRIB_TIME_MINUTE_SHIFT)

// Second set/get macros.
#define TIMERTRIB_TIME_SECOND_SHIFT		(11)
#define TIMERTRIB_TIME_SECOND_MASK		(0x3F)
#define TIMERTRIB_TIME_GET_SECOND(__t)		\
	(((__t) >> TIMERTRIB_TIME_SECOND_SHIFT) & TIMERTRIB_TIME_SECOND_MASK)

#define TIMERTRIB_TIME_SET_SECOND(__s)		\
	(((__s) & TIMERTRIB_TIME_SECOND_MASK) << TIMERTRIB_TIME_SECOND_SHIFT)

// Milli-second set/get macros.
#define TIMERTRIB_TIME_MSECOND_SHIFT		(17)
#define TIMERTRIB_TIME_MSECOND_MASK		(0x3FF)
#define TIMERTRIB_TIME_GET_MSECOND(__t)		\
	(((__t) >> TIMERTRIB_TIME_MSECOND_SHIFT) & TIMERTRIB_TIME_MSECOND_MASK)

#define TIMERTRIB_TIME_SET_SECOND(__ms)		\
	(((__ms) & TIMERTRIB_TIME_MSECOND_MASK) << TIMERTRIB_TIME_MSECOND_SHIFT)

typedef ubit32		time_t;

#endif

