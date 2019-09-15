
#include <arch/io.h>
#include <arch/interrupts.h>
#include <arch/cpuControl.h>
#include <chipset/zkcm/picDevice.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kbitManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include "i8259a.h"
#include "zkcmIbmPcState.h"

#define PIC_PIC1_CMD		0x20
#define PIC_PIC1_DATA		0x21
#define PIC_PIC2_CMD		0xA0
#define PIC_PIC2_DATA		0xA1

#define PIC_PIC1_VECTOR_BASE	(ARCH_INTERRUPTS_VECTOR_PIN_START)
#define PIC_PIC2_VECTOR_BASE	(PIC_PIC1_VECTOR_BASE + 8)

#define PIC_IO_DELAY_COUNTER	3
#define PIC_IO_DELAY(v,x)	for (v=0; v<x; v++) {}


I8259APic		i8259aPic(16);

error_t I8259APic::initialize(void)
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

	maskAll();
	/* Send EOI to both PICs; Can't use ibmPc_i8269a_sendEoi() here
	 * because no __kpin IDs have been assigned yet, and that API call
	 * requires a __kpin ID as an argument.
	 **/
	io::write8(PIC_PIC1_CMD, 0x20);
	io::write8(PIC_PIC2_CMD, 0x20);

	return ERROR_SUCCESS;
}

error_t I8259APic::shutdown(void)
{
	return ERROR_SUCCESS;
}

error_t I8259APic::suspend(void)
{
	return ERROR_SUCCESS;
}

error_t I8259APic::restore(void)
{
	return ERROR_SUCCESS;
}

void I8259APic::chipsetEventNotification(ubit8 event, uarch_t)
{
	switch (event)
	{
	case __KPOWER_EVENT_HEAP_AVAIL:

		// Allocate and zero memory for the __kpin mapping list.
		irqPinList = new sZkcmIrqPin[16];
		if (irqPinList == NULL)
		{
			panic(FATAL i8259a"Failed to allocate memory for "
				"internal __kpin mapping array.\n");
		};

		/**	EXPLANATION:
		 * Prepare the irqPinList of i8259 IRQ pins for use by the
		 * kernel.
		 **/
		for (ubit8 i=0, j=PIC_PIC1_VECTOR_BASE; i<16; i++, j++)
		{
			irqPinList[i].flags = 0;
			irqPinList[i].cpu = ibmPcState.bspInfo.bspId;
			irqPinList[i].vector = j;
			irqPinList[i].triggerMode =
				IRQCTL_IRQPIN_TRIGGMODE_LEVEL;

			irqPinList[i].polarity = IRQCTL_IRQPIN_POLARITY_HIGH;
		};

		interruptTrib.registerIrqPins(16, irqPinList);
		__kpinBase = irqPinList[0].__kid;
		printf(NOTICE i8259a"Registered for __kpins. Base: %d.\n",
			__kpinBase);

		break;

	case __KPOWER_EVENT_PRE_SMP_MODE_SWITCH:
		/**	EXPLANATION:
		 * Tell the kernel to remove all i8259a IRQ pins from the __kpin
		 * list, because on IBM-PC, when symmetric multiprocessing mode
		 * is activated, we use only the IO-APICs and not mixed mode.
		 **/
		if (irqPinList != NULL) {
			interruptTrib.removeIrqPins(16, irqPinList);
		};

		__kpinBase = 0;
		break;

	default: break;
	};
}


error_t I8259APic::get__kpinFor(ubit8 pinNo, ubit16 *__kpin)
{
	if (pinNo > 15) { return ERROR_UNSUPPORTED; };
	// Return immediately if the array hasn't yet been allocated.
	if (irqPinList == NULL) { return ERROR_RESOURCE_UNAVAILABLE; };

	*__kpin = irqPinList[pinNo].__kid;
	return ERROR_SUCCESS;
}

status_t I8259APic::identifyActiveIrq(
	cpu_t cpu, uarch_t vector, ubit16 *__kpin, ubit8 *triggerMode
	)
{
	ubit8		mask=0;
	uarch_t		flags;

	/**	EXPLANATION:
	 * Simply read the In-service register. If a bit is set, and the vector
	 * matches the programmed vector for that bit, return the __kpin for
	 * the pin that matches that bit.
	 *
	 * We can do a bit of minor optimization here: since we know that each
	 * pin of the i8259 interrupts on a different CPU vector, we can use
	 * the vector argument to guess which i8259 PIC has signaled us. If the
	 * vector is within the range for the master i8259, then we won't check
	 * the slave, and vice-versa.
	 *
	 * Also, since the i8259s are hardwired to only interrupt on the BSP CPU
	 * it is impossible for them to interrupt any CPU other than the BSP. An
	 * IO-APIC in virtual-wire mode will also emulate this behaviour.
	 **/
	if (irqPinList == NULL) {
		return IRQCTL_IDENTIFY_ACTIVE_IRQ_UNIDENTIFIABLE;
	};

	if (cpu != ibmPcState.bspInfo.bspId) { return ERROR_UNSUPPORTED; };

	*triggerMode = IRQCTL_IRQPIN_TRIGGMODE_LEVEL;
	if (vector >= PIC_PIC1_VECTOR_BASE && vector < PIC_PIC2_VECTOR_BASE)
	{
		cpuControl::safeDisableInterrupts(&flags);
		io::write8(PIC_PIC1_CMD, 0x4B);
		mask = io::read8(PIC_PIC1_CMD);
		cpuControl::safeEnableInterrupts(flags);
	};

	if (vector >= PIC_PIC2_VECTOR_BASE && vector < PIC_PIC2_VECTOR_BASE + 8)
	{
		cpuControl::safeDisableInterrupts(&flags);
		io::write8(PIC_PIC2_CMD, 0x4B);
		mask = io::read8(PIC_PIC2_CMD);
		cpuControl::safeEnableInterrupts(flags);
	};

	if (mask)
	{
		/* On the i8259s, lower pin values have higher IRQ
		 * priority. Start checking from bit 0.
		 **/
		for (ubit8 i=0; i<8; i++)
		{
			if (__KBIT_TEST(mask, i)) {
				return get__kpinFor(i, __kpin);
			};
		};
	}
	else
	{
		/* The i8259s issue IRQ signals set up to look like they
		 * were generated by the 8th pin, when they generate a
		 * spurious IRQ signal, and they do not set any bits in
		 * the In-service register.
		 **/
		if (vector == PIC_PIC1_VECTOR_BASE + 7
			|| vector == PIC_PIC2_VECTOR_BASE + 7)
		{
			return IRQCTL_IDENTIFY_ACTIVE_IRQ_SPURIOUS;
		};

		return IRQCTL_IDENTIFY_ACTIVE_IRQ_UNIDENTIFIABLE;
	};

	return IRQCTL_IDENTIFY_ACTIVE_IRQ_UNIDENTIFIABLE;
}

