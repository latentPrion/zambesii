#ifndef _CXX_RTL_H
	#define _CXX_RTL_H

	#include <__kstdlib/__ktypes.h>

namespace cxxrtl {

status_t callGlobalConstructors(void);
status_t callGlobalDestructors(void);

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

#endif
