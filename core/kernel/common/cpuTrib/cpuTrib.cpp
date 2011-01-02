
#include <scaling.h>
#include <chipset/pkg/chipsetPackage.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <__kthreads/__korientation.h>


cpuTribC::cpuTribC(void)
{
}

error_t cpuTribC::initialize2(void)
{
#if __SCALING__ >= SCALING_CC_NUMA
	chipsetNumaMapS		*numaMap;
	numaCpuBankC		*ncb;
#endif

#if __SCALING__ >= SCALING_SMP
	chipsetSmpMapS		*smpMap;
	cpuModS			*cpuMod;
	error_t			err;

	cpuMod = chipsetPkg.cpus;
	if (cpuMod == __KNULL)
	{
		__kprintf(FATAL"Chipset package contains no CPU info module.");
		return ERROR_FATAL;
	};

	err = cpuMod->initialize();
	if (err != ERROR_SUCCESS) { return err; };
#endif

#if __SCALING__ >= SCALING_CC_NUMA
	numaMap = cpuMod->getNumaMap();
	if (numaMap == __KNULL || numaMap->nCpuEntries == 0)
	{
		__kprintf(WARNING CPUTRIB"NUMA: getNumaMap: NULL or 0 entries."
			"\n");

		goto doSmpMap;
	};

	__kprintf(NOTICE CPUTRIB"NUMA map: %d entries.\n",
		numaMap->nCpuEntries);

	// Print out each entry and spawn an CPU bank object for it.
	for (ubit32 i=0; i<numaMap->nCpuEntries; i++)
	{
		__kprintf(NOTICE CPUTRIB"Entry %d, CPU ID %d, bank ID %d, "
			"Flags 0x%x.\n",
			i,
			numaMap->cpuEntries[i].cpuId,
			numaMap->cpuEntries[i].bankId,
			numaMap->cpuEntries[i].flags);

		ncb = getBank(numaMap->cpuEntries[i].bankId);
		// If the bank doesn't already exist, create it.
		if (ncb == __KNULL)
		{
			err = createBank(numaMap->cpuEntries[i].bankId);
			if (err != ERROR_SUCCESS)
			{
				__kprintf(ERROR"Failed to create numaCpuBankC "
					"object for detected CPU bank %d.\n",
					numaMap->cpuEntries[i].bankId);
			};
		};
	};

	// Go through each entry an set the bit in the CPU bank for the CPU.
	for (ubit32 i=0; i<numaMap->nCpuEntries; i++)
	{
		ncb = getBank(numaMap->cpuEntries[i].bankId);

		if (ncb == __KNULL)
		{
			__kprintf(ERROR"NUMA CPU bank %d exists in NUMA map, "
				"numaCpuBankC object does not exist for it in "
				"NUMA Tributary.\n",
				numaMap->cpuEntries[i].bankId);

			continue;
		};

		ncb->cpus.setSingle(numaMap->cpuEntries[i].bankId);
	};
#endif

doSmpMap:
#if __SCALING__ >= SCALING_SMP
	smpMap = cpuMod->getSmpMap();
	if (smpMap == __KNULL || smpMap->nEntries == 0)
	{
		__kprintf(WARNING CPUTRIB"getSmpMap: Returned NULL or 0 "
			"entries.\n");

		return ERROR_SUCCESS;
	};

	for (uarch_t i=0; i<smpMap->nEntries; i++)
	{
		__kprintf(NOTICE"SMP Map %d: id %d, flags 0x%x.\n",
			i, smpMap->entries[i].cpuId, smpMap->entries[i].flags);
	};
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

