
#include <debug.h>
#include <scaling.h>
#include <chipset/cpus.h>
#include <chipset/zkcm/zkcmCore.h>
#include <arch/cpuControl.h>
#include <arch/atomic.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <__kthreads/__korientation.h>
#include <__kthreads/__kcpuPowerOn.h>
#include <arch/atomic.h>


cpu_t		CpuStream::highestCpuId=CPUID_INVALID;
numaBankId_t	CpuStream::highestBankId=NUMABANKID_INVALID;
uarch_t		CpuStream::nCpusInExcessOfConfigMaxNcpus=0;

/**
 * Helper function to check if a CPU ID exceeds CONFIG_MAX_NCPUS
 *
 * @param mapType A string describing the map type for error messages
 * @param cpuId The CPU ID to check
 * @return non-zero if the CPU ID is valid (less than CONFIG_MAX_NCPUS), zero otherwise
 */
static sarch_t checkCpuIdLimit(const char* mapType, cpu_t cpuId)
{
    if (cpuId >= CONFIG_MAX_NCPUS)
    {
        printf(WARNING CPUTRIB"checkCpuIdLimit(%s): CPU ID %d exceeds "
			"CONFIG_MAX_NCPUS (%d). Ignoring this CPU.\n",
            mapType, cpuId, CONFIG_MAX_NCPUS);

        return 0;
    }
    return 1;
}

/**
 * Get CPU ID from a NUMA map entry
 */
static cpu_t getNumaMapCpuId(const sZkcmNumaMap* numaMap, ubit32 index)
{
    return numaMap->cpuEntries[index].cpuId;
}

/**
 * Get bank ID from a NUMA map entry
 */
static numaBankId_t getNumaMapBankId(const sZkcmNumaMap* numaMap, ubit32 index)
{
    return numaMap->cpuEntries[index].bankId;
}

/**
 * Get CPU ID from an SMP map entry
 */
static cpu_t getSmpMapCpuId(const sZkcmSmpMap* smpMap, ubit32 index)
{
    return smpMap->entries[index].cpuId;
}

/**
 * Helper function to find the highest ID in a collection
 * This is a common implementation that can be used for different ID types
 */
template <typename IdType, typename MapType>
static IdType findHighestId(
    const char* mapType,
    const MapType* map,
    ubit32 nEntries,
    IdType currHighest,
    IdType (*getIdFunc)(const MapType*, ubit32),
    sarch_t checkLimit)
{
    for (ubit32 i = 0; i < nEntries; i++)
    {
        const IdType entryId = getIdFunc(map, i);
        if ((signed)entryId > (signed)currHighest)
        {
            // Skip CPUs with IDs exceeding CONFIG_MAX_NCPUS if checkLimit is true
            if (checkLimit && sizeof(IdType) == sizeof(cpu_t))
            {
                // Only check CPU IDs against CONFIG_MAX_NCPUS
                if (!checkCpuIdLimit(mapType, (cpu_t)entryId))
					{ continue; };
            }
            currHighest = entryId;
        }
    }
    return currHighest;
}

/**
 * Get the highest CPU ID from a NUMA map
 */
static cpu_t getHighestCpuIdFromNumaMap(
    const sZkcmNumaMap* numaMap,
    cpu_t currHighest)
{
    return findHighestId<cpu_t, sZkcmNumaMap>(
        "NUMA map",
        numaMap, numaMap->nCpuEntries,
        currHighest,
        getNumaMapCpuId, 1);
}

/**
 * Get the highest CPU ID from an SMP map
 */
static cpu_t getHighestCpuIdFromSmpMap(
    const sZkcmSmpMap* smpMap,
    cpu_t currHighest)
{
    return findHighestId<cpu_t, sZkcmSmpMap>(
        "SMP map",
        smpMap, smpMap->nEntries,
        currHighest,
        getSmpMapCpuId, 1);
}

/**
 * Get the highest bank ID from a NUMA map
 */
static numaBankId_t getHighestBankIdFromNumaMap(
    const sZkcmNumaMap* numaMap,
    numaBankId_t currHighest)
{
    return findHighestId<numaBankId_t, sZkcmNumaMap>(
        "NUMA bank",
		numaMap, numaMap->nCpuEntries,
        currHighest,
		getNumaMapBankId, 0);
}

CpuTrib::CpuTrib(void)
:
_usingChipsetSmpMode(0)
{
	/**	EXPLANATION
	 * Even on a uniprocessor build of the kernel, we set this to
	 * CPUID_INVALID at boot, because "bspId" being equal to INVALID lets
	 * the kernel know that CPU detection has not yet been run. This fact is
	 * used when initializing the BSP's scheduler.
	 *
	 * In addition, we could have been compiled as a uniprocessor kernel,
	 * but be running on a possibly multprocessor board. In that case, we
	 * will be taking advantage of the LAPICs and IO-APICs if possible, and
	 * will thus need to dynamically detect the BSP's ID anyway. Can't
	 * safely assume we can call the BSP "0" just because it's a
	 * uniprocessor kernel build.
	 **/
}

CpuTrib::~CpuTrib(void)
{
}

#include <__kclasses/memReservoir.h>
error_t CpuTrib::initialize(void)
{
	error_t		ret;

#if __SCALING__ >= SCALING_CC_NUMA
	ret = availableBanks.initialize(0);
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = cpuBanks.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
#endif
#if __SCALING__ >= SCALING_SMP
	ret = availableCpus.initialize(0);
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = onlineCpus.initialize(0);
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = cpuStreams.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
#endif

	return ERROR_SUCCESS;
}

