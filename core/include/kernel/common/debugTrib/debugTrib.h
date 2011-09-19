#ifndef DEBUG_TRIB_H
	#define DEBUG_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

#define DEBUGTRIB			"Debug Trib: "

#define DEBUGTRIB_ENTER_DELAY		2000

class debugTribC
{
public:
	debugTribC(void);

public:
	void enterFromIrq(void);
	void enterNoIrq(void);
	void exit(void);

private:
	// The number of CPUs currently inside of the debugger.
	sharedResourceGroupC<waitLockC, ubit16>	cpuCount;
};

#endif

