
#include <arch/io.h>
#include <chipset/zkcm/irqControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kbitManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include "i8259a.h"
#include "zkcmIbmPcState.h"

#define PIC_PIC1_CMD		0x20
#define PIC_PIC1_DATA		0x21
#define PIC_PIC2_CMD		0xA0
#define PIC_PIC2_DATA		0xA1

#define PIC_PIC1_VECTOR_BASE	0x20
#define PIC_PIC2_VECTOR_BASE	0x28

#define PIC_IO_DELAY_COUNTER	3
#define PIC_IO_DELAY(v,x)		for (v=0; v<x; v++) {}


ubit16 				__kpinBase=0;
static zkcmIrqPinS		irqPinList[16];

static inline error_t lookupPinBy__kid(ubit16 __kpin, ubit8 *pin)
{
	*pin = __kpin - __kpinBase;
	if (*pin > 15) { return ERROR_INVALID_ARG_VAL; };
	return ERROR_SUCCESS;
}

error_t ibmPc_i8259a_initialize(void)
{
	ubit8		i;

	/**	EXPLANATION:
	 * On IBM-PC, there's no watchdog, so we have no need to initialize
	 * a timer early. As a result of that, we have no need to enable local
	 * IRQs on the BSP.
	 *
	 * All we'll do here is re-initialize the 8259 cascades, and then exit.
	 * We may expect IRQs to be disabled on the BSP right now, so there is
	 * no need to explicitly disable them before sending commands to the
	 * PIC.
	 **/
	/**	NOTE:
	 * In the datasheet for the 8259, it says that bit 3 must be 1 (for
	 * level sensitive, since the 8259 does not support edge triggered,
	 * as written in the docs.), but in the Linux source, and pretty much
	 * every other code example where the PIC is initialized, no-one else
	 * sets the level triggered bit.
	 *
	 * I'll test this on real hardware sometime, but for now, we'll send
	 * 0x11 instead of 0x19.
	 **/
	io::write8(PIC_PIC1_CMD, 0x11); PIC_IO_DELAY(i, PIC_IO_DELAY_COUNTER);
	io::write8(PIC_PIC2_CMD, 0x11); PIC_IO_DELAY(i, PIC_IO_DELAY_COUNTER);

	io::write8(PIC_PIC1_DATA, PIC_PIC1_VECTOR_BASE);
	PIC_IO_DELAY(i, PIC_IO_DELAY_COUNTER);
	io::write8(PIC_PIC2_DATA, PIC_PIC2_VECTOR_BASE);
	PIC_IO_DELAY(i, PIC_IO_DELAY_COUNTER);

	io::write8(PIC_PIC1_DATA, 0x04); PIC_IO_DELAY(i, PIC_IO_DELAY_COUNTER);
	io::write8(PIC_PIC2_DATA, 0x02); PIC_IO_DELAY(i, PIC_IO_DELAY_COUNTER);

	io::write8(PIC_PIC1_DATA, 0x01); PIC_IO_DELAY(i, PIC_IO_DELAY_COUNTER);
	io::write8(PIC_PIC2_DATA, 0x01); PIC_IO_DELAY(i, PIC_IO_DELAY_COUNTER);

	ibmPc_i8259a_maskAll();
	// Send EOI to both PICs;
	io::write8(PIC_PIC1_CMD, 0x20);
	io::write8(PIC_PIC2_CMD, 0x20);

	return ERROR_SUCCESS;
}

error_t ibmPc_i8259a_shutdown(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_i8259a_suspend(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_i8259a_restore(void)
{
	return ERROR_SUCCESS;
}

void ibmPc_i8259a_chipsetEventNotification(ubit8 event, uarch_t)
{
	switch (event)
	{
	case IRQCTL_EVENT_MEMMGT_AVAIL:
		/**	EXPLANATION:
		 * Prepare the irqPinList of i8259 IRQ pins for use by the
		 * kernel.
		 **/
		for (ubit8 i=0, j=PIC_PIC1_VECTOR_BASE; i<16; i++, j++)
		{
			irqPinList[i].flags = 0;
			irqPinList[i].cpu = ibmPcState.bspInfo.bspId;
			irqPinList[i].vector = j;
			irqPinList[i].triggerMode = IRQPIN_TRIGGMODE_LEVEL;
			irqPinList[i].polarity = IRQPIN_POLARITY_HIGH;
		};

		interruptTrib.registerIrqPins(16, irqPinList);
		__kpinBase = irqPinList[0].__kid;
		break;

	case IRQCTL_EVENT_SMP_MODE_SWITCH:
		/**	EXPLANATION:
		 * Tell the kernel to remove all i8259a IRQ pins from the __kpin
		 * list, because on IBM-PC, when symmetric multiprocessing mode
		 * is activated, we use only the IO-APICs and not mixed mode.
		 **/
		interruptTrib.removeIrqPins(16, irqPinList);
		__kpinBase = 0;
		break;

	default: break;
	};
}

error_t ibmPc_i8259a_identifyIrq(uarch_t physicalId, ubit16 *__kpin)
{
	if (physicalId > 15) { return ERROR_INVALID_ARG_VAL; };

	*__kpin = irqPinList[physicalId].__kid;
	return ERROR_SUCCESS;
}

status_t ibmPc_i8259a_getIrqStatus(
	uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
	ubit8 *triggerMode, ubit8 *polarity
	)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return IRQCTL_IRQSTATUS_INEXISTENT; };

	*cpu = irqPinList[pin].cpu;
	*vector = irqPinList[pin].vector;
	*triggerMode = IRQPIN_TRIGGMODE_LEVEL;
	// FIXME: Not sure what polarity ISA IRQs are.
	*polarity = irqPinList[pin].polarity;

	return __KFLAG_TEST(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED);
}