#include <arch/debug.h>
error_t CpuTrib::loadBspInformation(void)
{
	error_t		ret;

	// Get BSP's hardware ID.
	ret = zkcmCore.cpuDetection.loadBspId(&CpuStream::bspCpuId, 1);
	if (ret != ERROR_SUCCESS)
	{
		printf(FATAL CPUTRIB"loadBspInformation: ZKCM loadBspId failed "
			"because %s.\n",
			strerror(ret));

		return ret;
	};

	if (CpuStream::bspCpuId == CPUID_INVALID)
	{
		printf(WARNING CPUTRIB"loadBspInformation: ZKCM loadBspId "
			"returned INVALID as ID for BSP.\n"
			"\tAssuming BSP CPU ID is 0.\n");

		CpuStream::bspCpuId = 0;
	};

	printf(NOTICE CPUTRIB"loadBspInformation: BSP CPU ID is %d.\n",
		CpuStream::bspCpuId);

#if __SCALING__ >= SCALING_CC_NUMA
	/**	EXPLANATION:
	 * If this is a NUMA build, then we also need to find out the ID of the
	 * bank in which the BSP is located. We get the NUMA map, look for the
	 * BSP CPU, and try to get its bank ID. If the BSP CPU is not listed in
	 * the NUMA map, or no NUMA map is obtained, we place the BSP CPU into
	 * the shared bank.
	 **/
	sZkcmNumaMap	*numaMap;

	numaMap = zkcmCore.cpuDetection.getNumaMap();
	if (numaMap != NULL && numaMap->nCpuEntries != 0)
	{
		for (uarch_t i=0; i<numaMap->nCpuEntries; i++)
		{
			if (numaMap->cpuEntries[i].cpuId == CpuStream::bspCpuId)
			{
				CpuStream::bspBankId =
					numaMap->cpuEntries[i].bankId;

				printf(NOTICE CPUTRIB"initializeBspStream: "
					"BSP CPU bank ID is %d.\n",
					CpuStream::bspBankId);
			};
		};
	};
#endif

	if (CpuStream::bspBankId == NUMABANKID_INVALID)
	{
		printf(WARNING CPUTRIB"loadBspInformation: Unable to "
			"determine BSP bank ID.\n"
			"\tAssuming BSP CPU is on SHBANKID (%d).\n",
			CHIPSET_NUMA_SHBANKID);

		CpuStream::bspBankId = CHIPSET_NUMA_SHBANKID;
	};

	/**	TODO:
	 * If possible, load the BSP's ACPI ID as well.
	 **/
	sZkcmSmpMap		*smpMap;

	smpMap = zkcmCore.cpuDetection.getSmpMap();
	if (smpMap != NULL && smpMap->nEntries > 0)
	{
		for (uarch_t i=0; i<smpMap->nEntries; i++)
		{
			if (smpMap->entries[i].cpuId == CpuStream::bspCpuId)
			{
				CpuStream::bspAcpiId =
					smpMap->entries[i].cpuAcpiId;

				break;
			};
		};
	};

	if (CpuStream::bspAcpiId == CPUID_INVALID)
	{
		printf(WARNING CPUTRIB"loadBspInformation: Failed to detect BSP ACPI "
			"ID.\n");
	};

	// Check if BSP CPU ID exceeds CONFIG_MAX_NCPUS
	if (CpuStream::bspCpuId >= CONFIG_MAX_NCPUS)
	{
		printf(ERROR CPUTRIB"loadBspInformation: BSP CPU ID %d "
			"exceeds CONFIG_MAX_NCPUS (%d).\n",
			CpuStream::bspCpuId, CONFIG_MAX_NCPUS);

		/** EXPLANATION:
		 * We must panic here because the BSP is essential and we can't manage it
		 * if its ID exceeds CONFIG_MAX_NCPUS (powerStacks array size limitation)
		 **/
		panic(ERROR CPUTRIB"Cannot proceed with BSP CPU ID that exceeds "
			"CONFIG_MAX_NCPUS.\n");
	}

	// Set highestCpuId atomically
	atomicAsm::set(&CpuStream::highestCpuId, CpuStream::bspCpuId);
	CpuStream::highestBankId = CpuStream::bspBankId;

	return ERROR_SUCCESS;
}

error_t CpuTrib::displayUpOperationOnMpBuildMessage(void)
{
	printf(WARNING"The kernel is operating with only one CPU on a\n"
		"\tmultiprocessor build;\n\t%s\n.",
		(_usingChipsetSmpMode == 1)
			? "\n\thowever, hotplug of new CPUs is allowed"
			: "\n\tthis chipset is not safe for multiprocessing");

	return ERROR_SUCCESS;
}

