#ifndef ___K_CPU_POWER_ON_H
	#define ___K_CPU_POWER_ON_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/thread.h>
	#include <kernel/common/cpuTrib/cpuTrib.h>

extern "C" void __kcpuPowerOnEntry(void);
extern "C" void __kcpuPowerOnMain(CpuStream *self);

extern "C" ubit8	*__kcpuPowerOnLapicVaddr;
extern "C" void		**__kcpuPowerStacks;
extern uarch_t		__kcpuPowerStacksLength;
extern WaitLock	__kcpuPowerStacksLock;
extern Thread	__kcpuPowerOnThread;

#endif

