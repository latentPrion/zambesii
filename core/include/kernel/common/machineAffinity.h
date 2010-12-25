#ifndef _PROCESS_OCEAN_MACHINE_AFFINITY_H
	#define _PROCESS_OCEAN_MACHINE_AFFINITY_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <kernel/common/numaTypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>

/**	EXPLANATION:
 * Oceann Zambezii is able to migrate whole processes across the cluster.
 * processes in Zambezii are given locality as follows:
 *	machine_name{node[cpu, ...], ...}, ...
 *	E.g:
 *	    john{[0,1,3], 3, [5]}, mary{[8,5,1], [3]}
 *
 * The CPU IDs are absolute CPU IDs. That is, CPU 8 is taken to be CPU8 on
 * that machine, and not CPU8 relative to that NUMA node.
 *
 * Every process has an array of machine names to which it is bound. Whenever
 * a process migrates to another machine on the Oceann cluster, that machine
 * sets that process's "thisMachine" pointer to the array index that matches
 * its own name.
 **/

struct localAffinityS
{
	utf8Char	*name;
	bitmapC		cpus;
	bitmapC		cpuBanks;
	sharedResourceGroupC<multipleReaderLockC, numaBankId_t>	def;
	bitmapC		memBanks;
};

struct affinityS
{
	ubit32		nEntries;
	localAffinityS	*machines;
};

namespace affinity
{
	error_t copyLocal(localAffinityS *dest, localAffinityS *src);
	error_t copyMachine(affinityS *dest, affinityS *src);
	localAffinityS *findLocal(affinityS *affinity, utf8Char *localName);
}

#endif

