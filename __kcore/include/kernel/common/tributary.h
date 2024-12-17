#ifndef _TRIBUTARY_C_H
	#define _TRIBUTARY_C_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/stream.h>

/**	EXPLANATION:
 * TributaryC is a base class with singleton design used to instantiate
 * singleton kernel classes. The way the Zambesii singleton model works
 * is:
 *
 * There is a global instance of each class in an __kclassInstances.cpp
 * file. When you include a header for a kernel class
 * (e.g.: memoryTrib.h), you get a declaration of the following form:
 *	extern kernelClassName		kernelClassInstance;
 *
 * Every kernel class has a static 'getHandle()' method which can be
 * used to return the singleton kernel class instance. For the rest of
 * the class, you can use the extern declaration to reference the class
 * instance. This applies generally only for singleton kernel classes.
 **/

class Tributary
{
};

#endif

