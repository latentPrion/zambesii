#ifndef _SMP_CONFIG_H
	#define _SMP_CONFIG_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>

/**	EXPLANATION:
 * SMP config and NUMA config work together to inform the scheduler on how to
 * handle pull requests. There is a flag in smpConfigS that indicates whether
 * a logical CPU must check SMP affinity before pulling.
 *
 * Every thread has NUMA affinity. The NUMA affinity determines which NUMA CPU
 * banks may pull a task. A bank must pull a task in order for it to be
 * scheduled.
 *
 * In the case of a thread using SMP affinity, after the thread has been pulled
 * by the NUMA bank, its SMP affinity flag is checked, and the kernel sees that
 * the thread has SMP affinity. Therefore only CPUs within the current bank to
 * whom the thread pertains will pull the task.
 *
 * There is no default affinity binding for SMP. If you wish to schedule a task
 * to be run on CPUs X, Y, and Z, yet favour Z, you are out of luck. The chances
 * of a thread being repeatedly pulled by the same bank are only high if that
 * bank is the only bank that it is bound to.
 *
 *	MODEL:
 *	1. Thread with NUMA affinity, and therefore no SMP affinity.
 *		NUMA-bound to: banks 0, 1, 2.
 *			Any CPU within those banks may pull this task.
 *
 *	2. Thread with SMP affinity, and therefore no NUMA affinity.
 *		SMP-bound to: cpus 0, 5, 11, 12, 13 & 18.
 *			In this case, assume that each bank has 4 cpus. Thus
 *			This thread has SMP affinity which spans CPUs in 5
 *			banks.
 *
 *			The kernel will configure this thread to be NUMA-bound
 *			therefore to all banks with whom its SMP config
 *			intersects. This task will be pulled first by any of
 *			those banks, then secondly by one of its preferred CPUs,
 *			which is on the banks that pulled it for that particular
 *			run.
 *
 * That is, for CPU configuration, SMP affinity and NUMA affinity for CPUs are
 * mutually exclusive.
 *
 * FIXME: Will most likely have to change this to be a class rather than a
 * struct later on. Remember the __korientation instance and the fact that
 * constructors are run *after* the kernel orientation process structure is
 * initialized.
 **/

#define SMPCONFIG_FLAGS_USES_SMP	(1<<2)


struct smpConfigS
{
	bitmapC		cpus;
	// Id of the CPU to which this task was last scheduled.
	cpu_t		last;
	uarch_t		flags;
};

#endif

