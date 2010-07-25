#ifndef _CPU_STREAM_H
	#define _CPU_STREAM_H

	#include <arch/tlbControl.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/task.h>
	#include <kernel/common/smpTypes.h>
	#include <kernel/common/numaTypes.h>
	#include <kernel/common/cpuFeatures.h>

class cpuStreamC
:
public streamC
{
public:
	cpuStreamC(void);
	cpuStreamC(cpu_t id);
	~cpuStreamC(void);

public:
	cpu_t		cpuId;
	numaBankId_t	bankId;
	taskS		*currentTask;
	cpuFeaturesS	cpuFeatures;
};

// The hardcoded stream for the BSP CPU.
extern cpuStreamC	bspCpu;

#endif

