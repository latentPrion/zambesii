#ifndef _CHIPSET_H
	#define _CHIPSET_H
	//IBM-PC

	#include <scaling.h>

#define CHIPSET_SHORT_STRING		"IBM-PC"
#define CHIPSET_LONG_STRING		"IBM-PC compatible chipset"
#define CHIPSET_PATH			ibmPc

#define CHIPSET_HAS_ACPI

// The mask which would tie/untie the debug pipe to/from all output devices.
#define CHIPSET_DEBUG_DEVICE_TERMINAL_MASK	0x1

/* Shared bank is always generated for IBM-PC. If you are building a custom
 * kernel for a NUMA supporting PC compatible, and you do not want a shared bank
 * you can comment this out though.
 *
 * Do not comment this out on a NON-NUMA build. Shared bank MUST be generated on
 * NON-NUMA builds. There are preprocessor checks to enforce this at compile
 * time.
 *
 * You can choose not to generate shared bank on a NUMA build. This would mean
 * that any CPUs for which the kernel cannot determine NUMA affinity will be
 * silently ignored.
 **/
#define CHIPSET_NUMA_GENERATE_SHBANK

#if __SCALING__ >= SCALING_CC_NUMA
	#define CHIPSET_NUMA___KSPACE_BANKID		32
	#define CHIPSET_NUMA_SHBANKID			31
#else
	// On non-NUMA there will only be __kspace & shbank. Set __kspace to 1.
	#define CHIPSET_NUMA___KSPACE_BANKID		1
	#define CHIPSET_NUMA_SHBANKID			0
#endif

#endif

