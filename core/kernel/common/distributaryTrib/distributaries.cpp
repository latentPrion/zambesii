
#include <config.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/distributaryTrib/dvfs.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>


void *getEsp(void)
{
	void		*esp;

	asm volatile (
		"movl	%%esp, %0\n\t"
		: "=r" (esp));

	return esp;
}

static void cisternnEntry(void)
{
	threadC		*self;

	self = static_cast<threadC *>(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask() );

	__kprintf(NOTICE"Cisternn executing; process ID: 0x%x. ESP: 0x%p. "
		"Dormanting.\n",
		self->getFullId(), getEsp());

	taskTrib.wake((processId_t)0x10001);
	taskTrib.dormant(self->getFullId());
}

const dvfs::distributaryDescriptorS	cisternnDescriptor=
{
	CC"cisternn", CC"Zambesii",
	CC"Cisternn storage distributary for Zambesii",
	{
		{
			CC"storage",
#ifdef CONFIG_DTRIB_CISTERNN
			1
#else
			0
#endif
		}
	},
	1,		// Provides only one category.
	0, 0, 0,	// v0.00.000.
	&cisternnEntry,
	0
};

const dvfs::distributaryDescriptorS
	* const distributaryTribC::distributaryDescriptors[] =
{
#ifdef CONFIG_DTRIB_CISTERNN
	&cisternnDescriptor,
#endif
#ifdef CONFIG_DTRIB_AQUEDUCTT
#endif
#ifdef CONFIG_DTRIB_REFLECTIONN
#endif
#ifdef CONFIG_DTRIB_CAURALL
#endif
	// Null entry to terminate the array. Do not remove.
	__KNULL
};

