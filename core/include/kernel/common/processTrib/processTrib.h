#ifndef _PROCESS_TRIBUTARY_H
	#define _PROCESS_TRIBUTARY_H

	#include <chipset/chipset.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/wrapAroundCounter.h>
	#include <kernel/common/process.h>
	#include <kernel/common/task.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/processId.h>

class processTribC
:
public tributaryC
{
public:
	processTribC(void);

public:
	// Init __kprocess, __korientation & boot CPU Stream.
	error_t initialize(void);
	error_t initialize2(void);

public:
	processS *__kgetProcess(void) { return &__kprocess; };
	processS *getProcess(processId_t id);
	taskS *getTask(processId_t id);

	processS *spawn(const utf8Char *pathName);
	error_t destroy(void);

private:
	processS		__kprocess;
	wrapAroundCounterC	nextProcId;
};

extern processTribC	processTrib;
extern sharedResourceGroupC<multipleReaderLockC, processS **>	processes;


/**	Inline Methods
 *****************************************************************************/

inline processS *processTribC::getProcess(processId_t id)
{
	processS	*ret;
	uarch_t		rwFlags;

	processes.lock.readAcquire(&rwFlags);
	ret = processes.rsrc[PROCID_PROCESS(id)];
	processes.lock.readRelease(rwFlags);

	return ret;
}

inline taskS *processTribC::getTask(processId_t id)
{
	processS	*p;
	taskS		*ret;
	uarch_t		rwFlags;

	p = getProcess(id);
	if (p == __KNULL) {
		return __KNULL;
	};

	p->taskLock.readAcquire(&rwFlags);
	ret = p->tasks[PROCID_TASK(id)];
	p->taskLock.readRelease(rwFlags);

	return ret;
}

#endif

