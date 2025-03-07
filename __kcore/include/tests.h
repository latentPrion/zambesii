#ifndef _TESTS_H
	#define _TESTS_H

	#include <__kstdlib/__ktypes.h>

#define TESTS_VARS_INIT(totvarp, succvarp, failvarp) do \
{ \
	*(totvarp) = *(succvarp) = *(failvarp) = 0; \
} while (0)

#define TESTS_VARS_INIT_DEFAULT_VARS(ntests) \
	TESTS_VARS_INIT(nTotal, nSucceeded, nFailed)

#define SUCCEEDED(totvarp, succvarp) do \
{ \
	*(totvarp) += 1; \
	*(succvarp) += 1; \
} while (0)

#define SUCCEEDED_DEFAULT_VARS()	SUCCEEDED(nTotal, nSucceeded)

#define FAILED(totvarp, failvarp, fmtstr, ...) do \
{ \
	*(totvarp) += 1; \
	*(failvarp) += 1; \
	printf(ERROR"test %s:" fmtstr, __FUNCTION__,##__VA_ARGS__); \
} while (0)

#define FAILED_DEFAULT_VARS0(fmtstr) do \
{ \
	*(nTotal) += 1; \
	*(nFailed) += 1; \
	printf(ERROR"test %s:" fmtstr, __FUNCTION__); \
} while (0)

#define FAILED_DEFAULT_VARS(fmtstr, ...) do \
{ \
	*(nTotal) += 1; \
	*(nFailed) += 1; \
	printf(ERROR"test %s:" fmtstr, __FUNCTION__,##__VA_ARGS__); \
} while (0)

#define TESTS_RETURN(failvarp) \
	((*(failvarp) > 0) ? ERROR_NON_CONFORMANT : ERROR_SUCCESS)

#define TESTS_RETURN_DEFAULT_VARS()		TESTS_RETURN(nFailed)

typedef status_t (testFn)(
	uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed);

#define TESTS_FN_MK_SIGNATURE_DEFAULT_VARS() \
	uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed

#define TESTS_FN_MAKE_PROTOTYPE_DEFAULT_VARS(fn_name) \
	status_t fn_name(TESTS_FN_MK_SIGNATURE_DEFAULT_VARS())

#define TESTS_VARS_DEFAULT_VARS_INC_ALL_BY(totval, succval, failval) do \
{ \
	*nTotal += (totval); \
	*nSucceeded += (succval); \
	*nFailed += (failval); \
} while (0)

inline status_t runTestArray(testFn *tests[], TESTS_FN_MK_SIGNATURE_DEFAULT_VARS())
{
	testFn		*curr;
	status_t	status=ERROR_SUCCESS;
	int		i;

	TESTS_VARS_INIT_DEFAULT_VARS();

	for (i=0, curr=tests[i]; curr != NULL; curr=tests[++i]) {
		uarch_t		tot, succ, fail;
		status_t	s;

		s = curr(&tot, &succ, &fail);
		if (s != ERROR_SUCCESS) {
			status = s;
		}

		TESTS_VARS_DEFAULT_VARS_INC_ALL_BY(tot, succ, fail);
	}

	return status;
}

TESTS_FN_MAKE_PROTOTYPE_DEFAULT_VARS(runTests);

#endif
