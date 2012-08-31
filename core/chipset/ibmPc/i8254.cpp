
#include <chipset/zkcm/timerControl.h>
#include "i8254.h"


/**	EXPLANATION:
 * While we define constants for the IO locations used by channels 2 and 3, we
 * do not have any code in this file that manipulates those channels. This file
 * is solely concerned with channel 0, the only channel that has its CLK-OUT
 * signal attached directly to an IRQ pin signal.
 *
 * Channel 2's CLK-OUT signal goes to the speaker subsystem, and channel 3's
 * CLK-OUT signal goes to the NMI logic. While it could have been feasible to
 * possibly use channel 3 as a timer source, sadly, it will only generate an
 * NMI when channel 0's IRQ is raised for too long without being acknowledged.
 * In other words, it cannot actually generate an NMI signal independently of
 * channel 0, since its ability to generate the NMI depends on channel 0's
 * IRQ being raised.
 **/
// Chan 0 and 2 counters are 16bit, chan 3 counter is 8bit.
#define i8254_CHAN0_IO_COUNTER		(0x40)
#define i8254_CHAN2_IO_COUNTER		(0x42)
#define i8254_CHAN3_IO_COUNTER		(0x44)
// All control regs are 8bit and write-only.
#define i8254_CHAN0_IO_CONTROL		(0x43)
#define i8254_CHAN2_IO_CONTROL		(0x43)
#define i8254_CHAN3_IO_CONTROL		(0x47)

#define i8254_CHAN0_CONTROL_SELECT_COUNTER		(0x0<<6)
#define i8254_CHAN0_CONTROL_COUNTER_RWLOW		(0x1<<4)
#define i8254_CHAN0_CONTROL_COUNTER_RWHIGH		(0x2<<4)
// Must set bits 4-6 to 0 when attempting to read the counter regs.
#define i8254_CHAN0_CONTROL_COUNTER_LATCH		(0x0<<4)
// All modes which are invalid for channel 0 are commented out.
#define i8254_CHAN0_CONTROL_MODE0_ONESHOT		(0x0<<1)
//#define i8254_CHAN0_CONTROL_MODE1_HARDWARE_PERIODIC	(0x1<<1)
#define i8254_CHAN0_CONTROL_MODE2_RATEGEN		(0x2<<1)
#define i8254_CHAN0_CONTROL_MODE3_PERIODIC		(0x3<<1)
#define i8254_CHAN0_CONTROL_MODE4_STROBE		(0x4<<1)
//#define i8254_CHAN0_CONTROL_MODE5_HARDWARE_STROBE	(0x5<<1)

#define i8254_CHAN2_CONTROL_SELECT_COUNTER		(0x2<<6)
#define i8254_CHAN2_CONTROL_COUNTER_RWLOW		(0x1<<4)
#define i8254_CHAN2_CONTROL_COUNTER_RWHIGH		(0x2<<4)
// Must set bits 4-6 to 0 when attempting to read the counter regs.
#define i8254_CHAN2_CONTROL_COUNTER_LATCH		(0x0<<4)
// All modes which are invalid for channel 2 are commented out.
#define i8254_CHAN2_CONTROL_MODE0_ONESHOT		(0x0<<1)
#define i8254_CHAN2_CONTROL_MODE1_HARDWARE_PERIODIC	(0x1<<1)
#define i8254_CHAN2_CONTROL_MODE2_RATEGEN		(0x2<<1)
#define i8254_CHAN2_CONTROL_MODE3_PERIODIC		(0x3<<1)
#define i8254_CHAN2_CONTROL_MODE4_STROBE		(0x4<<1)
#define i8254_CHAN2_CONTROL_MODE5_HARDWARE_STROBE	(0x5<<1)

#define i8254_CHAN3_CONTROL_SELECT_COUNTER	(0x0<<6)
#define i8254_CHAN3_CONTROL_COUNTER_RWLOW	(0x1<<4)

// Since there will only ever be one source exposed, statically allocate it.
static zkcmTimerSourceS		channel0;

error_t ibmPc_i8254_initialize(void)
{
	// Expose the i8254 channel 0 timer source to the Timer Control mod.
}

error_t ibmPc_i8254_shutdown(void)
{
	// Unregister the i8254 channel 0 timer source.
}

error_t ibmPc_i8254_suspend(void)
{
}

error_t ibmPc_i8254_restore(void)
{
}

status_t ibmPc_i8254_enableTimerSource(struct zkcmTimerSourceS *timerSource);
void ibmPc_i8254_disableTimerSource(struct zkcmTimerSourceS *timerSource);

status_t ibmPc_i8254_setTimerSourcePeriodic(
	struct zkcmTimerSourceS *timerSource,
	struct timeS interval);

status_t ibmPc_i8254_setTimerSourceOneshot(
	struct zkcmTimerSourceS *timerSource,
	struct timestampS timeout);

uarch_t ibmPc_i8254_getPrecisionDiscrepancyForPeriod(
	struct zkcmTimerSourceS *timerSource,
	ubit32 period);

