#ifndef ___K_CPU_POWER_ON_H
	#define ___K_CPU_POWER_ON_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/task.h>
	#include <kernel/common/cpuTrib/cpuTrib.h>

struct __kcpuPowerOnBlockS
{
	void		*sleepStack;
	cpuStreamC	*cpuStream;
	waitLockC	lock;
};


extern "C" void __kcpuPowerOnEntry(void);
extern "C" void __kcpuPowerOnHll(void);
void __kcpuPowerOnMain(cpuStreamC *self);

extern "C" struct __kcpuPowerOnBlockS	__kcpuPowerOnBlock;
extern taskC __kcpuPowerOnThread;

#endif

