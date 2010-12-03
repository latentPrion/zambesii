
#include <scaling.h>
#include <arch/smpInfo.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <__kthreads/__korientation.h>


cpuTribC::cpuTribC(void)
{
}

error_t cpuTribC::initialize2(void)
{
	chipsetNumaMapS		*numaMap;
	archSmpMapS		*smpMap;

#if __SCALING__ >= SCALING_CC_NUMA
	numaMap = smpInfo::getNumaMap();
#endif
#if __SCALING__ >= SCALING_SMP
	smpMap = smpInfo::getSmpMap();
	if (smpMap == __KNULL) {
		return ERROR_UNKNOWN;
	};

	for (uarch_t i=0; i<smpMap->nEntries; i++)
	{
		__kprintf(NOTICE"SMP Map %d: id %d, flags 0x%X.\n",
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

