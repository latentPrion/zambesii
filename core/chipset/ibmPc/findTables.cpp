
#include <chipset/findTables.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


#define FINDTABLES		"chipset_findTables: "
#define LOWMEM_NPAGES		256

static ubit8	*lowmem;

static error_t ibmPc_mapLowmem(void)
{
	status_t	nMapped;

	// Attempt to map in lowmem first.
	lowmem = (ubit8 *)(memoryTrib.__kmemoryStream.vaddrSpaceStream
		.*memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(
		LOWMEM_NPAGES);

	if (lowmem == __KNULL) {
		return ERROR_MEMORY_NOMEM_VIRTUAL;
	};

	nMapped = walkerPageRanger::mapInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		lowmem, 0x0, LOWMEM_NPAGES,
		PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (nMapped < LOWMEM_NPAGES)
	{
		memoryTrib.__kmemoryStream.vaddrSpaceStream
			.releasePages(lowmem, LOWMEM_NPAGES);

		return ERROR_MEMORY_VIRTUAL_PAGEMAP;
	};

	return ERROR_SUCCESS;
}


#ifdef CONFIG_ARCH_x86_32
	#if  __SCALING__ >= SCALING_SMP
void *chipset_findx86MpFp(void)
{
	if (lowmem == __KNULL)
	{
		if (ibmPc_mapLowmem() != ERROR_SUCCESS)
		{
			__kprintf(ERROR FINDTABLES"MPFP: Failed to map lowmem."
				"\n");

			return __KNULL;
		};
	};

	// Look for the MP Floating Pointer Structure.
	for (uarch_t tmp = (uarch_t)lowmem + 0x80000;
		tmp < (uarch_t)lowmem + 0x100000;
		tmp += 0x10)
	{
		if (strncmp((char *)tmp, "_MP_", 4) == 0)
		{
			__kprintf(NOTICE FINDTABLES"MPFP: Found MP FP: 0x%X.\n",
				tmp - (uarch_t)lowmem);

			return (void *)tmp;
		};
	};

	__kprintf(WARNING FINDTABLES"MPFP: No MP FP found.\n");
	return __KNULL;
}
	#endif
#endif

void *chipset_findAcpiRsdp(void)
{
	if (lowmem == __KNULL)
	{
		if (ibmPc_mapLowmem() != ERROR_SUCCESS)
		{
			__kprintf(ERROR FINDTABLES"RSDP: Failed to map lowmem."
				"\n");

			return __KNULL;
		};
	};

	for (uarch_t tmp = (uarch_t)lowmem + 0x80000;
		tmp < (uarch_t)lowmem + 0x100000;
		tmp += 0x10)
	{
		if (strncmp((char *)tmp, "RSD PTR ", 8) == 0)
		{
			__kprintf(NOTICE FINDTABLES"RSDP: Found RSDP: 0x%X.\n",
				tmp - (uarch_t)lowmem);

			return (void *)tmp;
		};
	};

	__kprintf(WARNING FINDTABLE"RSDP: No RSDP found.\n");
	return __KNULL;
}

