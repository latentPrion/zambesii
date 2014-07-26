
#include <config.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/distributaryTrib/dvfs.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>


static void *getEsp(void)
{
	void		*esp;

	asm volatile (
		"movl	%%esp, %0\n\t"
		: "=r" (esp));

	return esp;
}

static void cisternnEntry(void)
{
	Thread		*self;

	self = static_cast<Thread *>(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask() );

	printf(NOTICE"Cisternn executing; process ID: 0x%x. ESP: 0x%p. "
		"Dormanting.\n",
		self->getFullId(), getEsp());

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

const dvfs::distributaryDescriptorS	floodplainnIndexer=
{
	CC"floodplainn-indexer", CC"Zambesii",
	CC"UDI Driver indexer and search service for Zambesii",
	{
		{ CC"udi-driver-indexer", 1 },
	},
	1,		// Provides only one category.
	0, 0, 0,	// v0.00.000.
	&Floodplainn::indexReaderEntry,
	0
};

const dvfs::distributaryDescriptorS
	* const DistributaryTrib::distributaryDescriptors[] =
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
	&floodplainnIndexer,
	// Null entry to terminate the array. Do not remove.
	NULL
};

