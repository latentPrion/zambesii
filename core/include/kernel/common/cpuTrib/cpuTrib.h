#ifndef _CPU_TRIB_H
	#define _CPU_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <__kclasses/hardwareIdList.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/smpTypes.h>
	#include <kernel/common/numaCpuBank.h>
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

#define CPUTRIB		"CPU Trib: "

class cpuTribC
:
public tributaryC
{
public:
	cpuTribC(void);
	error_t initialize(void);
	error_t initialize2(void);
	error_t wakeupMpCpus(void);
	~cpuTribC(void);

public:
	// Quickly acquires the current CPU's Stream.
	cpuStreamC *getCurrentCpuStream(void);
	// Gets the Stream for 'cpu'.
	cpuStreamC *getStream(cpu_t cpu);

	numaCpuBankC *getBank(numaBankId_t bankId);
	error_t createBank(numaBankId_t id);
	void destroyBank(numaBankId_t id);

public:
	bitmapC		availableBanks, availableCpus, onlineCpus;

private:
#if __SCALING__ >= SCALING_SMP
	hardwareIdListC		cpuStreams;
	hardwareIdListC		cpuBanks;
#else
	cpuStreamC		*cpu;
	numaCpuBankC		*cpuBank;
#endif

};

extern cpuTribC		cpuTrib;


/**	Inline Methods
 **************************************************************************/

#if __SCALING__ < SCALING_SMP
inline cpuStreamC *cpuTribC::getStream(cpu_t)
{
	return cpu;
}

inline numaCpuBankC *cpuTribC::getBank(numaBankId_t id)
{
	return cpuBank;
}
#else
inline cpuStreamC *cpuTribC::getStream(cpu_t cpu)
{
	/* This should be okay for now. We can reshuffle the pointers when we
	 * have the hardware IDs of the CPUs.
	 **/
	return static_cast<cpuStreamC *>( cpuStreams.getItem(cpu) );
}

inline numaCpuBankC *cpuTribC::getBank(numaBankId_t id)
{
	return static_cast<numaCpuBankC *>( cpuBanks.getItem(id) );
}
#endif

#endif

