
#include <arch/schedTimer.h>
#include <kernel/common/moduleApis/chipsetSupportPackage.h>


// If this doesn't return ERROR_SUCCESS, that's a lot of trouble.
error_t schedTimerC::initialize(void)
{
	/**	EXPLANATION:
	 * Detect LAPIC. If it exists, initialize it to a known state and
	 * return.
	 *
	 * If no LAPIC, ask Chipset Support Package for a new timer source for
	 * the scheduler. If it cannot provide that, then we return non-success
	 * status.
	 **/
	
