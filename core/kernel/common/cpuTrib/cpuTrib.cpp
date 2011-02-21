
#include <scaling.h>
#include <chipset/cpus.h>
#include <chipset/pkg/chipsetPackage.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <__kthreads/__korientation.h>


static cpu_t		highestCpuId=0;

cpuTribC::cpuTribC(void)
{
}

error_t cpuTribC::initialize2(void)
{
	error_t			ret;
	chipsetSmpMapS		*smpMap=__KNULL;
	chipsetNumaMapS		*numaMap=__KNULL;
	cpuModS			*cpuMod;
#if (__SCALING__ == SCALING_SMP) || defined(CHIPSET_CPU_NUMA_GENERATE_SHBANK)
	sarch_t			found;
#endif
	numaCpuBankC		*ncb;

	/**	EXPLANATION:
	 * Several abstractions need to be initialized for the CPU Tributary
	 * to work as it should. These are the internal BMP of all CPUs,
	 * and the per-bank BMPs of CPUs on each bank.
	 *
	 * The kernel will first initialize the "SMP" BMP, which simply has one
	 * bit set for each present CPU, regardless of its NUMA locality.
	 *
	 * Next, for each bank in the NUMA map, the kernel will generate a
	 * numaCpuBankC object which is (for now) nothing more than a BMP of
	 * all CPUs on that bank.
	 *
	 * After that, if CHIPSET_CPU_NUMA_GENERATE_SHBANK is defined, the
	 * the kernel will run through each entry in the SMP map (if present)
	 * and for each CPU in the SMP map that doesn't appear in the NUMA map,
	 * the kernel will add that CPU to the Shared Bank to be treated as a
	 * CPU without any known NUMA locality.
	 *
	 *	NOTES:
	 * ^ In the event that no NUMA map is found (on a NUMA kernel build) or
	 *   that the kernel is an SMP build, all CPUs will be added on the
	 *   shared bank.
	 *
	 * ^ If a NUMA map is found, but no SMP map is found, shbank will not be
	 *   generated.
	 *
	 * ^ On an SMP build, the NUMA map is ignored.
	 * ^ On uni-processor build, both the SMP and NUMA maps are ignored and
	 *   a single CPU is assumed regardless of presence of other CPUs.
	 **/
#if __SCALING__ >= SCALING_SMP
	if (chipsetPkg.cpus != __KNULL)
	{
		if ((*chipsetPkg.cpus->initialize)() == ERROR_SUCCESS)
		{
			cpuMod = chipsetPkg.cpus;

#if __SCALING__ >= SCALING_CC_NUMA
			numaMap = (*cpuMod->getNumaMap)();
#endif
			smpMap = (*cpuMod->getSmpMap)();
		}
		else
		{
			__kprintf(ERROR CPUTRIB"CPU Info mod present, but "
				"initialization failed.\n");

			goto fallbackToUp;
		};
	}
	else
	{
		__kprintf(ERROR CPUTRIB"No CPU Info mod present.\n");
		goto fallbackToUp;
	};
#endif

#if __SCALING__ >= SCALING_SMP
	// First: Find out the highest CPU ID number present on the system.
#if __SCALING__ >= SCALING_CC_NUMA
	if (numaMap != __KNULL)
	{
		for (ubit32 i=0; i<numaMap->nCpuEntries; i++)
		{
			if (numaMap->cpuEntries[i].cpuId > highestCpuId) {
				highestCpuId = numaMap->cpuEntries[i].cpuId;
			};
		};
	};
#endif
	if (smpMap != __KNULL)
	{
		for (ubit32 i=0; i<smpMap->nEntries; i++)
		{
			if (smpMap->entries[i].cpuId > highestCpuId) {
				highestCpuId = smpMap->entries[i].cpuId;
			};
		};
	};
__kprintf(NOTICE"Highest cpu id: %d.\n", highestCpuId);
	// Now initialize "all cpus" bmp using our knowledge of the higest id.
	ret = onlineCpus.initialize(highestCpuId + 1);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR CPUTRIB"Failed to init() online CPU bmp with "
			"%d as the highest detected CPU Id.\n",
			highestCpuId);

		return ret;
	};

