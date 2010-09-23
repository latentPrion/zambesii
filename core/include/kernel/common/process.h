#ifndef _PROCESS_H
	#define _PROCESS_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <__kclasses/wrapAroundCounter.h>
	#include <kernel/common/task.h>
	#include <kernel/common/memoryTrib/memoryStream.h>

struct taskS;

struct processS
{
	uarch_t		id;
	taskS		*head;
	utf16Char	*fileName, *filePath, *argString, *env;

	// Tells which CPUs this process has run on.
	bitmapC		cpuTrace;

	memoryStreamC		*memoryStream;
	wrapAroundCounterC	nextTaskId;
	
};

#endif

