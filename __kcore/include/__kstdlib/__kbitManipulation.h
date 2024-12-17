#ifndef __KBIT_MANIPULATION_H
	#define __KBIT_MANIPULATION_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>
#include <__kclasses/debugPipe.h>

#define __KBIT_ON		1
#define __KBIT_OFF		0

#define __KBIT_NBITS_IN(var)		(sizeof((var)) * __BITS_PER_BYTE__ )

#define __KBIT_SET(var,bit)		((var) |= (1 << (bit)))
#define __KBIT_UNSET(var,bit)		((var) &= ~(1 << (bit)))
#define __KBIT_TEST(var,bit)		(!!((var) & (1 << (bit))))

//Only use these for bitmaps which are arrays of an integral type.
#define BITMAP_WHICH_INDEX(__b,__bit)			\
	( (__bit) / __KBIT_NBITS_IN(__b) )

#define BITMAP_WHICH_BIT(__b,__bit)			\
	( (__bit) % __KBIT_NBITS_IN(__b) )

template <class wordtype>
inline sbit8 bitIsSet(wordtype *const bmp, uarch_t bit)
{
	return __KBIT_TEST(
		bmp[BITMAP_WHICH_INDEX(*bmp, bit)],
		BITMAP_WHICH_BIT(*bmp, bit));
}

template <class wordtype>
inline void setBit(wordtype *const bmp, uarch_t bit)
{
	__KBIT_SET(
		bmp[BITMAP_WHICH_INDEX(*bmp, bit)],
		BITMAP_WHICH_BIT(*bmp, bit));
}

template <class wordtype>
inline void unsetBit(wordtype *const bmp, uarch_t bit)
{
	__KBIT_UNSET(
		bmp[BITMAP_WHICH_INDEX(*bmp, bit)],
		BITMAP_WHICH_BIT(*bmp, bit));
}

template <class wordtype>
static status_t checkForContiguousBitsAt(
	wordtype *const bmp, const uarch_t startBit,
	const uarch_t minNFrames, const uarch_t maxNFrames,
	const uarch_t nValidBits, uarch_t *const nFound
	)
{
	/**	EXPLANATION:
	 * Checks for a stretch of at least minNFrames and at most maxNFrames
	 * contiguously situated.
	 *
	 * Returns the start index of such a range if one is found.
	 **/

	if (maxNFrames < minNFrames) { return ERROR_INVALID_ARG_VAL; };

	*nFound = 0;
	for (uarch_t i=startBit; i<startBit + nValidBits; i++)
	{
		if (bitIsSet(bmp, i))
		{
			if (*nFound == 0) { continue; };

			/* If we were already counting a stretch of contiguous
			 * bits of at least minNFrames, we have already
			 * succeeded. We can return successfully now.
			 **/
			if (*nFound >= minNFrames) {
				return i - *nFound;
			}
			else
			{
				/* We were on a contiguous stretch, but it
				 * wasn't at least minNFrames bits long.
				 **/
				*nFound = 0;
				continue;
			};
		};

		*nFound += 1;
		/**	EXPLANATION:
		 * If we already have minNFrames, try to keep going until we
		 * find maxNFrames.
		 *
		 * If we ever arrive at maxNFrames, return immediately.
		 **/
		if (*nFound >= minNFrames) {
			if (*nFound == maxNFrames) {
				return i - (*nFound - 1);
			};
		};
	};

	// Reached the end of the valid bits and didn't find minNFrames.
	return ERROR_NOT_FOUND;
}

namespace tests
{
status_t bitIsSet(uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed);
status_t setBit(uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed);
status_t unsetBit(uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed);
status_t checkForContiguousBitsAt(
	uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed);
}

#endif
