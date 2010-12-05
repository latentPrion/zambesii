
#include <arch/walkerPageRanger.h>
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
	status_t	nMapped;

	buff = new ((memoryTrib.__kmemoryStream.vaddrSpaceStream.*
		memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(1))
		ibmPc_terminalMod_fbS;

	if (buff == __KNULL) {
		return ERROR_MEMORY_NOMEM_VIRTUAL;
	};

	nMapped = walkerPageRanger::mapInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		buff, 0xB8000, 1,
		PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE
		| PAGEATTRIB_CACHE_WRITE_THROUGH | PAGEATTRIB_SUPERVISOR);

	if (nMapped < 1)
	{
		memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(
			buff, 1);

		buff = __KNULL;
		return ERROR_MEMORY_VIRTUAL_PAGEMAP;
	};

	origBuff = buff;
	row = col = 0;

	// Try to map in the BDA. If that fails, just assume 80 * 25.
	bda = new ((memoryTrib.__kmemoryStream.vaddrSpaceStream.*
		memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(1)) ubit8;

	if (bda != __KNULL)
	{
		nMapped = walkerPageRanger::mapInc(
			&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
			bda, 0x0, 1,
			PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

		if (nMapped == 1)
		{
			bda = __KNULL;
			maxRow = *(bda + 0x484) + 1;
			maxCol = *(ubit16 *)(bda + 0x44A);
			return ERROR_SUCCESS;
		}
		memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(
			bda, 1);
	};
	maxRow = 25;
	maxCol = 80;

	// If we're still here, then the range should be mapped in.
	return ERROR_SUCCESS;
}

error_t ibmPc_terminalMod_shutdown(void)
{
	uarch_t		p, f;

	// Unmap and free BDA mapping.
	if (bda != __KNULL)
	{
		walkerPageRanger::unmap(
			&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
			bda, &p, 1, &f);

		memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(
			bda, 1);
	};

	// Unmap and free text mode buffer mapping.
	if (buff != __KNULL)
	{
		walkerPageRanger::unmap(
			&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
			buff, &p, 1, &f);

		memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(
			buff, 1);
	};

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

