#ifndef ___KORIENTATION_PRE_CONSTRUCT_H
	#define ___KORIENTATION_PRE_CONSTRUCT_H

	#include <kernel/common/task.h>

/**	EXPLANATION:
 * __korientationPreConstruct is a namespace containing several functions which
 * set up various structures before the kernel's constructors are run.
 *
 * The idea is that they are all initialization functions which run before the
 * constructors, so we should be neat and group them into a single namespace.
 **/
namespace __korientationPreConstruct
{
	/* Initialize the kernel process structure.
	 * Defined in /core/kernel/common/__kprocess.cpp
	 **/
	void __kprocessInit(void);

	/* Initialize the kernel orientation thread structure.
	 * Defined in /core/__kthreads/__korientationThreadInit.cpp
	 **/
	void __korientationThreadInit(void);

	/* Initialize the BSP CPU Stream object.
	 * Defined in /core/kernel/<ARCH_HERE>/cpuTrib/cpuTrib.cpp
	 **/
	void bspInit(void);
}

#endif

