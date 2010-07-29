
#include <__kstdlib/__kcxxlib/cstring>
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
	memset(this, 0, sizeof(*this));
}

error_t firmwareTribC::initialize(void)
{
	FIRMWARETRIB_CHOOSE(watchdogFwRiv);
	FIRMWARETRIB_CHOOSE(debugSupportRiv1);
	FIRMWARETRIB_CHOOSE(debugSupportRiv2);
	FIRMWARETRIB_CHOOSE(debugSupportRiv3);
	FIRMWARETRIB_CHOOSE(debugSupportRiv4);
	return ERROR_SUCCESS;
}

firmwareTribC::~firmwareTribC(void)
{
}

debugSupportRivS *firmwareTribC::getDebugSupportRiv1(void)
{
	return descriptor.debugSupportRiv1;
}

debugSupportRivS *firmwareTribC::getDebugSupportRiv2(void)
{
	return descriptor.debugSupportRiv2;
}

debugSupportRivS *firmwareTribC::getDebugSupportRiv3(void)
{
	return descriptor.debugSupportRiv3;
}

debugSupportRivS *firmwareTribC::getDebugSupportRiv4(void)
{
	return descriptor.debugSupportRiv4;
}

watchdogFwRivS *firmwareTribC::getWatchdogFwRiv(void)
{
	return descriptor.watchdogFwRiv;
}

