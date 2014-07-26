#ifndef _CPU_POWER_OPERATIONS_H
	#define _CPU_POWER_OPERATIONS_H

	#include <__kstdlib/__ktypes.h>

#define CPUSTREAM_POWEROPS_POWER_ON		0
#define CPUSTREAM_POWEROPS_POWER_OFF		1
#define CPUSTREAM_POWEROPS_LIGHT_SLEEP		2
#define CPUSTREAM_POWEROPS_DEEP_SLEEP		3

/* This is an array of architecture specific power operation preludes.
 * See kernel/<arch-here>/cpuTrib/powerOperations.cpp.
 **/
extern status_t		(*powerOperations[])(uarch_t);

#endif

