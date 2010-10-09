
#include <arch/smpInfo.h>
#include <arch/smpMap.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <commonlibs/libacpi/libacpi.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <__kclasses/debugPipe.h>


chipsetNumaMapS *smpInfo::getNumaMap(void)
{
	/**	NOTES:
	 * This function will return a distinct chipsetNumaMapS *, which is
	 * in no way associated with the one returned from any chipset's
	 * getNumaMap().
	 *
	 * The kernel is expected to only parse for the cpu related entries
	 * when it asks the smpInfo:: namespace for a NUMA Map anyway.
	 *
	 *	EXPLANATION:
	 * This function works by first scanning for the ACPI RSDP. If it finds
	 * it, it will then proceed to parse until it finds an SRAT. If it finds
	 * one, it will then parse for any CPU entries and generate a NUMA
	 * map of all CPUs present in any particular bank.
	 *
	 * This function depends on the kernel libacpi.
	 **/
	return __KNULL;
}

archSmpMapS *smpInfo::getSmpMap(void)
{
	x86_mpCfgCpuS	*mpCpu;
	void		*handle, *handle2;
	uarch_t		pos=0, nEntries, i;
	archSmpMapS	*ret;
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
	acpi::flushCache();
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

	// Now look for a MADT.
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

	ret = new archSmpMapS;
	if (ret == __KNULL)
	{
		__kprintf(ERROR SMPINFO"getSmpMap: Failed to alloc SMP map.\n");
		return __KNULL;
	};

	ret->entries = new archSmpMapEntryS[nEntries];
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

	x86Mp::initializeCache();
	if (!x86Mp::mpTablesFound())
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

		ret = new archSmpMapS;
		if (ret == __KNULL)
		{
			__kprintf(ERROR SMPINFO"getSmpMap: Failed to alloc SMP "
				"Map.\n");

			return __KNULL;
		};

		ret->entries = new archSmpMapEntryS[nEntries];
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
					ARCHSMPMAP_FLAGS_BADCPU);

				ret->entries[i].cpuId = mpCpu->lapicId;

				if (__KFLAG_TEST(
					mpCpu->flags, x86_MPCFG_CPU_FLAGS_BSP))
				{
					__KFLAG_SET(
						ret->entries[i].flags,
						ARCHSMPMAP_FLAGS_BSP);
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

