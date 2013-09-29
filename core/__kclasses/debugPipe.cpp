
#include <stdarg.h>
#include <arch/arch.h>
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kbitManipulation.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/processTrib/processTrib.h>


#define DEBUGPIPE_FLAGS_NOLOG		(1<<0)

debugPipeC::debugPipeC(void)
{
	convBuff.rsrc = NULL;
	devices.rsrc = 0;
}

#ifdef CONFIG_DEBUGPIPE_STATIC
static ubit8	buffMem[DEBUGPIPE_CONVERSION_BUFF_NPAGES * PAGING_BASE_SIZE];
#endif

error_t debugPipeC::initialize(void)
{
	uarch_t		bound;
	utf8Char	*mem;

	devices.rsrc = 0;
	// Allocate four pages for UTF-8 expansion buffer.
#ifndef CONFIG_DEBUGPIPE_STATIC
	mem = new (processTrib.__kgetStream()->memoryStream.memAlloc(
			DEBUGPIPE_CONVERSION_BUFF_NPAGES, MEMALLOC_NO_FAKEMAP))
			utf8Char;
#else
	mem = new (buffMem) utf8Char;
#endif

	if (mem == NULL) {
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
	zkcmDebugDeviceC	*mod;
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
	for (ubit8 i=0; i<4; i++)
	{
		if (__KBIT_TEST(device, i))
		{
			mod = zkcmCore.debug[i];
			if (mod != NULL)
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
			err = zkcmCore.debug[i]->shutdown();
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
			zkcmCore.debug[i]->clear();
		};
	};

	handle = debugBuff.lock();

	for (buff = debugBuff.extract(&handle, &len); buff != NULL; )
	{
		for (ubit8 i=0; i<4; i++)
		{
			if (__KFLAG_TEST(devices.rsrc, (1<<i))) {
				zkcmCore.debug[i]->syphon(buff, len);
			};
		};

		buff = debugBuff.extract(&handle, &len);
	};
	debugBuff.unlock();
}

// Expects the lock to already be held.
void unsignedToStr(uarch_t num, uarch_t *curLen, utf8Char *buff)
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
		buff[*curLen] = b[blen-1];
	};
}

// Expects the lock to already be held.
void signedToStr(sarch_t num, uarch_t *curLen, utf8Char *buff)
{
	utf8Char	b[28];
	uarch_t		blen=0;

	if (num < 0)
	{
		buff[*curLen] = '-';
		*curLen += 1;
		num = -num;
	};

	for (; num / 10 ; blen++)
	{
		b[blen] = (num % 10) + '0';
		num /= 10;
	};
	b[blen] = (num % 10) + '0';
	blen++;

	for (; blen; blen--, *curLen += 1) {
		buff[*curLen] = b[blen-1];
	};
}

// Expects the lock to already be held.
void unsignedToStrHexUpper(uarch_t num, uarch_t *curLen, utf8Char *buff)
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
		buff[*curLen] = b[blen-1];
	};
}

// Expects the lock to already be held.
void unsignedToStrHexLower(uarch_t num, uarch_t *curLen, utf8Char *buff)
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
		buff[*curLen] = b[blen-1];
	};
}

void paddrToStrHex(paddr_t num, uarch_t *curLen, utf8Char *buff)
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
		buff[*curLen] = b[blen-1];
	};
}

void __kprintf(const utf8Char *str, ...)
{
	va_list		args;

	va_start(args, str);
	__kdebug.printf(str, args);
	va_end(args);
}

void __kprintf(
	sharedResourceGroupC<waitLockC, void *> *buff, uarch_t buffSize,
	utf8Char *str, ...
	)
{
	va_list		args;

	va_start(args, str);
	__kdebug.printf(buff, buffSize, str, args);
	va_end(args);
}

static ubit16 getNumberOfFormatArgsN(utf8Char *format, uarch_t maxLength)
{
	uarch_t		i;
	ubit16		ret=0;

	for (i=0; i < maxLength && format[i] != '\0'; i++)
	{
		if (format[i] != '%') { continue; };

		i++;
		if (format[i] == '\0') { return 0; };

		switch (format[i])
		{
		case '%':
		case '-':
			break;

		default:
			ret++;
			break;
		};
	};

	return ret;
}

static inline uarch_t handleNullPointerArg(utf8Char *buff, uarch_t maxLength)
{
	strncpy8(buff, CC"(null)", maxLength);
	return strnlen8(CC"(null)", maxLength);
}