#if __SCALING__ >= SCALING_CC_NUMA
	// Now parse the NUMA map.
	if (numaMap != __KNULL)
	{
		for (ubit32 i=0; i<numaMap->nCpuEntries; i++)
		{
			if (getBank(numaMap->cpuEntries[i].bankId) == __KNULL)
			{
				// Create a bank for it if one doesn't exist.
				ret = createBank(numaMap->cpuEntries[i].bankId);
				if (ret != ERROR_SUCCESS)
				{
					__kprintf(ERROR CPUTRIB"createBank(%d) "
						"failed failed returning %d.\n",
						numaMap->cpuEntries[i].bankId,
						ret);

					continue;
				};

				// Initialize the new bank's CPU bmp.
				ret = getBank(numaMap->cpuEntries[i].bankId)
					->cpus.initialize(highestCpuId + 1);

				if (ret != ERROR_SUCCESS)
				{
					__kprintf(ERROR CPUTRIB"Failed to init "
						"CPU bmp on bank %d.\n",
						numaMap->cpuEntries[i].bankId);

					// Destroy if it didn't work.
					destroyBank(
						numaMap->cpuEntries[i].bankId);
				}
				else
				{
					__kprintf(NOTICE"Created bank for bank "
						"id %d.\n",
						numaMap->cpuEntries[i].bankId);
				};
			};
		};
	};

	__kprintf(NOTICE"Finished creating and initializing numa banks.\n");
	__kprintf(NOTICE"Setting per-bank CPU bits.\n");

	if (numaMap != __KNULL)
	{
		for (ubit32 i=0; i<numaMap->nCpuEntries; i++)
		{
			// For each entry, set the bit on its bank.
			ncb = getBank(numaMap->cpuEntries[i].bankId);
			if (ncb == __KNULL)
			{
				__kprintf(WARNING"Bank %d found in NUMA map, "
					"but no bank object was created for it"
					"\n", numaMap->cpuEntries[i].bankId);

				continue;
			};
			ncb->cpus.setSingle(numaMap->cpuEntries[i].cpuId);
		};
	};
#endif

#endif

#if (__SCALING__ == SCALING_SMP) || defined(CHIPSET_CPU_NUMA_GENERATE_SHBANK)
__kprintf(NOTICE"About to create shbank.\n");
	// Generate Shbank.
	ret = createBank(CHIPSET_CPU_NUMA_SHBANKID);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR CPUTRIB"Failed to generate shbank.\n");

#if __SCALING__ >= SCALING_CC_NUMA
		/* If the shared bank could not be generated, and there is no
		 * NUMA map, then we have no CPU to use except (we assume)
		 * the one we're running on. Only option is to fall back to
		 * UP mode.
		 **/
		if (numaMap == __KNULL || numaMap->nCpuEntries == 0) {
			goto fallbackToUp;
		}
		else
		{
			/**	NOTE:
			 * Reasoning here is that we already have the NUMA banks
			 *
			 * done and everything, so we can just set their bits,
			 * wake them up, and then move on without a shared bank.
			 **/
			goto setBits;
		};
#else
		goto fallbackToUp;
#endif
	};
__kprintf(NOTICE"Successfully created shbank, ret is 0x%p.\n", getBank(CHIPSET_CPU_NUMA_SHBANKID));
	ret = getBank(CHIPSET_CPU_NUMA_SHBANKID)->initialize(highestCpuId + 1);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR CPUTRIB"Failed to init bmp on shared bank.\n");
		return ERROR_SUCCESS;
	};

__kprintf(NOTICE"Created and initialized SHBANK.\n");
__kprintf(NOTICE"Dumping Shbank bits before anything gets set officially.\n");
	for (ubit32 i=0;
		i<getBank(CHIPSET_CPU_NUMA_SHBANKID)->cpus.getNBits();
		i++)
	{
		if (getBank(CHIPSET_CPU_NUMA_SHBANKID)->cpus.testSingle(i))
		{
			__kprintf((utf8Char *)"%d ", i);
		};
	};
