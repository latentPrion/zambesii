
#include <arch/arch.h>
#include <__kstdlib/utf8.h>
#include <__kstdlib/utf16.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/stdarg.h>
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
	convBuff.rsrc = __KNULL;
	devices.rsrc = 0;
}

error_t debugPipeC::initialize(void)
{
	uarch_t		bound;
	utf16Char	*mem;

	devices.rsrc = 0;
	// Allocate four pages for UTF-8 expansion buffer. That's 4096 codepts.
	mem = new ((memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(
			DEBUGPIPE_CONVERSION_BUFF_NPAGES, MEMALLOC_NO_FAKEMAP))
			utf16Char;

	if (mem == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	bound = (PAGING_BASE_SIZE * DEBUGPIPE_CONVERSION_BUFF_NPAGES)
		/ sizeof(utf16Char);

	convBuff.lock.acquire();

	convBuff.rsrc = mem;
	for (uarch_t i=0; i<bound; i++) {
		convBuff.rsrc[i] = 0;
	};

	convBuff.lock.release();
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
	debugRivS	*riv;
	error_t		err;
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

	DEBUGPIPE_TEST_AND_INITIALIZE(
		device, devices.rsrc, DEBUGPIPE_DEVICE1, riv,
		getDebugRiv1, err);

	DEBUGPIPE_TEST_AND_INITIALIZE(
		device, devices.rsrc, DEBUGPIPE_DEVICE2, riv,
		getDebugRiv2, err);

	DEBUGPIPE_TEST_AND_INITIALIZE(
		device, devices.rsrc, DEBUGPIPE_DEVICE3, riv,
		getDebugRiv3, err);

	DEBUGPIPE_TEST_AND_INITIALIZE(
		device, devices.rsrc, DEBUGPIPE_DEVICE4, riv,
		getDebugRiv4, err);

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
		getDebugRiv1, err);

	DEBUGPIPE_TEST_AND_SHUTDOWN(
		device, devices.rsrc, DEBUGPIPE_DEVICE2,
		getDebugRiv2, err);

	DEBUGPIPE_TEST_AND_SHUTDOWN(
		device, devices.rsrc, DEBUGPIPE_DEVICE3,
		getDebugRiv3, err);

	DEBUGPIPE_TEST_AND_SHUTDOWN(
		device, devices.rsrc, DEBUGPIPE_DEVICE4,
		getDebugRiv4, err);

	devices.lock.acquire();
	ret = devices.rsrc;
	devices.lock.release();
	return ret;
}

void debugPipeC::refresh(void)
{
	utf16Char	*buff;
	void		*handle;
	uarch_t		len, n=0;

	// Send the buffer to all devices.
	DEBUGPIPE_TEST_AND_CLEAR(
		devices.rsrc, DEBUGPIPE_DEVICE1, getDebugRiv1);

	DEBUGPIPE_TEST_AND_CLEAR(
		devices.rsrc, DEBUGPIPE_DEVICE2, getDebugRiv2);

	DEBUGPIPE_TEST_AND_CLEAR(
		devices.rsrc, DEBUGPIPE_DEVICE3, getDebugRiv3);

	DEBUGPIPE_TEST_AND_CLEAR(
		devices.rsrc, DEBUGPIPE_DEVICE4, getDebugRiv4);

	handle = debugBuff.lock();

	for (buff = debugBuff.extract(&handle, &len); buff != __KNULL; n++)
	{
		DEBUGPIPE_TEST_AND_SYPHON(
			devices.rsrc, DEBUGPIPE_DEVICE1, getDebugRiv1,
			buff, len);

		DEBUGPIPE_TEST_AND_SYPHON(
			devices.rsrc, DEBUGPIPE_DEVICE2, getDebugRiv2,
			buff, len);

		DEBUGPIPE_TEST_AND_SYPHON(
			devices.rsrc, DEBUGPIPE_DEVICE3, getDebugRiv3,
			buff, len);

		DEBUGPIPE_TEST_AND_SYPHON(
			devices.rsrc, DEBUGPIPE_DEVICE4, getDebugRiv4,
			buff, len);

		buff = debugBuff.extract(&handle, &len);
	};
	debugBuff.unlock();
}

// Expects the lock to already be held.
void debugPipeC::unsignedToStr(uarch_t num, uarch_t *curLen)
{
	utf8Char	b[28];
	uarch_t		blen=0;

	for (; num / 10 ; blen++)
	{
		b[blen] = (num % 10) + '0';
		num /= 10;
	};
	b[blen] = (num % 10) + '0';
	blen++;

	for (; blen; blen--, *curLen += 1) {
		convBuff.rsrc[*curLen] = b[blen-1];
	};
}

// Expects the lock to already be held.
void debugPipeC::signedToStr(sarch_t num, uarch_t *curLen)
{
	utf8Char	b[28];
	uarch_t		blen=0;

	if (num & 1<<((__BITS_PER_BYTE__ * sizeof(uarch_t)) - 1))
	{
		num &= ~(1<<((__BITS_PER_BYTE__ * sizeof(uarch_t)) - 1) );
		convBuff.rsrc[*curLen] = '-';
		*curLen += 1;
	};

	for (; num / 10 ; blen++)
	{
		b[blen] = (num % 10) + '0';
		num /= 10;
	};
	b[blen] = (num % 10) + '0';
	blen++;

	for (; blen; blen--, *curLen += 1) {
		convBuff.rsrc[*curLen] = b[blen-1];
	};
}

// Expects the lock to already be held.
void debugPipeC::numToStrHexUpper(uarch_t num, uarch_t *curLen)
{
	utf8Char	b[28];
	uarch_t		blen=0;

	for (; num >> 4 ; blen++)
	{
		b[blen] = (num & 0xF) + (((num & 0xF) > 9) ? ('A'-10):'0');
		num >>= 4;
	};
	b[blen] = (num & 0xF) + (((num & 0xF) > 9) ? ('A'-10):'0');
	blen++;

	for (; blen; blen--, *curLen += 1) {
		convBuff.rsrc[*curLen] = b[blen-1];
	};
}

// Expects the lock to already be held.
void debugPipeC::numToStrHexLower(uarch_t num, uarch_t *curLen)
{
	utf8Char	b[28];
	uarch_t		blen=0;

	for (; num >> 4 ; blen++)
	{
		b[blen] = (num & 0xF) + (((num & 0xF) > 9) ? ('a'-10):'0');
		num >>= 4;
	};
	b[blen] = (num & 0xF) + (((num & 0xF) > 9) ? ('a'-10):'0');
	blen++;

	for (; blen; blen--, *curLen += 1) {
		convBuff.rsrc[*curLen] = b[blen-1];
	};
}

void debugPipeC::paddrToStrHex(paddr_t num, uarch_t *curLen)
{
	utf8Char	b[28];
	uarch_t		blen=0;

	for (; num >> 4 ; blen++)
	{
		b[blen] = (num & 0xF) + (((num & 0xF) > 9) ? ('A'-10):'0');
		num >>= 4;
	};
	b[blen] = (num & 0xF) + (((num & 0xF) > 9) ? ('A'-10):'0');
	blen++;

	for (; blen; blen--, *curLen += 1) {
		convBuff.rsrc[*curLen] = b[blen-1];
	};
}

void debugPipeC::blitUtf8(utf8Char *str, uarch_t *curLen)
{
	for (; *str != 0; *curLen += 1, str++) {
		convBuff.rsrc[*curLen] = *str;
	};
}

void debugPipeC::blitUtf16(utf16Char *str, uarch_t *curLen)
{
	for (; *str != 0; *curLen += 1, str++) {
		convBuff.rsrc[*curLen] = *str;
	};
}

void __kprintf(const utf8Char *str, ...)
{
	va_list		args;

	va_start_forward(args, str);
	__kdebug.printf(str, args);
	va_end(args);
}

void debugPipeC::printf(const utf8Char *str, va_list args)
{
	uarch_t		unum, buffLen=0, buffMax;
	sarch_t		snum;
	paddr_t		pnum;
	unicodePoint	c=0;
	utf8Char	*u8Str;
	utf16Char	h, l;
	ubit8		nolog=0;

	convBuff.lock.acquire();

	// Make sure we're not printing to an unallocated buffer.
	if (convBuff.rsrc == __KNULL)
	{
		convBuff.lock.release();
		return;
	};

	buffMax = (PAGING_BASE_SIZE * DEBUGPIPE_CONVERSION_BUFF_NPAGES)
		/ sizeof(utf16Char);

	// Expand the string of UTF-8. Process printf formatting.
	for (; (*str != 0) && (buffLen < buffMax); str++)
	{
		if (!(*str & 0x80))
		{
			if (*str == '%')
			{
				str++;
				switch (*str)
				{
				case '%':
					convBuff.rsrc[buffLen++] = *str;
					break;

				case 'd':
				case 'i':
					snum = va_arg(args, sarch_t);
					signedToStr(snum, &buffLen);

					break;

				case 'u':
					unum = va_arg(args, uarch_t);
					unsignedToStr(unum, &buffLen);

					break;

				case 'X':
					unum = va_arg(args, uarch_t);
					numToStrHexUpper(unum, &buffLen);
					break;

				case 'P':
					pnum = va_arg(args, paddr_t);
					paddrToStrHex(pnum, &buffLen);
					break;

				case 'x':
				case 'p':
					unum = va_arg(args, uarch_t);
					numToStrHexLower(unum, &buffLen);

					break;

				case 's':
					u8Str = va_arg(args, utf8Char *);
					blitUtf8(u8Str, &buffLen);
					break;

				case '[':
					str++;
					for (; *str != ']'; str++)
					{
						switch (*str)
						{
						case 'n':
							nolog = 1;
							break;

						default:
							break;
						};
					};
					break;

				default:
					unum = va_arg(args, uarch_t);
					break;
				};	
			}
			else {
				convBuff.rsrc[buffLen++] = *str;
			};
		}
		else
		{
			if ((*str & 0xE0) == 0xC0)
			{
				c = utf8::toCodepoint2(str);
				str++;
				goto blitToBuff;
			};
			if ((*str & 0xF0) == 0xE0)
			{
				c = utf8::toCodepoint3(str);
				str = &str[2];
				goto blitToBuff;
			};
			if ((*str & 0xF8) == 0xF0)
			{
				c = utf8::toCodepoint4(str);
				str = &str[3];
				goto blitToBuff;
			};

blitToBuff:
			if (c > 0xFFFF)
			{
				utf16::toUtf16(c, &h, &l);
				convBuff.rsrc[buffLen++] = h;
				convBuff.rsrc[buffLen++] = l;
			}
			else {
				convBuff.rsrc[buffLen++] = c;
			};
		};
	};

	if (__KFLAG_TEST(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER) && !nolog) {
		debugBuff.syphon(convBuff.rsrc, buffLen);
	};

	DEBUGPIPE_TEST_AND_SYPHON(
		devices.rsrc, DEBUGPIPE_DEVICE1, getDebugRiv1,
		convBuff.rsrc, buffLen);

	DEBUGPIPE_TEST_AND_SYPHON(
		devices.rsrc, DEBUGPIPE_DEVICE2, getDebugRiv2,
		convBuff.rsrc, buffLen);

	DEBUGPIPE_TEST_AND_SYPHON(
		devices.rsrc, DEBUGPIPE_DEVICE3, getDebugRiv3,
		convBuff.rsrc, buffLen);

	DEBUGPIPE_TEST_AND_SYPHON(
		devices.rsrc, DEBUGPIPE_DEVICE4, getDebugRiv4,
		convBuff.rsrc, buffLen);

	convBuff.lock.release();
}

