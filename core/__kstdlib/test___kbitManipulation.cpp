
#include <__kstdlib/__kbitManipulation.h>


namespace tests
{

status_t bitIsSet(uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed)
{
	return ERROR_SUCCESS;
}

status_t setBit(uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed)
{
	return ERROR_SUCCESS;
}

status_t unsetBit(uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed)
{
	return ERROR_SUCCESS;
}

status_t checkForContiguousBitsAt(
	uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed
	)
{
	status_t	st;
	uarch_t		nf;
	uint32_t	bmp[4] = {
		0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
	};

	*nTotal = 6;
	*nSucceeded = *nFailed = 0;

	/**	TEST:
	 * In a completely occupied BMP, no free bits should be found.
	 **/
	st = ::checkForContiguousBitsAt(
		bmp, 0, 1, 1, 32*4, &nf);
	if (st >= 0) {
		*nFailed += 1;
		printf(ERROR"Shouldn't have found any free bits. st %d, nf %d.\n"
			"bmp is %x %x %x %x.\n",
			st, nf,
			bmp[0], bmp[1], bmp[2], bmp[3]);
	} else {
		*nSucceeded += 1;
	}

	/**	TEST:
	 * In a BMP with a free bit in a known position (64), 1 free bit should
	 * be found at the expected position.
	 **/
	bmp[2] = 0xFFFFFFFE;
	st = ::checkForContiguousBitsAt(
		bmp, 0, 1, 1, 32*4, &nf);
	if (st != 64 || nf != 1) {
		*nFailed += 1;
		printf(ERROR"Should have found 1 free bit at 64. st %d, nf %d.\n"
			"bmp is %x %x %x %x.\n",
			st, nf,
			bmp[0], bmp[1], bmp[2], bmp[3]);
	}
	else {
		*nSucceeded += 1;
	}

	/**	TEST:
	 * In a BMP with only 1 free bit at a known position, 2 free contig bits
	 * should not be found.
	 **/
	bmp[2] = 0x7FFFFFFF;
	st = ::checkForContiguousBitsAt(
		bmp, 0, 2, 2, 32*4, &nf);
	if (st >= 0) {
		*nFailed += 1;
		printf(ERROR"Shouldn't have found 2 contig bits. st %d, nf %d.\n"
			"bmp is %x %x %x %x.\n",
			st, nf,
			bmp[0], bmp[1], bmp[2], bmp[3]);
	}
	else {
		*nSucceeded += 1;
	}

	/**	TEST:
	 * In a BMP with 2 free bits *not* contiguous, at known positions,
	 * a search for 2 contig free bits should fail.
	 **/
	bmp[2] = 0xFFFFFFFA;
	st = ::checkForContiguousBitsAt(
		bmp, 0, 2, 2, 32*4, &nf);
	if (st >= 0) {
		*nFailed += 1;
		printf(ERROR"Shouldn't have found 2 contig bits. st %d, nf %d.\n"
			"bmp is %x %x %x %x.\n",
			st, nf,
			bmp[0], bmp[1], bmp[2], bmp[3]);
	}
	else {
		*nSucceeded += 1;
	}

	/**	TEST:
	 * In a BMP with 2 free bits *contiguous* at known positions, a search
	 * for 2 contig free bits should succeed.
	 **/
	bmp[2] = 0xFFFFFFF9;
	st = ::checkForContiguousBitsAt(
		bmp, 0, 2, 2, 32*4, &nf);
	if (st != 65 || nf != 2) {
		*nFailed += 1;
		printf(ERROR"Should have found 2 bits at 65. st %d, nf %d.\n"
			"bmp is %x %x %x %x.\n",
			st, nf,
			bmp[0], bmp[1], bmp[2], bmp[3]);
	}
	else {
		*nSucceeded += 1;
	}

	/**	TEST:
	 * In a BMP with free bits past the bounds given to
	 * checkForContiguousBits, the function should not search past the
	 * bounds and find those free bits.
	 **/
	bmp[2] = 0;
	st = ::checkForContiguousBitsAt(
		bmp, 0, 1, 1, 32*2, &nf);
	if (st >= 0) {
		*nFailed += 1;
		printf(ERROR"Should not have found any bits. st %d, nf %d.\n"
			"bmp is %x %x %x %x.\n",
			st, nf,
			bmp[0], bmp[1], bmp[2], bmp[3]);
	}
	else {
		*nSucceeded += 1;
	}

	return (*nFailed > 0) ? ERROR_NON_CONFORMANT : ERROR_SUCCESS;
}

}
