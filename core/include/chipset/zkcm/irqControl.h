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

struct zkcmIrqPinS
{
	uarch_t		flags;
	ubit16		__kid;
	cpu_t		cpu;
	ubit16		vector;
	ubit8		triggerMode, polarity;
};

struct zkcmIrqControlModS
{
	error_t (*initialize)(void);
	// A second initialize(); should report all PIC pins to kernel.
	error_t (*detectPins)(ubit16 *nPins, struct zkcmIrqPinS **ret);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*restore)(void);

	void (*__kregisterPinIds)(struct zkcmIrqPinS *list);

	void (*maskIrq)(ubit16 __kid);
	void (*unmaskIrq)(ubit16 __kid);
	void (*maskAll)(void);
	void (*unmaskAll)(void);

	void (*maskIrqsByPriority)(ubit16 __kid, cpu_t cpuId, uarch_t *mask0);
	void (*unmaskIrqsByPriority)(ubit16 __kid, cpu_t cpuId, uarch_t mask0);

	sarch_t (*getIrqStatus)(ubit16 __kid);
};

#endif

