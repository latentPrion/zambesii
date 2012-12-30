#ifndef _ZKCM_ISR_H
	#define _ZKCM_ISR_H

	#include <chipset/zkcm/device.h>
	#include <__kstdlib/__kerror.h>

/* Return values for a ZKCM ISR function. */
#define ZKCM_ISR_NOT_MY_IRQ		(1)
#define ZKCM_ISR_SUCCESS		(ERROR_SUCCESS)

#define ZKCM_ISR_NACK_UNRESPONSIVE	(2)
#define ZKCM_ISR_NACK_SHUTDOWN_POSSIBLE	(3)
#define ZKCM_ISR_NACK_RESET_POSSIBLE	(4)

#define ZKCM_ISR_ACK_UNRESPONSIVE	(5)
#define ZKCM_ISR_ACK_SHUTDOWN_POSSIBLE	(6)
#define ZKCM_ISR_ACK_RESET_POSSIBLE	(7)

typedef status_t (zkcmIsrFn)(zkcmDeviceBaseC *self, ubit32 flags);

#endif

