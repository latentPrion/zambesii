#ifndef _ZKCM_IRQ_CONTROL_H
	#define _ZKCM_IRQ_CONTROL_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/smpTypes.h>

/**	EXPLANATION:
 * The Zambesii kernel IRQ-controller module subsystem. This subsytem is
 * responsible for abstracting access to IRQ controllers from the kernel. Seeing
 * as the only example case at the time of writing is the IBM-PC, this case will
 * be explored as an explanation.
 *
 * On the IBM-PC there are two models of IRQ-control that can be used. The
 * i8259a model and the APIC model. The kernel proper (Interrupt Tributary)
 * should not need to care about the actual underlying PIC layout on the board.
 * The kernel uses a set of abstract "__kpin" IDs for PIC pin inputs.
 * As PICs are detected, the chipset code reports the PIC and all of its pins
 * to the kernel; The kernel uses the pin information to assemble a list of
 * devices interrupting on that pin and thus compile information on IRQs on the
 * board.
 *
 * For the PC, the chipset layer has to keep track of whether or not the kernel
 * is in SMP mode or uniprocessor mode, and use the appropriate PIC code to
 * handle kernel requests. When the chipset is in PIC/virtual-wire mode, the
 * chipset layer routes requests to the code in i8259.cpp; for Symmetric I/O
 * mode, requests are sent to LibIOApic (core/commonlibs/libx86mp).
 *
 * In any case, new PIC pins are reported to the kernel and the kernel directly
 * associates metadata with these pins internally.
 **/

/* Values used for sZkcmIrqPin.triggerMode.
 **/
#define IRQCTL_IRQPIN_TRIGGMODE_LEVEL		0
#define IRQCTL_IRQPIN_TRIGGMODE_EDGE		1

/* Values for sZkcmIrqPin.polarity.
 **/
#define IRQCTL_IRQPIN_POLARITY_HIGH		0
#define IRQCTL_IRQPIN_POLARITY_LOW		1

/* Values for sZkcmIrqPin.flags.
 **/
#define IRQCTL_IRQPIN_FLAGS_ENABLED		(1<<0)

/* Values for sZkcmIrqPin.intelMpId and sZkcmIrqPin.acpiId.
 * These two values are used for when the chipset does not know the mappings
 * between IRQ Pins and the pins described by the ACPI/MP tables.
 **/
#define IRQCTL_IRQPIN_ACPIID_INVALID			(-1)
#define IRQCTL_IRQPIN_INTELMPID_INVALID			(-1)

/**	XXX:
 * These are the values to be returned from calls to
 *	irqControlModC::getActiveIrq().
 *
 * When the chipset logic determines that the IRQ was a spurious IRQ, it should
 * return IRQCTL_GETACTVIVEIRQ_SPURIOUS.
 *
 * On successful identification of the IRQ pin that has been raised, the chipset
 * should set the __kpin pointer argument and the triggerMode argument, then
 * return ERROR_SUCCESS.
 *
 * On the (hopefully inexistent) occasion that the kernel asks the chipset to
 * identify an IRQ signal and the chipset is unable to do so, the chipset should
 * return IRQCTL_GETACTIVEIRQ_UNIDENTIFIABLE. This will probably happen when
 * a message-signal IRQ and a pin-based IRQ are programmed to interrupt on the
 * same CPU vector and CPU. The kernel will generally avoid allowing message-
 * signaled and pin-based IRQs to share the same interrupt vector though.
 **/
#define IRQCTL_IDENTIFY_ACTIVE_IRQ_SPURIOUS			(1)
#define IRQCTL_IDENTIFY_ACTIVE_IRQ_UNIDENTIFIABLE		(2)

struct sZkcmIrqPin
{
	ubit32		physId;
	sbit32		acpiId;
	sbit32		intelMpId;
	uarch_t		flags;
	cpu_t		cpu;
	ubit16		__kid;
	ubit16		vector;
	ubit8		triggerMode, polarity;
};


/* Return values for loadBusPinMappings.
 **/
#define IRQCTL_BPM_UNSUPPORTED_BUS			(1)
#define IRQCTL_BPM_NO_MAPPINGS_FOUND			(2)

