#ifndef ___K_CPU_POWER_ON_H
	#define ___K_CPU_POWER_ON_H

	#include <config.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/thread.h>
	#include <kernel/common/cpuTrib/cpuTrib.h>

extern "C" void __kcpuPowerOnEntry(void);
extern "C" void __kcpuPowerOnMain(CpuStream *self);

extern "C" ubit8	*__kcpuPowerOnLapicVaddr;

/** EXPLANTION:
 * We preallocate the __kcpuPowerStacks pointer array in the kernel image
 * based on CONFIG_MAX_NCPUS. NB: Notice my language. I said that we
 * preallocate the pointer array and not the power stacks themselves. The
 * power stack for each CPU is stored within its CpuStream structure. Hence
 * the stacks themselves are allocated dynamically at runtime. The __kcpuPowerStacks
 * array is a a preallocated array of pointers to the power stacks.
 **/
extern "C" void		*__kcpuPowerStacks[CONFIG_MAX_NCPUS];
extern Thread	__kcpuPowerOnThread;

#endif

