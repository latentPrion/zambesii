#ifndef _CPU_TRIB_H
	#define _CPU_TRIB_H

	#include <__kstdlib/__ktypes.h>
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
	/* The idea behind this is that at boot, since we have no assurance of
	 * the BSP being given the ID of CPU#0 in hardware, yet we have the BSP
	 * pre-allocated struct at index 0 in the CPU list, we have an offset
	 * field to add to the cpuids passed to the CPU Tributary.
	 *
	 * This should be good enough to compensate for any platform's naming
	 * of CPUs, however inconsistent that platform's firmware may be.
	 **/
	sarch_t		offset;
	// Quickly acquires the current CPU's Stream.
	cpuStreamC *getCurrentCpuStream(void);
	// Gets the Stream for 'cpu'.
	cpuStreamC *getStream(cpu_t cpu);
};

/* We globalise the CPU list so it can be accessed from ASM. This is critical
 * to things like IRQ entry, for example. We must be able to get the current
 * CPU's stack pointer, etc immediately at no cost.
 **/
extern sharedResourceGroupC<multipleReaderLockC, cpuStreamC **> cpuStreams;

extern cpuTribC		cpuTrib;


/**	Inline Methods
 **************************************************************************/

#if __SCALING__ < SCALING_SMP
inline cpuStreamC *cpuTribC::getCurrentCpuStream(void)
{
	// On a Uni processor build, the current cpu is always cpu0.
	return &(*cpuStreams.rsrc)[0];
}

inline cpuStreamC *cpuTribC::getStream(cpu_t cpu)
{
	if (cpu != 0) {
		return __KNULL;
	};
	return &(*cpuStreams.rsrc)[0];
}
#endif

#endif

