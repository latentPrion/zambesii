
#include <__kstdlib/__kcxxlib/cstring>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/kernel_include.h>

// And we define the symbol out here.
sharedResourceGroupC<multipleReaderLockC, cpuStreamC **> cpuStreams;

cpuTribC::cpuTribC(void)
{
	memset(this, 0, sizeof(*this));
}

cpuTribC::~cpuTribC(void)
{
}

