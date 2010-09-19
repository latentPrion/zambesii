#ifndef _CPU_STREAM_H
	#define _CPU_STREAM_H

	#include <arch/tlbControl.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/task.h>
	#include <kernel/common/smpTypes.h>
	#include <kernel/common/numaTypes.h>
	#include <kernel/common/cpuFeatures.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

#define CPUSTREAM_INIT_MAGIC		0xC101101C

class cpuStreamC
:
public streamC
{
public:
	cpuStreamC(void);
	error_t initialize(void);
	~cpuStreamC(void);

public:
	// Do *NOT* move currentTask from where it is.
	taskS		*currentTask;
	cpu_t		cpuId;
	numaBankId_t	bankId;
	cpuFeaturesS	cpuFeatures;
	uarch_t		initMagic;
	sharedResourceGroupC<waitLockC, uarch_t>	nTasks;
	// Used to tell the CPU whether to fetch from the bank or the local Q.
	ubit8		schedFlipFlop;
};

// The hardcoded stream for the BSP CPU.
extern cpuStreamC	bspCpu;

#endif

