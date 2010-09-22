
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/taskTrib/taskTrib.h>


taskTribC::taskTribC(void)
{
	quantumClass[QUANTUMCLASS_NORMAL] = QUANTUMCLASS_NORMAL_INITVAL;
	quantumClass[QUANTUMCLASS_DRIVER] = QUANTUMCLASS_DRIVER_INITVAL;
	custQuantumClass.rsrc.arr = __KNULL;
	custQuantumClass.rsrc.nClasses = 0;
	deadQ.rsrc = __KNULL;
}

status_t taskTribC::createQuantumClass(utf16Char *name, prio_t prio)
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
	for (j=0; j<64 && *name; j++) {
		custQuantumClass.rsrc.arr[pos].name[j] = name[j];
	};
	custQuantumClass.rsrc.arr[pos].name[((j < 64) ? j:63)] = '\0';

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

	getTask(id)->prio = &custQuantumClass.rsrc.arr[qc].prio;

	custQuantumClass.lock.release();
}


