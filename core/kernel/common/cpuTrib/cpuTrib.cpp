
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/kernel_include.h>

// And we define the symbol out here.
sharedResourceGroupC<multipleReaderLockC, cpuStreamC **> cpuStreams;

cpuTribC::cpuTribC(void)
{
}

cpuTribC::~cpuTribC(void)
{
}

