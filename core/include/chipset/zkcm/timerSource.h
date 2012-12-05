#ifndef _ZKCM_TIMER_SOURCE_H
	#define _ZKCM_TIMER_SOURCE_H

	#include <chipset/zkcm/timerDriver.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/timerTrib/timeTypes.h>
	#include <kernel/common/processId.h>

struct zkcmTimerDeviceS
{
	// Uniquely identifies this child to its parent driver.
	ubit32		childId;
	ubit32		nChildren;

	struct
	{
		// Driver can use these to store linked lists of children, etc.
		ubit8	data0[128];
		ubit8	data1[64];
	} scratch;

	utf8Char	shortName[64];
	utf8Char	longName[256];

	utf8Char	vendorName[256];
	utf8Char	vendorContact[256];
};

/**	Constants used with struct zkcmTimerSourceS.
 **/
// Values for timersource.capabilities.type.
#define ZKCM_TIMERSRC_CAP_TYPE_PER_CPU		(0x0)
#define ZKCM_TIMERSRC_CAP_TYPE_CHIPSET		(0x1)

// Values for zkcmTimerSourceS.capabilities.ioLatency.
#define ZKCM_TIMERSRC_CAP_IO_LOW		(0x0)
#define ZKCM_TIMERSRC_CAP_IO_MODERATE		(0x1)
#define ZKCM_TIMERSRC_CAP_IO_HIGH		(0x2)

// Values for zkcmTimerSourceS.capabilities.precision.
#define ZKCM_TIMERSRC_CAP_PRECISION_EXACT	(0x0)
#define ZKCM_TIMERSRC_CAP_PRECISION_NEGLIGABLE	(0x1)
#define ZKCM_TIMERSRC_CAP_PRECISION_UNDERFLOW	(0x2)
#define ZKCM_TIMERSRC_CAP_PRECISION_OVERFLOW	(0x3)
#define ZKCM_TIMERSRC_CAP_PRECISION_ANY		(0x4)

// Values for zkcmTimerSourceS.capabilities.modes.
#define ZKCM_TIMERSRC_CAP_MODE_PERIODIC		(1<<0)
#define ZKCM_TIMERSRC_CAP_MODE_ONESHOT		(1<<1)

// Values for zkcmTimerSourceS.capabilities.resolutions.
#define ZKCM_TIMERSRC_CAP_RES_1S		(1<<0)
#define ZKCM_TIMERSRC_CAP_RES_100ms		(1<<1)
#define ZKCM_TIMERSRC_CAP_RES_10ms		(1<<2)
#define ZKCM_TIMERSRC_CAP_RES_1ms		(1<<3)
#define ZKCM_TIMERSRC_CAP_RES_100ns		(1<<4)
#define ZKCM_TIMERSRC_CAP_RES_10ns		(1<<5)
#define ZKCM_TIMERSRC_CAP_RES_1ns		(1<<6)

// Values for zkcmTimerSourceS.state.flags.
#define ZKCM_TIMERSRC_STATE_FLAGS_ENABLED	(1<<0)
#define ZKCM_TIMERSRC_STATE_FLAGS_LATCHED	(1<<1)

// Values for zkcmTimerSourceS.state.mode.
#define ZKCM_TIMERSRC_STATE_MODE_PERIODIC	(0x0)
#define ZKCM_TIMERSRC_STATE_MODE_ONESHOT	(0x1)

struct zkcmTimerSourceS
{
	// Uniquely identifies this child to its parent device.
	ubit32		childId;
	// Private to the IRQ source.
	struct
	{
		ubit8	data[64];
	} scratch;

	struct
	{
		/* Types: PER_CPU, CHIPSET;
		 * latencies: LOW, MODERATE, HIGH.
		 **/
		ubit8		type, ioLatency, precision;
		// Capabilities (bitfield): PERIODIC, ONESHOT.
		ubit32		modes;
		// Resolutions (bitfield): 1s, 100ms 10ms 1ms, 100ns 10ns 1ns.
		ubit32		resolutions;
	} capabilities;

	struct
	{
		ubit32		flags;
		// Current mode: periodic/oneshot. Valid if FLAGS_ENABLED set.
		ubit8		mode;
		// For periodic mode: stores the current timer period in ns.
		ubit32		period;
		// For oneshot mode: stores the current timeout date and time.
		timestampS	timeout;
	} state;

	// Process ID using this timer source. Valid if FLAGS_LATCHED is set.
	processId_t			latchedProcess;
	struct zkcmTimerDevS		*parentDevice;
	struct zkcmTimerDriverS		*driver;
};

#endif

