
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

error_t zkcmIrqControlModC::initialize(void)
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

error_t zkcmIrqControlModC::shutdown(void)
{
	return ERROR_SUCCESS;
}

error_t zkcmIrqControlModC::suspend(void)
{
	return ERROR_SUCCESS;
}

error_t zkcmIrqControlModC::restore(void)
{
	return ERROR_SUCCESS;
}

status_t zkcmIrqControlModC::identifyActiveIrq(
	cpu_t cpu, uarch_t vector, ubit16 *__kpin, ubit8 *triggerMode
	)
{
	x86IoApic::ioApicC	*ioApic;

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

error_t zkcmIrqControlModC::registerIrqController(void)
{
	return ERROR_SUCCESS;
}

void zkcmIrqControlModC::destroyIrqController(void)
{
}

void zkcmIrqControlModC::chipsetEventNotification(ubit8 event, uarch_t flags)
{
	switch (event)
	{
	case IRQCTL_EVENT___KSPACE_MEMMGT_AVAIL:
		ibmPcVgaTerminal.chipsetEventNotification(event, flags);
		break;

	case IRQCTL_EVENT_MEMMGT_AVAIL:
		/**	EXPLANATION:
		 * Tell the i8259 code to advertise its IRQ pins to the kernel.
		 **/
		i8259aPic.chipsetEventNotification(event, flags);
		break;

	case IRQCTL_EVENT_SMP_MODE_SWITCH:
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

status_t zkcmIrqControlModC::getIrqStatus(
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

status_t zkcmIrqControlModC::setIrqStatus(
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

void zkcmIrqControlModC::maskIrq(ubit16 __kpin)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		x86IoApic::maskIrq(__kpin);
	}
	else {
		i8259aPic.maskIrq(__kpin);
	};
}

void zkcmIrqControlModC::unmaskIrq(ubit16 __kpin)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		x86IoApic::unmaskIrq(__kpin);
	}
	else {
		i8259aPic.unmaskIrq(__kpin);
	};
}

void zkcmIrqControlModC::maskAll(void)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		x86IoApic::maskAll();
	}
	else {
		i8259aPic.maskAll();
	};
}

void zkcmIrqControlModC::unmaskAll(void)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		x86IoApic::unmaskAll();
	}
	else {
		i8259aPic.unmaskAll();
	};
}

sarch_t zkcmIrqControlModC::irqIsEnabled(ubit16 __kpin)
{
	if (ibmPcState.smpInfo.chipsetState == SMPSTATE_SMP) {
		return x86IoApic::irqIsEnabled(__kpin);
	}
	else {
		return i8259aPic.irqIsEnabled(__kpin);
	};
}

void zkcmIrqControlModC::maskIrqsByPriority(
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

void zkcmIrqControlModC::unmaskIrqsByPriority(
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

void zkcmIrqControlModC::sendEoi(ubit16 __kpin)
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

