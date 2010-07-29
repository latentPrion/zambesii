#ifndef _CLOCK_T_H
	#define _CLOCK_T_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * Must be implemented as an unsigned integral type (in behaviour) with the
 * following operators:
 *	+ += = == > < >= <= != ++
 *
 **/

#ifdef __32_BIT__
#define CLOCK_UARCH_MAX		0xFFFFFFFF
#elif defined(__64_BIT__)
#define CLOCK_UARCH_MAX		0xFFFFFFFFFFFFFFFF
#endif

class clock_t
{
public:
	clock_t(void)
	:
	low(0), high(0)
	{}

	clock_t(uarch_t high, uarch_t low)
	:
	low(high), high(low)
	{}

public:
	uarch_t operator+(uarch_t i);
	clock_t &operator+=(uarch_t i);
//	clock_t &operator=(clock_t i);
	// Only allow comparison with other clocks.
	int operator==(clock_t c);
	int operator>(clock_t c);
	int operator<(clock_t c);
	int operator>=(clock_t c);
	int operator<=(clock_t c);
	int operator!=(clock_t c);
	clock_t &operator++(int);

private:
	uarch_t		low;
	uarch_t		high;
};


/* Inline methods
 ******************************************************************************/

uarch_t clock_t::operator+(uarch_t i)
{
	return low+i;
}

clock_t &clock_t::operator+=(uarch_t i)
{
	/* I'm not sure whether or not there are CPUs that will fault on an
	 * integer overflow. So this is the safest, most accurate way to handle
	 * a 'bignum' without allowing any overflows.
	 **/
	if ((CLOCK_UARCH_MAX - i) < low)
	{
		high++;
		i -= (CLOCK_UARCH_MAX - low);
		low = 0;
		low += i-1;
		return *this;
	};
	low += i;
	return *this;
}

int clock_t::operator==(clock_t c)
{
	return ((c.low == low) && (c.high == high));
}

int clock_t::operator>(clock_t c)
{
	return ((high > c.high) || (high == c.high && low > c.low));
}

int clock_t::operator<(clock_t c)
{
	return ((high < c.high) || (high == c.high && low < c.low));
}

int clock_t::operator>=(clock_t c)
{
	return ((high >= c.high) || (high == c.high && low >= c.low));
}

int clock_t::operator<=(clock_t c)
{
	return ((high <= c.high) || (high == c.high && low <= c.low));
}

int clock_t::operator!=(clock_t c)
{
	return (high != c.high || low != c.low);
}

clock_t &clock_t::operator++(int)
{
	if (low == CLOCK_UARCH_MAX)
	{
		high++;
		low = 0;
	}
	else {
		low++;
	};
	return *this;
}

#endif

