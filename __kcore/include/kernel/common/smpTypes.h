#ifndef _SMP_TYPES_H
	#define _SMP_TYPES_H

	#include <__kstdlib/__ktypes.h>

#define CPUID_INVALID		((cpu_t)~0)

typedef uarch_t		cpu_t;

enum bspPlugTypeE
{
	BSP_PLUGTYPE_NOTBSP=0,
	BSP_PLUGTYPE_FIRSTPLUG,
	BSP_PLUGTYPE_HOTPLUG
};

#ifdef __cplusplus
	/* There are C files which include this file, so any C++ code in here
	 * should be properly guarded.
	 **/
#endif

#endif

