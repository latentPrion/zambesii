#ifndef _CPU_STREAM_H
	#define _CPU_STREAM_H

	#include <arch/tlbControl.h>
	#include <arch/paging.h>
	#include <__kclasses/ptrlessList.h>
	#include <__kclasses/cachePool.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/task.h>
	#include <kernel/common/smpTypes.h>
	#include <kernel/common/numaTypes.h>
	#include <kernel/common/cpuTrib/cpuFeatures.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/taskTrib/taskStream.h>

// "CPU OK" :/
#define CPUSTREAM_MAGIC			0xC1D1101C
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

#define CPUMSG				"CPU Messager "

#define CPUMSGR_STATUS_NORMAL		0
#define CPUMSGR_STATUS_PROCESSING	1

#define CPUMESSAGE_TYPE_TLBFLUSH	0x0

class cpuStreamC
:
public streamC
{
public:
#if __SCALING__ >= SCALING_CC_NUMA
	cpuStreamC(numaBankId_t bid, cpu_t id, ubit32 acpiId);
#else
	cpuStreamC(cpu_t id, ubit32 cpuAcpiId);
#endif

	void baseInit(void);
	error_t initialize(void);
	sarch_t isInitialized(void);
	~cpuStreamC(void);

	error_t initializeBspCpuLocking(void);
	error_t initializeBspCpuTaskStream(void);

public:
	status_t powerControl(ubit16 command, uarch_t flags);
	status_t enumerate(void);
	cpuFeaturesS *getCpuFeatureBlock(void);

private:
#if __SCALING__ >= SCALING_SMP
	class interCpuMessagerC
	{
	private: struct messageS;
	public:
		interCpuMessagerC(cpuStreamC *parent);
		error_t initialize(void);

	public:
		error_t flushTlbRange(void *vaddr, uarch_t nPages);
		error_t dispatch(void);
		void setReceiveStateReady(void);

	private:
		void set(messageS *msg, ubit8 type,
			uarch_t val0=0, uarch_t val1=0,
			uarch_t val2=0, uarch_t val3=0);

	private:
		struct messageS
		{
			ptrlessListC<messageS>::headerS	listHeader;
			volatile ubit8			type;
			volatile uarch_t		val0;
			volatile uarch_t		val1;
			volatile uarch_t		val2;
			volatile uarch_t		val3;
		};
		ptrlessListC<messageS>		messageQueue;
		slamCacheC			*cache;
		sharedResourceGroupC<waitLockC, uarch_t> statusFlag;
		cpuStreamC	*parent;
	};
#endif

public:
	cpu_t			cpuId;
	ubit32			cpuAcpiId;
#if __SCALING__ >= SCALING_CC_NUMA
	numaBankId_t		bankId;
#endif
	cpuFeaturesS		cpuFeatures;
	// Per CPU scheduler.
	taskStreamC		taskStream;

	ubit32			flags, magic;
	/* Very small stack used to wake and power down CPUs.
	 * The number of elements in the array indicates how many pushes the
	 * stack can handle. Assuming each push is a native word's size,
	 * the stack can handle N pushes of the arch's word size.
	 **/
	ubit8			sleepStack[PAGING_BASE_SIZE];
#if __SCALING__ >= SCALING_SMP
	interCpuMessagerC	interCpuMessager;
#endif
};

// The hardcoded stream for the BSP CPU.
extern cpuStreamC	bspCpu;

#endif

