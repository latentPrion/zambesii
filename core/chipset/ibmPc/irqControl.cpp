
#include <chipset/zkcm/irqControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include "i8259a.h"
#include "zkcmIbmPcState.h"
#include "irqControl.h"

error_t ibmPc_irqControl_initialize(void)
{
	/**	EXPLANATION:
	 * At this point, we would like to initialize the i8259 PICs and mask
	 * all IRQs at their pins.
	 **/
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		return ERROR_SUCCESS;
	};

	return ibmPc_i8259a_initialize();
}

error_t ibmPc_irqControl_getInitialPinInfo(ubit16 *nPins, zkcmIrqPinS **ret)
{
	/**	EXPLANATION:
	 * This is only ever called once, and is called by the Int Trib. The
	 * first time the IRQ control module's initialize() is called, it is
	 * likely that memory management is not yet operational, so pin
	 * detection cannot be done then.
	 *
	 * When memory management is up, the kernel calls this function to have
	 * the chipset report pins.
	 *
	 * For the IBM-PC, we just report the pins on the 8259a chips in this
	 * function.
	 **/
	return ibmPc_i8259a_getPinInfo(nPins, ret);
}

error_t ibmPc_irqControl_shutdown(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_irqControl_suspend(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_irqControl_restore(void)
{
	return ERROR_SUCCESS;
}

void ibmPc_irqControl___kregisterPinIds(ubit16, zkcmIrqPinS *)
{
	// If SMP mode, pass the pins back to libIOAPIC.
}

void ibmPc_irqControl_maskIrq(ubit16 __kpin)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		// Call libIOAPIC to perform the action.
	}
	else {
		ibmPc_i8259a_maskIrq(__kpin);
	};
}

void ibmPc_irqControl_unmaskIrq(ubit16 __kpin)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		// Call libIOAPIC to perform the action.
	}
	else {
		ibmPc_i8259a_unmaskIrq(__kpin);
	};
}

void ibmPc_irqControl_maskAll(void)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		// Call libIOAPIC.
	}
	else {
		ibmPc_i8259a_maskAll();
	};
}

void ibmPc_irqControl_unmaskAll(void)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		// Call libIOAPIC.
	}
	else {
		ibmPc_i8259a_unmaskAll();
	};
}

sarch_t ibmPc_irqControl_getIrqStatus(ubit16 __kpin)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		// Call LibIOAPIC for info.
		return 0;
	}
	else {
		return ibmPc_i8259a_irqIsEnabled(__kpin);
	};
}

void ibmPc_irqControl_maskIrqsByPriority(
	ubit16 __kpin, cpu_t cpuId, uarch_t *mask
	)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		// To be implemented.
	}
	else {
		ibmPc_i8259a_maskIrqsByPriority(__kpin, cpuId, mask);
	};
}

void ibmPc_irqControl_unmaskIrqsByPriority(
	ubit16 __kpin, cpu_t cpu, uarch_t mask
	)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		// To be implemented.
	}
	else {
		ibmPc_i8259a_unmaskIrqsByPriority(__kpin, cpu, mask);
	};
}

