
#include <kernel/common/numaCpuBank.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>


#if __SCALING__ >= SCALING_SMP
error_t numaCpuBankC::schedule(taskS *task)
{
	ubit32		lowestLoad = 0xFFFFFFFF;
	cpuStreamC	*bestCpu=__KNULL, *curCpu;

	for (uarch_t i=0; i<task->localAffinity.cpus.getNBits(); i++)
	{
		if (task->localAffinity.cpus.testSingle(i))
		{
			curCpu = cpuTrib.getStream(i);
			if (curCpu == __KNULL) { continue; };
			if (curCpu->scheduler.getLoad()	< lowestLoad)
			{
				bestCpu = curCpu;
				lowestLoad = curCpu->scheduler.getLoad();
			};
		};
	};

	if (bestCpu == __KNULL) {
		return ERROR_UNKNOWN;
	};

	return bestCpu->scheduler.addTask(task);
}
#endif

void numaCpuBankC::updateCapacity(ubit8 action, uarch_t val)
{
	switch (action)
	{
	case PROCESSTRIB_UPDATE_ADD:
		capacity += val;
		return;

	case PROCESSTRIB_UPDATE_SUBTRACT:
		capacity -= val;
		return;

	case PROCESSTRIB_UPDATE_SET:
		capacity = val;
		return;

	default: return;
	};
}

void numaCpuBankC::updateLoad(ubit8 action, uarch_t val)
{
	switch (action)
	{
	case PROCESSTRIB_UPDATE_ADD:
		load += val;
		return;

	case PROCESSTRIB_UPDATE_SUBTRACT:
		load -= val;
		return;

	case PROCESSTRIB_UPDATE_SET:
		load = val;
		return;

	default: return;
	};
}

