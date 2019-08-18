
#include <tests.h>
#include <__kclasses/tests.h>
#include <__kstdlib/tests.h>

#define TESTS_CHECK_RETVAL_AND_UPDATE_RETVAL(retval, tmpretval) do \
{ \
	if ((tmpretval) != ERROR_SUCCESS) { \
		(retval) = (tmpretval); \
	} \
} while (0)

TESTS_FN_MAKE_PROTOTYPE_DEFVARS(runTests)
{
	uarch_t tot, succ, fail;
	status_t status, s;

	TESTS_VARS_INIT_DEFVARS();

	s = runTestArray(
		tests::__kstdlib::tests, &tot, &succ, &fail);
	TESTS_CHECK_RETVAL_AND_UPDATE_RETVAL(status, s);
	TESTS_VARS_DEFVARS_INC_ALL_BY(tot, succ, fail);

	s = runTestArray(
		tests::__kclasses::memBmp::tests,
		&tot, &succ, &fail);
	TESTS_CHECK_RETVAL_AND_UPDATE_RETVAL(status, s);
	TESTS_VARS_DEFVARS_INC_ALL_BY(tot, succ, fail);

	return status;
}