// The Kernel sends these event notifications to the IRQ Control mod.
enum e__kPowerEvent
{
	__KPOWER_EVENT_INTERRUPT_TRIB_AVAIL = 0,
	__KPOWER_EVENT___KSPACE_PMM_AVAIL,
	__KPOWER_EVENT___KMEMORY_STREAM_AVAIL,
	__KPOWER_EVENT___KPRINTF_AVAIL,
	__KPOWER_EVENT_FULL_PMM_AVAIL,
	__KPOWER_EVENT_PRE_SMP_MODE_SWITCH,
	__KPOWER_EVENT_SMP_AVAIL,
	__KPOWER_EVENT_HEAP_AVAIL,
	__KPOWER_EVENT_SCHED_COOP_AVAIL,
	__KPOWER_EVENT_SCHED_PREEMPT_AVAIL,
	__KPOWER_EVENT_VFS_TRIB_AVAIL,
	__KPOWER_EVENT_FVFS_AVAIL,
	__KPOWER_EVENT_DVFS_AVAIL
};

/* Return values for getIrqStatus.
 **/
#define IRQCTL_GETIRQSTATUS_ENABLED	0x1
#define IRQCTL_GETIRQSTATUS_DISABLED	0x2
// IRQ pin with __kid provided does not exist.
#define IRQCTL_GETIRQSTATUS_INEXISTENT	0x3

/* Return values for setIrqStatus.
 **/
// Underlying PIC chip does not support remapping to other vectors.
#define IRQCTL_SETIRQSTATUS_VECTOR_UNSUPPORTED		0x1
// Underlying PIC chip doesn't support routing to other CPUs.
#define IRQCTL_SETIRQSTATUS_CPU_UNSUPPORTED		0x2
// Underlying PIC chip doesn't support indicated trigger mode.
#define IRQCTL_SETIRQSTATUS_TRIGGMODE_UNSUPPORTED	0x3
// Underlying PIC chip doesn't support given polarity.
#define IRQCTL_SETIRQSTATUS_POLARITY_UNSUPPORTED	0x4

class ZkcmIrqControlMod
{
public:
	error_t initialize(void);
	// A second initialize(); should report all PIC pins to kernel.
	error_t shutdown(void);
	error_t suspend(void);
	error_t restore(void);

	error_t registerIrqController(void);
	void destroyIrqController(void);

	/**	EXPLANATION:
	 * This function is almost the back-bone of IRQ handling. It identifies,
	 * given the ID of the CPU being interrupted and the CPU interrupt
	 * vector that was raised by the IRQ, the __kpin responsible for the
	 * IRQ signal.
	 *
	 * From there, the kernel will run every IRQ handler on that __kpin
	 * until one of the handlers claims the IRQ, or it reaches the end of
	 * the list of handlers without any claims (which is trouble).
	 *
	 * The function returns the trigger mode of the IRQ pin as well, so that
	 * the kernel doesn't have to manually ask for it every time a pin based
	 * IRQ comes in.
	 **/
	status_t identifyActiveIrq(
		cpu_t cpu, uarch_t vector, ubit16 *__kpin, ubit8 *triggerMode);

	status_t getIrqStatus(
		uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
		ubit8 *triggerMode, ubit8 *polarity);

	status_t setIrqStatus(
		uarch_t __kpin, cpu_t cpu, uarch_t vector, ubit8 enabled);

	void maskIrq(ubit16 __kpin);
	void unmaskIrq(ubit16 __kpin);
	void maskAll(void);
	void unmaskAll(void);
	sarch_t irqIsEnabled(ubit16 __kpin);

	void maskIrqsByPriority(
		ubit16 __kpin, cpu_t cpuId, uarch_t *mask0);

	void unmaskIrqsByPriority(
		ubit16 __kpin, cpu_t cpuId, uarch_t mask0);

	void sendEoi(ubit16 __kpin);

	void chipsetEventNotification(ubit8 event, uarch_t flags);

	// Bus-Pin Mapping API calls.
	class Bpm
	{
	public:
		status_t loadBusPinMappings(utf8Char *bus);
		error_t get__kpinFor(
			utf8Char *bus, ubit32 busIrqId, ubit16 *__kpin);

		/**	NOTE:
		 * Deprecated in favour of forcing all calls to maskIrq() and
		 * unmaskIrq() to go through Interrupt Trib and the IRQ Control
		 * module's maskIrq() and unmaskIrq().
		 *
		 * status_t maskIrq(utf8Char *bus, ubit32 busIrqId);
		 * status_t unmaskIrq(utf8Char *bus, ubit32 busIrqId);
		 **/
	} bpm;
};

#endif

