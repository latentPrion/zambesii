
#define UDI_VERSION 0x101
#include <udi.h>
#undef UDI_VERSION

/* Data type used to represent i8254-pit CLK cycles. CLK cycles are /not/ 1:1
 * related to actual microseconds. The i8254 PIT has an input frequency of
 * 1,193,180Hz.
 **/
typedef udi_ubit16_t	timer_clk_t;

struct sTimerRData
{
	enum timerIrqStatusE { IRQ_DISABLED, IRQ_ENABLED };
	enum timerApiStatusE { API_DISABLED, API_ENABLED };
	enum timerModeE { ONESHOT, PERIODIC };

	// Whether or not the channel 0 IRQ is active.
	timerIrqStatusE		irqStatus;
	// Whether or not the device is logically enabled currently.
	timerApiStatusE		apiStatus;
	timerModeE		timerMode;
	// The current value programmed into the counter register.
	timer_clk_t		CurrenttlkVal;
	// The current microsecond time requested by the bound child.
	udi_time_t		currentTimeoutVal;
};

struct sSpeakerRData
{
};

