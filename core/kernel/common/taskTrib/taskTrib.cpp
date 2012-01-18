
#include <scaling.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/numaCpuBank.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


taskTribC::taskTribC(void)
{
	capacity = 0;
	load = 0;
}

#if __SCALING__ >= SCALING_CC_NUMA
error_t taskTribC::schedule(taskC*task)
{
	ubit32		lowestLoad=0xFFFFFFFF;
	numaCpuBankC	*bestBank=__KNULL, *curBank;

	// Go according to NUMA affinity.
	for (uarch_t i=0; i<task->localAffinity.cpuBanks.getNBits(); i++)
	{
		if (task->localAffinity.cpuBanks.testSingle(i))
		{
			curBank = cpuTrib.getBank(i);
			if (curBank == __KNULL) { continue; };
			if (curBank->getLoad() < lowestLoad)
			{
				bestBank = curBank;
				lowestLoad = curBank->getLoad();
			};
		};
	};

	if (bestBank == __KNULL) {
		return ERROR_UNKNOWN;
	};

	return bestBank->schedule(task);
}
#endif

void taskTribC::updateCapacity(ubit8 action, uarch_t val)
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
}

void taskTribC::updateLoad(ubit8 action, uarch_t val)
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
}

#if 0
status_t taskTribC::createQuantumClass(utf8Char *name, prio_t prio)
{
	sarch_t		pos=-1;
	uarch_t		j;
	quantumClassS	*mem, *old;

	custQuantumClass.lock.acquire();

	for (uarch_t i=0; i<custQuantumClass.rsrc.nClasses; i++)
	{
		if (custQuantumClass.rsrc.arr[i].name == __KNULL) {
			pos = static_cast<sarch_t>( i );
		};
	};

	if (pos == -1)
	{
		mem = new quantumClassS[custQuantumClass.rsrc.nClasses + 1];
		if (mem == __KNULL)
		{
			custQuantumClass.lock.release();
			return ERROR_MEMORY_NOMEM;
		};

		memcpy(
			mem, custQuantumClass.rsrc.arr,
			sizeof(quantumClassS) * custQuantumClass.rsrc.nClasses);

		old = custQuantumClass.rsrc.arr;
		delete old;
		custQuantumClass.rsrc.arr = mem;
		pos = custQuantumClass.rsrc.nClasses;
		custQuantumClass.rsrc.nClasses++;
	};

	// FIXME: use soft->hard conversion here.
	custQuantumClass.rsrc.arr[pos].prio = prio;
	for (j=0; j<127 && *name; j++) {
		custQuantumClass.rsrc.arr[pos].name[j] = name[j];
	};
	custQuantumClass.rsrc.arr[pos].name[((j < 127) ? j:127)] = '\0';

	return pos;
}

void taskTribC::setClassQuantum(sarch_t qc, prio_t prio)
{
	custQuantumClass.lock.acquire();

	if (qc >= static_cast<sarch_t>( custQuantumClass.rsrc.nClasses ))
	{
		custQuantumClass.lock.release();
		return;
	};

	// FIXME: Use soft->hard prio conversion here.
	custQuantumClass.rsrc.arr[qc].prio = prio;

	custQuantumClass.lock.release();
}

void taskTribC::setTaskQuantumClass(processId_t id, sarch_t qc)
{
	custQuantumClass.lock.acquire();

	if (qc >= static_cast<sarch_t>( custQuantumClass.rsrc.nClasses ))
	{
		custQuantumClass.lock.release();
		return;
	};

	processTrib.getTask(id)->prio = &custQuantumClass.rsrc.arr[qc].prio;

	custQuantumClass.lock.release();
}
#endif

