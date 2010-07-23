
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>

debugPipeC::debugPipeC(void)
{
	buff = __KNULL;
	devices = 0;
}

error_t debugPipeC::intialize(void)
{
	buff = (memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(1);

	if (buff == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	for (uarch_t i=0; i<BUFFPAGE_MAX_NCHARS; i++) {
		buff->data[i] = 0;
	};

	if (tieTo(DEBUGPIPE_DEVICE_BUFFER) != ERROR_SUCCESS) {
		return ERROR_UNKNOWN;
	};

	return ERROR_SUCCESS;
}

error_t debugPipeC::tieTo(uarch_t device)
{
	// FIXME: Should check the device's state first.
	devices |= device;
}

error_t debugPipeC::untieFrom(uarch_t device)
{
	// FIXME: Should de-init the device.
	devices &= ~(device);
}

void debugPipeC::refresh(void)
{
	tmp = 
	for (
