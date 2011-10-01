#ifndef _CPU_STREAM_H
	#define _CPU_STREAM_H

	#include <arch/tlbControl.h>
	#include <arch/paging.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/task.h>
	#include <kernel/common/smpTypes.h>
	#include <kernel/common/numaTypes.h>
	#include <kernel/common/cpuTrib/cpuFeatures.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/cpuTrib/cpuStreamArchBlock.h>
	#include <kernel/common/taskTrib/taskStream.h>

// "CPU OK" :/
#define CPUSTREAM			"CPU Stream "

#define CPUSTREAM_FLAGS_INITIALIZED	(1<<0)
#define CPUSTREAM_FLAGS_ENUMERATED	(1<<1)
#define CPUSTREAM_FLAGS_BSP		(1<<2)

#define CPUSTREAM_POWER_ON		0x0
#define CPUSTREAM_POWER_CHECK		0x1
#define CPUSTREAM_POWER_LIGHTSLEEP	0x2
#define CPUSTREAM_POWER_DEEPSLEEP	0x3
#define CPUSTREAM_POWER_OFF		0x4

#define CPUSTREAM_ENUMERATE_UNSUPPORTED_CPU	0x1
#define CPUSTREAM_ENUMERATE_CPU_MODEL_UNCLEAR	0x2
#define CPUSTREAM_ENUMERATE_CPU_MODEL_UNKNOWN	0x3

class cpuStreamC
:
public streamC
{
public:
	cpuStreamC(numaBankId_t bid, cpu_t id, ubit32 acpiId);

	void baseInit(void);
	error_t initialize(void);
	sarch_t isInitialized(void);
	~cpuStreamC(void);

public:
	status_t powerControl(ubit16 command, uarch_t flags);
	status_t enumerate(void);
	cpuFeaturesS *getCpuFeatureBlock(void);

public:
	// Do *NOT* move currentTask from where it is.
	taskC			*currentTask;
	cpu_t			cpuId;
	ubit32			cpuAcpiId;
	numaBankId_t		bankId;
	cpuFeaturesS		cpuFeatures;
	// Per CPU scheduler.
	taskStreamC		scheduler;

	ubit32			flags;
	/* Very small stack used to wake and power down CPUs.
	 * The number of elements in the array indicates how many pushes the
	 * stack can handle. Assuming each push is a native word's size,
	 * the stack can handle N pushes of the arch's word size.
	 **/
	ubit8			sleepStack[PAGING_BASE_SIZE];
	cpuStreamArchBlockS	archBlock;
};

// The hardcoded stream for the BSP CPU.
extern cpuStreamC	bspCpu;

#endif

