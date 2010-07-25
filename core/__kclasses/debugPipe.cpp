
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>

debugPipeC::debugPipeC(void)
{
	devices.rsrc = 0;
}

error_t debugPipeC::initialize(void)
{
	uarch_t		bound;

	tmpBuff.rsrc = new ((memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(1)) utf16Char;

	if (tmpBuff.rsrc == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	bound = PAGING_BASE_SIZE / sizeof(utf16Char);
	for (uarch_t i=0; i<bound; i++) {
		tmpBuff.rsrc[i] = 0;
	};
	return ERROR_SUCCESS;
}

debugPipeC::~debugPipeC(void)
{
	untieFrom(DEBUGPIPE_DEVICE_BUFFER);
	untieFrom(DEBUGPIPE_DEVICE_TERMINAL);
	untieFrom(DEBUGPIPE_DEVICE_SERIAL);
	untieFrom(DEBUGPIPE_DEVICE_PARALLEL);
	untieFrom(DEBUGPIPE_DEVICE_NIC);
}

error_t debugPipeC::tieTo(uarch_t device)
{
	void		*riv;
	error_t		ret;

/*	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE_BUFFER))
	{
		ret = buffer.initialize();
		if (ret == ERROR_SUCCESS) {
			__KFLAG_SET(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER);
		};
		return ret;
	};
*/
	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE_TERMINAL))
	{
		riv = firmwareTrib.getTerminalFwRiv();
		if (riv != __KNULL)
		{
			ret = (*static_cast<terminalFwRivS *>( riv )
				->initialize)();

			if (ret == ERROR_SUCCESS) {
				__KFLAG_SET(devices.rsrc, DEBUGPIPE_DEVICE_TERMINAL);
			};
			return ret;
		};
		return ERROR_UNKNOWN;
	};
	return ERROR_INVALID_ARG_VAL;
}

error_t debugPipeC::untieFrom(uarch_t device)
{
/*	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE_BUFFER))
	{
		__KFLAG_UNSET(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER);
		return buffer.shutdown();
	};
*/	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE_TERMINAL))
	{
		__KFLAG_UNSET(devices.rsrc, DEBUGPIPE_DEVICE_TERMINAL);
		return (*firmwareTrib.getTerminalFwRiv()->shutdown)();
	};
	return ERROR_INVALID_ARG_VAL;	
}

void debugPipeC::refresh(void)
{
}

void debugPipeC::printf(const utf16Char *str, ...)
{
	// Code to process the printf format string here.

	// Send the processed output to all tied devices.rsrc.
/*	if (__KFLAG_TEST(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER)) {
		buffer.read(str);
	}; */
	if (__KFLAG_TEST(devices.rsrc, DEBUGPIPE_DEVICE_TERMINAL)) {
		(*firmwareTrib.getTerminalFwRiv()->read)(str);
	};
}

