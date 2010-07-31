
#include <__kstdlib/utf8.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>


#define DEBUGPIPE_TEST_AND_INITIALIZE(__bp,__db,__di,__rp,__m,__e)	\
	if (__KFLAG_TEST(__bp, __di)) \
	{ \
		__rp = firmwareTrib.__m(); \
		if(__rp != __KNULL) \
		{ \
			__e = (*__rp->initialize)(); \
			if (__e == ERROR_SUCCESS) \
			{ \
				devices.lock.acquire(); \
				__KFLAG_SET(__db, __di); \
				devices.lock.release(); \
			}; \
		}; \
	}

#define DEBUGPIPE_TEST_AND_SHUTDOWN(__dp,__db,__di,__m,__e)		\
	if (__KFLAG_TEST(__dp, __di)) \
	{ \
		__e = (*firmwareTrib.__m()->shutdown)(); \
		if (__e == ERROR_SUCCESS) \
		{ \
			devices.lock.acquire(); \
			__KFLAG_UNSET(__db, __di); \
			devices.lock.release(); \
		}; \
	}

#define DEBUGPIPE_TEST_AND_SYPHON(__db,__di,__m,__tcb,__bl)		\
	if (__KFLAG_TEST(__db, __di)) { \
		(*firmwareTrib.__m()->syphon)(__tcb,__bl); \
	}

#define DEBUGPIPE_TEST_AND_CLEAR(__db,__di,__m)				\
	if (__KFLAG_TEST(__db, __di)) { \
		(*firmwareTrib.__m()->clear)(); \
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
	untieFrom(
		DEBUGPIPE_DEVICE1 | DEBUGPIPE_DEVICE2 | DEBUGPIPE_DEVICE3
		| DEBUGPIPE_DEVICE4);
}

uarch_t debugPipeC::tieTo(uarch_t device)
{
	debugSupportRivS	*riv;
	error_t			err;
	uarch_t			ret;

	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE_BUFFER))
	{
		if (debugBuff.initialize() == ERROR_SUCCESS)
		{
			devices.lock.acquire();
			__KFLAG_SET(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER);
			devices.lock.release();
		};
	};

	DEBUGPIPE_TEST_AND_INITIALIZE(
		device, devices.rsrc, DEBUGPIPE_DEVICE1, riv,
		getDebugSupportRiv1, err);

	DEBUGPIPE_TEST_AND_INITIALIZE(
		device, devices.rsrc, DEBUGPIPE_DEVICE2, riv,
		getDebugSupportRiv2, err);

	DEBUGPIPE_TEST_AND_INITIALIZE(
		device, devices.rsrc, DEBUGPIPE_DEVICE3, riv,
		getDebugSupportRiv3, err);

	DEBUGPIPE_TEST_AND_INITIALIZE(
		device, devices.rsrc, DEBUGPIPE_DEVICE4, riv,
		getDebugSupportRiv4, err);

	devices.lock.acquire();
	ret = devices.rsrc;
	devices.lock.release();
	return ret;
}

uarch_t debugPipeC::untieFrom(uarch_t device)
{
	uarch_t		ret;
	error_t		err;

	if (__KFLAG_TEST(device, DEBUGPIPE_DEVICE_BUFFER))
	{
		if (debugBuff.shutdown() == ERROR_SUCCESS)
		{
			devices.lock.acquire();
			__KFLAG_UNSET(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER);
			devices.lock.release();
		};
	};

	DEBUGPIPE_TEST_AND_SHUTDOWN(
		device, devices.rsrc, DEBUGPIPE_DEVICE1,
		getDebugSupportRiv1, err);

	DEBUGPIPE_TEST_AND_SHUTDOWN(
		device, devices.rsrc, DEBUGPIPE_DEVICE2,
		getDebugSupportRiv2, err);

	DEBUGPIPE_TEST_AND_SHUTDOWN(
		device, devices.rsrc, DEBUGPIPE_DEVICE3,
		getDebugSupportRiv3, err);

	DEBUGPIPE_TEST_AND_SHUTDOWN(
		device, devices.rsrc, DEBUGPIPE_DEVICE4,
		getDebugSupportRiv4, err);

	devices.lock.acquire();
	ret = devices.rsrc;
	devices.lock.release();
	return ret;
}

void debugPipeC::refresh(void)
{
	unicodePoint	*buff;
	void		*handle;
	uarch_t		len, n=0;

	// Send the buffer to all devices.
	handle = debugBuff.lock();

	DEBUGPIPE_TEST_AND_CLEAR(
		devices.rsrc, DEBUGPIPE_DEVICE1, getDebugSupportRiv1);

	DEBUGPIPE_TEST_AND_CLEAR(
		devices.rsrc, DEBUGPIPE_DEVICE2, getDebugSupportRiv2);

	DEBUGPIPE_TEST_AND_CLEAR(
		devices.rsrc, DEBUGPIPE_DEVICE3, getDebugSupportRiv3);

	DEBUGPIPE_TEST_AND_CLEAR(
		devices.rsrc, DEBUGPIPE_DEVICE4, getDebugSupportRiv4);


	for (buff = debugBuff.extract(&handle, &len); buff != __KNULL; n++)
	{
		DEBUGPIPE_TEST_AND_SYPHON(
			devices.rsrc, DEBUGPIPE_DEVICE1, getDebugSupportRiv1,
			buff, len);

		DEBUGPIPE_TEST_AND_SYPHON(
			devices.rsrc, DEBUGPIPE_DEVICE2, getDebugSupportRiv2,
			buff, len);

		DEBUGPIPE_TEST_AND_SYPHON(
			devices.rsrc, DEBUGPIPE_DEVICE3, getDebugSupportRiv3,
			buff, len);

		DEBUGPIPE_TEST_AND_SYPHON(
			devices.rsrc, DEBUGPIPE_DEVICE4, getDebugSupportRiv4,
			buff, len);

		buff = debugBuff.extract(&handle, &len);
	};
	debugBuff.unlock();
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

			// Printf format string parsing here.
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

	// Make sure not to send to the buffer if the memoryTrib is printing.
	if (!__KFLAG_TEST(flags, DEBUGPIPE_FLAGS_NOBUFF))
	{
		if (__KFLAG_TEST(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER)) {
			debugBuff.syphon(tmpBuff.rsrc, buffLen);
		};
	};

	DEBUGPIPE_TEST_AND_SYPHON(
		devices.rsrc, DEBUGPIPE_DEVICE1, getDebugSupportRiv1,
		tmpBuff.rsrc, buffLen);

	DEBUGPIPE_TEST_AND_SYPHON(
		devices.rsrc, DEBUGPIPE_DEVICE2, getDebugSupportRiv2,
		tmpBuff.rsrc, buffLen);

	DEBUGPIPE_TEST_AND_SYPHON(
		devices.rsrc, DEBUGPIPE_DEVICE3, getDebugSupportRiv3,
		tmpBuff.rsrc, buffLen);

	DEBUGPIPE_TEST_AND_SYPHON(
		devices.rsrc, DEBUGPIPE_DEVICE4, getDebugSupportRiv4,
		tmpBuff.rsrc, buffLen);

	tmpBuff.lock.release();
}

