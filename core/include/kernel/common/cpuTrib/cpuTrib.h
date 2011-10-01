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

/**	EXPLANATION:
 * In Zambezii, as long as a CPU physically exists and is plugged in on the
 * chipset, the kernel will maintain a CPU Stream object for it. The sequence
 * of events upon discovering a new CPU is:
 *
 * New CPU detected:
 *	-> Spawn CPU Stream object;
 *	   -> Perform object construction and basic state initialization.
 *	   -> Initialize(): Allocate any dynamic memory needed, prepare the
 *	      new software objct in case it will be used.
 *	-> Attempt to wake up the new CPU.
 *	   -> powerControl(CPUSTREAM_POWER_ON): Get the CPU in a software
 *	      controllable state.
 *	   -> cpuInfo(CPUSTREAM_INFO_ENUMERATE): Obtain information on CPU.
 *	-> Prepare the new CPU for scheduler use.
 *	   -> cpuStream->taskStream->Initialize(): Prepare the scheduler object
 *	      to be used.
 *	   -> cpuStream->taskStream->attach(): Cause the CPU to advertise itself
 *	      as a candidate for new threads to be scheduled to and for load
 *	      balancing.
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
	~cpuTribC(void);

public:
	// Gets the Stream for 'cpu'.
	cpuStreamC *getStream(cpu_t cpu);
	error_t spawnStream(numaBankId_t bid, cpu_t cid, ubit32 cpuAcpiId);
	void destroyStream(cpu_t cpuId);

	// Quickly acquires the current CPU's Stream.
	cpuStreamC *getCurrentCpuStream(void);

	numaCpuBankC *getBank(numaBankId_t bankId);
	error_t createBank(numaBankId_t id);
	void destroyBank(numaBankId_t id);

	sarch_t usingSmpMode(void) { return usingChipsetSmpMode; };

public:
	bitmapC		availableBanks, availableCpus, onlineCpus;
	cpu_t		bspId;
	ubit8		usingChipsetSmpMode;

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

