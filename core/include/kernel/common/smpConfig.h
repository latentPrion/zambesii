#ifndef _SMP_CONFIG_H
	#define _SMP_CONFIG_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <kernel/common/smpTypes.h>

/**	EXPLANATION:
 * Every thread inherently has SMP config. SMP config is the bitmap of all the
 * CPUs on which this thread may run. Every time a thread is migrated to a new
 * CPU, that CPU's bit is set in the process's bitmap of CPUs on which the
 * process has run. The reason this is important is that it greatly optimizes
 * TLB shootdown: if we can determine which CPUs a process has run on, we can
 * avoid unnecessary TLB shootdown on CPUs it hasn't touched yet. The purpose
 * of the 'last' member is to keep track of the last CPU on which the task was
 * run: using this, we can avoid having to acquire the lock on the process's
 * BMP to set the CPU's bit every time a task is scheduled.
 *
 * A thread which is NUMA bound to a bank for scheduling is implicitly SMP
 * bound to the CPUs on that bank. When a thread's NUMA CPU config changes,
 * its SMP config inherently changes as well. That is, the kernel will set/unset
 * bits in the SMP config for each CPU in the bank, unless the required changes
 * are much finer grained.
 **/

struct sSmpConfig
{
	Bitmap		cpus;
	// Id of the CPU to which this task was last scheduled.
	cpu_t		last;
	uarch_t		flags;
};

#endif

