
#include <__kstdlib/__ktypes.h>
#include <kernel/common/firmwareTrib/firmwareRivApi.h>
#include <kernel/common/firmwareTrib/terminalFwRiv.h>

struct ibmPc_terminal_fbS
{
	ubit8	ch;
	ubit8	attr;
};

static struct ibmPc_terminal_fbS	*buff, *origBuff;
static uarch_t				row, col, maxRow, maxCol;
ubit8					*bda;

static error_t ibmPc_terminal_initialize(void)
{
	status_t	nMapped;

	buff = __KNULL;

	buff = (struct ibmPc_terminal_fbS *) __kvaddrSpaceStream_getPages(1);
	if (buff == __KNULL) {
		return ERROR_MEMORY_NOMEM_VIRTUAL;
	};

	nMapped = walkerPageRanger_mapInc(
		buff, 0xB8000, 1,
		PAGEATTRIB_PRESENT | PAGEATTRIB_WRITE
		| PAGEATTRIB_CACHE_WRITE_THROUGH | PAGEATTRIB_SUPERVISOR);

	if (nMapped < 1)
	{
		__kvaddrSpaceStream_releasePages(buff, 1);
		return ERROR_MEMORY_VIRTUAL_PAGEMAP;
	};

	origBuff = buff;
	row = col = 0;

	// Try to map in the BDA. If that fails, just assume 80 * 25.
	bda = __kvaddrSpaceStream_getPages(1);
	if (bda != __KNULL)
	{
		nMapped = walkerPageRanger_mapInc(
			bda, 0x0, 1,
			PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

		if (nMapped == 1)
		{	
			maxRow = *(bda + 0x484) + 1;
			maxCol = *(ubit16 *)(bda + 0x44A);
			return ERROR_SUCCESS;
		}
		__kvaddrSpaceStream_releasePages(bda, 1);
	};
	maxRow = 25;
	maxCol = 80;

	// If we're still here, then the range should be mapped in.
	return ERROR_SUCCESS;
}

static error_t ibmPc_terminal_shutdown(void)
{
	uarch_t		p, f;

	// Unmap the page from the kernel VAS.
	walkerPageRanger_unmap(buff, &p, 1, &f);
	// Free the virtual address back to the kernel swamp.
	__kvaddrSpaceStream_releasePages(buff, 1);

	return ERROR_SUCCESS;
}

static error_t ibmPc_terminal_suspend(void)
{
}

static error_t ibmPc_terminal_awake(void)
{
}

static void ibmPc_terminal_scrollDown(void)
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

static void ibmPc_terminal_read(const unicodePoint *str)
{
	if (str == __KNULL) {
		return;
	};

	for (; *str != 0; str++)
	{
		switch(*str)
		{
		case '\n':
			if (row >= maxRow-1)
			{
				ibmPc_terminal_scrollDown();
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
					ibmPc_terminal_scrollDown();
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

void ibmPc_terminal_clear(void)
{
	uarch_t		i, bound=(maxRow * maxCol);

	for (i=0; i<bound; i++) {
		*(ubit16 *)&buff[i] = 0;
	};
	row = col = 0;
}

struct terminalFwRivS chipsetTerminalFwRiv =
{
	&ibmPc_terminal_initialize,
	&ibmPc_terminal_shutdown,
	&ibmPc_terminal_suspend,
	&ibmPc_terminal_awake,
	&ibmPc_terminal_read,
	&ibmPc_terminal_clear
};

