
#include <config.h>
#include <kernel/common/distributaryTrib/dvfs.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>

extern const dvfs::distributaryDescriptorS	cisternnDescriptor;

const dvfs::distributaryDescriptorS	cisternnDescriptor=
{
	{ 's','t','o','r','a','g','e','\0' },
	{ 'c','i','s','t','e','r','n','n','\0' },
	{ 'T','h','e',' ','C','i','s','t','e','r','n','n',' ',
		's','t','o','r','a','g','e',' ',
		'd','i','s','t','r','i','b','u','t','a','r','y',' ','f','o','r',
		't','h','e',' ','Z','a','m','b','e','s','i','i',' ',
		'k','e','r','n','e','l','\0' },
	{ 'Z','a','m','b','e','s','i','i','\0' },
	0, 0, 0,
	__KNULL
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

