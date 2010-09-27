
#include <scaling.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>
#include <__kthreads/__korientation.h>


cpuTribC::cpuTribC(void)
{
}

error_t cpuTribC::initialize2(void)
{
	cpuInfoRivS	*cpuInfoRiv;

#if __SCALING__ >= SCALING_SMP
	cpuInfoRiv = firmwareTrib.getMemInfoRiv();
	assert_fatal(cpuInfoRiv != __KNULL);
#endif

#if __SCALING__ >= SCALING_CC_NUMA
	

cpuTribC::~cpuTribC(void)
{
}

