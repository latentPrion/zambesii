
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

DebugPipe::DebugPipe(void)
{
	memset(convBuff.rsrc, 0, sizeof(convBuff.rsrc));
	devices.rsrc = 0;
}

error_t DebugPipe::initialize(void)
{
	zkcmCore.chipsetEventNotification(__KPOWER_EVENT___KPRINTF_AVAIL, 0);
	return ERROR_SUCCESS;
}

DebugPipe::~DebugPipe(void)
{
	untieFrom(DEBUGPIPE_DEVICE_BUFFER);
	untieFrom(
		DEBUGPIPE_DEVICE1 | DEBUGPIPE_DEVICE2 | DEBUGPIPE_DEVICE3
		| DEBUGPIPE_DEVICE4);
}

uarch_t DebugPipe::tieTo(uarch_t device)
{
	ZkcmDebugDevice	*mod;
	error_t			err;
	uarch_t			ret;

	if (FLAG_TEST(device, DEBUGPIPE_DEVICE_BUFFER))
	{
		if (debugBuff.initialize() == ERROR_SUCCESS)
		{
			devices.lock.acquire();
			FLAG_SET(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER);
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

uarch_t DebugPipe::untieFrom(uarch_t device)
{
	uarch_t		ret;
	error_t		err;

	if (FLAG_TEST(device, DEBUGPIPE_DEVICE_BUFFER))
	{
		if (debugBuff.shutdown() == ERROR_SUCCESS)
		{
			devices.lock.acquire();
			FLAG_UNSET(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER);
			devices.lock.release();
		};
	};

	for (ubit8 i=0; i<4; i++)
	{
		if (FLAG_TEST(device, (1<<i)))
		{
			if (!FLAG_TEST(devices.rsrc, (1<<i))) {
				continue;
			};

			// Assume that mod exists if its bit is set.
			err = zkcmCore.debug[i]->shutdown();
			if (err == ERROR_SUCCESS) {
				FLAG_UNSET(devices.rsrc, (1<<i));
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

void DebugPipe::refresh(void)
{
	utf8Char	*buff;
	void		*handle;
	uarch_t		len;

	// Clear "screen" on each device, whatever that means to that device.
	for (ubit8 i=0; i<4; i++)
	{
		if (FLAG_TEST(devices.rsrc, (1<<i))) {
			zkcmCore.debug[i]->clear();
		};
	};

	handle = debugBuff.lock();

	for (buff = debugBuff.extract(&handle, &len); buff != NULL; )
	{
		for (ubit8 i=0; i<4; i++)
		{
			if (FLAG_TEST(devices.rsrc, (1<<i))) {
				zkcmCore.debug[i]->syphon(buff, len);
			};
		};

		buff = debugBuff.extract(&handle, &len);
	};
	debugBuff.unlock();
}

// Expects the lock to already be held.
sarch_t sprintnum(
	utf8Char *buff, uarch_t buffsize, uarch_t num, const ubit8 base,
	sbit8 isSigned, sbit8 doUpperCase
	)
{
	const int	TMPBUFF_SIZE=28;
	utf8Char	b[TMPBUFF_SIZE];
	uarch_t		nDigitsPrinted=0, buffCursor=0;
	char		hexBaseChar = 'a';

	if (base != 10 && base != 16) {
		return -1;
	};

	if (num == 0)
	{
		buff[buffCursor++] = '0';
		return buffCursor;
	};

	if (isSigned)
	{
		sarch_t snum = (sarch_t)num;

		if (snum < 0)
		{
			buff[buffCursor++] = '-';
			snum = -snum;
			num = (uarch_t)snum;
		};
	};

	if (base > 10 && doUpperCase) {
		hexBaseChar = 'A';
	};

	for (; num; num /= base)
	{
		ubit8 digit = num % base;

		if (digit < 10) {
			b[nDigitsPrinted++] = digit + '0';
		} else {
			b[nDigitsPrinted++] = digit - 10 + hexBaseChar;
		};
	};

	for (uarch_t i=0; i<nDigitsPrinted && buffCursor < buffsize; i++) {
		buff[buffCursor++] = b[nDigitsPrinted - i - 1];
	};

	return buffCursor;
}

void printf(const utf8Char *str, ...)
{
	va_list		args;

	va_start(args, str);
	__kdebug.printf(str, args);
	va_end(args);
}

void printf(
	SharedResourceGroup<WaitLock, utf8Char *> *buff, uarch_t buffSize,
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
	// Number of chars consumed in the output buffer.
	uarch_t		buffIndex,
	// Number of chars printed in the current loop iteration.
			nPrinted;

	/**	CAVEAT:
	 * If you ever change the format specifiers (add new ones, take some
	 * away, etc) be sure to check and update the format-specifier-counter
	 * function above so that the value it returns for a format-specifier
	 * count is consistent with the format specifiers that this function
	 * actually takes.
	 **/
	for (
		buffIndex=0;
		(*str != '\0') && (buffIndex < buffMax);
		str++, buffIndex+=nPrinted)
	{
		nPrinted = 0;

		if (*str != '%')
		{
			buff[buffIndex] = *str;
			nPrinted++;
			continue;
		};

		str++;

		// Avoid the exploit "foo %", which would cause buffer overread.
		if (*str == '\0') { return -1; };

		switch (*str)
		{
		case '%':
			buff[buffIndex] = *str;
			nPrinted++;
			break;

		case 'i':
		case 'd': {
			sarch_t snum = va_arg(args, sarch_t);

			nPrinted += sprintnum(
				&buff[buffIndex], buffMax - buffIndex,
				snum, 10, 1, 0);
			break;
		}

		case 'u': {
			uarch_t unum = va_arg(args, uarch_t);

			nPrinted += sprintnum(
				&buff[buffIndex], buffMax - buffIndex,
				unum, 10, 0, 0);
			break;
		}

		case 'x':
		case 'p': {
			uarch_t		unum = va_arg(args, uarch_t);
			utf8Char	*prefix;

			prefix = (*str == 'x')
				? CC"0x"
				: CC"vX";

			strncpy8(&buff[buffIndex], prefix, buffMax - buffIndex);
			nPrinted += strnlen8(prefix, buffMax - buffIndex);

			nPrinted += sprintnum(
				&buff[buffIndex + nPrinted],
				buffMax - (buffIndex + nPrinted),
				unum, 16, 0, 0);
			break;
		}

		case 'X': {
			uarch_t unum = va_arg(args, uarch_t);
			utf8Char	*prefix = CC"0x";

			strncpy8(&buff[buffIndex], prefix, buffMax - buffIndex);
			nPrinted += strnlen8(prefix, buffMax - buffIndex);

			nPrinted += sprintnum(
				&buff[buffIndex + nPrinted],
				buffMax - (buffIndex + nPrinted),
				unum, 16, 0, 1);
			break;
		}

		case 'P': {
			paddr_t *ppnum = va_arg(args, paddr_t*);
			utf8Char	*prefix = CC"Px";

			strncpy8(&buff[buffIndex], prefix, buffMax - buffIndex);
			nPrinted += strnlen8(prefix, buffMax - buffIndex);

			if (sizeof(*ppnum) > (__VADDR_NBITS__ / __BITS_PER_BYTE__)) {
				paddr_t pnum = *ppnum;

				pnum >>= __VADDR_NBITS__;

				nPrinted += sprintnum(
					&buff[buffIndex + nPrinted],
					buffMax - (buffIndex + nPrinted),
					pnum.getLow(), 16, 0, 1);

				buff[buffIndex + nPrinted++] = '_';
			};

			nPrinted += sprintnum(
				&buff[buffIndex + nPrinted], buffMax - buffIndex,
				ppnum->getLow(), 16, 0, 1);
			break;
		}

		// Non recursive string printing.
		case 's': {
			utf8Char	*u8Str;
			uarch_t		u8Strlen;

			u8Str = va_arg(args, utf8Char *);

			// Handle NULL pointers passed as string args:
			if ((uintptr_t)u8Str < PAGING_BASE_SIZE)
			{
				nPrinted += handleNullPointerArg(
					&buff[buffIndex], buffMax - buffIndex);

				break;
			};

			u8Strlen = strnlen8(u8Str, buffMax - buffIndex);
			strncpy8(&buff[buffIndex], u8Str, u8Strlen);
			nPrinted += u8Strlen;
			break;
		}

		// Same as 's', but recursively parses format strings.
		case 'r': {
			utf8Char	*fmtStr;
			ubit16		numFormatArgs;

			fmtStr = va_arg(args, utf8Char *);

			// Handle NULL pointers passed a string args:
			if ((uintptr_t)fmtStr < PAGING_BASE_SIZE)
			{
				nPrinted += handleNullPointerArg(
					&buff[buffIndex], buffMax - buffIndex);

				break;
			};

			nPrinted += expandPrintfFormatting(
				&buff[buffIndex], buffMax - buffIndex,
				fmtStr, args, printfFlags);

			numFormatArgs = getNumberOfFormatArgsN(
				fmtStr, buffMax - buffIndex);

			/* Advance the vararg pointer past the args for the
			 * recursive format string argument (basically discard
			 * them).
			 **/
			for (uarch_t i=0; i<numFormatArgs; i++)
				{ va_arg(args, uarch_t); };

			break;
		}

		// Options.
		case '-':
			str++;
			for (; *str != ' '; str++)
			{
				switch (*str)
				{
				case 'n':
					FLAG_SET(
						*printfFlags,
						DEBUGPIPE_FLAGS_NOLOG);
					break;

				default:
					break;
				};
			};
			break;

		default:
			va_arg(args, uarch_t);
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

void DebugPipe::printf(const utf8Char *str, va_list args)
{
	sarch_t		buffLen=0, buffMax;
	uarch_t		printfFlags=0;

	convBuff.lock.acquire();

	// Make sure we're not printing to an unallocated buffer.
/*	if (convBuff.rsrc == NULL)
	{
		convBuff.lock.release();
		return;
	};*/

	buffMax = DEBUGPIPE_CONVERSION_BUFF_NBYTES;

	// Expand printf formatting into convBuff.
	buffLen = expandPrintfFormatting(
		convBuff.rsrc, buffMax, str, args, &printfFlags);

	if (buffLen < 0)
	{
		convBuff.lock.release();
		return;
	};

	if (FLAG_TEST(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER)
		&& !FLAG_TEST(printfFlags, DEBUGPIPE_FLAGS_NOLOG))
	{
		debugBuff.write(convBuff.rsrc, buffLen);
	};

	for (ubit8 i=0; i<4; i++)
	{
		if (FLAG_TEST(devices.rsrc, (1<<i))) {
			zkcmCore.debug[i]->syphon(convBuff.rsrc, buffLen);
		};
	};

	convBuff.lock.release();
}

void DebugPipe::printf(
	SharedResourceGroup<WaitLock, utf8Char *> *buff, uarch_t buffSize,
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
		buff->rsrc, buffMax,
		str, args, &printfFlags);

	if (buffLen < 0)
	{
		buff->lock.release();
		return;
	};

	if (FLAG_TEST(devices.rsrc, DEBUGPIPE_DEVICE_BUFFER)
		&& !FLAG_TEST(printfFlags, DEBUGPIPE_FLAGS_NOLOG))
	{
		debugBuff.write(
			static_cast<utf8Char *>( buff->rsrc ), buffLen);
	};

	for (ubit8 i=0; i<4; i++)
	{
		if (FLAG_TEST(devices.rsrc, (1<<i)))
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

