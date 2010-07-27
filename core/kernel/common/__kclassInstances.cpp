
#include <__kclasses/debugPipe.h>
#include <kernel/common/numaTrib/numaTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>

/**	EXPLANATION:
 * These are the instances of the kernel classes which don't require any
 * arch-specific information to be passed to their constructors. As a rule,
 * most of the kernel classes will be this way, with only one or two requiring
 * arch-specific construction information.
 *
 * The order in which they are placed here does not matter.
 **/
numaTribC		numaTrib;
cpuTribC		cpuTrib;
firmwareTribC		firmwareTrib;
debugPipeC		__kdebug;

