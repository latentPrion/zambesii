
#include <config.h>
#include <kernel/common/distributaryTrib/dvfs.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>


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
		},
		{ CC"video output", 0 },
		{ CC"audio input", 1 }
	},
	3,		// Provides only one category.
	0, 0, 0,	// v0.00.000.
	__KNULL,
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

