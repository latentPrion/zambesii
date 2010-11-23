#ifndef _PROCESS_H
	#define _PROCESS_H

	#include <chipset/chipset.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <__kclasses/wrapAroundCounter.h>
	#include <kernel/common/task.h>
	#include <kernel/common/machineAffinity.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/memoryTrib/memoryStream.h>

#define PROCESS_INIT_MAGIC	0x1D0C3551

struct taskS;

class processStreamC
{
public:
	processStreamC(processId_t id);
	error_t initialize(utf8Char *absName);
	~processStreamC(void);

public:
	taskS *getTask(processId_t processId);
	
public:
	uarch_t			id;
	taskS			*tasks[CHIPSET_MAX_NTASKS];
	multipleReaderLockC	taskLock;

	utf8Char		*absName, *argString, *env;

	// Oceann and local affinity.
	affinityS		affinity;
	localAffinityS		*localAffinity;

	// Tells which CPUs this process has run on.
	bitmapC			cpuTrace;
	wrapAroundCounterC	nextTaskId;
	ubit32			initMagic;

	memoryStreamC		*memoryStream;
};


/**	Inline Methods:
 *****************************************************************************/

inline taskS *processStreamC::getTask(processId_t id)
{
	taskS		*ret;
	uarch_t		rwFlags;

	taskLock.readAcquire(&rwFlags);
	ret = tasks[PROCID_TASK(id)];
	taskLock.readRelease(rwFlags);

	return ret;
}

#endif

