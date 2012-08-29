#ifndef _TIMER_TRIB_H
	#define _TIMER_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/clock_t.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/timerTrib/timeTypes.h>
	#include <kernel/common/timerTrib/timerQueue.h>
	#include <kernel/common/interruptTrib/zkcmIsrFn.h>

#define TIMERTRIB_WATCHDOG_ALREADY_REGISTERED	(1)

#define TIMERTRIB_PER_CPU_CLOCK_EMU		(1<<0)

class timerTribC
:
public tributaryC
{
public:
	timerTribC(void);
	error_t initialize(void);
	~timerTribC(void);

public:
	status_t registerWatchdogIsr(zkcmIsrFn *, uarch_t interval);
	void updateWatchdogIsr(uarch_t interval);
	void unregisterWatchdogIsr(void);

	void updateContinuousClock(void);
	void updateScheduledClock(uarch_t sourceId);

	void getCurrentTime(timeS *);
	date_t getCurrentDate(void);

	/**	Pending redesign.
	mstime_t	getCurrentTickMs(void);
	mstime_t	getUptimeTickMs(void);
	mstime_t	getTickIntervalSinceMs(mstime_t);

	void	setTimeoutMs(mstime_t, void (*)());
	void	setContinuousTimerMs(mstime_t, void (*)()); */

	void dump(void);

private:
	// The watchdog timer for the chipset, if it exists.
	struct watchdogIsrS
	{
		zkcmIsrFn	*isr;
		clock_t		feedTime;
		uarch_t		interval;
	};

	timerQueueC	period100ms, period10ms, period1ms;
	uarch_t		flags;
	sharedResourceGroupC<waitLockC, watchdogIsrS>	watchdog;

	// To be deprecated in favour of a timestampS object.
	sharedResourceGroupC<waitLockC, clock_t>	continuousClock;
};

extern timerTribC		timerTrib;

#endif

