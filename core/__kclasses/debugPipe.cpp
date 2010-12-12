
#include <arch/arch.h>
#include <chipset/pkg/chipsetPackage.h>
#include <__kstdlib/utf8.h>
#include <__kstdlib/utf16.h>
#include <__kstdlib/__kbitManipulation.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/stdarg.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


#define DEBUGPIPE_FLAGS_NOLOG		(1<<0)

debugPipeC::debugPipeC(void)
{
	convBuff.rsrc = __KNULL;
	devices.rsrc = 0;
}

error_t debugPipeC::initialize(void)
{
	uarch_t		bound;
	utf8Char	*mem;

	devices.rsrc = 0;
	// Allocate four pages for UTF-8 expansion buffer.
	mem = new ((memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(
			DEBUGPIPE_CONVERSION_BUFF_NPAGES, MEMALLOC_NO_FAKEMAP))
			utf8Char;

	if (mem == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	bound = (PAGING_BASE_SIZE * DEBUGPIPE_CONVERSION_BUFF_NPAGES)
		/ sizeof(utf8Char);

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
	debugModS	*mod;
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
	for (ubit8 i=0; i<4; i++)
	{
		if (__KBIT_TEST(device, i))
		{
			mod = chipsetPkg.debug[i];
			if (mod != __KNULL)
			{
				err = mod->initialize();
				if (err == ERROR_SUCCESS) {
					__KBIT_SET(devices.rsrc, i);
				};
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

	for (ubit8 i=0; i<4; i++)
	{
		if (__KFLAG_TEST(device, (1<<i)))
		{
			if (!__KFLAG_TEST(devices.rsrc, (1<<i))) {
				continue;
			};

			// Assume that mod exists if its bit is set.
			err = chipsetPkg.debug[i]->shutdown();
			if (err == ERROR_SUCCESS) {
				__KFLAG_UNSET(devices.rsrc, (1<<i));
			}
			else {
				// Find some way to warn the user.
			};
		};
	};

	devices.lock.acquire();
	ret = devices.rsrc;
	devices.lock.release();
	return ret;
}

void debugPipeC::refresh(void)
{
	utf8Char	*buff;
	void		*handle;
	uarch_t		len;
	

	// Clear "screen" on each device, whatever that means to that device.
	for (ubit8 i=0; i<4; i++)
	{
		if (__KFLAG_TEST(devices.rsrc, (1<<i))) {
			chipsetPkg.debug[i]->clear();
		};
	};

	handle = debugBuff.lock();

	for (buff = debugBuff.extract(&handle, &len); buff != __KNULL; )
	{
		for (ubit8 i=0; i<4; i++)
		{
			if (__KFLAG_TEST(devices.rsrc, (1<<i))) {
				chipsetPkg.debug[i]->syphon(buff, len);
			};
		};

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
		convBuff.rsrc[*curLen] = '-';
		*curLen += 1;
		num -= 1;
		num = ~num;
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

void __kprintf(const utf8Char *str, ...)
{
	va_list		args;

	va_start_forward(args, str);
	__kdebug.printf(str, args);
	va_end(args);
}

void debugPipeC::processPrintfFormatting(
	const utf8Char *str, va_list args,
	uarch_t buffMax, uarch_t *buffLen, uarch_t *printfFlags
	)
{
	uarch_t		unum;
	sarch_t		snum;
	paddr_t		pnum;
	utf8Char	*u8Str;

	for (; (*str != '\0') && (*buffLen < buffMax); str++)
	{
		if (*str == '%')
		{
			str++;
			switch (*str)
			{
			case '%':
				convBuff.rsrc[*buffLen] = *str;
				*buffLen += 1;
				break;

			case 'd':
			case 'i':
				snum = va_arg(args, sarch_t);
				signedToStr(snum, buffLen);

				break;

			case 'u':
				unum = va_arg(args, uarch_t);
				unsignedToStr(unum, buffLen);

				break;

			case 'X':
				unum = va_arg(args, uarch_t);
				numToStrHexUpper(unum, buffLen);
				break;

			case 'P':
				pnum = va_arg(args, paddr_t);
				paddrToStrHex(pnum, buffLen);
				break;

			case 'x':
			case 'p':
				unum = va_arg(args, uarch_t);
				numToStrHexLower(unum, buffLen);

				break;

			case 's':
				u8Str = va_arg(args, utf8Char *);
				processPrintfFormatting(
					u8Str, args,
					buffMax, buffLen, printfFlags);

				break;

			case '[':
				str++;
				for (; *str != ']'; str++)
				{
					switch (*str)
					{
					case 'n':
						__KFLAG_SET(
							*printfFlags,
							DEBUGPIPE_FLAGS_NOLOG);
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
		else
		{
			convBuff.rsrc[*buffLen] = *str;
			*buffLen += 1;
		};
	};
}

void debugPipeC::printf(const utf8Char *str, va_list args)
{
	uarch_t		buffLen=0, buffMax;
	uarch_t		printfFlags=0;

	convBuff.lock.acquire();

	// Make sure we're not printing to an unallocated buffer.
	if (convBuff.rsrc == __KNULL)
	{
		convBuff.lock.release();
		return;
	};

	buffMax = (PAGING_BASE_SIZE * DEBUGPIPE_CONVERSION_BUFF_NPAGES)
		/ sizeof(utf8Char);

	// Expand printf formatting into convBuff.
	processPrintfFormatting(str, args, buffMax, &buffLen, &printfFlags);

	if (__KFLAG_TEST(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER)
		&& !__KFLAG_TEST(printfFlags, DEBUGPIPE_FLAGS_NOLOG))
	{
		debugBuff.syphon(convBuff.rsrc, buffLen);
	};

	for (ubit8 i=0; i<4; i++)
	{
		if (__KFLAG_TEST(devices.rsrc, (1<<i))) {
			chipsetPkg.debug[i]->syphon(convBuff.rsrc, buffLen);
		};
	};

	convBuff.lock.release();
}

#if 0
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
#endif