error_t CpuTrib::initializeAllCpus(void)
{
	error_t			ret;
#if __SCALING__ >= SCALING_CC_NUMA
	sZkcmNumaMap		*numaMap;
#endif
#if __SCALING__ == SCALING_SMP || defined(CHIPSET_NUMA_GENERATE_SHBANK)
	sZkcmSmpMap		*smpMap;
#endif

	/**	EXPLANATION:
	 * Second initialization seqence for the CPU Tributary. This routine
	 * detects all CPUs present at boot and powers them up.
	 *
	 * The chipset is asked to run a check to see if it is safe for the
	 * kernel to use SMP/multi-CPU modes on the machine. If the report
	 * comes back with a positive result, the kernel will proceed to
	 * use one of the processing mode initialization functions (numaInit(),
	 * smpInit(), uniProcessorInit()).
	 *
	 * If on a build with scaling higher than uni-processor, the kernel is
	 * told by the chipset that MP is NOT safe on that board, the kernel
	 * will fall into a false uni-processor mode, and hot-plug of new
	 * CPUs will be rejected.
	 *
	 * The reasoning behind this is that for example, on x86-based boards,
	 * checkSmpSanity() will detect the presence of the APIC logic. If that
	 * doesn't exist, it will return FALSE for SMP safety. If the kernel
	 * were to treat such a board as an SMP board which just only had 1 CPU,
	 * then on a hotplug event, since there are no LAPICs/IO-APICs, we would
	 * have a board failure possibly.
	 *
	 * The safest way to handle the case where the chipset module reports an
	 * unsafe MP environment is to force the kernel not to use MP operation.
	 **/
#if __SCALING__ >= SCALING_SMP
	/**	EXPLANATION:
	 * Next ask the chipset if multi-cpu is safe on this machine. If SMP
	 * is safe, then the kernel immediately switches the machine into SMP
	 * mode.
	 **/
	if (zkcmCore.cpuDetection.checkSmpSanity())
	{
		ret = zkcmCore.cpuDetection.setSmpMode();
		if (ret != ERROR_SUCCESS)
		{
			printf(ERROR CPUTRIB"initialize2: Chipset failed "
				"to safely transition to SMP mode.\n"
				"\tProceeding in uniprocessor mode.\n");
		}
		else {
			_usingChipsetSmpMode = 1;
		};
	}
	else
	{

		printf(ERROR CPUTRIB"initialize2:\n"
			"\tIMPORTANT: Your kernel was compiled as a multi-cpu\n"
			"\tbuild kernel, but your chipset reports that it is\n"
			"\tnot safe to use multi-cpu processing on it.\n"
			"\t\tIf your board is a new board, this may indicate\n"
			"\tthat it is flawed or defective. If it is an old\n"
			"\tboard, it may just mean that your kernel was built\n"
			"\twith multi-cpu features that your board simply\n"
			"\tcan't handle.\n");

		// TODO: Small delay here, maybe?
		displayUpOperationOnMpBuildMessage();
		return ERROR_SUCCESS;
	};

	/* On MP build it's beneficial to pre-determine the size of the BMPs
	 * so that when CPUs are being spawned the BMPs aren't constantly
	 * resized.
	 **/
#if __SCALING__ >= SCALING_CC_NUMA
	numaMap = zkcmCore.cpuDetection.getNumaMap();
	if (numaMap != NULL && numaMap->nCpuEntries > 0)
	{
		CpuStream::highestBankId = getHighestBankIdFromNumaMap(
			numaMap, CpuStream::highestBankId);

		cpu_t localHighestCpuId = atomicAsm::read(&CpuStream::highestCpuId);

		localHighestCpuId = getHighestCpuIdFromNumaMap(
			numaMap, localHighestCpuId);

		// Update highestCpuId atomically
		atomicAsm::set(&CpuStream::highestCpuId, localHighestCpuId);
	};

	if (availableBanks.resizeTo(
		CpuStream::highestBankId + 1) != ERROR_SUCCESS)
	{
		panic(ERROR CPUTRIB"AvailableBanks BMP initialize() failed.\n");
	};
#endif
#endif

#if __SCALING__ == SCALING_SMP || defined(CHIPSET_NUMA_GENERATE_SHBANK)
	smpMap = zkcmCore.cpuDetection.getSmpMap();
	if (smpMap != NULL && smpMap->nEntries > 0)
	{
		if (CpuStream::highestCpuId == CPUID_INVALID)
			{ atomicAsm::set(&CpuStream::highestCpuId, 0); };

		// Update highestCpuId atomically
		cpu_t localHighestCpuId = atomicAsm::read(&CpuStream::highestCpuId);

		localHighestCpuId = getHighestCpuIdFromSmpMap(
			smpMap, localHighestCpuId);

		atomicAsm::set(&CpuStream::highestCpuId, localHighestCpuId);
	};

	// Final check to ensure highestCpuId doesn't exceed CONFIG_MAX_NCPUS
	cpu_t finalHighestCpuId = atomicAsm::read(&CpuStream::highestCpuId);
	if (finalHighestCpuId >= CONFIG_MAX_NCPUS)
	{
		printf(WARNING CPUTRIB"initializeAllCpus: Highest CPU ID %d "
			"exceeds CONFIG_MAX_NCPUS (%d). Capping to %d.\n",
			finalHighestCpuId, CONFIG_MAX_NCPUS, CONFIG_MAX_NCPUS - 1);

		atomicAsm::set(&CpuStream::highestCpuId, CONFIG_MAX_NCPUS - 1);
		finalHighestCpuId = CONFIG_MAX_NCPUS - 1;
	}

	if (availableCpus.resizeTo(
		finalHighestCpuId + 1) != ERROR_SUCCESS)
	{
		panic(ERROR CPUTRIB"Failed to initialize() availableCpus bmp.\n");
	};

	if (onlineCpus.resizeTo(
		finalHighestCpuId + 1) != ERROR_SUCCESS)
	{
		panic(ERROR CPUTRIB"Failed to initialize() onlineCpus bmp.\n");
	};
#endif

	zkcmCore.newCpuIdNotification(atomicAsm::read(&CpuStream::highestCpuId));

#if __SCALING__ >= SCALING_CC_NUMA
	return numaInit();
#endif
#if __SCALING__ == SCALING_SMP
	return smpInit();
#endif
#if __SCALING__ == SCALING_UNIPROCESSOR
	ret = uniProcessorInit();
#endif
	if (ret != ERROR_SUCCESS) { return ret; }

assert_fatal(0);
	zkcmCore.chipsetEventNotification(__KPOWER_EVENT_SMP_AVAIL, 0);
	return ERROR_SUCCESS;
}

#if __SCALING__ >= SCALING_CC_NUMA
/**
 * Notification of a CPU detected at boot time
 *
 * @param bid The NUMA bank ID for the CPU
 * @param cid The CPU ID
 * @param acpiId The ACPI ID for the CPU
 * @return ERROR_SUCCESS if successful, ERROR_LIMIT_OVERFLOWED if CPU ID exceeds CONFIG_MAX_NCPUS,
 *         or another error code if spawnStream fails
 *
 * @note The ERROR_LIMIT_OVERFLOWED return value has special significance in the kernel.
 *       It should ONLY be used to signal that a CPU ID exceeds CONFIG_MAX_NCPUS.
 *       The kernel relies on this specific error code to handle CPUs with IDs that
 *       exceed CONFIG_MAX_NCPUS differently from other error conditions.
 */
