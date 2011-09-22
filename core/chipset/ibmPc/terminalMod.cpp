
#include <debug.h>
#include <arch/walkerPageRanger.h>
#include <chipset/memoryAreas.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include "terminalMod.h"


struct ibmPc_terminalMod_fbS
{
	ubit8	ch;
	ubit8	attr;
};

static struct ibmPc_terminalMod_fbS	*buff=__KNULL, *origBuff;
static uarch_t				row, col, maxRow, maxCol;
ubit8					*bda;

error_t ibmPc_terminalMod_initialize(void)
{
	error_t		ret;
	ubit8		*lowmem;

	// The memoryAreas API maps ranges as PAGEATTRIB_CACHE_WRITE_THROUGH.
	ret = chipset_mapArea(CHIPSET_MEMAREA_LOWMEM);
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	lowmem = reinterpret_cast<ubit8 *>(
		chipset_getArea(CHIPSET_MEMAREA_LOWMEM) );

	origBuff = buff = reinterpret_cast<ibmPc_terminalMod_fbS *>(
		&lowmem[0xB8000] );

	row = col = 0;

	bda = lowmem;
	maxRow = bda[0x484] + 1;
	maxCol = *(ubit16 *)&bda[0x44A];

	// If we're still here, then the range should be mapped in.
	return ERROR_SUCCESS;
}

error_t ibmPc_terminalMod_shutdown(void)
{
	buff = __KNULL;
	return ERROR_SUCCESS;
}

error_t ibmPc_terminalMod_suspend(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_terminalMod_awake(void)
{
	return ERROR_SUCCESS;
}

sarch_t ibmPc_terminalMod_isInitialized(void)
{
	if (buff == __KNULL) {
		return 0;
	}
	else {
		return 1;
	};
}

static void ibmPc_terminalMod_scrollDown(void)
{
	uarch_t		i;
	uarch_t		bound=(maxRow * maxCol) - maxCol;

	for (i=0; i<bound; i++) {
		*(ubit16 *)&buff[i] = *(ubit16 *)&buff[i + maxCol];
	}

	bound += maxCol;
	for (; i<bound; i++) {
		*(ubit16 *)&buff[i] = 0;
	};
}	

void ibmPc_terminalMod_syphon(const utf8Char *str, uarch_t len)
{
	// FIXME: Remember to do utf-8 expansion here.

	if (buff == __KNULL || str == __KNULL) {
		return;
	};

	for (; len != 0; len--, str++)
	{
		switch(*str)
		{
		case '\n':
			if (row >= maxRow-1)
			{
				ibmPc_terminalMod_scrollDown();
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
					ibmPc_terminalMod_scrollDown();
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

void ibmPc_terminalMod_clear(void)
{
	uarch_t		i, bound=(maxRow * maxCol);

	for (i=0; i<bound; i++) {
		*(ubit16 *)&buff[i] = 0;
	};
	row = col = 0;
}

