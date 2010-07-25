
#include <kernel/common/task.h>
#include <kernel/common/cpuTrib/cpuStream.h>

// We make a global cpuStream for the bspCpu.
cpuStreamC		bspCpu;

cpuStreamC::cpuStreamC(void)
{
	/**	NOTE:
	 * Remember never to run a memset(this, 0, sizeof(*this)) inside of
	 * here since the cpuStream for the BSP is filled with values *BEFORE*
	 * any of the kernel classes can even be constructed, so if you go
	 * and zero CPU Streams out in their constructors, you'll just
	 * go and erase all the values that have been filled into the BSP
	 * CPU during __korientationPreConstruct::bspInit().
	 **/
}

cpuStreamC::cpuStreamC(cpu_t id)
:
cpuId(id)
{
}

cpuStreamC::~cpuStreamC(void)
{
}

