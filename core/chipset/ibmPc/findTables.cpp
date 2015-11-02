
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
		printf(ERROR FINDTABLES"MPFP: Failed to map lowmem.\n");
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

		checksum = mpFpChecksum(tmp, ((x86Mp::sFloatingPtr *)tmp)->length * 16);

		printf(NOTICE FINDTABLES"MPFP: checksum was: %d, result was "
			"%d. %s checksum.\n",
			((x86Mp::sFloatingPtr *)tmp)->checksum, checksum,
			((checksum == 0) ? "Valid" : "Invalid"));

		if (checksum != 0) { continue; };

		paddr_t			p = (uintptr_t)tmp - (uintptr_t)lowmem;

		printf(NOTICE FINDTABLES"MPFP: Found MP FP: 0x%P.\n", &p);

		return (void *)tmp;
	};

	printf(WARNING OPTS(LOGONCE(LOGONCE_FINDTABLES(0))) FINDTABLES
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
		printf(ERROR FINDTABLES"RSDP: Failed to map lowmem.\n");
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

		printf(NOTICE FINDTABLES"RSDP: Checksum was %d, result was "
			"%d. %s checksum.\n",
			((acpi::sRsdp *)tmp)->checksum,
			checksum,
			(checksum) ? "Invalid" : "Valid");

		if (checksum != 0) { continue; };

		paddr_t			p = (uintptr_t)tmp - (uintptr_t)lowmem;

		printf(NOTICE FINDTABLES"RSDP: Found RSDP: 0x%P.\n", &p);
		return (void *)tmp;
	};

	printf(WARNING OPTS(LOGONCE(LOGONCE_FINDTABLES(1))) FINDTABLES
		"RSDP: No RSDP found.\n");

	return NULL;
}

