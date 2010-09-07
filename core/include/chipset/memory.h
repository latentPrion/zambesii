#include <scaling.h>
#include <chipset/chipset_include.h>
#include CHIPSET_INCLUDE(memory.h)

#if __SCALING__ >= SCALING_CC_NUMA

/**	EXPLANATION:
 * For a NUMA build, The following rules must be enforced at compile time to
 * ensure sanity:
 *
 * ^ CHIPSET_MEMORY_NUMA___KSPACE_BANKID must be defined and greater than 0.
 *
 * ^ if CHIPSET_MEMORY_NUMA_GENERATE_SHBANK is defined, then 
 *   CHIPSET_MEMORY_NUMA_SHBANKID must also be defined, and must be greater
 *   than 0, and NOT equal to CHIPSET_MEMORY_NUMA___KSPACE_BANKID.
 **/
	#if (!defined(CHIPSET_MEMORY_NUMA___KSPACE_BANKID)) \
		|| (CHIPSET_MEMORY_NUMA___KSPACE_BANKID <= 0)

		#error "CHIPSET_MEMORY_NUMA___KSPACE_BANKID must be defined and its value must be greater than 0."
	#endif

	#ifdef CHIPSET_MEMORY_NUMA_GENERATE_SHBANK
		#if (!defined(CHIPSET_MEMORY_NUMA_SHBANKID)) \
			|| (CHIPSET_MEMORY_NUMA_SHBANKID == CHIPSET_MEMORY_NUMA___KSPACE_BANKID) \
			|| (CHIPSET_MEMORY_NUMA_SHBANKID == 0)

			#error "If CHIPSET_MEMORY_NUMA_GENERATE_SHBANK is defined, CHIPSET_MEMORY_NUMA_SHBANKID must also be defined, and its value must be greater than 0, and not equal to the value set for CHIPSET_MEMORY_NUMA___KSPACE_BANKID."
		#endif
	#endif

#else

/**	EXPLANATION:
 * For a non-NUMA build, the following rules must be enforced at compile time
 * to ensure sanity:
 *
 * ^ CHIPSET_MEMORY_NUMA___KSPACE_BANKID must be defined and greater than 0.
 *
 * ^ CHIPSET_MEMORY_NUMA_GENERATE_SHBANK will be force-defined if it is not
 *   not already defined.
 *
 * ^ CHIPSET_MEMORY_NUMA_SHBANKID will be force-defined to 0.
 **/

	#if (!defined(CHIPSET_MEMORY_NUMA___KSPACE_BANKID)) \
		|| (CHIPSET_MEMORY_NUMA___KSPACE_BANKID <= 0)

		#error "CHIPSET_MEMORY_NUMA___KSPACE_BANKID must be defined and its value must be greater than 0."
	#endif

	#ifdef CHIPSET_MEMORY_NUMA_SHBANKID
		#undef CHIPSET_MEMORY_NUMA_SHBANKID
	#endif

	#ifndef CHIPSET_MEMORY_NUMA_GENERATE_SHBANK
		#define CHIPSET_MEMORY_NUMA_GENERATE_SHBANK
	#endif
	#define CHIPSET_MEMORY_NUMA_SHBANKID		0

#endif

