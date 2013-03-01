#ifndef _TIMER_TRIBUTARY_TIME_TYPES_H
	#define _TIMER_TRIBUTARY_TIME_TYPES_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/processId.h>

	/**	EXPLANATION:
	 * Timer Tributary time types: Zambesii types used to represent dates
	 * and times.
	 *
	 *	DATE:
	 * Dates are represented as gregorian dates packed into a 32-bit
	 * integer. Any locale which does not use gregorian dates can easily
	 * convert to and from them, and most locales today do use them, so I
	 * feel that this is a sensible and natural choice, as opposed to going
	 * full retard and using "seconds elapsed since January X XXXX".
	 *
	 *	TIME:
	 * Time is stored as the offset, in seconds from the current date. I
	 * prefer this to using seconds elapsed since January X XXXX.
	 *
	 * The time structure also has an 'nseconds' member, which represents
	 * the number of nanoseconds elapsed since the last concrete second.
	 * Therefore the structure is able to record time with up to nanosecond
	 * accuracy, though most of the time the actual value stored in the
	 * structure will be accurate to the millisecond rather than the
	 * microsecond or nanosecond.
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

/**	EXPLANATION:
 * Year is given 18 bits to represent a set of dates ranging from 100,000 B.C.E
 * to 100,000 A.D. I think that is adequate for any program having been written
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


/**	EXPLANATION:
 * The kernel stores time as the number of seconds elapsed since the start of
 * the current day.
 *
 * Timestamps are a combination of a dateS and a timeS, which together give
 * an absolute value for the current date and time, along with a nanosecond
 * magnitude value for up to nanosecond time precision.
 *
 * 17 bits are needed to store the number of seconds in a day. A further approx
 * 30 bits are required to store the number of nanoseconds elapsed for up to
 * nanosecond time accuracy.
 *
 * The time data type will take up more than 32 bits, no matter how I represent
 * it, so I decided to split the units into two 32-bit integers. It's still the
 * same 64 bits I was going to use anyway. Usage should be intuitive.
 *
 *	USAGE:
 * timeS	myTime = { 0, 0 };
 * ubit8	h, m, s;
 *
 * myTime.seconds = 12345;
 * myTime.nseconds = 12345;
 *
 * h = myTime.seconds / 3600;
 * m = myTime.seconds / 60 - (h * 60);
 * s = myTime.seconds % 60;
 * __kprintf(NOTICE"Current time %02d:%02d:%02d, %dns.",
 *	h, m, s, myTime.nseconds);
 **/
struct timeS
{
#ifdef __cplusplus
	timeS(void)
	:
	nseconds(0), seconds(0)
	{}

	timeS(ubit32 seconds, ubit32 nseconds)
	:
	nseconds(nseconds), seconds(seconds)
	{}

	inline int operator ==(timeS &t)
	{
		return (seconds == t.seconds) && (nseconds == t.nseconds);
	}

	inline int operator >(timeS &t)
	{
		return (seconds > t.seconds)
			|| ((nseconds > t.nseconds) && (seconds >= t.seconds));
	}

	inline int operator >=(timeS &t)
	{
		return (*this == t) || (*this > t);
	}

	inline int operator <(timeS &t)
	{
		return (seconds < t.seconds)
			|| ((nseconds < t.nseconds) && (seconds <= t.seconds));
	}

	inline int operator <=(timeS &t)
	{
		return (*this == t) || (*this < t);
	}
#endif

	ubit32		nseconds, seconds;
};

struct timestampS
{
#ifdef __cplusplus
	timestampS(void)
	:
	time(0, 0), date(0)
	{}

	timestampS(ubit32 date, ubit32 sec, ubit32 nsec)
	:
	time(sec, nsec), date(date)
	{}

	timestampS(ubit32 date)
	:
	date(date)
	{}

	timestampS(ubit32 sec, ubit32 nsec)
	:
	time(sec, nsec), date(0)
	{}

	inline int operator ==(timestampS &t)
	{
		return (date == t.date) && (time == t.time);
	}

	inline int operator >(timestampS &t)
	{
		return (date > t.date)
			|| ((time > t.time) && (date >= t.date));
	}

	inline int operator >=(timestampS &t)
	{
		return (*this == t) || (*this > t);
	}

	inline int operator <(timestampS &t)
	{
		return (date < t.date)
			|| ((time < t.time) && (date <= t.date));
	}

	inline int operator <=(timestampS &t)
	{
		return (*this == t) || (*this < t);
	}
#endif

	struct timeS	time;
	date_t		date;
};

// This data type is used by Timer Streams to represent timer service requests.
struct timerObjectS
{
	enum objectTypeE { PERIODIC=1, ONESHOT } type;
	// Process and threadID to wake up when this object expires.
	processId_t		thread;
	timestampS		expirationStamp, placementStamp;
};

#endif

