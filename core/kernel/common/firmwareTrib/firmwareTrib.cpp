
#include <__kstdlib/__kclib/string.h>
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
	memset(this, 0, sizeof(*this));

	FIRMWARETRIB_CHOOSE(debugRiv1);
	FIRMWARETRIB_CHOOSE(debugRiv2);
	FIRMWARETRIB_CHOOSE(debugRiv3);
	FIRMWARETRIB_CHOOSE(debugRiv4);
	return ERROR_SUCCESS;
}

firmwareTribC::~firmwareTribC(void)
{
}

firmwareStreamS *firmwareTribC::getChipsetStream(void)
{
	return &chipsetFwStream;
}

firmwareStreamS *firmwareTribC::getFirmwareStream(void)
{
	return &firmwareFwStream;
}

debugRivS *firmwareTribC::getDebugRiv1(void)
{
	return descriptor.debugRiv1;
}

debugRivS *firmwareTribC::getDebugRiv2(void)
{
	return descriptor.debugRiv2;
}

debugRivS *firmwareTribC::getDebugRiv3(void)
{
	return descriptor.debugRiv3;
}

debugRivS *firmwareTribC::getDebugRiv4(void)
{
	return descriptor.debugRiv4;
}

