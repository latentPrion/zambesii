
#include <chipset/zkcm/irqControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <kernel/common/panic.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include "i8259a.h"
#include "vgaTerminal.h"
#include "zkcmIbmPcState.h"


/**	NOTES:
 * SCI: Active low, level triggered, shareable (ACPI spec).
 **/

#define IBMPCIRQCTL		"IBMPC Irq-Ctl: "

error_t ZkcmIrqControlMod::initialize(void)
{
	/**	EXPLANATION:
	 * At this point, we would like to initialize the i8259 PICs and mask
	 * all IRQs at their pins.
	 **/
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		return ERROR_SUCCESS;
	};

	return i8259aPic.initialize();
}

error_t ZkcmIrqControlMod::shutdown(void)
{
	return ERROR_SUCCESS;
}

error_t ZkcmIrqControlMod::suspend(void)
{
	return ERROR_SUCCESS;
}

error_t ZkcmIrqControlMod::restore(void)
{
	return ERROR_SUCCESS;
}

status_t ZkcmIrqControlMod::identifyActiveIrq(
	cpu_t cpu, uarch_t vector, ubit16 *__kpin, ubit8 *triggerMode
	)
{
	x86IoApic::IoApic	*ioApic;

	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP)
	{
		// Check vector number for spurious, return immediately if true.
		if (vector == x86LAPIC_VECTOR_SPURIOUS) {
			return IRQCTL_IDENTIFY_ACTIVE_IRQ_SPURIOUS;
		};

		if (!x86IoApic::ioApicsAreDetected())
		{
			panic(FATAL IBMPCIRQCTL"identifyActiveIrq: Chipset "
				"is in SMP mode, but IO-APICs are\n"
				"\tundetected. Unable to call into "
				"x86IoApic::identifyActiveIrq.\n");
		};

		ioApic = x86IoApic::getIoApicByVector(vector);
		if (ioApic == NULL) {
			return IRQCTL_IDENTIFY_ACTIVE_IRQ_UNIDENTIFIABLE;
		};

		return ioApic->identifyActiveIrq(
			cpu, vector, __kpin, triggerMode);
	}
	else
	{
		return i8259aPic.identifyActiveIrq(
			cpu, vector, __kpin, triggerMode);
	};
}

error_t ZkcmIrqControlMod::registerIrqController(void)
{
	return ERROR_SUCCESS;
}

void ZkcmIrqControlMod::destroyIrqController(void)
{
}

void ZkcmIrqControlMod::chipsetEventNotification(ubit8 event, uarch_t flags)
{
	printf(FATAL IBMPCIRQCTL"chipsetEventNotification has been "
		"disconnected from the kernel code path.\n\tYou will have to "
		"re-organize and reconnect this code path.\n"
		"\tONLY the IBMPC VGA terminal will receive this message.\n"
		"\tEvent is %d.\n",
		event);

	return;

	switch (event)
	{
	case __KPOWER_EVENT_HEAP_AVAIL:
		/**	EXPLANATION:
		 * Tell the i8259 code to advertise its IRQ pins to the kernel.
		 **/
		i8259aPic.chipsetEventNotification(event, flags);
		break;

	case __KPOWER_EVENT_PRE_SMP_MODE_SWITCH:
		/**	EXPLANATION:
		 * When switching to multiprocessor mode on the IBM-PC, the
		 * i8259a code must first be told, so it can remove its IRQ
		 * pins from the Interrupt Trib. Then, the irqControl mod must
		 * switch over to delegating all calls to libIoApic.
		 **/
		i8259aPic.chipsetEventNotification(event, flags);
		break;

	default:
		panic(CC IBMPCIRQCTL"chipsetEventNotification: invalid "
			"event ID.\n");

		break;
	};
}

status_t ZkcmIrqControlMod::getIrqStatus(
	uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
	ubit8 *triggerMode, ubit8 *polarity
	)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP)
	{
		return x86IoApic::getIrqStatus(
			__kpin, cpu, vector, triggerMode, polarity);
	}
	else
	{
		return i8259aPic.getIrqStatus(
			__kpin, cpu, vector, triggerMode, polarity);
	};
}

status_t ZkcmIrqControlMod::setIrqStatus(
	uarch_t __kpin, cpu_t cpu, uarch_t vector, ubit8 enabled
	)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP)
	{
		return x86IoApic::setIrqStatus(
			__kpin, cpu, vector, enabled);
	}
	else {
		return i8259aPic.setIrqStatus(__kpin, cpu, vector, enabled);
	};
}

void ZkcmIrqControlMod::maskIrq(ubit16 __kpin)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		x86IoApic::maskIrq(__kpin);
	}
	else {
		i8259aPic.maskIrq(__kpin);
	};
}

void ZkcmIrqControlMod::unmaskIrq(ubit16 __kpin)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		x86IoApic::unmaskIrq(__kpin);
	}
	else {
		i8259aPic.unmaskIrq(__kpin);
	};
}

void ZkcmIrqControlMod::maskAll(void)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		x86IoApic::maskAll();
	}
	else {
		i8259aPic.maskAll();
	};
}

void ZkcmIrqControlMod::unmaskAll(void)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		x86IoApic::unmaskAll();
	}
	else {
		i8259aPic.unmaskAll();
	};
}

sarch_t ZkcmIrqControlMod::irqIsEnabled(ubit16 __kpin)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		return x86IoApic::irqIsEnabled(__kpin);
	}
	else {
		return i8259aPic.irqIsEnabled(__kpin);
	};
}

void ZkcmIrqControlMod::maskIrqsByPriority(
	ubit16 __kpin, cpu_t cpuId, uarch_t *mask
	)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP)
	{
		// To be implemented.
		panic(ERROR_UNIMPLEMENTED, FATAL IBMPCIRQCTL
			"maskIrqsByPriority: Lib IO-APIC has no back-end.\n");
	}
	else {
		i8259aPic.maskIrqsByPriority(__kpin, cpuId, mask);
	};
}

void ZkcmIrqControlMod::unmaskIrqsByPriority(
	ubit16 __kpin, cpu_t cpu, uarch_t mask
	)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP)
	{
		// To be implemented.
		panic(ERROR_UNIMPLEMENTED, FATAL IBMPCIRQCTL
			"unmaskIrqsByPriority: Lib IO-APIC has no back-end.\n");
	}
	else {
		i8259aPic.unmaskIrqsByPriority(__kpin, cpu, mask);
	};
}

void ZkcmIrqControlMod::sendEoi(ubit16 __kpin)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP)
	{
		// Directly call on lib LAPIC to do the EOI.
		cpuTrib.getCurrentCpuStream()->lapic.sendEoi();
	}
	else {
		i8259aPic.sendEoi(__kpin);
	}
}

