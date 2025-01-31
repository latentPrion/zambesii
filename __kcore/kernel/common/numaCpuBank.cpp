
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/numaCpuBank.h>
#include <kernel/common/thread.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>


#if __SCALING__ >= SCALING_CC_NUMA
NumaCpuBank::NumaCpuBank(void)
:
capacity(0), load(0)
{
	nTasks.rsrc = 0;
}

error_t NumaCpuBank::initialize(uarch_t nBits)
{
	error_t		ret;

	ret = memProximityMatrix.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
	return cpus.initialize(nBits);
}

#if __SCALING__ >= SCALING_SMP
error_t NumaCpuBank::schedule(Thread *thread)
{
	ubit32		lowestLoad = 0xFFFFFFFF;
	CpuStream	*bestCpu=NULL, *curCpu;

	for (uarch_t i=0; i<thread->cpuAffinity.getNBits(); i++)
	{
		if (thread->cpuAffinity.testSingle(i))
		{
			curCpu = cpuTrib.getStream(i);
			if (curCpu == NULL) { continue; };
			if (curCpu->taskStream.getLoad() < lowestLoad)
			{
				bestCpu = curCpu;
				lowestLoad = curCpu->taskStream.getLoad();
			};
		};
	};

	if (bestCpu == NULL) {
		return ERROR_UNKNOWN;
	};

	return bestCpu->taskStream.schedule(thread);
}
#endif

void NumaCpuBank::updateCapacity(ubit8 action, uarch_t val)
{
	switch (action)
	{
	case CAPACITY_UPDATE_ADD:
		capacity += val;
		return;

	case CAPACITY_UPDATE_SUBTRACT:
		capacity -= val;
		return;

	case CAPACITY_UPDATE_SET:
		capacity = val;
		return;

	default: return;
	};

	taskTrib.updateCapacity(action, val);
}

void NumaCpuBank::updateLoad(ubit8 action, uarch_t val)
{
	switch (action)
	{
	case LOAD_UPDATE_ADD:
		load += val;
		return;

	case LOAD_UPDATE_SUBTRACT:
		load -= val;
		return;

	case LOAD_UPDATE_SET:
		load = val;
		return;

	default: return;
	};

	taskTrib.updateLoad(action, val);
}

void NumaCpuBank::__kspaceInitialize(void)
{
	/**	EXPLANATION:
	 * On a NUMA build, __kspace's CPU Bank should hold the __kspace
	 * memory bank ID in its memory proximity matrix.
	 *
	 * At boot, the Memory Trib spawns itself and sets the affinity of the
	 **/
}

#endif /* if __SCALING__ >= SCALING_CC_NUMA */

