#ifndef _ASSERT_H
	#define _ASSERT_H

#define ASSERT_QUOTE(x)		#x
#define ASSERT_DUAL_QUOTE(x)	ASSERT_QUOTE(x)

#define assert_warn(e)			\
	(((e) == 0) \
		? printf(WARNING"Assertion failed: " \
			ASSERT_QUOTE(e) \
			" on line " \
			ASSERT_DUAL_QUOTE(__LINE__) \
			" of " \
			ASSERT_DUAL_QUOTE(__FILE__) \
			".\n") \
		: \
		((void)0))

#define assert_error(e)			\
	(((e) == 0) \
		? printf(ERROR"Assertion failed: " \
			ASSERT_QUOTE(e) \
			" on line " \
			ASSERT_DUAL_QUOTE(__LINE__) \
			" of " \
			ASSERT_DUAL_QUOTE(__FILE__) \
			".\n") \
		: \
		((void)0))

#define assert_fatal(e)			\
	(((e) == 0) \
		? panic(FATAL"Assertion failed: " \
			ASSERT_QUOTE(e) \
			" on line " \
			ASSERT_DUAL_QUOTE(__LINE__) \
			" of " \
			ASSERT_DUAL_QUOTE(__FILE__) \
			".\n") \
		: \
		((void)0))

#define assert(e) assert_fatal((e))

#endif

