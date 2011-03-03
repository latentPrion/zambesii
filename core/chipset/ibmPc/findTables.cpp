
#include <chipset/memoryAreas.h>
#include <chipset/findTables.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


#define FINDTABLES		"chipset_findTables: "

static ubit8	*lowmem;

#ifdef CONFIG_ARCH_x86_32
	#if  __SCALING__ >= SCALING_SMP
void *chipset_findx86MpFp(void)
{
	if (chipset_getArea(CHIPSET_MEMAREA_LOWMEM) == __KNULL)
	{
		if (chipset_mapArea(CHIPSET_MEMAREA_LOWMEM) != ERROR_SUCCESS)
		{
			__kprintf(ERROR FINDTABLES"MPFP: Failed to map lowmem."
				"\n");

			return __KNULL;
		};
		lowmem = (ubit8 *)chipset_getArea(CHIPSET_MEMAREA_LOWMEM);
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
	if (chipset_getArea(CHIPSET_MEMAREA_LOWMEM) == __KNULL)
	{
		if (chipset_mapArea(CHIPSET_MEMAREA_LOWMEM) != ERROR_SUCCESS)
		{
			__kprintf(ERROR FINDTABLES"MPFP: Failed to map lowmem."
				"\n");

			return __KNULL;
		};
		lowmem = (ubit8 *)chipset_getArea(CHIPSET_MEMAREA_LOWMEM);
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

	__kprintf(WARNING FINDTABLES"RSDP: No RSDP found.\n");
	return __KNULL;
}

