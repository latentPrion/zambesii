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

#define PROCESSTRIB_UPDATE_ADD		0x0
#define PROCESSTRIB_UPDATE_SUBTRACT	0x1
#define PROCESSTRIB_UPDATE_SET		0x2

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
	processStreamC *__kgetProcess(void) { return &__kprocess; };
	processStreamC *getStream(processId_t id);

	processStreamC *spawn(const utf8Char *pathName);
	error_t destroy(void);

private:
	processStreamC		__kprocess;
	wrapAroundCounterC	nextProcId;
};

extern processTribC	processTrib;
extern sharedResourceGroupC<multipleReaderLockC, processStreamC **>	processes;


/**	Inline Methods
 *****************************************************************************/

inline processStreamC *processTribC::getStream(processId_t id)
{
	processStreamC	*ret;
	uarch_t		rwFlags;

	processes.lock.readAcquire(&rwFlags);
	ret = processes.rsrc[PROCID_PROCESS(id)];
	processes.lock.readRelease(rwFlags);

	return ret;
}

#endif

