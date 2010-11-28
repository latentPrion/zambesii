#ifndef _PROCESS_TRIBUTARY_H
	#define _PROCESS_TRIBUTARY_H

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

// Inherit affinity from Spawning Thread.
#define PROCESSTRIB_SPAWN_FLAGS_INHERIT_STAFFINITY	(1<<0)
// Inherit affinity from spawning thread's process object.
#define PROCESSTRIB_SPAWN_FLAGS_INHERIT_AFFINITY	(1<<1)
// Inherit elevation from spawning thread's process object.
#define PROCESSTRIB_SPAWN_FLAGS_INHERIT_ELEVATION	(1<<2)
// Inherit exec domain from spawning thread's process object.
#define PROCESSTRIB_SPAWN_FLAGS_INHERIT_EXECDOMAIN	(1<<3)

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

	processStreamC *spawn(
		const utf8Char *pathName/*,
		void *affinity, void *elevation, uarch_t flags*/);

	error_t destroy(void);

public:
	processStreamC		__kprocess;

private:
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

