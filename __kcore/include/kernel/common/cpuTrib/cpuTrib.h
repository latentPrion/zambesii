#ifndef _CPU_TRIB_H
	#define _CPU_TRIB_H

	#include <scaling.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <__kclasses/hardwareIdList.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/smpTypes.h>
	#include <kernel/common/numaCpuBank.h>
	#include <kernel/common/waitLock.h>
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
 * In Zambesii, as long as a CPU physically exists and is plugged in on the
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
 *	   -> CpuStream->taskStream->Initialize(): Prepare the scheduler object
 *	      to be used.
 *	   -> CpuStream->taskStream->attach(): Cause the CPU to advertise itself
 *	      as a candidate for new threads to be scheduled to and for load
 *	      balancing.
 **/

#define CPUTRIB		"CPU Trib: "

#define CPUTRIB___KUPDATEAFFINITY_ADD			0
#define CPUTRIB___KUPDATEAFFINITY_REMOVE		1

class CpuTrib
:
public Tributary
{
public:
	CpuTrib(void);
	error_t initialize(void);
	error_t loadBspInformation(void);
	error_t initializeAllCpus(void);

#if __SCALING__ >= SCALING_CC_NUMA
	error_t numaInit(void);
#endif
#if __SCALING__ == SCALING_SMP
	error_t smpInit(void);
#endif
	error_t uniProcessorInit(void);
	~CpuTrib(void);

public:
#if __SCALING__ >= SCALING_CC_NUMA
	error_t spawnStream(numaBankId_t bid, cpu_t cid, ubit32 cpuAcpiId);
#else
	error_t spawnStream(cpu_t cid, ubit32 cpuAcpiId);
#endif
	void destroyStream(cpu_t cpuId);
	// Gets the Stream for 'cpu'.
	CpuStream *getStream(cpu_t cpu);

	// Quickly acquires the current CPU's Stream.
	CpuStream *getCurrentCpuStream(void);

#if __SCALING__ >= SCALING_CC_NUMA
	NumaCpuBank *getBank(numaBankId_t bankId);
	error_t createBank(numaBankId_t id);
	void destroyBank(numaBankId_t id);
#endif

	sarch_t usingChipsetSmpMode(void) { return _usingChipsetSmpMode; };

private:
	error_t __kupdateAffinity(cpu_t cid, ubit8 action);
	error_t displayUpOperationOnMpBuildMessage(void);
#if __SCALING__ >= SCALING_CC_NUMA
	error_t newCpuNotification(numaBankId_t bid, cpu_t cid, ubit32 acpiId);
	error_t bootCpuNotification(
		numaBankId_t bid, cpu_t cid, ubit32 acpiId);

	void bootParseNumaMap(struct sZkcmNumaMap *numaMap);
	void bootParseNumaMap(
		struct sZkcmNumaMap *numaMap, struct sZkcmSmpMap *smpMap);

	void bootConfirmNumaCpusBooted(struct sZkcmNumaMap *numaMap);
	void bootConfirmNumaCpusBooted(
		struct sZkcmNumaMap *numaMap, struct sZkcmSmpMap *smpMap);
#elif __SCALING__ == SCALING_SMP
	error_t newCpuNotification(cpu_t cid, ubit32 acpiId);
	error_t bootCpuNotification(cpu_t cid, ubit32 acpiId);
#endif
#if __SCALING__ >= SCALING_SMP
	void bootParseSmpMap(struct sZkcmSmpMap *smpMap);
	void bootConfirmSmpCpusBooted(struct sZkcmSmpMap *smpMap);
#endif

public:
#if __SCALING__ >= SCALING_CC_NUMA
	Bitmap			availableBanks;
#endif
#if __SCALING__ >= SCALING_SMP
	Bitmap			availableCpus, onlineCpus;
	ubit8			_usingChipsetSmpMode;
#endif

private:
#if __SCALING__ >= SCALING_CC_NUMA
	HardwareIdList		cpuBanks;
#endif
#if __SCALING__ >= SCALING_SMP
	HardwareIdList		cpuStreams;
#else
	CpuStream		*cpu;
#endif
};

extern CpuTrib		cpuTrib;


/**	Inline Methods
 **************************************************************************/

#if __SCALING__ >= SCALING_CC_NUMA
inline NumaCpuBank *CpuTrib::getBank(numaBankId_t id)
{
	return static_cast<NumaCpuBank *>( cpuBanks.getItem(id) );
}
#endif

#if __SCALING__ >= SCALING_SMP
inline CpuStream *CpuTrib::getStream(cpu_t cpu)
{
	/* This should be okay for now. We can reshuffle the pointers when we
	 * have the hardware IDs of the CPUs.
	 **/
	return static_cast<CpuStream *>( cpuStreams.getItem(cpu) );
}
#else
inline CpuStream *CpuTrib::getStream(cpu_t)
{
	return cpu;
}
#endif

#endif

