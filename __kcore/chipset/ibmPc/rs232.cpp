
#include <arch/io.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kclib/string.h>

#include "rs232.h"

IbmPcRs232	ibmPcRs2320(0), ibmPcRs2321(1);

error_t IbmPcRs232::initialize(void)
{
	switch (baseDeviceInfo.childId) {
	case 0:
		s.port = 0x3f8;
		break;

	case 1:
		s.port = 0x2f8;
		break;

	default:
		return -1;
	};

	/* This next portion of code with the I/O port transactions is taken
	 * from the seL4 microkernel, and maintains their copyright.
	 */
	while (!(io::read8(s.port + 5) & 0x60)); /* wait until not busy */

	io::write8(s.port + 1, 0x00); /* disable generating interrupts */
	io::write8(s.port + 3, 0x80); /* line control register: command: set divisor */
	io::write8(s.port,     0x01); /* set low byte of divisor to 0x01 = 115200 baud */
	io::write8(s.port + 1, 0x00); /* set high byte of divisor to 0x00 */
	io::write8(s.port + 3, 0x03); /* line control register: set 8 bit, no parity, 1 stop bit */
	io::write8(s.port + 4, 0x0b); /* modem control register: set DTR/RTS/OUT2 */

	io::read8(s.port);     /* clear recevier s.port */
	io::read8(s.port + 5); /* clear line status s.port */
	io::read8(s.port + 6); /* clear modem status s.port */

	return ERROR_SUCCESS;
}

error_t IbmPcRs232::shutdown(void)
{
	s.port = 0;
	return ERROR_SUCCESS;
}

error_t IbmPcRs232::suspend(void)
{
	return ERROR_SUCCESS;
}

error_t IbmPcRs232::restore(void)
{
	return ERROR_SUCCESS;
}

void IbmPcRs232::syphon(const utf8Char *str, uarch_t len)
{
	for (; len > 0; len--)
	{
		while (s.port && !(io::read8(s.port + 5) & 0x20));
		io::write8(s.port, *str++);
	};
}

static utf8Char	*clearMsg = CC"\n\t== End of last burst, screen clearing. ==\n";

void IbmPcRs232::clear(void)
{
	syphon(clearMsg, strlen8(clearMsg));
}

sarch_t IbmPcRs232::isInitialized(void)
{
	return s.port != 0;
}