status_t ibmPc_i8259a_setIrqStatus(
	uarch_t __kpin, cpu_t cpu, uarch_t vector, ubit8 enabled
	)
{
	/**	EXPLANATION:
	 * The i8259s don't support IRQ routing to any CPU other than the BSP.
	 * Additionally, to make implementation simpler, we just don't allow
	 * i8259a IRQs to be set to vectors other than the ones they were set
	 * to in ibmPc_i8259a_initialize().
	 **/

	if (cpu != ibmPcState.bspInfo.bspId) {
		return IRQCTL_SETIRQSTATUS_CPU_UNSUPPORTED;
	};

	if (vector < PIC_PIC1_VECTOR_BASE || vector > PIC_PIC2_VECTOR_BASE+7) {
		return IRQCTL_SETIRQSTATUS_VECTOR_UNSUPPORTED;
	};

	if (enabled) {
		ibmPc_i8259a_unmaskIrq(__kpin);
	}
	else {
		ibmPc_i8259a_maskIrq(__kpin);
	};

	return ERROR_SUCCESS;
}

void ibmPc_i8259a_maskAll(void)
{
	io::write8(PIC_PIC1_DATA, 0xFF);
	io::write8(PIC_PIC2_DATA, 0xFF);
}
	
void ibmPc_i8259a_unmaskAll(void)
{
	io::write8(PIC_PIC1_DATA, 0x0);
	io::write8(PIC_PIC2_DATA, 0x0);
}

void ibmPc_i8259a_sendEoi(ubit16 __kid)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kid, &pin);
	if (err != ERROR_SUCCESS) { return; };

	if (pin > 7)
		{ io::write8(PIC_PIC2_CMD, 0x20); };

	io::write8(PIC_PIC1_CMD, 0x20);
}

void ibmPc_i8259a_maskIrq(ubit16 __kpin)
{
	ubit8		mask;
	error_t		err;
	ubit8		pin;

	if ((err = lookupPinBy__kid(__kpin, &pin)) != ERROR_SUCCESS) {
		return;
	};

	if (pin < 0x8)
	{
		mask = io::read8(PIC_PIC1_DATA);
		__KBIT_SET(mask, pin);
		io::write8(PIC_PIC1_DATA, mask);
	}
	else
	{
		mask = io::read8(PIC_PIC2_DATA);
		__KBIT_SET(mask, pin-8);
		io::write8(PIC_PIC2_DATA, mask);
	};

	__KFLAG_UNSET(
		irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED);

}

void ibmPc_i8259a_unmaskIrq(ubit16 __kpin)
{
	ubit8		mask;
	error_t		err;
	ubit8		pin;

	if ((err = lookupPinBy__kid(__kpin, &pin)) != ERROR_SUCCESS) {
		return;
	};

	if (pin < 0x8)
	{
		mask = io::read8(PIC_PIC1_DATA);
		__KBIT_UNSET(mask, pin);
		io::write8(PIC_PIC1_DATA, mask);
	}
	else
	{
		mask = io::read8(PIC_PIC2_DATA);
		__KBIT_UNSET(mask, pin-8);
		io::write8(PIC_PIC2_DATA, mask);
	};

	__KFLAG_SET(irqPinList[pin].flags, IRQPIN_FLAGS_ENABLED);
}

void ibmPc_i8259a_maskIrqsByPriority(ubit16 __kid, cpu_t, uarch_t *mask)
{
	ubit8		irqNo;
	ubit16		newMask=0;
	error_t		err;

	if ((err = lookupPinBy__kid(__kid, &irqNo)) != ERROR_SUCCESS) {
		return;
	};

	// Save the current state.
	*mask = io::read8(PIC_PIC1_DATA);
	*mask |= io::read8(PIC_PIC2_DATA) << 8;

	// Mask all IRQs lower than and higher than irqNo.
	newMask = ~(1<<irqNo);
	// Mask irqNo itself.
	newMask |= 1 << irqNo;
	// Use a bitshift to unmask IRQs higher than irqNo.
	newMask <<= 15 - irqNo;
	// Reshift 0s into the bits.
	newMask >>= 15 - irqNo;
	// OR the old state for the higher bits into the new.
	newMask |= *mask;
	// Write the new mask out.
	io::write8(PIC_PIC1_DATA, newMask & 0xFF);
	if (irqNo > 7)
		{ io::write8(PIC_PIC2_DATA, newMask >> 8); };
}

void ibmPc_i8259a_unmaskIrqsByPriority(ubit16, cpu_t, uarch_t mask)
{
	// Just write the old mask out unconditionally.
	io::write8(PIC_PIC1_DATA, mask & 0xFF);
	io::write8(PIC_PIC2_DATA, mask >> 8);
}

sarch_t ibmPc_i8259a_irqIsEnabled(ubit16 __kid)
{
	error_t		err;
	ubit8		pin;

	if ((err = lookupPinBy__kid(__kid, &pin)) != ERROR_SUCCESS) {
		return 0;
	};

	if (pin < 8) {
		return __KBIT_TEST(io::read8(PIC_PIC1_DATA), pin);
	}
	else
	{
		return __KBIT_TEST(
			io::read8(PIC_PIC2_DATA), pin-8);
	};
}

