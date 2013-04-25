
#include <config.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>


extern const distributaryTribC::distributaryDescriptorS	cisternnDescriptor;

const distributaryTribC::distributaryDescriptorS	cisternnDescriptor=
{
	CC"storage", CC"cisternn",
	CC"The Cisternn storage distributary for the Zambesii kernel",
	CC"Zambesii",
	0, 0, 0,
	__KNULL
};

const distributaryTribC::distributaryDescriptorS
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

