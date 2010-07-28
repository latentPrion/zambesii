
#include <kernel/common/firmwareTrib/firmwareTrib.h>
#include <kernel/common/firmwareTrib/firmwareStream.h>

#define FIRMWARETRIB_CHOOSE(__riv)					\
	if (chipsetFwStream.__riv != __KNULL) { \
		descriptor.__riv = chipsetFwStream.__riv; \
	} \
	else { \
		descriptor.__riv = firmwareFwStream.__riv; \
	}

firmwareTribC::firmwareTribC(void)
{
}

error_t firmwareTribC::initialize(void)
{
	FIRMWARETRIB_CHOOSE(terminalFwRiv);
	FIRMWARETRIB_CHOOSE(watchdogFwRiv);
	return ERROR_SUCCESS;
}

firmwareTribC::~firmwareTribC(void)
{
}

terminalFwRivS *firmwareTribC::getTerminalFwRiv(void)
{
	return descriptor.terminalFwRiv;
}

watchdogFwRivS *firmwareTribC::getWatchdogFwRiv(void)
{
	return descriptor.watchdogFwRiv;
}

