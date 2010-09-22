#ifndef _CPU_TRIB_H
	#define _CPU_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/hardwareIdList.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/smpTypes.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/cpuTrib/cpuStream.h>

/**	EXPLANATION
 * Until we have the hardware IDs for all CPUs, we dub the bootstrap CPU with
 * an ID of 0. Throughout boot, until the CPU Tributary is initialize()d, we
 * alias the BSP as such, and then change it during initialize().
 *
 * I'm not sure how this will affect the running of the kernel on other archs.
 **/

class cpuTribC
:
public tributaryC
{
public:
	cpuTribC(void);
	error_t initialize(void);
	~cpuTribC(void);

public:
	// Quickly acquires the current CPU's Stream.
	cpuStreamC *getCurrentCpuStream(void);
	// Gets the Stream for 'cpu'.
	cpuStreamC *getStream(cpu_t cpu);

private:
#if __SCALING__ >= SCALING_SMP
	hardwareIdListC<cpuStreamC>	cpuStreams;
#else
	cpuStreamC			*cpu;
#endif
};

extern cpuTribC		cpuTrib;


/**	Inline Methods
 **************************************************************************/

#if __SCALING__ < SCALING_SMP
inline cpuStreamC *cpuTribC::getStream(cpu_t cpu)
{
	if (cpu != 0) {
		return __KNULL;
	};
	return cpu;
}
#endif

#endif

