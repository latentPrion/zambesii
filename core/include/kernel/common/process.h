#ifndef _PROCESS_H
	#define _PROCESS_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/task.h>
	#include <kernel/common/memoryTrib/memoryStream.h>

struct taskS;

struct processS
{
	uarch_t		id;
	taskS		*head;
	char		*fileName, *filePath, *argString;

	memoryStreamC	*memoryStream;
};

void __kprocessPreConstruct(void);
extern processS		__kprocess;

#endif

