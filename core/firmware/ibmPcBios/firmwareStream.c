
#include <arch/paging.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/firmwareTrib/firmwareStream.h>
#include <kernel/common/firmwareTrib/rivMemoryApi.h>
#include <kernel/common/firmwareTrib/rivLockApi.h>
#include "x86emu.h"
#include "x86EmuAuxFuncs.h"

#define LOWMEM_NPAGES		(0x100000 / PAGING_BASE_SIZE)

static void		*ibmPcBiosLock;

error_t ibmPcBios_initialize(void)
{
	status_t	nMapped;

	memset(&M, 0, sizeof(M));

	// Allocate vmem and map in lower memory.
	M.mem_base = (uarch_t)__kvaddrSpaceStream_getPages(LOWMEM_NPAGES);
	if (M.mem_base == __KNULL) {
		return ERROR_MEMORY_NOMEM_VIRTUAL;
	};

	/* Note carefully the PAGEATTRIB_CACHE_WRITE_THROUGH flag. I did this
	 * because I'm very sure that the firmware will be writing to all kinds
	 * of weird places, and doing all kinds of crazy stuff. Therefore,
	 * since I'm not at all sure of how any particular chipset's firmware
	 * will behave, I'll take the safe route and make sure that all data
	 * writes are immediately seen, and in the right order.
	 **/
	nMapped = walkerPageRanger_mapInc(
		(void *)M.mem_base, 0x0, LOWMEM_NPAGES,
		PAGEATTRIB_WRITE | PAGEATTRIB_CACHE_WRITE_THROUGH |
		PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (nMapped < LOWMEM_NPAGES) {
		return ERROR_MEMORY_VIRTUAL_PAGEMAP;
	};

	// Low memory is mapped into the kernel addrspace. Allocate lock.
	ibmPcBiosLock = waitLock_create();
	if (ibmPcBiosLock == __KNULL)
	{
		__kvaddrSpaceStream_releasePages((
			void *)M.mem_base, LOWMEM_NPAGES);

		return ERROR_MEMORY_NOMEM;
	};

	X86EMU_setupMemFuncs(&x86Emu_memFuncs);
	X86EMU_setupPioFuncs(&x86Emu_ioFuncs);

	return ERROR_SUCCESS;
}

error_t ibmPcBios_shutdown(void)
{
	waitLock_acquire(ibmPcBiosLock);

	__kvaddrSpaceStream_releasePages((void *)M.mem_base, LOWMEM_NPAGES);
	M.mem_base = __KNULL;

	waitLock_release(ibmPcBiosLock);
	waitLock_destroy(ibmPcBiosLock);
}

error_t ibmPcBios_suspend(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPcBios_awake(void)
{
	return ERROR_SUCCESS;
}

static error_t ibmPcBios_nop_failure(void)
{
	return ERROR_UNKNOWN;
}

static error_t ibmPcBios_nop_failure2(uarch_t)
{
	return ERROR_UNKNOWN;
}

struct firmwareStreamS		firmwareFwStream =
{
	&ibmPcBios_initialize,
	&ibmPcBios_shutdown,
	&ibmPcBios_suspend,
	&ibmPcBios_awake,

	// Interrupt vector control.
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,

	/**	Devices:
	 * Watchdog, debug 1-4.
	 **/
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL
};

