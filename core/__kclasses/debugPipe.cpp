
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

uarch_t debugPipeC::jik(uarch_t bufflen)
{
	return bufflen;
}

void debugPipeC::printf(const utf8Char *str, ...)
{
	uarch_t		buffLen=0, buffMax;

	buffMax = (PAGING_BASE_SIZE * DEBUGPIPE_CONVERSION_BUFF_NPAGES)
		/ sizeof(unicodePoint);

	// Convert the input UTF-8 into a codepoint.
	for (; (*str != 0) && (buffLen < buffMax); buffLen++, str++)
	{
		// Byte 1.
		tmpBuff.rsrc[buffLen] = *str & 0x7F;
		// Is this a multibyte sequence?
		if (*str & 0x80)
		{
			str++;
			// Get the bits from byte 2.
			tmpBuff.rsrc[buffLen] |= (*str & 0x3F) << 7;
			// Does it continue?
			if ((*str & 0xC0) == 0x80)
			{
				str++;
				// Get the bits from byte 3.
				tmpBuff.rsrc[buffLen] |= (*str & 0x3F) << 14;
				// More bytes yet?
				if ((*str & 0xC0) == 0x80)
				{
					str++;
					// Get the bits from byte 4.
					tmpBuff.rsrc[buffLen] |= (*str & 0x3F)
						<< 20;
				};
			};
		};
	};
	tmpBuff.rsrc[buffLen] = 0;

	// At this point the codepoints are in the buffer, completely expanded.

	// Send the processed output to all tied devices.rsrc.
/*	if (__KFLAG_TEST(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER)) {
		buffer.read(tmpBuff.rsrc);
	}; */
	if (__KFLAG_TEST(devices.rsrc, DEBUGPIPE_DEVICE_TERMINAL)) {
		(*firmwareTrib.getTerminalFwRiv()->read)(tmpBuff.rsrc);
	};
}

