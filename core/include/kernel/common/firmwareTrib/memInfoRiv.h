#ifndef _MEMORY_INFORMATION_RIVULET_H
	#define _MEMORY_INFORMATION_RIVULET_H

	#include <chipset/memoryConfig.h>
	#include <chipset/memoryMap.h>
	#include <chipset/numaMap.h>
	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * Every firmware will of course provide its own memory enumeration interface.
 * If a chipset which uses a particular firmware interface (OFW, EFI, BIOS, etc)
 * would prefer to provide its *own* memory map, and it would prefer for us to
 * rely on information explicitly given by the vendor than information provided
 * from the firmware (this barely makes sense, but provision must be made), that
 * vendor can have his chipset code's Firmware Stream provide a memInfoRiv that
 * would override the firmware interface's memInfoRiv, and thus the kernel would
 * parse the chipset code's memory informatio structures instead of the firmware
 * interface's.
 *
 * This works because the kernel, on Firmware Tributary initialization, scans
 * the Firmware Streams provided by both the chipset code and the firmware code,
 * and in every case where the chipset code provides a rivulet, that rivulet is
 * henceforth preferred to the firmware code's rivulet. Where no rivulet is
 * provided by the chipset code, the firmware code's rivulet takes precedence.
 *
 *	EXPECTED BEHAVIOUR:
 * getMemoryConfig():
 *	Expects the called rivulet to allocate, *ON THE KERNEL HEAP*, enough
 *	space to hold a structure to hold an instance of chipsetMemConfigS,
 *	and fill it out before returning the address of that structure to the
 *	kernel.
 * getNumaConfig():
 *	Expects the called rivulet to allocate, *ON THE KERNEL HEAP*, enough
 *	space to hold an instance of chipsetNumaMapS, and an array of NUMA
 *	memory bank information structures, and make the chipsetNumaMapS
 *	structure point to the array of NUMA bank info, then return the address
 *	of the chipsetNumaMapS structure.
 * getMemoryMap():
 *	Expects the called rivulet to allocate, *ON THE KERNEL HEAP*, enough
 *	space to hold an instance of chipsetMemMapS, and an array of information
 *	structures describing logical memory regions on the chipset, and
 *	detailing which parts of physical memory are usable, which are reserved,
 *	reclaimable, MMIO, etc.
 *
 *	The instance of chipsetMemMapS is to be made to point to the array of
 *	descriptors, and its address given to the kernel as the return value.
 *
 *	REQUIRED FUNCTIONALITY:
 * Zambezii *requires* at minimum, a figure for "total amount of RAM", which
 * will be expected to be returned on a call to getMemoryConfig(). The chipset
 * code may choose to provide this either explicitly when getMemoryConfig is
 * called, or to simply return a memory map when getMemoryMap() is called, and
 * let the kernel count up the sum of the usable and reserved ranges to derive
 * total RAM on its own.
 *
 * That is, providing nothing but a memory map is sufficient. If your chipset
 * has NUMA functionality, and you wish to have Zambezii act in a NUMA aware
 * manner, you must return a valid value on the call to getNumaMap(). Otherwise
 * Zambezii will assume you have no NUMA on your board and behave as if all
 * physical memory is one shared bank with no latency difference between CPUs.
 **/

struct memInfoRivS
{
	error_t (*initialize)(void);
	error_t (*shutdown)(void);
	error_t (*suspend)(void);
	error_t (*awake)(void);

	// Memory information querying interface.
	struct chipsetMemConfigS *(*getMemoryConfig)(void);
	struct chipsetNumaMapS *(*getNumaMap)(void);
	struct chipsetMemMapS *(*getMemoryMap)(void);
};

#endif

