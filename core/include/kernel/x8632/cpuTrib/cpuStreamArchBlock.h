#ifndef x86_32_CPU_STREAM_ARCH_SPECIFIC_BLOCK_H
	#define x86_32_CPU_STREAM_ARCH_SPECIFIC_BLOCK_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

	/**	EXPLANATION:
	 * The arch specific block holds in a neat manner, various things that
	 * are arch specific that the per-arch code for the CPU stream needs
	 * to operate.
	 *
	 * For x86-32 this is mostly needed due to the need for the per-CPU
	 * IPI lock. The per-CPU IPI locks prevent multiple IPIs from being sent
	 * to a single CPU. The reason this is necessary is because IPIs are
	 * used to power up and down CPUs.
	 *
	 * Any type of IPI being receieved while a CPU is doing power management
	 * I/O commands on its LAPIC are going to be detrimental to the sanity
	 * of that CPU's power state.
	 **/

struct cpuStreamArchBlockS
{
	sharedResourceGroupC<waitLockC, uarch_t>	ipiFlag;
};

#endif

