#include <scaling.h>
#include <chipset/chipset_include.h>
#include CHIPSET_INCLUDE(chipset.h)


/**	EXPLANATION:
 * Ensure that CHIPSET_NUMA___KSPACE_BANKID is defined. The kernel cannot work
 * unless CHIPSET_NUMA___KSPACE_BANKID is defined.
 **/
#if !defined(CHIPSET_NUMA___KSPACE_BANKID) || CHIPSET_NUMA___KSPACE_BANKID <= 0
	#error "CHIPSET_NUMA___KSPACE_BANKID must be defined, and greater than 0."
#endif

#if __SCALING__ < SCALING_CC_NUMA
	/* For a non-NUMA build of the kernel, CHIPSET_NUMA_GENERATE_SHBANK must
	 * be defined, because the kernel must generate a shared bank on a
	 * non-NUMA build.
	 **/
	#ifndef CHIPSET_NUMA_GENERATE_SHBANK
		#error "CHIPSET_NUMA_GENERATE_SHBANK must be defined for a non-NUMA build."
	#endif
#endif

#ifdef CHIPSET_NUMA_GENERATE_SHBANK
	/* Several things to check if SHBANK is to be generated:
	 *
	 * * CHIPSET_NUMA_SHBANKID must be defined if shbank is to be generated.
	 * * CHIPSET_NUMA_SHBANKID absolutely must not be the same as
	 *   CHIPSET_NUMA___KSPACE_BANKID.
	 **/
	#ifndef CHIPSET_NUMA_SHBANKID
		#error "If CHIPSET_NUMA_GENERATE_SHBANK is defined, CHIPSET_NUMA_SHBANKID must also be defined."
	#endif

	#if CHIPSET_NUMA_SHBANKID == CHIPSET_NUMA___KSPACE_BANKID
		#error "Shared bank ID *CANNOT* be the same as __kspace bank ID."
	#endif
#endif