error_t CpuTrib::bootCpuNotification(numaBankId_t bid, cpu_t cid, ubit32 acpiId)
#elif __SCALING__ == SCALING_SMP
error_t CpuTrib::bootCpuNotification(cpu_t cid, ubit32 acpiId)
#endif
{
	error_t			ret;

	// Check that CPU ID doesn't exceed CONFIG_MAX_NCPUS
	if (cid >= CONFIG_MAX_NCPUS)
	{
#if __SCALING__ >= SCALING_CC_NUMA
		printf(WARNING CPUTRIB"bootCpuNotification(%d,%d,%d): CPU ID %d exceeds "
			"CONFIG_MAX_NCPUS (%d). Ignoring this CPU.\n",
			bid, cid, acpiId, cid, CONFIG_MAX_NCPUS);
#elif __SCALING__ == SCALING_SMP
		printf(WARNING CPUTRIB"bootCpuNotification(%d,%d): CPU ID %d exceeds "
			"CONFIG_MAX_NCPUS (%d). Ignoring this CPU.\n",
			cid, acpiId, cid, CONFIG_MAX_NCPUS);
#endif
		return ERROR_LIMIT_OVERFLOWED;
	}

#if __SCALING__ >= SCALING_CC_NUMA
	ret = spawnStream(bid, cid, acpiId);
#elif __SCALING__ == SCALING_SMP
	ret = spawnStream(cid, acpiId);
#endif

	if (ret != ERROR_SUCCESS)
	{
#if __SCALING__ >= SCALING_CC_NUMA
		printf(ERROR CPUTRIB"bootCpuNotification(%d,%d,%d): Failed "
			"to spawn stream for CPU.\n",
			bid, cid, acpiId);
#elif __SCALING__ == SCALING_SMP
		printf(ERROR CPUTRIB"bootCpuNotification(%d,%d): Failed to "
			"spawn stream for CPU.\n",
			cid, acpiId);
#endif

		return ret;
	};

	return ERROR_SUCCESS;
}

#if __SCALING__ >= SCALING_SMP
class BootParseCpuMapCb;
typedef void (BootParseCpuMapCbFn)(
	MessageStream::sHeader *msg, BootParseCpuMapCb *cb);

static BootParseCpuMapCbFn	bootParseCpuMap_contd1;

class BootParseCpuMapCb
: public MessageStreamCallback<BootParseCpuMapCbFn *>
{
public:
	BootParseCpuMapCb(
		BootParseCpuMapCbFn *_function, ubit16 _callbackFunctionId,
		volatile uarch_t *_nTotalIsFinalCount,
		volatile uarch_t *_nSucceeded, volatile uarch_t *_nFailed,
		volatile uarch_t *_nTotal,
		volatile uarch_t *_nInExcessOfConfigMaxNcpus)
	: MessageStreamCallback<BootParseCpuMapCbFn *>(_function),
	nTotalIsFinalCount(_nTotalIsFinalCount), nSucceeded(_nSucceeded),
	nFailed(_nFailed), nTotal(_nTotal),
	nInExcessOfConfigMaxNcpus(_nInExcessOfConfigMaxNcpus),
	callbackFunctionId(_callbackFunctionId)
	{}

	virtual void operator()(MessageStream::sHeader *msg)
		{ function(msg, this); }

public:
	volatile uarch_t	*nTotalIsFinalCount,
				*nSucceeded, *nFailed, *nTotal,
				*nInExcessOfConfigMaxNcpus;
	ubit16			callbackFunctionId;
};
#endif

#if __SCALING__ >= SCALING_CC_NUMA
static MessageStream::DispatchFn	bootParseNumaMap_syncDispatcher;

static sbit8 bootParseNumaMap_syncDispatcher(MessageStream::sHeader *msg)
{
	if (msg->subsystem == MSGSTREAM_SUBSYSTEM_USER0
		&& msg->function
			== CPUTRIB_BOOT_PARSE_NUMA_MAP_ACK)
	{
		return 1;
	}

	__korientationMainDispatchOne(msg);
	return 0;
}

error_t CpuTrib::bootParseNumaMap(sZkcmNumaMap *numaMap)
{
	error_t			ret;
	volatile uarch_t	nTotalIsFinalCount=0,
				nSucceeded=0, nFailed=0, nTotal=0,
				nInExcessOfConfigMaxNcpus=0;

printf(FATAL CPUTRIB"bootParseNumaMap: about to parse raw numaMap without SMP map comparison.\n");
	for (uarch_t i=0; i<numaMap->nCpuEntries; i++)
	{
		if (numaMap->cpuEntries[i].cpuId == CpuStream::bspCpuId)
			{ continue; };

		atomicAsm::increment(&nTotal);

		ret = bootCpuNotification(
			numaMap->cpuEntries[i].bankId,
			numaMap->cpuEntries[i].cpuId,
			numaMap->cpuEntries[i].cpuAcpiId);

printf(FATAL CPUTRIB"Just called bootCpuNotification for CPU %d.\n", numaMap->cpuEntries[i].cpuId);
		if (ret != ERROR_SUCCESS)
		{
			printf(ERROR CPUTRIB"bootParseNumaMap: bootCpuNotification() "
				"failed for CPU %d with error %s.\n",
				numaMap->cpuEntries[i].cpuId,
				strerror(ret));

			/* If the error is due to CPU ID exceeding
			 * CONFIG_MAX_NCPUS, we've already logged a warning
			 * and we can continue with other CPUs.
			 **/
			if (ret == ERROR_LIMIT_OVERFLOWED)
			{
				atomicAsm::increment(
					&nInExcessOfConfigMaxNcpus);
			};

			atomicAsm::increment(&nFailed);
			continue;
		}

printf(FATAL CPUTRIB"About to call bootPowerOn() for CPU %d.\n", numaMap->cpuEntries[i].cpuId);
		ret = getStream(numaMap->cpuEntries[i].cpuId)
			->powerManager.bootPowerOnReq(
				0,
				new BootParseCpuMapCb(
					bootParseCpuMap_contd1,
					CPUTRIB_BOOT_PARSE_NUMA_MAP_ACK,
					&nTotalIsFinalCount,
					&nSucceeded, &nFailed, &nTotal,
					&nInExcessOfConfigMaxNcpus));

		if (ret != ERROR_SUCCESS)
		{
			printf(ERROR CPUTRIB"bootParseNumaMap: "
				"Failed to power on CPU Stream "
				"for CPU %d with error %s.",
				numaMap->cpuEntries[i].cpuId,
				strerror(ret));

			atomicAsm::increment(&nFailed);
			continue;
		}
	}

	atomicAsm::set(&nTotalIsFinalCount, 1);

	MessageStream::sHeader *msg;

printf(FATAL CPUTRIB"About to pullAndDispatchUntil() for %d CPUs.\n",
	nTotal);

	ret = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
		->messageStream.pullAndDispatchUntil(
			&msg, 0, NULL,
			&bootParseNumaMap_syncDispatcher);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR CPUTRIB"bootParseNumaMap: "
			"Failed to pullAndDispatchUntil (err=%d).\n",
			ret);

		return ret;
	}

	printf(NOTICE CPUTRIB"bootParseNumaMap: Processed %d CPUs. "
		"Succeeded: %d, Failed: %d; %d CPUs with ID exceeding "
		"CONFIG_MAX_NCPUS.\n",
		nTotal, nSucceeded, nFailed,
		nInExcessOfConfigMaxNcpus);

	CpuStream::nCpusInExcessOfConfigMaxNcpus
		+= nInExcessOfConfigMaxNcpus;

	ret = (nFailed > nInExcessOfConfigMaxNcpus)
		? ERROR_INITIALIZATION_FAILURE
		: ERROR_SUCCESS;

	delete msg;
	return ret;
}

