
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/numaCpuBank.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>


numaCpuBankC::numaCpuBankC(void)
:
capacity(0), load(0)
{
}

error_t numaCpuBankC::initialize(uarch_t nBits)
{
	return cpus.initialize(nBits);
}

#if __SCALING__ >= SCALING_SMP
error_t numaCpuBankC::schedule(taskC*task)
{
	ubit32		lowestLoad = 0xFFFFFFFF;
	cpuStreamC	*bestCpu=__KNULL, *curCpu;

	for (uarch_t i=0; i<task->cpuAffinity.getNBits(); i++)
	{
		if (task->cpuAffinity.testSingle(i))
		{
			curCpu = cpuTrib.getStream(i);
			if (curCpu == __KNULL) { continue; };
			if (curCpu->taskStream.getLoad() < lowestLoad)
			{
				bestCpu = curCpu;
				lowestLoad = curCpu->taskStream.getLoad();
			};
		};
	};

	if (bestCpu == __KNULL) {
		return ERROR_UNKNOWN;
	};

	return bestCpu->taskStream.schedule(task);
}
#endif

void numaCpuBankC::updateCapacity(ubit8 action, uarch_t val)
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

void numaCpuBankC::updateLoad(ubit8 action, uarch_t val)
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

