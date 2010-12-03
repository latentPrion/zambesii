#ifndef _NUMA_TRIB_H
	#define _NUMA_TRIB_H

	#include <scaling.h>
	#include <chipset/memory.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/hardwareIdList.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/machineAffinity.h>
	#include <kernel/common/numaTrib/numaStream.h>

/**	EXPLANATION:
 * The NUMA tributary is a software abstraction for handling the different
 * numa nodes which may exist on a chipset. In Zambezii, we refer to nodes
 * as 'banks' where numa is concerned. So a c-bank is a group of CPUs
 * which all exist in the same numa node vicinity. An m-bank is a range of
 * physical RAM which is numa-contiguous.
 *
 * The NUMA trib is reponsible for maintaining a list of all numa banks
 * present, and holding information on which cpus belong to which c-banks, 
 * which mem ranges belong to which c-banks, etc.
 *
 * When it is fully implemented, it will be able to issue the commands
 * necessary to mount and unmount whole banks of cpus, or whole mbanks of
 * RAM.
 *
 * There is a global list of NUMA Streams. A NUMA Stream is a class which
 * holds info on the numa bank as a whole. It holds a numaMemoryBankC and a 
 * numaCpuBankC, which are the relevant cpu and mem related info for the bank.
 *
 * Eventually, we may also include much more complex information in each
 * NUMA bank, such as a numaDeviceBankC, which holds information on I/O
 * device locations, and latencies between CPUs and individual hardware
 * devices so that we can best schedule drivers to run on the closest bank
 * to their device.
 **/

#define NUMATRIB		"Numa Tributary: "

#define NUMATRIB_SHBANK_INVALID		(-1)

class numaTribC
:
public tributaryC
{
public:
	numaTribC(void);
	// Initialize __kspace NUMA bank.
	error_t initialize(void);
	// Detect physical memory, merge with __kspace.
	error_t initialize2(void);
	~numaTribC(void);

public:
	numaStreamC *getStream(numaBankId_t bankId);
	error_t spawnStream(numaBankId_t id);
	void destroyStream(numaBankId_t id);

	void dump(void);

private:
#if __SCALING__ >= SCALING_CC_NUMA \
	&& defined(CHIPSET_MEMORY_NUMA_GENERATE_SHBANK)
	numaBankId_t		sharedBank;
#endif

	uarch_t			nStreams;
	hardwareIdListC		numaStreams;
};

extern numaTribC		numaTrib;

/**	Inline Methods
 *****************************************************************************/

#if __SCALING__ < SCALING_CC_NUMA
inline numaStreamC *numaTribC::getStream(numaBankId_t)
{
	/* In a non-NUMA build we always spawn a single stream. All calls to
	 * numaTribC::getStream(bankId) will return the default stream.
	 * There is no locking on accesses to defaultConfig.rsrc.def since this
	 * should never change on a non-NUMa build.
	 **/
	return static_cast<numaStreamC *>(
		numaStreams.getItem(defaultAffinity.def.rsrc) );
}
#else
inline numaStreamC *numaTribC::getStream(numaBankId_t id)
{
	return static_cast<numaStreamC *>( numaStreams.getItem(id) );
}
#endif

#endif
