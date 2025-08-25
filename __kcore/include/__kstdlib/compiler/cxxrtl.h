#ifndef _CXX_RTL_H
	#define _CXX_RTL_H

	#include <__kstdlib/__ktypes.h>

namespace cxxrtl {

class __kConstructorTester
{
public:
	// Magic num for success (0x5CCCE551 = "SUCCESS!")
	enum successResultE{ SUCCESS = 0x5CCCE551 };

	__kConstructorTester(void)
	: value(SUCCESS)
	{}

	sbit8 wasSuccessful(void) const
		{ return value == SUCCESS; }

private:
	uarch_t value;
};

extern __kConstructorTester __kconstructorTester;

} // namespace cxxrtl

extern "C" void *__dso_handle;
extern "C" int __cxa_atexit(void (*func)(void *), void *arg, void *dsoHandle);
extern "C" void __cxa_finalize(void *dsoHandle);

#endif