static MessageStream::DispatchFn bootParseNumaMapAgainstSmpMap_syncDispatcher;

static sbit8 bootParseNumaMapAgainstSmpMap_syncDispatcher(
	MessageStream::sHeader *msg
	)
{
	if (msg->subsystem == MSGSTREAM_SUBSYSTEM_USER0
		&& msg->function
			== CPUTRIB_BOOT_PARSE_NUMA_MAP_AGAINST_SMP_MAP_ACK)
	{
		return 1;
	}

	__korientationMainDispatchOne(msg);
	return 0;
}

error_t CpuTrib::bootParseNumaMapAgainstSmpMap(
	sZkcmNumaMap *numaMap, sZkcmSmpMap *smpMap
	)
{
	error_t			ret;
	volatile uarch_t	nTotalIsFinalCount=0,
				nSucceeded=0, nFailed=0, nTotal=0,
				nInExcessOfConfigMaxNcpus=0;

	for (uarch_t i=0; i<smpMap->nEntries; i++)
	{
		sarch_t		found=0;

		if (smpMap->entries[i].cpuId == CpuStream::bspCpuId)
			{ continue; };

		for (uarch_t j=0; j<numaMap->nCpuEntries; j++)
		{
			// If the cpu is also in the NUMA map:
			if (numaMap->cpuEntries[j].cpuId
				== smpMap->entries[i].cpuId)
			{
				found = 1;
				break;
			};
		};

		// If the CPU is not an extra, don't process it.
		if (found) { continue; };

		atomicAsm::increment(&nTotal);

		ret = bootCpuNotification(
			CHIPSET_NUMA_SHBANKID,
			smpMap->entries[i].cpuId,
			smpMap->entries[i].cpuId);

		if (ret != ERROR_SUCCESS)
		{
			printf(ERROR CPUTRIB"bootParseNumaMapAgainstSmpMap: "
				"Failed to notify chipset before attempting to "
				"boot CPU Stream %d.\n",
				smpMap->entries[i].cpuId);

			if (ret == ERROR_LIMIT_OVERFLOWED)
			{
				atomicAsm::increment(
					&nInExcessOfConfigMaxNcpus);
			};

			atomicAsm::increment(&nFailed);
			continue;
		}

		ret = getStream(smpMap->entries[i].cpuId)
			->powerManager.bootPowerOnReq(
				0,
				new BootParseCpuMapCb(
					bootParseCpuMap_contd1,
					CPUTRIB_BOOT_PARSE_NUMA_MAP_AGAINST_SMP_MAP_ACK,
					&nTotalIsFinalCount, &nSucceeded, &nFailed,
					&nTotal, &nInExcessOfConfigMaxNcpus));

		if (ret != ERROR_SUCCESS)
		{
			printf(ERROR CPUTRIB"bootParseNumaMapAgainstSmpMap: "
				"Failed to power on CPU Stream "
				"for CPU %d with error %s.",
				smpMap->entries[i].cpuId,
				strerror(ret));

			atomicAsm::increment(&nFailed);
			continue;
		}
	}

	atomicAsm::set(&nTotalIsFinalCount, 1);

	MessageStream::sHeader *msg;

	ret = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
		->messageStream.pullAndDispatchUntil(
			&msg, 0, NULL,
			&bootParseNumaMapAgainstSmpMap_syncDispatcher);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR CPUTRIB"bootParseNumaMapAgainstSmpMap: "
			"Failed to pullAndDispatchUntil (err=%d).\n",
			ret);
		
		return ret;
	}

	printf(NOTICE CPUTRIB"bootParseNumaMapAgainstSmpMap: Processed %d CPUs. "
		"Succeeded: %d, Failed: %d; %d CPUs with ID exceeding "
		"CONFIG_MAX_NCPUS.\n",
		nTotal, nSucceeded, nFailed,
		nInExcessOfConfigMaxNcpus);

	CpuStream::nCpusInExcessOfConfigMaxNcpus
		+= nInExcessOfConfigMaxNcpus;

	ret = (nFailed > nInExcessOfConfigMaxNcpus)
		? ERROR_INITIALIZATION_FAILURE
		: ERROR_SUCCESS;

	delete msg;
	return ret;
}
#endif

