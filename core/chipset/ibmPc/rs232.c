
#include <__kstdlib/__ktypes.h>
#include <kernel/common/firmwareTrib/firmwareStream.h>
#include <kernel/common/firmwareTrib/rivIoApi.h>

uarch_t		initialized = 0;

static error_t ibmPc_rs232_initialize(void)
{
	initialized = 1;
	return ERROR_SUCCESS;
}

static error_t ibmPc_rs232_shutdown(void)
{
	initialized = 0;
	return ERROR_SUCCESS;
}

static error_t ibmPc_rs232_suspend(void)
{
	initialized = 0;
	return ERROR_SUCCESS;
}

static error_t ibmPc_rs232_awake(void)
{
	initialized = 1;
	return ERROR_SUCCESS;
}

static void ibmPc_rs232_syphon(unicodePoint *str, uarch_t len)
{
	for (; len > 0; len--)
	{
		if (*str == '\n') {
			io_write8(0x3F8, '\r');
		};

		io_write8(0x3F8, *str++);
	};
}

static char	*clearMsg = "\n\t=== End of last burst, screen clearing.===\n";

static void ibmPc_rs232_clear(void)
{
	ibmPc_rs232_syphon((unicodePoint *)clearMsg, 45);
}

static sarch_t ibmPc_rs232_isInitialized(void)
{
	return (initialized);
}

struct debugRivS	ibmPc_rs232 =
{
	&ibmPc_rs232_initialize,
	&ibmPc_rs232_shutdown,
	&ibmPc_rs232_suspend,
	&ibmPc_rs232_awake,
	&ibmPc_rs232_isInitialized,

	&ibmPc_rs232_syphon,
	&ibmPc_rs232_clear
};

