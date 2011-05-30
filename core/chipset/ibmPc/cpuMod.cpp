
#include <arch/arch.h>
#include <chipset/pkg/cpuMod.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/libacpi.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include "cpuMod.h"


#define SMPINFO		"IBMPC CPU Mod: SMP: "

error_t ibmPc_cpuMod_initialize(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_cpuMod_shutdown(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_cpuMod_suspend(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_cpuMod_restore(void)
{
	return ERROR_SUCCESS;
}

chipsetNumaMapS *ibmPc_cm_rGnm(void)
{
	chipsetNumaMapS		*ret;
	acpi_rsdtS		*rsdt;
	acpi_rSratS		*srat;
	acpi_rSratCpuS		*cpuEntry;
	void			*handle, *handle2;
	ubit32			nEntries=0, i=0;
	

	// Caller has determined that there is an RSDT present on the chipset.
	if (acpi::mapRsdt() != ERROR_SUCCESS)
	{
		__kprintf(NOTICE"Failed to map in the RSDT.\n");
		return __KNULL;
	};

	rsdt = acpi::getRsdt();

	// Get the SRAT (all if multiple).
	handle = __KNULL;
	srat = acpiRsdt::getNextSrat(rsdt, &handle);
	for (; srat != __KNULL;
		srat = acpiRsdt::getNextSrat(rsdt, &handle))
	{
		// For each SRAT (if multiple), get the LAPIC entries.
		handle2 = __KNULL;
		cpuEntry = acpiRSrat::getNextCpuEntry(srat, &handle2);
		for (; cpuEntry != __KNULL;
			cpuEntry = acpiRSrat::getNextCpuEntry(srat, &handle2))
		{
			// Count the number of entries there are in all.
			nEntries++;
		};
	};

	__kprintf(NOTICE"IBMPC CPU Mod: NUMA: %d LAPIC SRAT entries.\n",
		nEntries);

	// Now we know how many entries exist. Allocate map and reparse.
	ret = new chipsetNumaMapS;
	if (ret == __KNULL)
	{
		__kprintf(ERROR"IBMPC Cpu Mod: NUMA: Failed to allocate room "
			"for CPU NUMA map, or 0 entries found.\n");

		return __KNULL;
	};

	ret->cpuEntries = new numaCpuMapEntryS[nEntries];
	if (ret->cpuEntries == __KNULL)
	{
		delete ret;
		__kprintf(ERROR"IBMPC CPU Mod: NUMA: Failed to alloc NUMA CPU "
			"map entries.\n");

		return __KNULL;
	};

	// Reparse entries and create generic kernel CPU map.
	handle = __KNULL;
	srat = acpiRsdt::getNextSrat(rsdt, &handle);
	for (; srat != __KNULL;
		srat = acpiRsdt::getNextSrat(rsdt, &handle))
	{
		handle2 = __KNULL;
		cpuEntry = acpiRSrat::getNextCpuEntry(srat, &handle2);
		for (; cpuEntry != __KNULL;
			cpuEntry = acpiRSrat::getNextCpuEntry(srat, &handle2),
			i++)
		{
			ret->cpuEntries[i].cpuId = cpuEntry->lapicId;
			ret->cpuEntries[i].bankId =
				cpuEntry->domain0
				| (cpuEntry->domain1 & 0xFFFFFF00);

			ret->cpuEntries[i].flags = 0;
			if (__KFLAG_TEST(
				cpuEntry->flags, ACPI_SRAT_CPU_FLAGS_ENABLED))
			{
				__KFLAG_SET(
					ret->cpuEntries[i].flags,
					NUMACPUMAP_FLAGS_ONLINE);
			};
		};
	};

	ret->nCpuEntries = nEntries;
	return ret;
}

chipsetNumaMapS *ibmPc_cpuMod_getNumaMap(void)
{
	/**	EXPLANATION:
	 * For the IBM-PC, a NUMA map of all CPUs is essentially obtained by
	 * asking ACPI's SRAT for its layout.
	 *
	 * We just call on the kernel's libacpi and then return the map. As
	 * simple as that.
	 **/
	if (acpi::findRsdp() != ERROR_SUCCESS) { return __KNULL; };

// Use XSDT on everything but 32-bit, since XSDT has 64-bit physical addresses.
#ifndef __32_BIT__
	if (acpi::testForXsdt())
	{
		ret = ibmPc_cm_xGnm();
		if (ret != __KNULL) { return ret; };
	};
#endif
	if (acpi::testForRsdt())
	{
		__kprintf(NOTICE"IBMPC CPU Mod: NUMA: Falling back to RSDT.\n");
		return ibmPc_cm_rGnm();
	};

	return __KNULL;
}

chipsetSmpMapS *ibmPc_cpuMod_getSmpMap(void)
{
	x86_mpCfgCpuS	*mpCpu;
	void		*handle, *handle2;
	uarch_t		pos=0, nEntries, i;
	chipsetSmpMapS	*ret;
	acpi_rsdtS	*rsdt;
	acpi_rMadtS	*madt;
	acpi_rMadtCpuS	*madtCpu;

	/**	NOTES:
	 *  This function will return a structure describing all CPUs at boot,
	 * regardless of NUMA bank membership, and regardless of whether or not
	 * that CPU is enabled, bad, or was already described in the NUMA Map.
	 *
	 * This SMP map is used to generate shared bank CPUs (CPUs which were
	 * not mentioned in the NUMA Map, yet are mentioned here), and to tell
	 * the kernel which CPUs are bad, and in an "un-enabled", or unusable
	 * state, though not bad.
	 *
	 *	EXPLANATION:
	 * This function will use the Intel x86 MP Specification structures to
	 * enumerate CPUs, and build the SMP map, combining it with the info
	 * returned from the NUMA map.
	 *
	 * This function depends on the kernel libx86mp.
	 **/
	// First try using ACPI. Trust ACPI over MP tables.
	if (acpi::findRsdp() != ERROR_SUCCESS)
	{
		__kprintf(NOTICE SMPINFO"getSmpMap: No ACPI. Trying MP.\n");
		goto tryMpTables;
	};

	if (!acpi::testForRsdt())
	{
		__kprintf(WARNING SMPINFO"getSmpMap: ACPI, but no RSDT.\n");
		if (acpi::testForXsdt())
		{
			__kprintf(WARNING SMPINFO"getSmpMap: XSDT found. "
				"Consider using 64-bit build for XSDT.\n");
		}
		goto tryMpTables;
	};

	if (acpi::mapRsdt() != ERROR_SUCCESS)
	{
		__kprintf(NOTICE SMPINFO"getSmpMap: ACPI: Failed to map RSDT."
			"\n");

		goto tryMpTables;
	};

	// Now look for an MADT.
	rsdt = acpi::getRsdt();
	handle = __KNULL;
	nEntries = 0;
	for (madt = acpiRsdt::getNextMadt(rsdt, &handle);
		madt != __KNULL; madt = acpiRsdt::getNextMadt(rsdt, &handle))
	{
		handle2 = __KNULL;
		for (madtCpu = acpiRMadt::getNextCpuEntry(madt, &handle2);
			madtCpu != __KNULL;
			madtCpu = acpiRMadt::getNextCpuEntry(madt, &handle2))
		{
			if (__KFLAG_TEST(
				madtCpu->flags, ACPI_MADT_CPU_FLAGS_ENABLED))
			{
				nEntries++;
			};
		};
	};

	__kprintf(NOTICE SMPINFO"getSmpMap: ACPI: %d valid CPU entries.\n",
		nEntries);

	ret = new chipsetSmpMapS;
	if (ret == __KNULL)
	{
		__kprintf(ERROR SMPINFO"getSmpMap: Failed to alloc SMP map.\n");
		return __KNULL;
	};

	ret->entries = new chipsetSmpMapEntryS[nEntries];
	if (ret->entries == __KNULL)
	{
		delete ret;
		__kprintf(ERROR SMPINFO"getSmpMap: Failed to alloc SMP map "
			"entries.\n");

		return __KNULL;
	};

	handle = __KNULL;
	for (i=0, madt = acpiRsdt::getNextMadt(rsdt, &handle);
		madt != __KNULL;
		madt = acpiRsdt::getNextMadt(rsdt, &handle))
	{
		handle2 = __KNULL;
		for (madtCpu = acpiRMadt::getNextCpuEntry(madt, &handle2);
			madtCpu != __KNULL;
			madtCpu = acpiRMadt::getNextCpuEntry(madt, &handle2))
		{
			if (__KFLAG_TEST(
				madtCpu->flags, ACPI_MADT_CPU_FLAGS_ENABLED))
			{
				ret->entries[i].cpuId = madtCpu->lapicId;
				ret->entries[i].flags = 0;
				i++;
				continue;
			};
			__kprintf(NOTICE SMPINFO"getSmpMap: ACPI: Skipping "
				"un-enabled CPU %d, ACPI ID %d.\n",
				madtCpu->lapicId, madtCpu->acpiLapicId);
		};
	};

	ret->nEntries = i;
	return ret;

tryMpTables:

	__kprintf(NOTICE SMPINFO"getSmpMap: Falling back to MP tables.\n");
	x86Mp::initializeCache();
	if (!x86Mp::mpFpFound())
	{
		if (x86Mp::findMpFp() == __KNULL)
		{
			__kprintf(NOTICE SMPINFO"getSmpMap: No MP tables.\n");
			return __KNULL;
		};

		if (x86Mp::mapMpConfigTable() == __KNULL)
		{
			__kprintf(ERROR SMPINFO"getSmpMap: Unable to map MP "
				"Config tables.\n");

			return __KNULL;
		};

		// Parse x86 MP table info and discover how many CPUs there are.
		nEntries = 0;
		handle = __KNULL;
		mpCpu = x86Mp::getNextCpuEntry(&pos, &handle);
		for (; mpCpu != __KNULL;
			mpCpu = x86Mp::getNextCpuEntry(&pos, &handle))
		{
			if (__KFLAG_TEST(
				mpCpu->flags, x86_MPCFG_CPU_FLAGS_ENABLED))
			{
				nEntries++;
			};
		};

		__kprintf(NOTICE SMPINFO"getSmpMap: %d valid CPU entries in MP "
			"config tables.\n", nEntries);

		ret = new chipsetSmpMapS;
		if (ret == __KNULL)
		{
			__kprintf(ERROR SMPINFO"getSmpMap: Failed to alloc SMP "
				"Map.\n");

			return __KNULL;
		};

		ret->entries = new chipsetSmpMapEntryS[nEntries];
		if (ret->entries == __KNULL)
		{
			__kprintf(ERROR SMPINFO"getSmpMap: Failed to alloc SMP "
				"map entries.\n");

			delete ret;
			return __KNULL;
		};

		// Iterate one more time and fill in SMP map.
		pos = 0;
		handle = __KNULL;
		mpCpu = x86Mp::getNextCpuEntry(&pos, &handle);
		for (i=0; mpCpu != __KNULL;
			mpCpu = x86Mp::getNextCpuEntry(&pos, &handle))
		{
			if (!__KFLAG_TEST(
				mpCpu->flags, x86_MPCFG_CPU_FLAGS_ENABLED))
			{
				__KFLAG_SET(
					ret->entries[i].flags,
					CHIPSETSMPMAP_FLAGS_BADCPU);

				ret->entries[i].cpuId = mpCpu->lapicId;

				if (__KFLAG_TEST(
					mpCpu->flags, x86_MPCFG_CPU_FLAGS_BSP))
				{
					__KFLAG_SET(
						ret->entries[i].flags,
						CHIPSETSMPMAP_FLAGS_BSP);
				};
				continue;
			};
			__kprintf(NOTICE SMPINFO"getSmpMap: Skipping un-enabled"
				" CPU %d in MP tables.\n",
				mpCpu->lapicId);

		};

		ret->nEntries = i;
		return ret;
	};

	return __KNULL;
}

