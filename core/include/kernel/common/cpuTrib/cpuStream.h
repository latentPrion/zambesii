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
	#include <kernel/common/taskTrib/taskStream.h>

#define CPUSTREAM_INIT_MAGIC		0xC101101C

class cpuStreamC
:
public streamC
{
public:
	cpuStreamC(numaBankId_t bid, cpu_t id);
	error_t initialize(void);
	~cpuStreamC(void);

public:
	// Do *NOT* move currentTask from where it is.
	taskC		*currentTask;
	cpu_t		cpuId;
	numaBankId_t	bankId;
	cpuFeaturesS	cpuFeatures;
	uarch_t		initMagic;
	// Per CPU scheduler.
	taskStreamC	scheduler;
};

// The hardcoded stream for the BSP CPU.
extern cpuStreamC	bspCpu;

#endif

