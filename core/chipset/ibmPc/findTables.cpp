
#include <chipset/memoryAreas.h>
#include <chipset/findTables.h>
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/mpTables.h>
#include <commonlibs/libacpi/baseTables.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


#define FINDTABLES		"chipset_findTables: "

static ubit8	*lowmem;

#ifdef CONFIG_ARCH_x86_32
	#if  __SCALING__ >= SCALING_SMP

static ubit8 mpFpChecksum(ubit8 *location, ubit8 nBytes)
{
	ubit8		ret=0;

	while (nBytes-- != 0) {
		ret += *location++;
	};

	return ret;
}

void *chipset_findx86MpFp(void)
{
	ubit8		checksum;

	if (chipsetMemAreas::mapArea(CHIPSET_MEMAREA_LOWMEM) != ERROR_SUCCESS)
	{
		__kprintf(ERROR FINDTABLES"MPFP: Failed to map lowmem.\n");
		return NULL;
	};

	lowmem = reinterpret_cast<ubit8 *>(
		chipsetMemAreas::getArea(CHIPSET_MEMAREA_LOWMEM) );

	// Look for the MP Floating Pointer Structure.
	for (ubit8 *tmp = &lowmem[0x80000];
		tmp < &lowmem[0x100000];
		tmp = reinterpret_cast<ubit8 *>( (uarch_t)tmp + 0x10 ))
	{
		if (strncmp8(CC(tmp), CC"_MP_", 4) != 0) { continue; };

		checksum = mpFpChecksum(tmp, ((x86_mpFpS *)tmp)->length * 16);

		__kprintf(NOTICE FINDTABLES"MPFP: checksum was: %d, result was "
			"%d. %s checksum.\n",
			((x86_mpFpS *)tmp)->checksum, checksum,
			((checksum == 0) ? "Valid" : "Invalid"));

		if (checksum != 0) { continue; };

		__kprintf(NOTICE FINDTABLES"MPFP: Found MP FP: 0x%P.\n",
			tmp - (uarch_t)lowmem);

		return (void *)tmp;
	};

	__kprintf(WARNING OPTS(LOGONCE(LOGONCE_FINDTABLES(0))) FINDTABLES
		"MPFP: No MP FP found.\n");

	return NULL;
}
	#endif
#endif

inline static ubit8 acpiRsdpChecksum(ubit8 *location, ubit8 nBytes)
{
	// They both work the same.
	return mpFpChecksum(location, nBytes);
}

void *chipset_findAcpiRsdp(void)
{
	ubit8		checksum;

	if (chipsetMemAreas::mapArea(CHIPSET_MEMAREA_LOWMEM) != ERROR_SUCCESS)
	{
		__kprintf(ERROR FINDTABLES"RSDP: Failed to map lowmem.\n");
		return NULL;
	};

	lowmem = reinterpret_cast<ubit8 *>(
		chipsetMemAreas::getArea(CHIPSET_MEMAREA_LOWMEM) );

	for (ubit8 *tmp = &lowmem[0x80000];
		tmp < &lowmem[0x100000];
		tmp = reinterpret_cast<ubit8 *>( (uarch_t)tmp + 0x10 ))
	{
		if (strncmp8(CC(tmp), CC"RSD PTR ", 8) != 0) { continue; };

		checksum = acpiRsdpChecksum(tmp, 20);

		__kprintf(NOTICE FINDTABLES"RSDP: Checksum was %d, result was "
			"%d. %s checksum.\n",
			((acpi_rsdpS *)tmp)->checksum,
			checksum,
			(checksum) ? "Invalid" : "Valid");

		if (checksum != 0) { continue; };

		__kprintf(NOTICE FINDTABLES"RSDP: Found RSDP: 0x%P.\n",
			tmp - (uarch_t)lowmem);

		return (void *)tmp;
	};

	__kprintf(WARNING OPTS(LOGONCE(LOGONCE_FINDTABLES(1))) FINDTABLES
		"RSDP: No RSDP found.\n");

	return NULL;
}

