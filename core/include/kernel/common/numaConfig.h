#ifndef _NUMA_CONFIG_H
	#define _NUMA_CONFIG_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <kernel/common/numaTypes.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/sharedResourceGroup.h>

/**	EXPLANATION:
 * NUMA Configuration is done via this simple interface. The idea is to set a
 * default memory bank when allocating memory for the kernel. This way, when
 * allocating memory according to a thread's NUMA policy, we don't waste time
 * searching empty banks, etc.
 *
 * A thread sets its NUMA config via syscalls, and the kernel will set a default
 * bank for it. Memory allocation is done via the default bank. If an allocation
 * attempt on the default bank for a thread fails, all of its configured banks
 * are searched in sequence, and the first one which returns memory is deemed
 * the new default bank.
 *
 * This reduces the amount of lock contention on the bitmap of NUMA banks.
 **/

struct numaConfigS
{
	// Simple bitmap for bank config.
	bitmapC		memBanks;
	// Last bank to which this task was scheduled.
	numaBankId_t	last;
	sharedResourceGroupC<multipleReaderLockC, numaBankId_t>	def;
};

#endif

