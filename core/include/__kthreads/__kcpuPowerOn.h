#ifndef ___K_CPU_POWER_ON_H
	#define ___K_CPU_POWER_ON_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/thread.h>
	#include <kernel/common/cpuTrib/cpuTrib.h>

extern "C" void __kcpuPowerOnEntry(void);
extern "C" void __kcpuPowerOnMain(cpuStreamC *self);

extern "C" ubit8	*__kcpuPowerOnLapicVaddr;
extern "C" void		**__kcpuPowerOnSleepStacks;
extern uarch_t		__kcpuPowerOnSleepStacksLength;
extern waitLockC	__kcpuPowerOnSleepStacksLock;
extern PerCpuThread	__kcpuPowerOnThread;

#endif

