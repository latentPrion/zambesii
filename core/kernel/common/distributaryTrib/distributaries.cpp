
#include <config.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>


const distributaryTribC::distributaryDescriptorS
	* const distributaryTribC::distributaryDescriptors[] =
{
#ifdef CONFIG_DTRIB_CISTERNN
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

