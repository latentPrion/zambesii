#ifndef _TIMER_TRIBUTARY_TIME_TYPES_H
	#define _TIMER_TRIBUTARY_TIME_TYPES_H

	#include <__kstdlib/__ktypes.h>

// ubit32 foo = TIME_TO_U32(myTime);
#define TIME_TO_U32(t)			(*(ubit32 *)&t)
// TIMEU32_TO_TIME(myTime,13,0,0,0);
#define TIMEU32_TO_TIME(t,h,m,s,_ms)			\
	t.hour = h; \
	t.min = m; \
	t.sec = s; \
	t.ms = _ms

// time_t myTime = TIME_INIT(13,0,0,0);
#define TIME_INIT(h,m,s,ms)		{ms,s,m,h}

// ubit32 foo = DATE_TO_U32(myDate);
#define DATE_TO_U32(d)			(*(ubit32 *)&d)
// DATEU32_TO_DATE(myDate,2009,10,11);
#define DATEU32_TO_DATE(dt,y,m,d)			\
	dt.year = y; \
	dt.month = m; \
	dt.day = d

// date_t myDate = DATE_INIT(2009,10,11);
#define DATE_INIT(y,m,d)		{y,m,d}

typedef struct timeS
{
	ubit8	ms;
	ubit8	sec;
	ubit8	min;
	ubit8	hour;
} time_t;

typedef struct dateS
{
	ubit16	year;
	ubit8	month;
	ubit8	day;
} date_t;

// mstime_t & nstime_t are only guaranteed to hold up to the arch's word size.
typedef uarch_t	mstime_t;
typedef uarch_t	nstime_t;

#endif

