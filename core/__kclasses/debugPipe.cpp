
#include <__kstdlib/utf8.h>
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

	// Allocate four pages for UTF-8 expansion buffer. That's 4096 codepts.
	tmpBuff.rsrc = new ((memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(
			DEBUGPIPE_CONVERSION_BUFF_NPAGES)) unicodePoint;

	if (tmpBuff.rsrc == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	bound = (PAGING_BASE_SIZE * DEBUGPIPE_CONVERSION_BUFF_NPAGES)
		/ sizeof(unicodePoint);

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

	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE_BUFFER))
	{
		ret = debugBuff.initialize();
		if (ret == ERROR_SUCCESS) {
			__KFLAG_SET(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER);
		};
		return ret;
	};

	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE_TERMINAL))
	{
		riv = firmwareTrib.getTerminalFwRiv();
		if (riv != __KNULL)
		{
			ret = (*static_cast<terminalFwRivS *>( riv )
				->initialize)();

			if (ret == ERROR_SUCCESS) {
				__KFLAG_SET(devices.rsrc,
					DEBUGPIPE_DEVICE_TERMINAL);
			};
			return ret;
		};
		return ERROR_UNKNOWN;
	};
	return ERROR_INVALID_ARG_VAL;
}

error_t debugPipeC::untieFrom(uarch_t device)
{
	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE_BUFFER))
	{
		__KFLAG_UNSET(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER);
		return debugBuff.shutdown();
	};
	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE_TERMINAL))
	{
		__KFLAG_UNSET(devices.rsrc, DEBUGPIPE_DEVICE_TERMINAL);
		return (*firmwareTrib.getTerminalFwRiv()->shutdown)();
	};
	return ERROR_INVALID_ARG_VAL;	
}

void debugPipeC::refresh(void)
{
}

void debugPipeC::printf(const utf8Char *str, uarch_t flags, ...)
{
	uarch_t		buffLen=0, buffMax;

	buffMax = (PAGING_BASE_SIZE * DEBUGPIPE_CONVERSION_BUFF_NPAGES)
		/ sizeof(unicodePoint);

	// Convert the input UTF-8 into a codepoint.
	for (; (*str != 0) && (buffLen < buffMax); buffLen++, str++)
	{
		if (!(*str & 0x80)) {
			tmpBuff.rsrc[buffLen] = *str;
		}
		else
		{
			if ((*str & 0xE0) == 0xC0)
			{
				tmpBuff.rsrc[buffLen] = utf8::parse2(&str);
				continue;
			};
			if ((*str & 0xF0) == 0xE0)
			{
				tmpBuff.rsrc[buffLen] = utf8::parse3(&str);
				continue;
			};
			if ((*str & 0xF8) == 0xF0)
			{
				tmpBuff.rsrc[buffLen] = utf8::parse4(&str);
				continue;
			};
		};
	};
	tmpBuff.rsrc[buffLen] = 0;

	// Make sure not to send to the buffer if the memoryTrib is printing.
	if (!__KFLAG_TEST(flags, DEBUGPIPE_FLAGS_NOBUFF))
	{
		if (__KFLAG_TEST(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER)) {
			debugBuff.read(tmpBuff.rsrc, buffLen);
		};
	};

	if (__KFLAG_TEST(devices.rsrc, DEBUGPIPE_DEVICE_TERMINAL)) {
		(*firmwareTrib.getTerminalFwRiv()->read)(tmpBuff.rsrc);
	};
}

