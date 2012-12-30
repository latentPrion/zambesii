#ifndef _IBM_PC_INTEL_8254_PIT_H
	#define _IBM_PC_INTEL_8254_PIT_H

	#include <chipset/zkcm/zkcmIsr.h>
	#include <chipset/zkcm/timerDevice.h>
	#include <__kstdlib/__ktypes.h>

#define i8254			"IBMPC-8254: "

/* See comments in validateOneshotTimevalLimit() to understand the need for
 * these values, and how they were derived. Feel free to submit more accurate
 * calculations too.
 **/
#define i8254_ONESHOT_MAX_NS		(54928)
#define i8254_ONESHOT_NS2CLK_10K	(11932)
#define i8254_ONESHOT_NS2CLK_1K		(1193)
#define i8254_ONESHOT_NS2CLK_100	(119)
#define i8254_ONESHOT_NS2CLK_10		(12)
#define i8254_ONESHOT_NS2CLK_1		(1)

class i8254PitC
:
public zkcmTimerDeviceC
{
public:
	i8254PitC(void)
	:
	zkcmTimerDeviceC(
		zkcmTimerDeviceC::CHIPSET,
		ZKCM_TIMERDEV_CAP_MODE_PERIODIC
			| ZKCM_TIMERDEV_CAP_MODE_ONESHOT,
		ZKCM_TIMERDEV_CAP_RES_1S
			| ZKCM_TIMERDEV_CAP_RES_100MS
			| ZKCM_TIMERDEV_CAP_RES_10MS
			| ZKCM_TIMERDEV_CAP_RES_1MS,
		zkcmTimerDeviceC::MODERATE,
		zkcmTimerDeviceC::NEGLIGABLE,
		&baseDeviceInfo),
	baseDeviceInfo(
		0, CC"i8254", CC"IBM-PC compatible i8254 PIT, channel 0 timer.",
		CC"Unknown vendor", CC"N/A"),
	isrRegistered(0), __kpinId(0)
	{}

public:
	virtual error_t initialize(void);
	virtual error_t shutdown(void);
	virtual error_t suspend(void) { return ERROR_SUCCESS; }
	virtual error_t restore(void) { return ERROR_SUCCESS; }

	virtual status_t enable(void);
	virtual void disable(void);
	// Call disable() before setting timer options, then enable() again.
	virtual status_t setPeriodicMode(struct timeS /*interval*/) { return 0; };
	virtual status_t setOneshotMode(struct timeS timeout);
	virtual uarch_t getPrecisionDiscrepancyForPeriod(ubit32 /*period*/) { return 0; };

	// IRQ Handler ISR for the i8254 PIT.
	static zkcmIsrFn	isr;

private:
	void sendEoi(void);
	void writeOneshotCount(void);
	sarch_t validateCallerIsLatched(void)
	{
		cpuStreamC	*currCpu;

		currCpu = cpuTrib.getCurrentCpuStream();

		state.lock.acquire();

		if (PROCID_PROCESS(currCpu->taskStream.currentTask->id)
			== PROCID_PROCESS(state.rsrc.latchedProcess))
		{
			state.lock.release();

			return 1;
		};

		state.lock.release();
		return 0;
	};

private:
	zkcmDeviceC		baseDeviceInfo;
	// Boolean to determine whether or not our ISR is registered.
	ubit8			isrRegistered;
	// __kpin ID assigned to the IRQ pin our device is currently tied to.
	ubit16			__kpinId;
	// i8254 CLK pulse equivalent of the nanosecond value in currentTimeout.
	ubit16			currentTimeoutClks;
};

extern i8254PitC	i8254Pit;

#endif