#if __SCALING__ >= SCALING_CC_NUMA
error_t CpuTrib::numaInit(void)
{
	sZkcmNumaMap		*numaMap;
#ifdef CHIPSET_NUMA_GENERATE_SHBANK
	sZkcmSmpMap		*smpMap;
#endif

	/**	EXPLANATION:
	 * Use the chipset's NUMA CPU map and spawn streams for each CPU. Each
	 * CPU is also powered on as soon as it is given a stream.
	 *
	 * The chipset CPU info module should have been initialized outside
	 * by the calling code. Should be able to just use the module from here
	 * on.
	 **/

	// Return error, because this shouldn't be called in this case.
	if (!usingChipsetSmpMode()) { return ERROR_UNSUPPORTED; };

	numaMap = zkcmCore.cpuDetection.getNumaMap();
	if (numaMap != NULL && numaMap->nCpuEntries > 0) {
		bootParseNumaMap(numaMap);
	}
	else
	{
		printf(WARNING CPUTRIB"numaInit: NUMA build, but CPU mod "
			"reports no NUMA CPUs"
#ifndef CHIPSET_NUMA_GENERATE_SHBANK
			", and no shbank configured"
#endif
			".\n");
	};

#if defined(CHIPSET_NUMA_GENERATE_SHBANK)

#ifndef CHIPSET_NUMA_SHBANKID
	panic(FATAL CPUTRIB"numaInit(): NUMA_SHBANKID not defined!\n");
#endif

	smpMap = zkcmCore.cpuDetection.getSmpMap();
	if (smpMap != NULL && smpMap->nEntries > 0)
	{
		/**	EXPLANATION:
		 * On a NUMA build, there are two cases in which this code will
		 * have to be executed.
		 *	1. There was a NUMA map, but the kernel was also asked
		 *	   to generate an extra bank for CPUs which were not
		 *	   listed in the NUMA map, and treat these CPUs as if
		 *	   they were all equally distanced from each other
		 *	   NUMA-wise (place them into the shared bank).
		 *	2. There was no NUMA map, so the kernel has fallen back
		 *	   to SMP operation. CPUs will be placed on a shared
		 *	   bank in this case also.
		 **/
		printf(NOTICE CPUTRIB"numaInit: Using shared CPU bank; ");

		// Case 1 from above.
		if (numaMap != NULL && numaMap->nCpuEntries > 0)
		{
			// Filter out the CPUs which need to be in shared bank.
			printf(NOTICE CPUTRIB"Filtering out NUMA CPUs.\n");
			bootParseNumaMapAgainstSmpMap(numaMap, smpMap);
		}
		else
		{
			/**	NOTE:
			 * Case 2 from above. We simply spawn all CPUs in the
			 * SMP map on shared bank.
			 *
			 * I may later choose to collapse this into a pure
			 * SMP method of handling, but for now, we use the
			 * shared bank method.
			 *
			 * This is imho best since on a hotplug chipset, the
			 * admin may insert new CPUs later on which have NUMA
			 * affinity, even if there were none at boot.
			 **/
			printf(CC"No NUMA map, all CPUs on shared bank (SMP "
				"operation).\n");
			bootParseSmpMap(smpMap);
		};
	}
	else
	{
		// No SMP map. If also no NUMA map, then assume single CPU.
		if (numaMap == NULL || numaMap->nCpuEntries == 0)
		{
			printf(WARNING CPUTRIB"numaInit: Falling back to "
				"uniprocessor mode.\n");

			return displayUpOperationOnMpBuildMessage();
		};
	};
#endif

	printf(NOTICE CPUTRIB"numaInit: Success.\n");
	return ERROR_SUCCESS;
}
#endif

#if __SCALING__ >= SCALING_SMP
static MessageStream::DispatchFn	bootParseSmpMap_syncDispatcher;

static sbit8 bootParseSmpMap_syncDispatcher(
	MessageStream::sHeader *msg
	)
{
	if (msg->subsystem == MSGSTREAM_SUBSYSTEM_USER0
		&& msg->function == CPUTRIB_BOOT_PARSE_SMP_MAP_ACK)
	{
		return 1;
	}

	__korientationMainDispatchOne(msg);
	return 0;
}

error_t CpuTrib::bootParseSmpMap(sZkcmSmpMap *smpMap)
{
	error_t			ret;
	volatile uarch_t	nTotalIsFinalCount=0,
				nSucceeded=0, nFailed=0, nTotal=0,
				nInExcessOfConfigMaxNcpus=0;

	for (uarch_t i=0; i<smpMap->nEntries; i++)
	{
		if (smpMap->entries[i].cpuId == CpuStream::bspCpuId)
			{ continue; };

		atomicAsm::increment(&nTotal);

		ret = bootCpuNotification(
#if __SCALING__ >= SCALING_CC_NUMA
			CHIPSET_NUMA_SHBANKID,
#endif
			smpMap->entries[i].cpuId,
			smpMap->entries[i].cpuAcpiId);

		if (ret != ERROR_SUCCESS)
		{
			printf(ERROR CPUTRIB"bootParseSmpMap: "
				"Failed to power on CPU Stream "
				"for CPU %d with error %s.",
				smpMap->entries[i].cpuId,
				strerror(ret));

			/* If the error is due to CPU ID exceeding
			 * CONFIG_MAX_NCPUS, we've already logged a warning
			 * and we can continue with other CPUs.
			 **/
			if (ret == ERROR_LIMIT_OVERFLOWED)
			{
				atomicAsm::increment(
					&nInExcessOfConfigMaxNcpus);
			};

			atomicAsm::increment(&nFailed);
			continue;
		}

		ret = getStream(smpMap->entries[i].cpuId)
			->powerManager.bootPowerOnReq(
				0,
				new BootParseCpuMapCb(
					bootParseCpuMap_contd1,
					CPUTRIB_BOOT_PARSE_SMP_MAP_ACK,
					&nTotalIsFinalCount,
					&nSucceeded, &nFailed, &nTotal,
					&nInExcessOfConfigMaxNcpus));

		if (ret != ERROR_SUCCESS)
		{
			printf(ERROR CPUTRIB"bootParseSmpMap: "
				"Failed to power on CPU Stream "
				"for CPU %d with error %s.",
				smpMap->entries[i].cpuId,
				strerror(ret));

			atomicAsm::increment(&nFailed);
			continue;
		}
	}

	atomicAsm::set(&nTotalIsFinalCount, 1);

	MessageStream::sHeader *msg;

	ret = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
		->messageStream.pullAndDispatchUntil(
			&msg, 0, NULL,
			&bootParseSmpMap_syncDispatcher);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR CPUTRIB"bootParseSmpMap: "
			"Failed to pullAndDispatchUntil (err=%d).\n",
			ret);

		return ret;
	}

	printf(NOTICE CPUTRIB"bootParseSmpMap: Processed %d CPUs. "
		"Succeeded: %d, Failed: %d; %d CPUs with ID exceeding "
		"CONFIG_MAX_NCPUS.\n",
		nTotal, nSucceeded, nFailed,
		nInExcessOfConfigMaxNcpus);

	CpuStream::nCpusInExcessOfConfigMaxNcpus
		+= nInExcessOfConfigMaxNcpus;

	ret = (nFailed > nInExcessOfConfigMaxNcpus)
		? ERROR_INITIALIZATION_FAILURE
		: ERROR_SUCCESS;

	delete msg;
	return ret;
}