__kprintf((utf8Char *)"\n");

	/* If there is a NUMA map, run through, and for each CPU in the SMP map
	 * that does not exist in the NUMA map, add it to the shared bank.
	 *
	 * If there is no NUMA map, add all CPUs to the shared bank.
	 **/
	__kprintf(NOTICE"Following cpus are in both smp and numa maps.\n");
	if (smpMap != __KNULL && smpMap->nEntries != 0
		&& numaMap != __KNULL && numaMap->nCpuEntries != 0)
	{
		/* For each entry in the SMP map, check to see if you find
		 * an equivalent CPU ID in the NUMA Map. If not, then add the
		 * CPU to the shared bank.
		 **/
		for (ubit32 i=0; i<smpMap->nEntries; i++)
		{
			found = 0;
			for (ubit32 j=0; j<numaMap->nCpuEntries; j++)
			{
				if (numaMap->cpuEntries[j].cpuId
					== smpMap->entries[i].cpuId)
				{
					found = 1;
					break;
				};
			};

			if (found) { __kprintf((utf8Char *)"%d ", smpMap->entries[i].cpuId); continue; };
			getBank(CHIPSET_CPU_NUMA_SHBANKID)->cpus.setSingle(
				smpMap->entries[i].cpuId);
		};
	}
	else if ((smpMap != __KNULL) && (numaMap == __KNULL))
	{
		for (uarch_t i=0; i<smpMap->nEntries; i++)
		{
			getBank(CHIPSET_CPU_NUMA_SHBANKID)->cpus.setSingle(
				smpMap->entries[i].cpuId);
		};
	}
	else {
		goto fallbackToUp;
	};
	__kprintf((utf8Char *)"\n");
#endif

setBits:
#if __SCALING__ >= SCALING_SMP

#if __SCALING__ >= SCALING_CC_NUMA
	if (numaMap != __KNULL)
	{
		// Set bits on banks and in global BMP.
		for (uarch_t i=0; i<numaMap->nCpuEntries; i++)
		{
			// Set the bit in the "global CPUs" bmp.
			onlineCpus.setSingle(numaMap->cpuEntries[i].cpuId);

			// Set the bit in the CPU's numa bank BMP.
			getBank(numaMap->cpuEntries[i].bankId)->cpus.setSingle(
				numaMap->cpuEntries[i].cpuId);
		};
	};
#endif

	if (smpMap != __KNULL)
	{
		// Do the same for the SMP map, but only set bits in global bmp.
		for (uarch_t i=0; i<smpMap->nEntries; i++)
		{
			if (getBank(CHIPSET_CPU_NUMA_SHBANKID)->cpus.testSingle(
				smpMap->entries[i].cpuId))
			{
				onlineCpus.setSingle(smpMap->entries[i].cpuId);
			};
		};
	};
#endif
	// Don't forget to wake up the CPUs.
	return ERROR_SUCCESS;

fallbackToUp:
#if __SCALING__ > SCALING_UNI_PROCESSOR
	__kprintf(WARNING CPUTRIB"CPU detection fell through to uni-processor "
		"mode. Assuming single CPU.\n");
#endif

#if __SCALING__ >= SCALING_SMP
	ncb = getBank(CHIPSET_CPU_NUMA_SHBANKID);
	if (ncb == __KNULL)
	{
		// Try to create it again for the sake of it.
		ret = createBank(CHIPSET_CPU_NUMA_SHBANKID);
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR CPUTRIB"Failed to create ShBank on a "
				"non-UP build, while attempting to fall back "
				"to UP mode. No CPU management in place.\n\n"

				"Kernel forced to halt orientation.\n");

			return ERROR_FATAL;
		};
	};

	// Set a single bit for CPU 0, our UP mode single CPU.
	ncb->cpus.setSingle(0);
#else
	cpu = getCurrentCpuStream();
#endif
	return ERROR_SUCCESS;
}
	

cpuTribC::~cpuTribC(void)
{
}

error_t cpuTribC::createBank(numaBankId_t id)
{
	error_t		ret;
	numaCpuBankC	*ncb;

	ncb = new (
		(memoryTrib.__kmemoryStream
			.*memoryTrib.__kmemoryStream.memAlloc)(
				PAGING_BYTES_TO_PAGES(sizeof(numaCpuBankC)),
				MEMALLOC_NO_FAKEMAP))
		numaCpuBankC;

	if (ncb == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	ret = cpuBanks.addItem(id, ncb);
	if (ret != ERROR_SUCCESS) {
		memoryTrib.__kmemoryStream.memFree(ncb);
	};

	return ret;
}

void cpuTribC::destroyBank(numaBankId_t id)
{
	cpuBanks.removeItem(id);
}

