
#include <kernel/common/kernel_include.h>
#include KERNEL_SOURCE_INCLUDE(cpuTrib/cpuTrib.cpp)

// And we define the symbol out here.
sharedResourceGroupC<multipleReaderLockC, cpuStreamC **> cpuStreams;

cpuTribC::cpuTribC(void)
{
}

cpuTribC::~cpuTribC(void)
{
}

