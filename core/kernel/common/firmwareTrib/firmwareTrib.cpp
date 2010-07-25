
#include <kernel/common/firmwareTrib/firmwareTrib.h>
#include <kernel/common/firmwareTrib/firmwareStream.h>

firmwareTribC::firmwareTribC(void)
{
}

error_t firmwareTribC::initialize(void)
{
	if (chipsetFwStream.terminalFwRiv != __KNULL) {
		descriptor.terminalFwRiv = chipsetFwStream.terminalFwRiv;
	}
/*	else {
		descriptor.terminalFwRiv = firmwareFwStream.terminalFwRiv;
	}*/;

	return ERROR_SUCCESS;
}

firmwareTribC::~firmwareTribC(void)
{
}

terminalFwRivS *firmwareTribC::getTerminalFwRiv(void)
{
	return descriptor.terminalFwRiv;
}

