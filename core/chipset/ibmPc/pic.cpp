
#include <arch/io.h>
#include <__kstdlib/__kflagManipulation.h>
#include "pic.h"

#define PIC_PIC1_CMD		0x20
#define PIC_PIC1_DATA		0x21
#define PIC_PIC2_CMD		0xA0
#define PIC_PIC2_DATA		0xA1

#define PIC_PIC1_VECTOR_BASE	0x20
#define PIC_PIC2_VECTOR_BASE	0x28

#define PIC_IO_DELAY_COUNTER	3
#define PIC_IO_DELAY(v,x)		for (v=0; v<x; v++) {}


static ubit8	firstMaskAllCall=1;

error_t ibmPc_pic_initialize(void)
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

	return ERROR_SUCCESS;
}

error_t ibmPc_pic_shutdown(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_pic_suspend(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_pic_restore(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_pic_maskSingle(uarch_t vector)
{
	uarch_t		mask;

	if (vector >= PIC_PIC1_VECTOR_BASE
		&& vector < (PIC_PIC2_VECTOR_BASE + 8)) 
	{
		if (vector < PIC_PIC2_VECTOR_BASE)
		{
			mask = io::read8(PIC_PIC1_DATA);
			__KFLAG_SET(
				mask, (1<<(vector - PIC_PIC1_VECTOR_BASE)));

			io::write8(PIC_PIC1_DATA, mask);
		}
		else
		{
			mask = io::read8(PIC_PIC2_DATA);
			__KFLAG_SET(
				mask, (1<<(vector - PIC_PIC2_VECTOR_BASE)));

			io::write8(PIC_PIC2_DATA, mask);
		};

	return ERROR_SUCCESS;
	};

	// PIC can't handle non-ISA interrupts.
	return ERROR_INVALID_ARG_VAL;
}

void ibmPc_pic_maskAll(void)
{
	io::write8(PIC_PIC1_DATA, 0xFF);
	io::write8(PIC_PIC2_DATA, 0xFF);
	if (firstMaskAllCall)
	{
		io::write8(PIC_PIC1_CMD, 0x20);
		io::write8(PIC_PIC2_CMD, 0x20);
		firstMaskAllCall = 0;
	};
}

error_t ibmPc_pic_unmaskSingle(uarch_t vector)
{
	uarch_t		mask;

	if (vector >= PIC_PIC1_VECTOR_BASE
		&& vector < (PIC_PIC2_VECTOR_BASE + 8)) 
	{
		if (vector < PIC_PIC2_VECTOR_BASE)
		{
			mask = io::read8(PIC_PIC1_DATA);
			__KFLAG_UNSET(
				mask, (1<<(vector - PIC_PIC1_VECTOR_BASE)));

			io::write8(PIC_PIC1_DATA, mask);
		}
		else
		{
			mask = io::read8(PIC_PIC2_DATA);
			__KFLAG_UNSET(
				mask, (1<<(vector - PIC_PIC2_VECTOR_BASE)));

			io::write8(PIC_PIC2_DATA, mask);
		};

	return ERROR_SUCCESS;
	};

	// PIC can't handle non-ISA interrupts.
	return ERROR_INVALID_ARG_VAL;
}
	
void ibmPc_pic_unmaskAll(void)
{
	io::write8(PIC_PIC1_DATA, 0x0);
	io::write8(PIC_PIC2_DATA, 0x0);
}

void ibmPc_pic_sendEoi(ubit8)
{
	// FIXME: implement.
}

