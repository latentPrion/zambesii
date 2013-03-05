
#include <arch/io.h>
#include <chipset/zkcm/timerControl.h>
#include <arch/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/libacpi.h>
#include <kernel/common/waitLock.h>
#include <kernel/common/sharedResourceGroup.h>
#include <kernel/common/timerTrib/timeTypes.h>
#include "rtcmos.h"

/**	EXPLANATION:
 *	IBM-PC RTC:
 * By default it is initialized to generate a 32.768 kHz base internal
 * frequency. This frequency cannot be safely changed:
 *	"The system initializes these bits (RT/CMOS regindex STATUS0, bits 4-6)
 *	to 0x2, which selects a 32.768 kHz time-base. This is the only value
 *	supported by the system for proper time keeping."
 *
 * The frequency divisor is set to a value which generates a 1.024 kHz output
 * signal (976.562us period). Basically, it is impossible to set a divisor value
 * which will accurately enough approach 1Hz, or any fraction thereof, so this
 * timer is unreliable for accurate timekeeping, or high-precision multimedia
 * timing.
 *
 * However, for things like scheduler timing, or non-critical timing requests
 * (simple sleep()s, etc), it would suffice. Sadly however, the RTC timer is
 * used by the firmware for the ACPI timer on many modern boards, so in the end
 * the kernel must leave it alone.
 *
 * The only thing Zambesii uses the RTC device registers for is to get the
 * current date/time.
 *
 *	SIDE NOTE:
 * The IBM-PC NMI will be asserted on machine check error, and can only be
 * de-asserted with a write of 0x0 to bit 6 of IO-port 0x90. (CMOS manual).
 **/

#define RTCCMOS		"RTC/CMOS: "

#define RTCCMOS_IO_ADDR		(0x70)
#define RTCCMOS_IO_DATA		(0x71)

/**	RTC Register indexes
 **/
#define RTC_REG_SECONDS		(0x0)
#define RTC_REG_SECOND_ALARM	(0x1)
#define RTC_REG_MINUTES		(0x2)
#define RTC_REG_MINUTE_ALARM	(0x3)
#define RTC_REG_HOURS		(0x4)
#define RTC_REG_HOUR_ALARM	(0x5)
#define RTC_REG_DAY_OF_WEEK	(0x6)
#define RTC_REG_DAY_OF_MONTH	(0x7)
#define RTC_REG_MONTH		(0x8)
#define RTC_REG_YEAR		(0x9)
#define RTC_REG_STATUS0		(0xA)
#define RTC_REG_STATUS1		(0xB)
#define RTC_REG_STATUS2		(0xC)
#define RTC_REG_STATUS3		(0xD)

/**	RTC per-register bit flags and values.
 **/
#define RTC_STATUS0_FLAGS_BUSY	(1<<7)
#define RTC_STATUS0_DIVIDER	(0)

#define RTC_STATUS1_FLAGS_REGISTER_LOCK		(1<<7)
// Set to enable periodic IRQ mode.
#define RTC_STATUS1_FLAGS_PERIODIC_IRQ_ENABLED	(1<<6)
// Set to enable alarm IRQ mode (oneshot).
#define RTC_STATUS1_FLAGS_ALARM_IRQ_ENABLED	(1<<5)
// Set to enable on-update IRQ mode.
#define RTC_STATUS1_FLAGS_UPDATEEND_IRQ_ENABLED	(1<<4)
#define RTC_STATUS1_FLAGS_USE_DIVIDER_VAL	(1<<3)
#define RTC_STATUS1_FLAGS_DATETIME_FORMAT_BIN	(1<<2)
#define RTC_STATUS1_FLAGS_TIME_FORMAT_24HRS	(1<<1)
#define RTC_STATUS1_FLAGS_DAYLIGHT_SAVINGS	(1<<0)

// States that the RT/CMOS chip is asserting an IRQ.
#define RTC_STATUS2_FLAGS_IRQ_ASSERTED		(1<<7)
// Use these two to extract the reason from STATUS2.
#define RTC_STATUS2_IRQ_REASON_SHIFT		(4)
#define RTC_STATUS2_IRQ_REASON_MASK		(0x7)
// Use these 3 to compare against the packed 3-bit IRQ reason value.
#define RTC_STATUS2_IRQ_PERIODIC		(0x4)
#define RTC_STATUS2_IRQ_ALARM			(0x2)
#define RTC_STATUS2_IRQ_UPDATEEND		(0x1)

