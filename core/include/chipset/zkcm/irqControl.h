#ifndef _ZKCM_IRQ_CONTROL_H
	#define _ZKCM_IRQ_CONTROL_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/smpTypes.h>

/**	EXPLANATION:
 * The Zambezii kernel IRQ-controller chipset subsystem. This subsytem is
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
 * handle kernel requests. When the chipset is in PIC/virtual-wire mode, the\
 * chipset layer routes requests to the code in i8259.cpp; for Symmetric I/O
 * mode, requests are sent to LibIOApic (core/commonlibs/libx86mp).
 *
 * In any case, new PIC pins are reported to the kernel and the kernel directly
 * associates metadata with these pins internally.
 **/

#define IRQPIN_TRIGGMODE_LEVEL		0
#define IRQPIN_TRIGGMODE_EDGE		1

#define IRQPIN_POLARITY_HIGH		0
#define IRQPIN_POLARITY_LOW		1

#define IRQPIN_FLAGS_ENABLED		(1<<0)

// The Kernel sends these event notifications to the IRQ Control mod.
#define IRQCTL_EVENT_MEMMGT_AVAIL	0x0
#define IRQCTL_EVENT_SMP_MODE_SWITCH	0x1

/* Return status values for getIrqStatus.
 **/
#define IRQCTL_IRQSTATUS_ENABLED	0x1
#define IRQCTL_IRQSTATUS_DISABLED	0x2
// IRQ pin with __kid provided does not exist.
#define IRQCTL_IRQSTATUS_INEXISTENT	0x3

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

struct zkcmIrqPinS
{
	ubit16		__kid;
	ubit32		acpiId;
	uarch_t		flags;
	cpu_t		cpu;
	ubit16		vector;
	ubit8		triggerMode, polarity;
};

struct zkcmIrqControlModS
{
	error_t (*initialize)(void);
	// A second initialize(); should report all PIC pins to kernel.
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*restore)(void);

	error_t (*registerIrqController)(void);
	void (*destroyIrqController)(void);

	void (*chipsetEventNotification)(ubit8 event, uarch_t flags);

	error_t (*identifyIrq)(uarch_t physicalId, ubit16 *__kpin);
	status_t (*getIrqStatus)(
		uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
		ubit8 *triggerMode, ubit8 *polarity);

	status_t (*setIrqStatus)(
		uarch_t __kpin, cpu_t cpu, uarch_t vector, ubit8 enabled);

	void (*maskIrq)(ubit16 __kpin);
	void (*unmaskIrq)(ubit16 __kpin);
	void (*maskAll)(void);
	void (*unmaskAll)(void);
	sarch_t (*irqIsEnabled)(ubit16 __kpin);

	void (*maskIrqsByPriority)(ubit16 __kpin, cpu_t cpuId, uarch_t *mask0);
	void (*unmaskIrqsByPriority)(ubit16 __kpin, cpu_t cpuId, uarch_t mask0);

	void (*sendEoi)(ubit16 __kpin);
};

#endif

