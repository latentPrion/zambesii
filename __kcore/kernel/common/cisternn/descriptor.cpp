
#include <arch/debug.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>
#include <kernel/common/distributaryTrib/dvfs.h>

static void cisternnEntry(void)
{
	Thread		*self;

	self = static_cast<Thread *>(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread() );

	printf(NOTICE"Cisternn executing; process ID: %x. ESP: %p. "
		"Killing.\n",
		self->getFullId(), debug::getStackPointer());

	taskTrib.kill(self->getFullId());
}

const dvfs::sDistributaryDescriptor	cisternnDescriptor=
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
