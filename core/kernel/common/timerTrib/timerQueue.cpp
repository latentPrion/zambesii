
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/timerTrib/timerQueue.h>


timerQueueC::timerQueueC(ubit32 nativePeriod)
:
currentPeriod(nativePeriod), nativePeriod(nativePeriod)
{
	memset(&schedTimer, 0, sizeof(schedTimer));
}

