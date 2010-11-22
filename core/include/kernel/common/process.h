#ifndef _PROCESS_H
	#define _PROCESS_H

	#include <chipset/chipset.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <__kclasses/wrapAroundCounter.h>
	#include <kernel/common/task.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/memoryTrib/memoryStream.h>

#define PROCESS_INIT_MAGIC	0x1D0C3551

struct taskS;

class processC
{
public:
	processC(processId_t id);
	error_t initialize(utf8Char *absName);
	~processC(void);

public:
	taskS *getTask(processId_t processId);
	
public:
	uarch_t			id;
	taskS			*tasks[CHIPSET_MAX_NTASKS];
	multipleReaderLockC	taskLock;

	utf8Char		*absName, *argString, *env;

	// Tells which CPUs this process has run on.
	bitmapC			cpuTrace;
	wrapAroundCounterC	nextTaskId;

	memoryStreamC		*memoryStream;
	ubit32			initMagic;
};


/**	Inline Methods:
 *****************************************************************************/

inline taskS *processC::getTask(processId_t id)
{
	taskS		*ret;
	uarch_t		rwFlags;

	taskLock.readAcquire(&rwFlags);
	ret = tasks[PROCID_TASK(id)];
	taskLock.readRelease(rwFlags);

	return ret;
}

#endif

