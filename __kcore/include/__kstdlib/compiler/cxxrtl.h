#ifndef _CXX_RTL_H
	#define _CXX_RTL_H

	#include <__kstdlib/__ktypes.h>

namespace cxxrtl
{
	status_t callGlobalConstructors(void);
	status_t callGlobalDestructors(void);
}

#endif

