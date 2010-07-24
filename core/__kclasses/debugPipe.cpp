
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>

debugPipeC::debugPipeC(void)
{
	devices = 0;
}

error_t debugPipeC::initialize(uarch_t device)
{
	return tieTo(device);
}

debugPipeC::~debugPipeC(void)
{
	untieFrom(
		DEBUGPIPE_DEVICE_BUFFER | DEBUGPIPE_DEVICE_TERMINAL
		| DEBUGPIPE_DEVICE_SERIAL | DEBUGPIPE_DEVICE_PARALLEL
		| DEBUGPIPE_DEVICE_NIC);
}

error_t debugPipeC::tieTo(uarch_t device)
{
	// FIXME: Should check the device's state first.
	devices |= device;
	return ERROR_SUCCESS;
}

error_t debugPipeC::untieFrom(uarch_t device)
{
	// FIXME: Should de-init the device.
	devices &= ~(device);
	return ERROR_SUCCESS;
}

void debugPipeC::refresh(void)
{
}

void debugPipeC::printf(utf16Char *str, ...)
{
	// Code to process the printf format string here.

	// Send the processed output to all tied devices.
	if (__KFLAG_TEST(devices, DEBUGPIPE_DEVICE_BUFFER)) {};
	if (__KFLAG_TEST(devices, DEBUGPIPE_DEVICE_TERMINAL)) {};
	if (__KFLAG_TEST(devices, DEBUGPIPE_DEVICE_SERIAL)) {};
	if (__KFLAG_TEST(devices, DEBUGPIPE_DEVICE_PARALLEL)) {};
	if (__KFLAG_TEST(devices, DEBUGPIPE_DEVICE_NIC)) {};
}

