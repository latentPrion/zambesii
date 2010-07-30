
#include <__kstdlib/utf8.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>

#define DEBUGPIPE_TEST_AND_SEND(__b,__d,__m,__tb)				\
	if (__KFLAG_TEST(__b, __d)) { \
		(*firmwareTrib.__m()->read)(__tb); \
	}

debugPipeC::debugPipeC(void)
{
}

error_t debugPipeC::initialize(void)
{
	uarch_t		bound;
	unicodePoint	*mem;

	devices.rsrc = 0;
	// Allocate four pages for UTF-8 expansion buffer. That's 4096 codepts.
	mem = new ((memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(
			DEBUGPIPE_CONVERSION_BUFF_NPAGES)) unicodePoint;

	if (mem == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	bound = (PAGING_BASE_SIZE * DEBUGPIPE_CONVERSION_BUFF_NPAGES)
		/ sizeof(unicodePoint);

	tmpBuff.lock.acquire();

	tmpBuff.rsrc = mem;
	for (uarch_t i=0; i<bound; i++) {
		tmpBuff.rsrc[i] = 0;
	};

	tmpBuff.lock.release();
	return ERROR_SUCCESS;
}

debugPipeC::~debugPipeC(void)
{
	untieFrom(DEBUGPIPE_DEVICE_BUFFER);
	untieFrom(DEBUGPIPE_DEVICE1);
	untieFrom(DEBUGPIPE_DEVICE2);
	untieFrom(DEBUGPIPE_DEVICE3);
	untieFrom(DEBUGPIPE_DEVICE4);
}

uarch_t debugPipeC::tieTo(uarch_t device)
{
	void		*riv;
	error_t		result;
	uarch_t		ret;

	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE_BUFFER))
	{
		if (debugBuff.initialize() == ERROR_SUCCESS)
		{
			devices.lock.acquire();
			__KFLAG_SET(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER);
			devices.lock.release();
		};
	};

	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE1))
	{
		riv = firmwareTrib.getDebugSupportRiv1();
		if (riv != __KNULL)
		{
			result = (*static_cast<debugSupportRivS *>( riv )
				->initialize)();

			if (result == ERROR_SUCCESS)
			{
				devices.lock.acquire();
				__KFLAG_SET(devices.rsrc, DEBUGPIPE_DEVICE1);
				devices.lock.release();
			};
		};
	};
	devices.lock.acquire();
	ret = devices.rsrc;
	devices.lock.release();
	return ret;
}

uarch_t debugPipeC::untieFrom(uarch_t device)
{
	uarch_t		ret;

	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE_BUFFER))
	{
		devices.lock.acquire();
		__KFLAG_UNSET(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER);
		devices.lock.release();
	};
	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE1))
	{
		devices.lock.acquire();
		__KFLAG_UNSET(devices.rsrc, DEBUGPIPE_DEVICE1);
		devices.lock.release();
	};
	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE2))
	{
		devices.lock.acquire();
		__KFLAG_UNSET(devices.rsrc, DEBUGPIPE_DEVICE2);
		devices.lock.release();
	};
	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE3))
	{
		devices.lock.acquire();
		__KFLAG_UNSET(devices.rsrc, DEBUGPIPE_DEVICE3);
		devices.lock.release();
	};
	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE4))
	{
		devices.lock.acquire();
		__KFLAG_UNSET(devices.rsrc, DEBUGPIPE_DEVICE4);
		devices.lock.release();
	};

	devices.lock.acquire();
	ret = devices.rsrc;
	devices.lock.release();
	return ret;
}

void debugPipeC::refresh(void)
{
}

void debugPipeC::printf(const utf8Char *str, uarch_t flags, ...)
{
	uarch_t		buffLen=0, buffMax;

	buffMax = (PAGING_BASE_SIZE * DEBUGPIPE_CONVERSION_BUFF_NPAGES)
		/ sizeof(unicodePoint);

	tmpBuff.lock.acquire();

	if (tmpBuff.rsrc == __KNULL)
	{
		tmpBuff.lock.release();
		return;
	};

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

	DEBUGPIPE_TEST_AND_SEND(
		devices.rsrc, DEBUGPIPE_DEVICE1, getDebugSupportRiv1,
		tmpBuff.rsrc);

	DEBUGPIPE_TEST_AND_SEND(
		devices.rsrc, DEBUGPIPE_DEVICE2, getDebugSupportRiv2,
		tmpBuff.rsrc);

	DEBUGPIPE_TEST_AND_SEND(
		devices.rsrc, DEBUGPIPE_DEVICE3, getDebugSupportRiv3,
		tmpBuff.rsrc);

	DEBUGPIPE_TEST_AND_SEND(
		devices.rsrc, DEBUGPIPE_DEVICE4, getDebugSupportRiv4,
		tmpBuff.rsrc);

	tmpBuff.lock.release();
}

