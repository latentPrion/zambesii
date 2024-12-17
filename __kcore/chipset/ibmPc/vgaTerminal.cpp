
#include <debug.h>
#include <arch/walkerPageRanger.h>
#include <chipset/memoryAreas.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/processTrib/processTrib.h>
#include "vgaTerminal.h"


IbmPcVgaTerminal		ibmPcVgaTerminal;

// Static member variable.
ubit8	*IbmPcVgaTerminal::bda=NULL;

error_t IbmPcVgaTerminal::initialize(void)
{
	// The memoryAreas API maps ranges as PAGEATTRIB_CACHE_WRITE_THROUGH.
	origBuff = buff = reinterpret_cast<sIbmPc_TerminalMod_Fb *>(
		(void *)0xB8000 );

	row = col = 0;

	bda = (ubit8*)0xB8000;
	maxRow = 25;
	maxCol = 80;

	// If we're still here, then the range should be mapped in.
	return ERROR_SUCCESS;
}

void IbmPcVgaTerminal::chipsetEventNotification(ubit8 event, uarch_t)
{
	error_t		ret;
	ubit8		*lowmem;

	assert_fatal(event == __KPOWER_EVENT___KMEMORY_STREAM_AVAIL);

	// The memoryAreas API maps ranges as PAGEATTRIB_CACHE_WRITE_THROUGH.
	ret = chipsetMemAreas::mapArea(CHIPSET_MEMAREA_LOWMEM);
	if (ret != ERROR_SUCCESS)
	{
		printf(FATAL"ibmPcVga: chipsetEventNotification: "
			"__KSPACE_AVAIL: Failed to map lowmem area.\n");

		return;
	};

	lowmem = reinterpret_cast<ubit8 *>(
		chipsetMemAreas::getArea(CHIPSET_MEMAREA_LOWMEM) );

	origBuff = buff = reinterpret_cast<sIbmPc_TerminalMod_Fb *>(
		&lowmem[0xB8000] );

	//row = col = 0;

	bda = lowmem;
	maxRow = bda[0x484] + 1;
	maxCol = *(ubit16 *)&bda[0x44A];

	// Also, unmap the old identity mapping.
	paddr_t		p;
	uarch_t		f;


	walkerPageRanger::unmap(
		&processTrib.__kgetStream()->getVaddrSpaceStream()->vaddrSpace,
		(void *)0xB8000, &p, 1, &f);
}

error_t IbmPcVgaTerminal::shutdown(void)
{
	buff = NULL;
	return ERROR_SUCCESS;
}

error_t IbmPcVgaTerminal::suspend(void)
{
	return ERROR_SUCCESS;
}

error_t IbmPcVgaTerminal::restore(void)
{
	return ERROR_SUCCESS;
}

sarch_t IbmPcVgaTerminal::isInitialized(void)
{
	if (buff == NULL) {
		return 0;
	}
	else {
		return 1;
	};
}

void IbmPcVgaTerminal::scrollDown(void)
{
	uarch_t			i;
	uarch_t			bound=(maxRow * maxCol) - maxCol;
	sIbmPc_TerminalMod_Fb	nullChar = { 0, 0 };

	for (i=0; i<bound; i++) {
		buff[i] = buff[i + maxCol];
	}

	bound += maxCol;
	for (; i<bound; i++) {
		buff[i] = nullChar;
	};
}

void IbmPcVgaTerminal::syphon(const utf8Char *str, uarch_t len)
{
	// FIXME: Remember to do utf-8 expansion here.

	if (buff == NULL || str == NULL) {
		return;
	};

	for (; len != 0; len--, str++)
	{
		switch(*str)
		{
		case '\n':
			if (row >= maxRow-1)
			{
				scrollDown();
				row = maxRow-1;
				col = 0;
				break;
			}
			row++;
			col = 0;
			break;

		case '\r':
			col = 0;
			break;

		case '\t':
			col += 8;
			col &= ~0x7;
			break;

		default:
			if (*str < 0x20) {
				break;
			};
			if (col >= maxCol)
			{
				row++;
				if (row >= maxRow-1)
				{
					scrollDown();
					row = maxRow-1;
				};
				col = 0;
			};
			// For now we truncate all codepoints.
			buff[row * maxCol + col].ch = (ubit8)*str;
			buff[row * maxCol + col].attr = 0x07;
			col++;
			break;
		};
	};
}

void IbmPcVgaTerminal::clear(void)
{
	uarch_t		i, bound=(maxRow * maxCol);

	for (i=0; i<bound; i++) {
		*(ubit16 *)&buff[i] = 0;
	};
	row = col = 0;
}

