
#include <config.h>
#include <arch/debug.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/distributaryTrib/dvfs.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/cisternn/cisternn.h>

const dvfs::sDistributaryDescriptor	floodplainnIndexer=
{
	CC"floodplainn-indexer", CC"Zambesii",
	CC"UDI Driver indexer and search service for Zambesii",
	{
		{ CC"udi-driver-indexer", 1 },
	},
	1,		// Provides only one category.
	0, 0, 0,	// v0.00.000.
	(void(*)(void))&fplainn::Zui::main,
	0
};

const dvfs::sDistributaryDescriptor
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
#ifdef CONFIG_DTRIB_WATERMARKK
#endif
	&floodplainnIndexer,
	// Null entry to terminate the array. Do not remove.
	NULL
};

