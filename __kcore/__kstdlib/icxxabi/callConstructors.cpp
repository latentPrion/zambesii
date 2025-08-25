
#include <debug.h>
#include <__ksymbols.h>
#include <arch/memory.h>
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
		if ((intptr_t)*ctorPtr == -1) { continue; }
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

	if (nCtors == (size_t)-1)
	{
		/**	EXPLANATION:
		 * Use GCC's logic for possibly-prefixed count. This is lifted
		 * directly from the GCC source code in `DO_GLOBAL_CTORS_BODY`,
		 * but modified to also check for entries whose value is -1.
		 */
		for (nCtors = 0;
			ctorPtr[nCtors + 1] != NULL
				&& (intptr_t)ctorPtr[nCtors + 1] != -1;
			nCtors++)
		{}
	}
	else if (!ARCH_MEMORY_VADDR_IS_IN___KVADDRSPACE(nCtors))
	{
		/**	EXPLANATION:
		 * If it's not a vaddr in the kernel's high vaddrspace, it's
		 * a count of constructors. Nothing else to do in here. This
		 * branch is for documentation only.
		 *
		 * NB: This branch also handles the case where nCtors is 0.
		 */
	}
	else
	{
		/**	EXPLANATION:
		 * If it's a vaddr in the kernel's high vaddrspace, that means
		 * that the first entry in the section is a
		 * function pointer. That means it's a NULL-terminated array
		 * of function pointers, potentially with -1s mixed in.
		 * We need to find the number of valid constructors.
		 */
		nCtors = 0;
		while (ctorPtr[nCtors] != NULL
			&& (intptr_t)ctorPtr[nCtors] != -1)
		{
			nCtors++;
		}
	}

	for (unsigned i = nCtors; i >= 1; i--)
	{
		if ((intptr_t)ctorPtr[i] == -1) { continue; }
		ctorPtr[i]();
	}

	return ERROR_SUCCESS;
}

status_t callGlobalConstructorsCallableSection(void *sectionStart)
{
	/**	EXPLANATION:
	 * The section has a function encoded into it. The section pointer is
	 * effectively a function pointer.
	 */
	ctorFn *ctorFn = reinterpret_cast<ctorFn *>(sectionStart);
	(*ctorFn)();

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
#ifdef CONFIG_CXXRTL_CTORS_CALLMETHOD_POSSIBLY_PREFIXED_COUNT_NULL_TERMINATED
	return callGlobalConstructorsPossiblyPrefixedCountNullTerminated(
		&__kctorStart);
#elif defined(CONFIG_CXXRTL_CTORS_CALLMETHOD_BOUNDED)
	return callGlobalConstructorsBounded(
		&__kctorStart,
		&__kctorEnd
	);
#elif defined(CONFIG_CXXRTL_CTORS_CALLMETHOD_NULL_TERMINATED)
	return callGlobalConstructorsNullTerminated(
		&__kctorStart);
#else
#  error "CXXRTL: No constructor call method defined."
#endif
}

namespace init_section {

status_t callGlobalConstructors(void)
{
	/**	EXPLANATION:
	 * The .init section tends to be either a bounded array of function
	 * pointers or a callable section.
	 */
#ifdef CONFIG_CXXRTL_CTORS_CALLMETHOD_BOUNDED
	return callGlobalConstructorsBounded(
		&__kinitCtorStart,
		&__kinitCtorEnd
	);

	__cxa_atexit(__cxa_finalize, NULL, &__dso_handle);
#elif defined(CONFIG_CXXRTL_CTORS_CALLMETHOD_CALLABLE_SECTION)
	return callGlobalConstructorsCallableSection(
		&__kinitCtorStart
	);
	// The provided function prolly calls __cxa_atexit itself.
#else
#  error "CXXRTL: No constructor call method defined."
#endif

	return ERROR_SUCCESS;
}

} // namespace init_section

} // namespace cxxrtl

extern "C" status_t cxxrtl_call_global_constructors(void);
status_t cxxrtl_call_global_constructors(void)
{
#ifdef CONFIG_CXXRTL_CTORS_PACKAGING_CTORS_SECTION
	return ctors_section::callGlobalConstructors();
#elif defined(CONFIG_CXXRTL_CTORS_PACKAGING_INIT_SECTION)
	return init_section::callGlobalConstructors();
#else
#  error "CXXRTL: No constructor packaging method defined."
#endif
}

extern "C" status_t cxxrtl_call_global_destructors(void);
status_t cxxrtl_call_global_destructors(void)
{
	return 0;
}
