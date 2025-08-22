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
	#include <kernel/common/messageStream.h>

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
	// Base class for tracking interrupt nesting levels
	class InterruptNestingCounter
	{
	public:
		InterruptNestingCounter()
		: nestingLevel(0)
		{}

		uarch_t enter(void) { return ++nestingLevel; }
		uarch_t exit(void) { return --nestingLevel; }
		sbit8 isBeingHandled(void) { return nestingLevel > 0; }
		uarch_t getNestingLevel(void) { return nestingLevel; }

	protected:
		volatile uarch_t nestingLevel;
	};

	// Extended class for async interrupts with the isNested method
	class AsyncInterruptNestingCounter
	: public InterruptNestingCounter
	{
	public:
		AsyncInterruptNestingCounter()
		: InterruptNestingCounter()
		{}

		sbit8 isBeingNested(void) { return nestingLevel > 1; }
	};

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
	static uarch_t			nCpusInExcessOfConfigMaxNcpus;

	status_t enumerate(void);
	sCpuFeatures *getCpuFeatureBlock(void);

	sbit8 isReadyForIrqs(void) { return readyForIrqs; }
	void setReadyForIrqs(sbit8 ready) { readyForIrqs = ready; }

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
		: Stream<CpuStream>(parentStream, id),
		powerStatus(CC"PowerManager powerStatus")
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

		struct sCpuPowerOnMsg
		{
			sCpuPowerOnMsg(
				processId_t targetPid,
				ubit16 function,
				uarch_t flags, void *privateData,
				CpuStream *cpuStream)
			: header(
				targetPid,
				MSGSTREAM_SUBSYSTEM_CPUSTREAM_POWER_MANAGER,
				function, sizeof(*this), flags, privateData),
			cpuStream(cpuStream)
			{}

			MessageStream::sHeader	header;
			CpuStream		*cpuStream;
		};

		enum PowerManagerOpE
		{
			OP_POWER_ON,
			OP_BOOT_POWER_ON_REQ,
			OP_HALT,
			OP_SLEEP,
			OP_POWER_OFF
		};

		status_t powerOn(ubit32 flags);
		status_t bootPowerOnReq(ubit32 flags, void *privateData);
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

	/* These are the nesting levels for each of the individual
	 * interrupt types.
	 *
	 * We don't need to know these for the most part, other than the
	 * nmiNestingLevel. Other than the nmiNestingLevel, they're purely
	 * useful for profiling and debugging.
	 *
	 * The nmiNestingLevel is needed because NMIs may defy the CPU's
	 * local interrupt masking mechanism. The NMI handler may need
	 * to know how many levels deep it is nested, and whether there
	 * have been multiple NMIs nested on the current CPU.
	 */
	InterruptNestingCounter nmiEvent;
#ifdef CONFIG_DEBUG_INTERRUPTS
	/* The following counters are built into the kernel when
	 * CONFIG_DEBUG_INTERRUPTS is enabled.
	 *
	 * swiEvent should never have a nesting level greater than 1.
	 * Nested SWIs are not supported.
	 **/
	InterruptNestingCounter irqEvent, excEvent, swiEvent;

	/* This is the number of levels of interrupts of any kind that are
	 * nested.
	 *
	 * "Interrupt" here is used in the loosest sense possible. It
	 * includes both async and sync interrupts (IRQs, NMIs, SWIs,
	 * exceptions).
	 */
	InterruptNestingCounter interruptEvent;

	/* Indicates how many levels of Sync interrupt events are nested.
	 * NB: Sync interrupts include: SWI, exceptions.
	 *
	 * * If we're handling a SWI/EXC, this should be > 0.
	 * * If we're not handling a SWI/EXC, this should be == 0.
	 * * If we're handling a SWI/EXC and it's the "first" (i.e: no nesting)
	 *   then this should be == 1.
	 * * When the first "nested" SWI/EXC comes in, this should be incremented
	 *   to 2.
	 *
	 * Threads that are executing a sync interrupt handler can be preempted
	 * by the timeslicer.
	 */
	InterruptNestingCounter syncInterruptEvent;
#endif

	/* Indicates how many levels of Async interrupt events are nested.
	 * NB: Async interrupts include: IRQs, NMIs.
	 * This is analogous to Linux's preempt_count, but we are doing things
	 * a bit more abstractly than the Linux devs.
	 *
	 * * If we're handling an IRQ/NMI/etc, this should be > 0.
	 * * If we're not handling an IRQ/NMI/etc, this should be == 0.
	 * * If we're handling an IRQ/NMI/etc and it's the "first" (i.e: no nesting)
	 *   then this should be == 1.
	 * * When the first "nested" IRQ/NMI/etc comes in, this should be incremented
	 *   to 2.
	 *
	 * This is used to control whether the timeslice preemptor should be
	 * invoked at the exit from the current async int event.
	 *
	 * The timeslicer should only be invoked when we're at the outermost
	 * level of async interrupt nesting (i.e: asyncInterruptNestingLevel == 1).
	 * Calling the timeslicer at higher levels of nesting is pointless, will
	 * cause the timeslicer to preempt one of the lower-nested async events.
	 * This means that all of the lower nested async events will be
	 * synchronously bound to the current thread on whose stack they're
	 * running. This effectively converts all of those lower-nested async
	 * events into sync events.
	 *
	 * Hence we must avoid calling the timeslicer during async int events.
	 *
	 * ----------------------------------------------------------------
	 * Built unconditionally into every version of the kernel because it's
	 * used to prevent the TaskTrib:: sched ops from performing an immediate
	 * thread switch from within an async interrupt handler.
	 *
	 * It also has an overloaded second use when CONFIG_SCHED_PREEMPT_TIMESLICE
	 * is enabled because it's used to prevent the timeslicer from being called
	 * during async int events whose nesting level is > 1.
	 *
	 * Also useful for debugging purposes.
	 **/
	AsyncInterruptNestingCounter asyncInterruptEvent;

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

#ifdef CONFIG_DEBUG_LOCKED_INTERRUPT_ENTRY
	uarch_t			nLocksHeld;
	Lock			*mostRecentlyAcquiredLock;
#endif

private:
	sbit8			readyForIrqs;
};

// The hardcoded stream for the BSP CPU.
extern CpuStream	bspCpu;

#endif

