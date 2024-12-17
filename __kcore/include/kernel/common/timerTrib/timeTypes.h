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
	 **/

struct sDate
{
	sDate(sbit32 year=0, ubit8 month=0, ubit8 day=0, sbit8 weekDay=0)
	:
	year(year), month(month), day(day), weekDay(weekDay)
	{}

	inline int operator ==(sDate &d)
	{
		return year == d.year && month == d.month && day == d.day;
	}

	inline int operator >(sDate &d)
	{
		return year > d.year
			|| (year == d.year && month > d.month)
			|| (year == d.year && month == d.month && day > d.day);
	}

	inline int operator >=(sDate &d)
	{
		return *this == d || *this > d;
	}

	inline int operator <(sDate &d)
	{
		return year < d.year
			|| (year == d.year && month < d.month)
			|| (year == d.year && month == d.month && day < d.day);
	}

	inline int operator <=(sDate &d)
	{
		return *this == d || *this < d;
	}

	sbit32		year;
	ubit8		month, day, weekDay;
};

/**	EXPLANATION:
 * The kernel stores time as the number of seconds elapsed since the start of
 * the current day.
 *
 * Timestamps are a combination of a sDate and a sTime, which together give
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
 * sTime	myTime = { 0, 0 };
 * ubit8	h, m, s;
 *
 * myTime.seconds = 12345;
 * myTime.nseconds = 12345;
 *
 * h = myTime.seconds / 3600;
 * m = myTime.seconds / 60 - (h * 60);
 * s = myTime.seconds % 60;
 * printf(NOTICE"Current time %02d:%02d:%02d, %dns.",
 *	h, m, s, myTime.nseconds);
 **/
struct sTime
{
#ifdef __cplusplus
	sTime(void)
	:
	nseconds(0), seconds(0)
	{}

	sTime(ubit32 seconds, ubit32 nseconds)
	:
	nseconds(nseconds), seconds(seconds)
	{}

	inline int operator ==(sTime &t)
	{
		return (seconds == t.seconds) && (nseconds == t.nseconds);
	}

	inline int operator >(sTime &t)
	{
		return (seconds > t.seconds)
			|| ((nseconds > t.nseconds) && (seconds >= t.seconds));
	}

	inline int operator >=(sTime &t)
	{
		return (*this == t) || (*this > t);
	}

	inline int operator <(sTime &t)
	{
		return (seconds < t.seconds)
			|| ((nseconds < t.nseconds) && (seconds <= t.seconds));
	}

	inline int operator <=(sTime &t)
	{
		return (*this == t) || (*this < t);
	}
#endif

	ubit32		nseconds, seconds;
};

struct sTimestamp
{
#ifdef __cplusplus
	sTimestamp(void)
	:
	time(0, 0), date(0)
	{}

	sTimestamp(ubit32 date, ubit32 sec, ubit32 nsec)
	:
	time(sec, nsec), date(date)
	{}

	sTimestamp(ubit32 date)
	:
	date(date)
	{}

	sTimestamp(ubit32 sec, ubit32 nsec)
	:
	time(sec, nsec), date(0)
	{}

	inline int operator ==(sTimestamp &t)
	{
		return (date == t.date) && (time == t.time);
	}

	inline int operator >(sTimestamp &t)
	{
		return (date > t.date)
			|| ((time > t.time) && (date >= t.date));
	}

	inline int operator >=(sTimestamp &t)
	{
		return (*this == t) || (*this > t);
	}

	inline int operator <(sTimestamp &t)
	{
		return (date < t.date)
			|| ((time < t.time) && (date <= t.date));
	}

	inline int operator <=(sTimestamp &t)
	{
		return (*this == t) || (*this < t);
	}
#endif

	sTime	time;
	sDate	date;
};

#endif

