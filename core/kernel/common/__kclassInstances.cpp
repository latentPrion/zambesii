
#include <__kclasses/debugPipe.h>
#include <__kclasses/memReservoir.h>
#include <__kclasses/cachePool.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/debugTrib/debugTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/execTrib/execTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>

/**	EXPLANATION:
 * These are the instances of the kernel classes which don't require any
 * arch-specific information to be passed to their constructors. As a rule,
 * most of the kernel classes will be this way, with only one or two requiring
 * arch-specific construction information.
 *
 * The order in which they are placed here does not matter. Of course,
 * initializing in order is preferable.
 **/
timerTribC		timerTrib;
interruptTribC		interruptTrib;
memoryTribC		memoryTrib;
debugTribC		debugTrib;
memReservoirC		memReservoir(&processTrib.__kgetStream()->memoryStream);
cpuTribC		cpuTrib;
debugPipeC		__kdebug;
taskTribC		taskTrib;
cachePoolC		cachePool;
execTribC		execTrib;
vfsTribC		vfsTrib;
distributaryTribC	distributaryTrib;
floodplainnC		floodplainn;

