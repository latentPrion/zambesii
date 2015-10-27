#ifndef _CPU_STREAM_H
	#define _CPU_STREAM_H

	#include <arch/tlbControl.h>
	#include <arch/paging.h>
	#include <chipset/memory.h>
	#include <__kclasses/list.h>
	#include <__kclasses/cachePool.h>
	#include <commonlibs/libx86mp/lapic.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/thread.h>
	#include <kernel/common/smpTypes.h>
	#include <kernel/common/numaTypes.h>
	#include <kernel/common/cpuTrib/cpuFeatures.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/taskTrib/taskStream.h>

#define CPUSTREAM			"CPU Stream "
#define CPUPWRMGR			"CPU PwrMgr "

#define CPUSTREAM_FLAGS_INITIALIZED	(1<<0)
#define CPUSTREAM_FLAGS_ENUMERATED	(1<<1)

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

class ProcessStream;

class CpuStream
{
friend class ProcessStream;
public:
#if __SCALING__ >= SCALING_CC_NUMA
	CpuStream(numaBankId_t bid, cpu_t id, ubit32 acpiId);
#else
	CpuStream(cpu_t id, ubit32 cpuAcpiId);
#endif

	void initializeBaseState(void);
	void initializeExceptions(void);
	error_t initialize(void);
	~CpuStream(void);

	error_t bind(void);
	void cut(void);

public:
	/**	NOTE:
	 * "highestCpuId" and "highestBankId" are defined in cpuTrib.cpp; not
	 * cpuStream.cpp.
	 **/
	static cpu_t			bspCpuId, highestCpuId;
	static numaBankId_t		bspBankId, highestBankId;
	static ubit32			bspAcpiId;
	static bspPlugTypeE		bspPlugType;
	static sCpuFeatures		baseCpuFeatures;

	status_t enumerate(void);
	sCpuFeatures *getCpuFeatureBlock(void);

	sbit8 isBspCpu(void) { return isBspCpuId(this->cpuId); }
	static sbit8 isBspCpuId(cpu_t cid)
		{ return cid == CPUID_INVALID || cid == bspCpuId; }

	static bspPlugTypeE		getBspPlugType(void)
		{ return bspPlugType; }

	static sbit8 isBspFirstPlug(void)
		{ return getBspPlugType() == BSP_PLUGTYPE_FIRSTPLUG; }

	static sbit8 isBspHotplug(void)
		{ return getBspPlugType() == BSP_PLUGTYPE_HOTPLUG; }

public:
	class PowerManager
	: public Stream<CpuStream>
	{
	public:
		enum powerStatusE {
			OFF=0, C0, C1, C2, C3,
			POWERING_ON, POWERING_ON_RETRY, POWERING_OFF,
			GOING_TO_SLEEP, WAKING,
			FAILED_BOOT };

		PowerManager(cpu_t id, CpuStream *parentStream)
		: Stream<CpuStream>(parentStream, id)
		{
			if (CpuStream::isBspCpuId(id) && CpuStream::isBspFirstPlug()) {
				powerStatus.rsrc = C0;
			} else {
				powerStatus.rsrc = OFF;
			};
		}

		error_t initialize(void) { return ERROR_SUCCESS; }

		~PowerManager(void)
			{ powerStatus.rsrc = OFF; }

		powerStatusE getPowerStatus(void)
		{
			powerStatusE	ret;
			uarch_t		rwFlags;

			powerStatus.lock.readAcquire(&rwFlags);
			ret = powerStatus.rsrc;
			powerStatus.lock.readRelease(rwFlags);

			return ret;
		}

		void setPowerStatus(powerStatusE status)
		{
			powerStatus.lock.writeAcquire();
			powerStatus.rsrc = status;
			powerStatus.lock.writeRelease();
		}

		status_t powerOn(ubit32 flags);
		status_t bootPowerOn(ubit32 flags);
		status_t halt(ubit32 flags);
		status_t sleep(ubit32 flags);
		status_t powerOff(ubit32 flags);

		void bootWaitForCpuToPowerOn(void);

	private:
		SharedResourceGroup<MultipleReaderLock, powerStatusE>
			powerStatus;
	};

private:
#if __SCALING__ >= SCALING_SMP
	class InterCpuMessager
	: public Stream<CpuStream>
	{
	private: struct sMessage;
	public:
		InterCpuMessager(cpu_t id, CpuStream *parent);
		error_t initialize(void);

		error_t bind(void);
		void cut(void);

	public:
		error_t flushTlbRange(void *vaddr, uarch_t nPages);
		error_t dispatch(void);

	private:
		enum statusE {
			NOT_TAKING_REQUESTS=0, NOT_PROCESSING, PROCESSING };

		void set(
			sMessage *msg, ubit8 type,
			uarch_t val0=0, uarch_t val1=0,
			uarch_t val2=0, uarch_t val3=0);

		statusE getStatus(void)
		{
			statusE		ret;

			statusFlag.lock.acquire();
			ret = statusFlag.rsrc;
			statusFlag.lock.release();

			return ret;
		}

		void setStatus(statusE status)
		{
			statusFlag.lock.acquire();
			statusFlag.rsrc = status;
			statusFlag.lock.release();
		}

	private:
		struct sMessage
		{
			List<sMessage>::sHeader	listHeader;
			volatile ubit8			type;
			volatile uarch_t		val0;
			volatile uarch_t		val1;
			volatile uarch_t		val2;
			volatile uarch_t		val3;
		};
		List<sMessage>				messageQueue;
		SlamCache				*cache;
		SharedResourceGroup<WaitLock, statusE>	statusFlag;
	};
#endif

public:
	cpu_t			cpuId;
	ubit32			cpuAcpiId;
#if __SCALING__ >= SCALING_CC_NUMA
	numaBankId_t		bankId;
#endif
	sCpuFeatures		cpuFeatures;
	// Per CPU scheduler.
	TaskStream		taskStream;

	ubit32			flags;
	// Small stack used for scheduler task switching.
	ubit8			schedStack[PAGING_BASE_SIZE / 2];
	PowerManager		powerManager;
#if __SCALING__ >= SCALING_SMP
	InterCpuMessager	interCpuMessager;
#endif
#if defined(CONFIG_ARCH_x86_32) || defined(CONFIG_ARCH_x86_64)
	class X86Lapic		lapic;
#endif
};

// The hardcoded stream for the BSP CPU.
extern CpuStream	bspCpu;

#endif