status_t I8259APic::getIrqStatus(
	uarch_t __kpin, cpu_t *cpu, uarch_t *vector,
	ubit8 *triggerMode, ubit8 *polarity
	)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kpin, &pin);
	if (err != ERROR_SUCCESS) { return IRQCTL_GETIRQSTATUS_INEXISTENT; };
	// Return immediately if the array hasn't been allocated.
	if (irqPinList == NULL) { return IRQCTL_GETIRQSTATUS_INEXISTENT; };

	*cpu = irqPinList[pin].cpu;
	*vector = irqPinList[pin].vector;
	*triggerMode = irqPinList[pin].triggerMode;
	// FIXME: Not sure what polarity ISA IRQs are.
	*polarity = irqPinList[pin].polarity;

	return (FLAG_TEST(irqPinList[pin].flags, IRQCTL_IRQPIN_FLAGS_ENABLED))
		? IRQCTL_GETIRQSTATUS_ENABLED
		: IRQCTL_GETIRQSTATUS_DISABLED;
}

status_t I8259APic::setIrqStatus(
	uarch_t __kpin, cpu_t cpu, uarch_t vector, ubit8 enabled
	)
{
	/**	EXPLANATION:
	 * The i8259s don't support IRQ routing to any CPU other than the BSP.
	 * Additionally, to make implementation simpler, we just don't allow
	 * i8259a IRQs to be set to vectors other than the ones they were set
	 * to in I8259APic::initialize().
	 **/

	if (cpu != ibmPcState.bspInfo.bspId) {
		return IRQCTL_SETIRQSTATUS_CPU_UNSUPPORTED;
	};

	if (vector < PIC_PIC1_VECTOR_BASE || vector > PIC_PIC2_VECTOR_BASE+7) {
		return IRQCTL_SETIRQSTATUS_VECTOR_UNSUPPORTED;
	};

	if (enabled) {
		unmaskIrq(__kpin);
	} else {
		maskIrq(__kpin);
	};

	return ERROR_SUCCESS;
}

void I8259APic::maskAll(void)
{
	io::write8(PIC_PIC1_DATA, 0xFF);
	io::write8(PIC_PIC2_DATA, 0xFF);

	if (irqPinList != NULL)
	{
		for (ubit8 i=0; i<16; i++)
		{
			FLAG_UNSET(
				irqPinList[i].flags,
				IRQCTL_IRQPIN_FLAGS_ENABLED);
		};
	};
}
	
void I8259APic::unmaskAll(void)
{
	io::write8(PIC_PIC1_DATA, 0x0);
	io::write8(PIC_PIC2_DATA, 0x0);

	if (irqPinList != NULL)
	{
		for (ubit8 i=0; i<16; i++)
		{
			FLAG_SET(
				irqPinList[i].flags,
				IRQCTL_IRQPIN_FLAGS_ENABLED);
		};
	};

}

void I8259APic::sendEoi(ubit16 __kid)
{
	error_t		err;
	ubit8		pin;

	err = lookupPinBy__kid(__kid, &pin);
	if (err != ERROR_SUCCESS) { return; };

	if (pin > 7)
		{ io::write8(PIC_PIC2_CMD, 0x20); };

	io::write8(PIC_PIC1_CMD, 0x20);
}

void I8259APic::maskIrq(ubit16 __kpin)
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

	if (irqPinList != NULL) {
		FLAG_UNSET(irqPinList[pin].flags, IRQCTL_IRQPIN_FLAGS_ENABLED);
	};
}

void I8259APic::unmaskIrq(ubit16 __kpin)
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

	if (irqPinList != NULL) {
		FLAG_SET(irqPinList[pin].flags, IRQCTL_IRQPIN_FLAGS_ENABLED);
	};
}

void I8259APic::maskIrqsByPriority(ubit16 __kid, cpu_t, uarch_t *mask)
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

void I8259APic::unmaskIrqsByPriority(ubit16, cpu_t, uarch_t mask)
{
	// Just write the old mask out unconditionally.
	io::write8(PIC_PIC1_DATA, mask & 0xFF);
	io::write8(PIC_PIC2_DATA, mask >> 8);
}

sarch_t I8259APic::irqIsEnabled(ubit16 __kid)
{
	error_t		err;
	ubit8		pin;

	if ((err = lookupPinBy__kid(__kid, &pin)) != ERROR_SUCCESS) {
		return 0;
	};

	if (pin < 8) {
		return !__KBIT_TEST(io::read8(PIC_PIC1_DATA), pin);
	}
	else
	{
		return !__KBIT_TEST(
			io::read8(PIC_PIC2_DATA), pin-8);
	};
}

