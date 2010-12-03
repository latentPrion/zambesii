
#include <debug.h>
#include <scaling.h>
#include <arch/paging.h>
#include <chipset/memory.h>
#include <chipset/numaMap.h>
#include <chipset/memoryMap.h>
#include <chipset/memoryConfig.h>
#include <chipset/regionMap.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>
#include <kernel/common/firmwareTrib/firmwareTrib.h>

/**	EXPLANATION:
 * To initialize the NUMA Tributary, the steps are:
 *	1. Pre-allocate an instance of NUMA StreamC. This stream must be
 *	   given the ID configured for CHIPSET_MEMORY_NUMA___KSPACE_BANKID.
 *	2. Pre-allocate an array of bytes with enough space to hold an array
 *	   of elements of size:
 *	   	sizeof(hardwareIdListC::arrayNodeS) * ({__kspace bankid} + 1).
 *
 *	3. Initialize a numaMemoryRangeC instance, and set it such that its
 *	   base physical address and size are those configured for
 *	   CHIPSET_MEMORY___KSPACE_BASE and CHIPSET_MEMORY___KSPACE_SIZE.
 *	4. Pre-allocate enough memory for the numaMemoryBankC object to use as
 *	   the single array index to point to the __kspace memory range.
 *	5. In numaTribC::initialize():
 *		a. Call numaStreams.__kspaceSetState() with the address of the
 *		   memory pre-allocated to hold the array with enough space to
 *		   hold the index for __kspace (from (2) above).
 *		b. Call numaStreams.addItem() with the configured bank ID of
 *		   __kspace, and the address of the __kspace NUMA stream.
 *		c. Now call getStream() with the __kspace bank Id, and call
 *		   __kspaceAddMemoryRange() on the handle with the address
 *		   of the __kspace memoryRange object, its array pointer index,
 *		   and the __kspaceInitMem which the chipset has reserved for
 *		   it.
 *
 * CAVEATS:
 * ^ __kspace must have its own bank which is guaranteed NOT to clash with any
 *   other bank ID during numaTribC::initialize2().
 **/


// Initialize the __kspace NUMA Stream to its configured Stream ID.
static numaStreamC	__kspaceNumaStream(CHIPSET_MEMORY_NUMA___KSPACE_BANKID);

numaTribC::numaTribC(void)
{
	nStreams = 0;
#ifdef CHIPSET_MEMORY_NUMA_GENERATE_SHBANK
	sharedBank = NUMATRIB_SHBANK_INVALID;
#endif
}

error_t numaTribC::initialize(void)
{
	error_t		ret;

	numaStreams.__kspaceSetState(
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID,
		static_cast<void *>( initialNumaStreamArray ));

	ret = numaStreams.addItem(
		CHIPSET_MEMORY_NUMA___KSPACE_BANKID, &__kspaceNumaStream);

	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	nStreams = 1;
	return ret;
}

void numaTribC::dump(void)
{
	numaBankId_t	cur;
	numaStreamC	*curStream;

	__kprintf(NOTICE NUMATRIB"Dumping. nStreams %d.\n", nStreams);

	cur = numaStreams.prepareForLoop();
	curStream = (numaStreamC *)numaStreams.getLoopItem(&cur);

	for (; curStream != __KNULL;
		curStream = (numaStreamC *)numaStreams.getLoopItem(&cur))
	{
		__kprintf(NOTICE NUMATRIB"Stream %d:\t",
			curStream->bankId);

		curStream->dump();
	}
}

numaTribC::~numaTribC(void)
{
	nStreams = 0;
}

