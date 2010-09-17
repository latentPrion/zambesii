#ifndef _PROCESS_TRIBUTARY_H
	#define _PROCESS_TRIBUTARY_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>

class processTribC
:
public tributaryC
{
public:
	processS *spawnProcess(void);
	error_t destroyProcess(void);

	processS *getProcess(void);
	processS *__kgetProcess(void);

private:
	processS	__kprocess;
};

#endif