static void bootParseCpuMap_contd1(
	MessageStream::sHeader *msg, BootParseCpuMapCb *cb
	)
{
	if (msg->error == ERROR_SUCCESS)
		{ atomicAsm::increment(cb->nSucceeded); }
	else
		{ atomicAsm::increment(cb->nFailed); }

	if (!atomicAsm::read(cb->nTotalIsFinalCount)) { return; };

	if (atomicAsm::read(cb->nSucceeded) + atomicAsm::read(cb->nFailed)
		!= atomicAsm::read(cb->nTotal))
		{ return; }

	error_t		err;

	err = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
		->messageStream.postUserQMessage(
			msg->sourceId,
			MSGSTREAM_USERQ(MSGSTREAM_SUBSYSTEM_USER0),
			cb->callbackFunctionId,
			NULL, NULL);

	if (err != ERROR_SUCCESS)
	{
		printf(ERROR CPUTRIB"bootParseNumaMap_contd2: "
			"Failed to post completion message (err=%d).\n",
			err);
	}
}
#endif

#if __SCALING__ == SCALING_SMP
error_t CpuTrib::smpInit(void)
{
	error_t			ret;
	sZkcmSmpMap		*smpMap;

	/**	EXPLANATION:
	 * Quite simple: look for an SMP map, and if none exists, return success
	 * since CPUs may still be hotplugged.
	 **/

	// Return error, because this shouldn't be called in this case.
	if (!usingChipsetSmpMode()) { return ERROR_UNSUPPORTED; };

	smpMap = zkcmCore.cpuDetection.getSmpMap();
	if (smpMap != NULL && smpMap->nEntries > 0)
	{
		bootParseSmpMap(smpMap);
		bootConfirmSmpCpusBooted(smpMap);
	}
	else {
		displayUpOperationOnMpBuildMessage();
	};

	return ERROR_SUCCESS;
}
#endif

#if __SCALING__ == SCALING_UNIPROCESSOR
error_t CpuTrib::uniProcessorInit(void)
{
	/**	EXPLANATION:
	 * Base setup for a UP build. Nothing very interesting. It just makes
	 * sure that if getStream(foo) is called, the BSP will always be
	 * returned, no matter what argument is passed.
	 **/
	/* TODO: Check if you should call spawnStream() and bind() on the
	 * BSP on a non-MP build. Most likely not, because this is dealt with in
	 * initializeBspCpuStream() anyway.
	 **/
	cpu = getCurrentCpuStream();
	return ERROR_SUCCESS;
}
#endif


error_t CpuTrib::spawnStream(
#if __SCALING__ >= SCALING_CC_NUMA
    numaBankId_t bid,
#endif
    cpu_t cid, ubit32 cpuAcpiId)
{
	error_t		ret;
#if __SCALING__ >= SCALING_CC_NUMA
	NumaCpuBank	*ncb;
#endif
	CpuStream	*cs;

	if (
#if __SCALING__ >= SCALING_CC_NUMA
		bid == NUMABANKID_INVALID ||
#endif
		cid == CPUID_INVALID) {
		return ERROR_INVALID_ARG_VAL;
	};

	// Check that CPU ID doesn't exceed CONFIG_MAX_NCPUS
	if (cid >= CONFIG_MAX_NCPUS)
	{
		printf(ERROR CPUTRIB"spawnStream(%d, %d, %d): CPU ID %d exceeds CONFIG_MAX_NCPUS (%d).\n",
			bid, cid, cpuAcpiId, cid, CONFIG_MAX_NCPUS);

		return ERROR_LIMIT_OVERFLOWED;
	}

#if __SCALING__ >= SCALING_CC_NUMA
	/**	EXPLANATION:
	 * The NUMA case requires that the CPU be assigned to its holding bank.
	 * The extra NUMA code is really just dealing with that.
	 **/
	if ((ret = createBank(bid)) != ERROR_SUCCESS)
	{
		printf(ERROR CPUTRIB"spawnStream(%d, %d, %d): "
			"Failed to create bank.\n",
			bid, cid, cpuAcpiId);

		return ret;
	};

	ncb = getBank(bid);
	CHECK_AND_RESIZE_BMP(
		&ncb->cpus, cid, &ret, "spawnStream", "resident bank");

	if (ret != ERROR_SUCCESS) { return ret; };
	ncb->cpus.setSingle(cid);
#endif

	/**	EXPLANATION:
	 * The SMP case requires that the onlineCpus and availableCpus bmps be
	 * resized to hold the new CPU's bit.
	 **/
	CHECK_AND_RESIZE_BMP(
		&availableCpus, cid, &ret, "spawnStream", "availableCpus");

	if (ret != ERROR_SUCCESS) { return ret; };

	CHECK_AND_RESIZE_BMP(
		&onlineCpus, cid, &ret, "spawnStream", "onlineCpus");

	if (ret != ERROR_SUCCESS) { return ret; };

	/* Now we simply allocate the CPU stream and add it to the global CPU
	 * array. Also, do not re-allocate the BSP CPU stream.
	 **/
	if (cid != CpuStream::bspCpuId)
	{
		cs = new (processTrib.__kgetStream()->memoryStream.memAlloc(
			PAGING_BYTES_TO_PAGES(sizeof(CpuStream)),
			MEMALLOC_NO_FAKEMAP))
				CpuStream(
#if __SCALING__ >= SCALING_CC_NUMA
					bid,
#endif
					cid, cpuAcpiId);
	}
	else
	{
		cs = new (&bspCpu) CpuStream(
#if __SCALING__ >= SCALING_CC_NUMA
			bid,
#endif
			cid, cpuAcpiId);
	}

	if (cs == NULL) { return ERROR_MEMORY_NOMEM; };

	ret = cs->initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = cpuStreams.addItem(cid, cs);
	if (ret != ERROR_SUCCESS)
	{
#if __SCALING__ >= SCALING_CC_NUMA
		printf(ERROR CPUTRIB"spawnStream(%d, %d, %d): "
			"Failed to add new CPU stream to list.\n",
			bid, cid, cpuAcpiId);
#else
		printf(ERROR CPUTRIB"spawnStream(%d, %d): Failed "
			"to add new CPU stream to list.\n",
			cid, cpuAcpiId);
#endif
		return ret;
	};

#if __SCALING__ >= SCALING_CC_NUMA
	printf(NOTICE CPUTRIB"spawnStream(%d,%d,%d): Success.\n",
		bid, cid, cpuAcpiId);
#else
	printf(NOTICE CPUTRIB"spawnStream(%d,%d): Success.\n",
		cid, cpuAcpiId);
#endif
	__kupdateAffinity(cid, CPUTRIB___KUPDATEAFFINITY_ADD);

	return ERROR_SUCCESS;
}

