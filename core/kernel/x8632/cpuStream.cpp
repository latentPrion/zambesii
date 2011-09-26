
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/cpuTrib/cpuStream.h>
#include <__kthreads/__kcpuPowerOn.h>


error_t cpuStreamC::initialize(void)
{
	if (!__KFLAG_TEST(flags, CPUSTREAM_FLAGS_BSP))
	{
		// Load DR0 with a pointer to this CPU's CPU Stream.
		asm volatile (
			"pushl	%%eax \n\t \
			movl	%%dr7, %%eax \n\t \
			andl	$0xFFFFFE00, %%eax \n\t \
			movl	%%eax, %%dr7 \n\t \
			popl	%%eax \n\t \
			\
			movl	%0, %%dr0 \n\t"
			:
			: "r" (this));

		// Load the __kcpuPowerOnThread into the currentTask holder.
		currentTask = &__kcpuPowerOnThread;
	};

	return ERROR_SUCCESS;
}