// Read-only, indicates whether or not the CMOS is getting power.
#define RTC_STATUS3_FLAGS_CMOS_BATTERY_LIVE	(1<<7)

/**	CMOS Register indexes.
 * We only care about diagnostic status, shutdown status and date century. The
 * other CMOS RAM bytes aren't generally meaningful enough to merit declaration
 * here.
 **/
#define CMOS_REG_DIAGNOSTIC_STATUS		(0x0E)
#define CMOS_REG_SHUTDOWN_STATUS		(0x0F)
#define CMOS_REG_DATE_CENTURY			(0x37)

#define CMOS_DIAGNOSTICS_FLAGS_NO_POWER	(1<<7)


static ubit8		rtccmosIsPowered=0;
static ubit8		rtccmos24HourTime=0;
static ubit8		rtccmosBcdDateTime=0;
static sharedResourceGroupC<waitLockC, timestampS>	systemTime;
waitLockC		rtccmosLock;
static ubit8		rtccmosDateCenturyChecked=0;
static ubit8		rtccmosDateCenturyOffset=0;

namespace rtc
{
}

namespace rtccmos
{
	void writeAddressRegister(ubit8 index)
	{
		// If NMI bit is set, refuse the write.
		if (index & 0x80)
		{
			__kprintf(ERROR RTCCMOS"Please use enableNmi"
				"/disableNmi explicitly to manipulate the NMI "
				"signal.\n");

			return;
		};

		io::write8(RTCCMOS_IO_ADDR, index);
	}

	void writeDataRegister(ubit8 val)
	{
		io::write8(RTCCMOS_IO_DATA, val);
	}

	ubit8 readDataRegister(void)
	{
		return io::read8(RTCCMOS_IO_DATA);
	}

	/**	EXPLANATION:
	 * IBM-PC Technical reference manual, Model 50:
	 *	"After I/O operations, the RT/CMOS and NMI mask register
	 *	(hex 0070) should be left pointing to status register D
	 *	(hex 00D).
	 *	WARNING: The operation following a write to hex 0070 should
	 *	access hex 0071; otherwise intermittent malfunctions and
	 *	unreliable operation of the RT/CMOS RAM can occur.
	 *
	 * Therefore, after every series of IO writes and reads to/from the
	 * CMOS, we must do the following:
	 *	1. Write 0x0D to the address index register.
	 *	2. Read a byte from the data register to ensure that an access
	 *	   to port 0x71 is generated.
	 **/
	void resetAddressRegister(void)
	{
		writeAddressRegister(RTC_REG_STATUS3);
		readDataRegister();
	}

	void lock(void)
	{
		rtccmosLock.acquire();
	}

	void unlock(void)
	{
		rtccmosLock.release();
	}
}

static error_t	 ibmPc_rtccmos_initialize(void)
{
	ubit8		batteryStatus2Safe, batteryStatusDiagnosticSafe;

	/**	EXPLANATION:
	 * There are two ways to see if the RTC/CMOS chip is receiving power:
	 *	1. Check RTC status byte 3, bit 7.
	 *	2. Check CMOS Diagnostic status byte, bit 7.
	 *
	 * If the CMOS chip is not getting power, then the CMOS RAM should
	 * not be trusted. However, in light of the fact that many boards may
	 * not respect these two values, the kernel will simply write a warning
	 * to the kernel log and continue operation.
	 **/
	rtccmos::lock();
	rtccmos::writeAddressRegister(RTC_REG_STATUS2);
	batteryStatus2Safe = rtccmos::readDataRegister();
	rtccmos::writeAddressRegister(CMOS_REG_DIAGNOSTIC_STATUS);
	batteryStatusDiagnosticSafe = rtccmos::readDataRegister();

	rtccmos::resetAddressRegister();
	rtccmos::unlock();

	batteryStatus2Safe &= RTC_STATUS3_FLAGS_CMOS_BATTERY_LIVE;
	batteryStatusDiagnosticSafe &= CMOS_DIAGNOSTICS_FLAGS_NO_POWER;
	batteryStatusDiagnosticSafe = !batteryStatusDiagnosticSafe;

	if (!batteryStatus2Safe || !batteryStatusDiagnosticSafe)
	{
		__kprintf(WARNING RTCCMOS"RTC/CMOS chip may not be powered "
			"adequately.\n"
			"\tCMOS RAM may be unreliable/invalid.\n");
	};

	rtccmosIsPowered = 1;
	return ERROR_SUCCESS;
}

