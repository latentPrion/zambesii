
#include <debug.h>
#include <__ksymbols.h>
#include <__kstdlib/compiler/cxxrtl.h>
#include <__kclasses/cachePool.h>


cxxrtl::__kConstructorTester	cxxrtl::__kconstructorTester;
typedef void ctorFn(void);

status_t cxxrtl::callGlobalConstructors(void)
{
	ctorFn **ctorPtr = reinterpret_cast<ctorFn **>(&__kctorStart);
	ctorFn **ctorEnd = reinterpret_cast<ctorFn **>(&__kctorEnd);

	for (; ctorPtr < ctorEnd; ++ctorPtr)
	{
		if (*ctorPtr == NULL || (intptr_t)*ctorPtr == -1) { continue; }
		(**ctorPtr)();
	}

	return ERROR_SUCCESS;
}

status_t cxxrtl::callGlobalDestructors(void)
{
	// Cast to function pointer array and call destructors in forward order
	ctorFn **dtorPtr = reinterpret_cast<ctorFn **>(__kdtorStart);
	ctorFn **dtorEnd = reinterpret_cast<ctorFn **>(__kdtorEnd);

	for (; dtorPtr < dtorEnd; dtorPtr++)
	{
		if (*dtorPtr == NULL || (intptr_t)*dtorPtr == -1) { continue; }
		(**dtorPtr)();
	}

	return ERROR_SUCCESS;
}

extern "C" status_t cxxrtl_call_global_constructors(void);
status_t cxxrtl_call_global_constructors(void)
{
	return cxxrtl::callGlobalConstructors();
}

extern "C" status_t cxxrtl_call_global_destructors(void);
status_t cxxrtl_call_global_destructors(void)
{
	return cxxrtl::callGlobalDestructors();
}

