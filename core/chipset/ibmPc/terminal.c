
// Switch the firmwareRiveApi header to use the firmwareRivTypes types.
#include <kernel/common/firmwareTrib/firmwareRivTypes.h>
#include <kernel/common/firmwareTrib/firmwareRivApi.h>

struct ibmPc_terminal_fbS
{
	ubit8	ch;
	ubit8	attr;
};

static struct ibmPc_terminal_fbS	*buff;

error_t ibmPc_terminal_initialize(void);
error_t ibmPc_terminal_shutdown(void);
error_t ibmPc_terminal_suspend(void);
error_t ibmPc_terminal_awake(void);

error_t ibmPc_terminal_initialize(void)
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

	// If we're still here, then the range should be mapped in.
	return ERROR_SUCCESS;
}

error_t ibmPc_terminal_shutdown(void)
{
	uarch_t		p, f;

	// Unmap the page from the kernel VAS.
	walkerPageRanger_unmap(buff, &p, 1, &f);
	// Free the virtual address back to the kernel swamp.
	__kvaddrSpaceStream_releasePages(buff, 1);

	return ERROR_SUCCESS;
}

error_t ibmPc_terminal_suspend(void)
{
}

error_t ibmPc_terminal_awake(void)
{
}

void ibmPc_terminal_test(void)
{
	uarch_t		i;

	for (i=0; i<0x2b2; i++)
	{
		buff[i].ch = 'Z';
		buff[i].attr = 0x07;
	};
}