/*static error_t ibmPc_rtccmos_shutdown(void) { return ERROR_SUCCESS; }
static error_t ibmPc_rtccmos_suspend(void) { return ERROR_SUCCESS; }
static error_t ibmPc_rtccmos_restore(void) { return ERROR_SUCCESS; }*/

error_t ibmPc_rtc_initialize(void)
{
	error_t		ret;
	ubit8		status1;

	ret = ibmPc_rtccmos_initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	rtccmos::lock();

	rtccmos::writeAddressRegister(RTC_REG_STATUS1);
	status1 = rtccmos::readDataRegister();
	// Clear all IRQs.
	__KFLAG_UNSET(status1,
		RTC_STATUS1_FLAGS_PERIODIC_IRQ_ENABLED
		| RTC_STATUS1_FLAGS_ALARM_IRQ_ENABLED
		| RTC_STATUS1_FLAGS_UPDATEEND_IRQ_ENABLED);

	rtccmos::writeAddressRegister(RTC_REG_STATUS1);
	rtccmos::writeDataRegister(status1);
	rtccmos::resetAddressRegister();

	rtccmos::unlock();


	rtccmos24HourTime =
		__KFLAG_TEST(status1, RTC_STATUS1_FLAGS_TIME_FORMAT_24HRS);

	if (!rtccmos24HourTime) {
		__kprintf(ERROR RTCCMOS"Chip reports 12 hour format time.\n");
	};

	rtccmosBcdDateTime =
		!__KFLAG_TEST(status1, RTC_STATUS1_FLAGS_DATETIME_FORMAT_BIN);

	if (!rtccmosBcdDateTime)
	{
		__kprintf(WARNING RTCCMOS"Date/Time is not in BCD. Possibly "
			"unstable or non-compliant chip.\n");
	};

	memset(&systemTime.rsrc, 0, sizeof(systemTime.rsrc));

	return ERROR_SUCCESS;
}

error_t ibmPc_rtc_shutdown(void) { return ERROR_SUCCESS; }
error_t ibmPc_rtc_suspend(void) { return ERROR_SUCCESS; }
error_t ibmPc_rtc_restore(void) { return ERROR_SUCCESS; }

static inline ubit8 bcd8ToUbit8(ubit8 bcdVal)
{
	return (((bcdVal >> 4) & 0xF) * 10) + (bcdVal & 0xF);
}

static sarch_t getCenturyOffset(ubit8 *ret)
{
	error_t		err;
	acpi_rsdtS	*rsdt;
	acpi_rFadtS	*fadt;
	void		*handle, *context;

	/* This function used to return CMOS_REG_DATE_CENTURY on error, to make
	 * the caller fetch from the "known" century offset, but while this
	 * yields reliable behaviour in emulators, it does not do so on real
	 * hardware. Thus we return -1 instead now, which tells the caller to
	 * just use a hardcoded value for century.
	 **/
	err = acpi::findRsdp();
	if (err != ERROR_SUCCESS) {
		return 0;
	};

	if (acpi::testForRsdt())
	{
		// Use RSDT.
		err = acpi::mapRsdt();
		if (err != ERROR_SUCCESS)
		{
			__kprintf(WARNING RTCCMOS"getCenturyOffset: failed to "
				"map RSDT.\n");

			return 0;
		};

		rsdt = acpi::getRsdt();

		context = handle = __KNULL;
		fadt = acpiRsdt::getNextFadt(rsdt, &context, &handle);
		if (fadt != __KNULL)
		{
			// Literal value temporarily used. Taken from Linux.
			if (fadt->hdr.revision < 3 || !fadt->cmosCentury)
			{
				__kprintf(NOTICE RTCCMOS"FADT century offset "
					"not used.\n");

				return 0;
			};

			*ret = fadt->cmosCentury;
			acpiRsdt::destroySdt((acpi_sdtS *)fadt);
			acpiRsdt::destroyContext(&context);
			__kprintf(NOTICE RTCCMOS"getCenturyOffset: ACPI: "
				"Offset is %d.\n", *ret);

			return 1;
		};
#if !defined(__32_BIT__) || defined(CONFIG_ARCH_x86_32_PAE)
	}
	else
	{
		// Use XSDT.
#endif
	};

	return 0;
}