static sarch_t expandPrintfFormatting(
	utf8Char *buff, uarch_t buffMax,
	const utf8Char *str, va_list args, uarch_t *printfFlags
	)
{
	uarch_t		buffIndex;

	/**	CAVEAT:
	 * If you ever change the format specifiers (add new ones, take some
	 * away, etc) be sure to check and update the format-specifier-counter
	 * function above so that the value it returns for a format-specifier
	 * count is consistent with the format specifiers that this function
	 * actually takes.
	 **/
	for (buffIndex=0; (*str != '\0') && (buffIndex < buffMax); str++)
	{
		uarch_t		unum;
		sarch_t		snum;
		paddr_t		pnum;
		utf8Char	*u8Str;

		if (*str != '%')
		{
			buff[buffIndex] = *str;
			buffIndex += 1;
			continue;
		};

		str++;
		// Avoid the exploit "foo %", which would cause buffer overread.
		if (*str == '\0') { return -1; };
		switch (*str)
		{
		case 'd':
			snum = va_arg(args, sarch_t);
			signedToStr(snum, &buffIndex, buff);
			break;

		case 'u':
			unum = va_arg(args, uarch_t);
			unsignedToStr(unum, &buffIndex, buff);
			break;

		case 'x':
		case 'p':
			unum = va_arg(args, uarch_t);
			unsignedToStrHexLower(unum, &buffIndex, buff);
			break;

		case 'X':
			unum = va_arg(args, uarch_t);
			unsignedToStrHexUpper(unum, &buffIndex, buff);
			break;

		case 'P':
			pnum = va_arg(args, paddr_t);
			paddrToStrHex(pnum, &buffIndex, buff);
			break;

		// Non recursive string printing.
		case 's':
			uarch_t		u8Strlen;

			u8Str = va_arg(args, utf8Char *);

			// Handle NULL pointers passed as string args:
			if ((uintptr_t)u8Str < PAGING_BASE_SIZE)
			{
				buffIndex += handleNullPointerArg(
					&buff[buffIndex], buffMax - buffIndex);

				break;
			};

			u8Strlen = strnlen8(u8Str, buffMax - buffIndex);
			strncpy8(&buff[buffIndex], u8Str, u8Strlen);
			buffIndex += u8Strlen;
			break;

		// Same as 's', but recursively parses format strings.
		case 'r':
			sarch_t		expandRet;
			ubit16		numFormatArgs;

			u8Str = va_arg(args, utf8Char *);

			// Handle NULL pointers passed a string args:
			if ((uintptr_t)u8Str < PAGING_BASE_SIZE)
			{
				buffIndex += handleNullPointerArg(
					&buff[buffIndex], buffMax - buffIndex);

				break;
			};

			expandRet = expandPrintfFormatting(
				&buff[buffIndex], buffMax - buffIndex,
				u8Str, args, printfFlags);

			if (expandRet > 0)
				{ buffIndex += (unsigned)expandRet; };

			numFormatArgs = getNumberOfFormatArgsN(
				u8Str, buffMax - buffIndex);

			/* Advance the vararg pointer past the args for the
			 * recursive format string argument (basically discard
			 * them).
			 **/
			for (uarch_t i=0; i<numFormatArgs; i++)
				{ va_arg(args, uarch_t); };

			break;

		// Options.
		case '-':
			str++;
			for (; *str != ' '; str++)
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

		case '%':
			buff[buffIndex] = *str;
			buffIndex += 1;
			break;

		default:
			unum = va_arg(args, uarch_t);
			break;
		};	
	};

	return buffIndex;
}

sarch_t snprintf(utf8Char *buff, uarch_t maxLength, utf8Char *format, ...)
{
	va_list		args;
	uarch_t		f;
	sarch_t		nPrinted;

	va_start(args, format);
	nPrinted = expandPrintfFormatting(buff, maxLength, format, args, &f);
	va_end(args);

	if (nPrinted < 0) { return nPrinted; };
	if (nPrinted >= (signed)maxLength) { nPrinted = maxLength - 1; };
	buff[nPrinted] = '\0';
	return nPrinted;
}

void debugPipeC::printf(const utf8Char *str, va_list args)
{
	sarch_t		buffLen=0, buffMax;
	uarch_t		printfFlags=0;

	convBuff.lock.acquire();

	// Make sure we're not printing to an unallocated buffer.
	if (convBuff.rsrc == NULL)
	{
		convBuff.lock.release();
		return;
	};

	buffMax = (PAGING_BASE_SIZE * DEBUGPIPE_CONVERSION_BUFF_NPAGES)
		/ sizeof(utf8Char);

	// Expand printf formatting into convBuff.
	buffLen = expandPrintfFormatting(
		convBuff.rsrc, buffMax, str, args, &printfFlags);

	if (buffLen < 0)
	{
		convBuff.lock.release();
		return;
	};

	if (__KFLAG_TEST(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER)
		&& !__KFLAG_TEST(printfFlags, DEBUGPIPE_FLAGS_NOLOG))
	{
		debugBuff.syphon(convBuff.rsrc, buffLen);
	};

	for (ubit8 i=0; i<4; i++)
	{
		if (__KFLAG_TEST(devices.rsrc, (1<<i))) {
			zkcmCore.debug[i]->syphon(convBuff.rsrc, buffLen);
		};
	};

	convBuff.lock.release();
}

void debugPipeC::printf(
	sharedResourceGroupC<waitLockC, void *> *buff, uarch_t buffSize,
	utf8Char *str, va_list args
	)
{
	sarch_t		buffLen=0, buffMax;
	uarch_t		printfFlags=0;

	buff->lock.acquire();

	// Make sure we're not printing to an unallocated buffer.
	if (buff->rsrc == NULL)
	{
		buff->lock.release();
		return;
	};

	buffMax = buffSize / sizeof(utf8Char);

	// Expand printf formatting into convBuff.
	buffLen = expandPrintfFormatting(
		static_cast<utf8Char *>( buff->rsrc ), buffMax,
		str, args, &printfFlags);

	if (buffLen < 0)
	{
		buff->lock.release();
		return;
	};

	if (__KFLAG_TEST(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER)
		&& !__KFLAG_TEST(printfFlags, DEBUGPIPE_FLAGS_NOLOG))
	{
		debugBuff.syphon(
			static_cast<utf8Char *>( buff->rsrc ), buffLen);
	};

	for (ubit8 i=0; i<4; i++)
	{
		if (__KFLAG_TEST(devices.rsrc, (1<<i)))
		{
			zkcmCore.debug[i]->syphon(
				static_cast<utf8Char *>( buff->rsrc ), buffLen);
		};
	};

	buff->lock.release();
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

