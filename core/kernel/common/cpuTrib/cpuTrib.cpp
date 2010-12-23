
#include <scaling.h>
#include <arch/smpInfo.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
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
		__kprintf(NOTICE"SMP Map %d: id %d, flags 0x%x.\n",
			i, smpMap->entries[i].cpuId, smpMap->entries[i].flags);
	};
#endif
	return ERROR_SUCCESS;
}
	

cpuTribC::~cpuTribC(void)
{
}

