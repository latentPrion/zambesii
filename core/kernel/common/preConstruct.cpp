
#include <__kthreads/__korientationPreConstruct.h>
#include <kernel/common/preConstruct.h>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

void preConstruct(void)
{
	__kprocessPreConstruct();
	__korientationPreConstruct();
	cpuTrib.preConstruct();
}