status_t ibmPc_rtc_getHardwareDate(dateS *ret)
{
	ubit8	century=0x20;

	// Check ACPI for the century register offset.
	if (!rtccmosDateCenturyChecked)
	{
		getCenturyOffset(&rtccmosDateCenturyOffset);
		rtccmosDateCenturyChecked = 1;
	};

	rtccmos::lock();

	// Only try to read century value if ACPI said it's valid.
	if (rtccmosDateCenturyOffset != 0)
	{
		rtccmos::writeAddressRegister(rtccmosDateCenturyOffset);
		century = rtccmos::readDataRegister();
	};

	rtccmos::writeAddressRegister(RTC_REG_YEAR);
	ret->year = rtccmos::readDataRegister();
	rtccmos::writeAddressRegister(RTC_REG_MONTH);
	ret->month = rtccmos::readDataRegister();
	rtccmos::writeAddressRegister(RTC_REG_DAY_OF_MONTH);
	ret->day = rtccmos::readDataRegister();
	rtccmos::writeAddressRegister(RTC_REG_DAY_OF_WEEK);
	ret->weekDay = rtccmos::readDataRegister();
	rtccmos::resetAddressRegister();

	rtccmos::unlock();

	if (rtccmosBcdDateTime)
	{
		ret->year = bcd8ToUbit8(century) * 100 + bcd8ToUbit8(ret->year);
		ret->month = bcd8ToUbit8(ret->month);
		ret->day = bcd8ToUbit8(ret->day);
		ret->weekDay = bcd8ToUbit8(ret->weekDay);
	}
	else {
		ret->year = century * 100 + ret->year;
	};

	return ERROR_SUCCESS;
}

status_t ibmPc_rtc_getHardwareTime(timeS *time)
{
	ubit8	hour, min, sec;

	rtccmos::lock();

	rtccmos::writeAddressRegister(RTC_REG_HOURS);
	hour = rtccmos::readDataRegister();
	rtccmos::writeAddressRegister(RTC_REG_MINUTES);
	min = rtccmos::readDataRegister();
	rtccmos::writeAddressRegister(RTC_REG_SECONDS);
	sec = rtccmos::readDataRegister();
	rtccmos::resetAddressRegister();

	rtccmos::unlock();

	if (rtccmosBcdDateTime)
	{
		hour = bcd8ToUbit8(hour);
		min = bcd8ToUbit8(min);
		sec = bcd8ToUbit8(sec);
	};

	/* If the RTC doesn't store the hours value as 24-hour time, then where
	 * do we read the "AM"/"PM" value from? Not shown anywhere in the
	 * PC-AT Technical reference guide, and it says in there that the
	 * default is 24-hour time, so if any chipset has non-24-hour
	 * values, we may just refuse to take its time value.
	 *
	 * Alternatively, I may decide to just return whatever value is in
	 * the hour register silently: broken board is broken, and it will fail
	 * somewhere else where the kernel will have a better reason to present
	 * noticeably whiny behaviour.
	 **/
	if (!rtccmos24HourTime) {
		// Do nothing, but leave this condition here for reference.
	};

	time->seconds = hour * 3600 + min * 60 + sec;
	time->nseconds = 0;
	return ERROR_SUCCESS;
}

void zkcmTimerControlModC::refreshCachedSystemTime(void)
{
	systemTime.lock.acquire();

	ibmPc_rtc_getHardwareDate(&systemTime.rsrc.date);
	ibmPc_rtc_getHardwareTime(&systemTime.rsrc.time);

	systemTime.lock.release();
}

status_t zkcmTimerControlModC::getCurrentDate(dateS *date)
{
	systemTime.lock.acquire();
	*date = systemTime.rsrc.date;
	systemTime.lock.release();

	return ERROR_SUCCESS;
}

status_t zkcmTimerControlModC::getCurrentTime(timeS *time)
{
	systemTime.lock.acquire();
	*time = systemTime.rsrc.time;
	systemTime.lock.release();

	return ERROR_SUCCESS;
}

void zkcmTimerControlModC::flushCachedSystemTime(void)
{
	UNIMPLEMENTED("zkcmTimerControlModC::flushCachedSystemTime()");
}

