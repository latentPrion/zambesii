
#include <debug.h>
#include <__ksymbols.h>
#include <__kstdlib/compiler/cxxrtl.h>
#include <__kclasses/cachePool.h>


cxxrtl::__kConstructorTester	cxxrtl::__kconstructorTester;
typedef void ctorFn(void);

namespace cxxrtl {

status_t callGlobalConstructorsBounded(void *sectionStart, void *sectionEnd)
{
	ctorFn **ctorPtr = reinterpret_cast<ctorFn **>(sectionStart);
	ctorFn **ctorEnd = reinterpret_cast<ctorFn **>(sectionEnd);

	for (; ctorPtr < ctorEnd; ++ctorPtr)
	{
		if (*ctorPtr == NULL || (intptr_t)*ctorPtr == -1) { continue; }
		(**ctorPtr)();
	}

	return ERROR_SUCCESS;
}

status_t callGlobalConstructorsNullTerminated(void *sectionStart)
{
	ctorFn **ctorPtr = reinterpret_cast<ctorFn **>(sectionStart);
	
	for (; *ctorPtr != NULL; ++ctorPtr)
	{
		if (*ctorPtr == NULL || (intptr_t)*ctorPtr == -1) { continue; }
		(**ctorPtr)();
	}

	return ERROR_SUCCESS;
}

status_t callGlobalConstructorsPossiblyPrefixedCountNullTerminated(
	void *sectionStart
	)
{
	ctorFn **ctorPtr = reinterpret_cast<ctorFn **>(sectionStart);
	size_t nCtors = (size_t)ctorPtr[0];

	// Use GCC's logic for possibly-prefixed count, using local naming style
	if (nCtors == (size_t)-1)
	{
		for (nCtors = 0;
			ctorPtr[nCtors + 1] != NULL
				&& (intptr_t)ctorPtr[nCtors + 1] != -1;
			nCtors++)
		{}
	}

	for (unsigned i = nCtors; i >= 1; i--)
		{ ctorPtr[i](); }

	return ERROR_SUCCESS;
}

namespace icxxabi {

status_t callGlobalConstructors(void)
{
	return callGlobalConstructorsBounded(&__kctorStart, &__kctorEnd);
}

} // namespace icxxabi

namespace ctors_section {

status_t callGlobalConstructors(void)
{
	return callGlobalConstructorsPossiblyPrefixedCountNullTerminated(
		&__kctorStart);
}

}

namespace init_section {

status_t callGlobalConstructors(void)
{
	/* Implementation based on __do_global_ctors_aux GCC source code.
	 * First call the ctors_section implementation. Then, in accordance with
	 * the Itanium C++ ABI, register the __cxa_finalize function to be
	 * called when the program exits.
	 **/
	callGlobalConstructorsBounded(&__kinitCtorStart, &__kinitCtorEnd);
	__cxa_atexit(__cxa_finalize, NULL, &__dso_handle);
	return ERROR_SUCCESS;
}

} // namespace init_section

} // namespace cxxrtl

extern "C" status_t cxxrtl_call_global_constructors(void);
status_t cxxrtl_call_global_constructors(void)
{
	return cxxrtl::icxxabi::callGlobalConstructors();
}

extern "C" status_t cxxrtl_call_global_destructors(void);
status_t cxxrtl_call_global_destructors(void)
{
	return 0;
}
