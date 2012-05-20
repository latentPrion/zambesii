
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


static zkcmIrqPinS		irqPinList[16];

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
	// Test the current state of IRQ __kpins.
	interruptTrib.removeIrqPins(3, &irqPinList[5]);
	interruptTrib.registerIrqPins(3, &irqPinList[5]);
	interruptTrib.removeIrqPins(2, &irqPinList[5]);
	interruptTrib.dumpIrqPins();
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
	// Find out which pin the __kid pertains to, and then send EOI to chip.
	for (ubit8 i=0; i<16; i++)
	{
		if (irqPinList[i].__kid == __kid)
		{
			if (i > 7)
				{ io::write8(PIC_PIC2_CMD, 0x20); };

			io::write8(PIC_PIC1_CMD, 0x20);
		};
	};
}

error_t ibmPc_i8259a_getPinInfo(ubit16 *nPins, zkcmIrqPinS **ret)
{
	*ret = irqPinList;

	for (ubit32 i=0, j=32; i<16; i++, j++)
	{
		(*ret)[i].cpu = ibmPcState.bspInfo.bspId;
		(*ret)[i].triggerMode = IRQPIN_TRIGGMODE_EDGE;
		(*ret)[i].polarity = IRQPIN_POLARITY_HIGH;
		(*ret)[i].flags = 0;
		(*ret)[i].vector = j;
	};

	*nPins = 16;
	return ERROR_SUCCESS;
}

void ibmPc_i8259a_maskIrq(ubit16 __kpin)
{
	ubit8		mask;

	for (ubit8 i=0; i<16; i++)
	{
		if (irqPinList[i].__kid == __kpin)
		{
			if (i < 0x8)
			{
				mask = io::read8(PIC_PIC1_DATA);
				__KBIT_SET(mask, i);
				io::write8(PIC_PIC1_DATA, mask);
			}
			else
			{
				mask = io::read8(PIC_PIC2_DATA);
				__KBIT_SET(mask, i-8);
				io::write8(PIC_PIC2_DATA, mask);
			};
			__KFLAG_UNSET(
				irqPinList[i].flags, IRQPIN_FLAGS_ENABLED);

		};
	};
}

void ibmPc_i8259a_unmaskIrq(ubit16 __kpin)
{
	ubit8		mask;

	for (ubit8 i=0; i<16; i++)
	{
		if (irqPinList[i].__kid == __kpin)
		{
			if (i < 0x8)
			{
				mask = io::read8(PIC_PIC1_DATA);
				__KBIT_UNSET(mask, i);
				io::write8(PIC_PIC1_DATA, mask);
			}
			else
			{
				mask = io::read8(PIC_PIC2_DATA);
				__KBIT_UNSET(mask, i-8);
				io::write8(PIC_PIC2_DATA, mask);
			};
			__KFLAG_SET(irqPinList[i].flags, IRQPIN_FLAGS_ENABLED);
		};
	};
}

void ibmPc_i8259a_maskIrqsByPriority(ubit16 __kid, cpu_t, uarch_t *mask)
{
	ubit8		irqNo;
	ubit16		newMask=0;

	for (irqNo=0; irqNo < 16; irqNo++)
	{
		if (irqPinList[irqNo].__kid == __kid)
		{
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
		};
	};
}

void ibmPc_i8259a_unmaskIrqsByPriority(ubit16, cpu_t, uarch_t mask)
{
	// Just write the old mask out unconditionally.
	io::write8(PIC_PIC1_DATA, mask & 0xFF);
	io::write8(PIC_PIC2_DATA, mask >> 8);
}

sarch_t ibmPc_i8259a_irqIsEnabled(ubit16 __kid)
{
	for (ubit8 i=0; i<16; i++)
	{
		if (irqPinList[i].__kid == __kid)
		{
			if (i < 8) {
				return __KBIT_TEST(io::read8(PIC_PIC1_DATA), i);
			}
			else
			{
				return __KBIT_TEST(
					io::read8(PIC_PIC2_DATA), i-8);
			};
		};
	};

	return 0;
}

