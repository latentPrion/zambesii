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

struct taskS;

struct processS
{
	uarch_t			id;
	taskS			*tasks[CHIPSET_MAX_NTASKS];
	multipleReaderLockC	taskLock;

	utf8Char		*fileName, *filePath, *argString, *env;

	// Tells which CPUs this process has run on.
	bitmapC			cpuTrace;
	wrapAroundCounterC	nextTaskId;

	memoryStreamC		*memoryStream;
};

namespace process
{
	error_t initialize(processS *process);
	void destroy(processS *process);
}

#endif