void CpuTrib::destroyStream(cpu_t cid)
{
#if __SCALING__ >= SCALING_CC_NUMA
	NumaCpuBank		*ncb;
#endif
	CpuStream		*cs;

	/**	NOTE:
	 * This will probably never be called on most machines. Called on CPU
	 * unplug/physical removal.
	 **/
	cs = getStream(cid);
	if (cs == NULL) { return; };

	// Now remove it from the list of CPUs and free the mem.
	cpuStreams.removeItem(cid);
	__kupdateAffinity(cid, CPUTRIB___KUPDATEAFFINITY_REMOVE);
	cs->~CpuStream();
	processTrib.__kgetStream()->memoryStream.memFree(cs);

#if __SCALING__ >= SCALING_CC_NUMA
	ncb = getBank(cs->bankId);
	if (ncb != NULL) {
		ncb->cpus.unsetSingle(cid);
	};
#endif
}

#if __SCALING__ >= SCALING_CC_NUMA
error_t CpuTrib::createBank(numaBankId_t bankId)
{
	error_t			err;
	NumaCpuBank		*ncb;

	if ((ncb = getBank(bankId)) != NULL) {
		return ERROR_SUCCESS;
	};

	CHECK_AND_RESIZE_BMP(
		&availableBanks, bankId, &err,
		"createBank", "available banks");

	if (err != ERROR_SUCCESS) { return err; };

	ncb = new (processTrib.__kgetStream()->memoryStream.memAlloc(
		PAGING_BYTES_TO_PAGES(sizeof(NumaCpuBank)),
		MEMALLOC_NO_FAKEMAP))
			NumaCpuBank;

	if (ncb == NULL) { return ERROR_MEMORY_NOMEM; };

	err = cpuBanks.addItem(bankId, ncb);
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR CPUTRIB"createBank(%d): Failed to add to list."
			"\n", bankId);

		ncb->~NumaCpuBank();
		processTrib.__kgetStream()->memoryStream.memFree(ncb);
		return err;
	}

	availableBanks.setSingle(bankId);
	printf(NOTICE CPUTRIB"createBank(%d): Successful.\n", bankId);
	return ERROR_SUCCESS;
}
#endif

#if __SCALING__ >= SCALING_CC_NUMA
void CpuTrib::destroyBank(numaBankId_t bid)
{
	NumaCpuBank		*ncb;

	ncb = getBank(bid);
	if (ncb == NULL) { return; };

	availableBanks.unsetSingle(bid);
	cpuBanks.removeItem(bid);
	ncb->~NumaCpuBank();
	processTrib.__kgetStream()->memoryStream.memFree(ncb);
}
#endif

error_t CpuTrib::__kupdateAffinity(cpu_t cid, ubit8 action)
{
	error_t		ret;

	/**	EXPLANATION:
	 * When a new CPU is detected (either at boot, or as hot-plug) the
	 * kernel automatically updates the affinity of all of its threads to
	 * include that new CPU.
	 **/
	switch (action)
	{
	case CPUTRIB___KUPDATEAFFINITY_ADD:
		CHECK_AND_RESIZE_BMP(
			&processTrib.__kgetStream()->cpuAffinity, cid, &ret,
			"__kupdateAffinity", "__kprocess CPU affinity");

		if (ret != ERROR_SUCCESS)
		{
			printf(ERROR CPUTRIB"__kprocess unable to use "
				"CPU %d.\n",
				cid);
		}
		else {
			processTrib.__kgetStream()->cpuAffinity.setSingle(cid);
		};

		return ERROR_SUCCESS;

	case CPUTRIB___KUPDATEAFFINITY_REMOVE:
		processTrib.__kgetStream()->cpuAffinity.unsetSingle(cid);
		return ERROR_SUCCESS;

	default: return ERROR_INVALID_ARG_VAL;
	};

	printf(ERROR CPUTRIB"__kupdateAffinity: Reached unreachable "
		"point.\n");

	return ERROR_UNKNOWN;
}

