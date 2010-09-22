#ifndef _PROCESS_TRIBUTARY_H
	#define _PROCESS_TRIBUTARY_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/hardwareIdList.h>
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

public:
	processS *__kgetProcess(void) { return &__kprocess; };
	processS *getProcess(processId_t id)
	{
		return processes.getItem(PROCID_PROCESS(id));
	};

	processS *getProcess(const utf16Char *absName);

	processS *spawn(const utf16Char *absName);
	error_t destroy(void);

private:
	processS	__kprocess;
	hardwareIdListC<processS>	processes;
};

extern processTribC	processTrib;

#endif

