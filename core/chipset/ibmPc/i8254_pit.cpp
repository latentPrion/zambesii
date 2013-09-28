
#define UDI_VERSION 0x101
#include <udi.h>
#undef UDI_VERSION

/* Data type used to represent i8254-pit CLK cycles. CLK cycles are /not/ 1:1
 * related to actual microseconds. The i8254 PIT has an input frequency of
 * 1,193,180Hz.
 **/
typedef udi_ubit16_t	timer_clk_t;

struct timerRDataS
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
	timer_clk_t		currentClkVal;
	// The current microsecond time requested by the bound child.
	udi_time_t		currentTimeoutVal;
};

struct speakerRDataS
{
};

static udi_channel_event_ind_op_t	timer_intr_channel_event_ind;
static udi_intr_event_ind_op_t		timer_intr_event_ind;

static udi_intr_dispatcher_ops_t
{
	&timer_intr_channel_event_ind,
	&timer_intr_event_ind
} timer_intr_dispatcher_ops;

void timer_intr_event_ind(udi_intr_event_cb_t *cb, udi_ubit8_t flags)
{
	timerRDataS	*timerData;

	timerData = (timerRDataS *)cb->gcb.context;

	// If the IRQ is disabled, don't claim the IRQ.
	if (timerData->irqStatus == timerRDataS::IRQ_DISABLED)
	{
		cb->intr_result = UDI_INTR_UNCLAIMED;
		udi_intr_event_rdy(cb);
		return;
	};

	/* ELSE:
	 * If API status is API_ENABLED:
	 *	This is a normal IRQ.
	 * If API status is API_DISABLED:
	 *	This is a "shutting down" IRQ.
	 **/
	switch (timerData->apiStatus)
	{
	case timerRDataS::API_DISABLED:
		// Call udi_intr_detach_req().
		return;

	default:
		/* Normal case: Normal IRQ has occured. Call
		 * zudi_timer_event_ind.
		 **/
		//udi_cb_alloc(&timer_intr_event_ind_normal_cb_alloc, ...);
		return;
}

