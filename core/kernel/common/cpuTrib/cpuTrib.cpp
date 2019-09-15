
#include <debug.h>
#include <scaling.h>
#include <chipset/cpus.h>
#include <chipset/zkcm/zkcmCore.h>
#include <arch/cpuControl.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <__kthreads/main.h>
#include <__kthreads/__kcpuPowerOn.h>


#define getHighestId(currHighest,map,member,item,hibound)	\
	do \
	{ \
		for (ubit32 tmpidx=0; tmpidx<hibound; tmpidx++) \
		{ \
			if (map->member[tmpidx].item > currHighest) { \
				currHighest = map->member[tmpidx].item; \
			}; \
		}; \
	} while (0)

cpu_t		CpuStream::highestCpuId=CPUID_INVALID;
numaBankId_t	CpuStream::highestBankId=NUMABANKID_INVALID;

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
			"returned INVALID as ID for BSP.\n");

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
		CpuStream::bspBankId = CHIPSET_NUMA_SHBANKID;
		printf(WARNING CPUTRIB"loadBspInformation: Unable to "
			"determine BSP bank ID.\n\tUsing SHBANKID (%d).\n",
			CHIPSET_NUMA_SHBANKID);
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
		if (CpuStream::highestCpuId == CPUID_INVALID)
			{ CpuStream::highestCpuId = 0; };
		if (CpuStream::highestBankId == NUMABANKID_INVALID)
			{ CpuStream::highestBankId = 0; };

		getHighestId(
			CpuStream::highestBankId, numaMap, cpuEntries,
			bankId, numaMap->nCpuEntries);

		getHighestId(
			CpuStream::highestCpuId, numaMap, cpuEntries,
			cpuId, numaMap->nCpuEntries);
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
			{ CpuStream::highestCpuId = 0; };

		getHighestId(
			CpuStream::highestCpuId, smpMap,
			entries, cpuId, smpMap->nEntries);
	};

	if (availableCpus.resizeTo(
		CpuStream::highestCpuId + 1) != ERROR_SUCCESS)
	{
		panic(ERROR CPUTRIB"Failed to initialize() availableCpus bmp.\n");
	};

	if (onlineCpus.resizeTo(
		CpuStream::highestCpuId + 1) != ERROR_SUCCESS)
	{
		panic(ERROR CPUTRIB"Failed to initialize() onlineCpus bmp.\n");
	};
#endif

	zkcmCore.newCpuIdNotification(CpuStream::highestCpuId);

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

	zkcmCore.chipsetEventNotification(__KPOWER_EVENT_SMP_AVAIL, 0);
	return ERROR_SUCCESS;
}

#if __SCALING__ >= SCALING_CC_NUMA
error_t CpuTrib::bootCpuNotification(numaBankId_t bid, cpu_t cid, ubit32 acpiId)
#elif __SCALING__ == SCALING_SMP
error_t CpuTrib::bootCpuNotification(cpu_t cid, ubit32 acpiId)
#endif
{
	error_t			ret;

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

#if __SCALING__ >= SCALING_CC_NUMA
void CpuTrib::bootParseNumaMap(sZkcmNumaMap *numaMap)
{
	error_t		err;

	for (uarch_t i=0; i<numaMap->nCpuEntries; i++)
	{
		if (numaMap->cpuEntries[i].cpuId == CpuStream::bspCpuId)
			{ continue; };

		err = bootCpuNotification(
			numaMap->cpuEntries[i].bankId,
			numaMap->cpuEntries[i].cpuId,
			numaMap->cpuEntries[i].cpuAcpiId);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR CPUTRIB"bootParseNumaMap: Failed to "
				"power on CPU %d.\n",
				numaMap->cpuEntries[i].cpuId);
		};

		getStream(numaMap->cpuEntries[i].cpuId)
			->powerManager.bootPowerOn(0);
	};
}

void CpuTrib::bootConfirmNumaCpusBooted(sZkcmNumaMap *numaMap)
{
	for (uarch_t i=0; i<numaMap->nCpuEntries; i++)
	{
		if (numaMap->cpuEntries[i].cpuId == CpuStream::bspCpuId)
			{ continue; };

		getStream(numaMap->cpuEntries[i].cpuId)->powerManager
			.bootWaitForCpuToPowerOn();
	};
}

void CpuTrib::bootParseNumaMap(
	sZkcmNumaMap *numaMap, sZkcmSmpMap *smpMap
	)
{
	error_t		err;

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
		err = bootCpuNotification(
			CHIPSET_NUMA_SHBANKID,
			smpMap->entries[i].cpuId,
			smpMap->entries[i].cpuId);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR CPUTRIB"bootParseNumaMapHoles: "
				"Failed to power on CPU Stream "
				"%d.\n",
				smpMap->entries[i].cpuId);
		};

		getStream(smpMap->entries[i].cpuId)
			->powerManager.bootPowerOn(0);
	};
}

void CpuTrib::bootConfirmNumaCpusBooted(
	sZkcmNumaMap *numaMap, sZkcmSmpMap *smpMap
	)
{
	for (uarch_t i=0; i<smpMap->nEntries; i++)
	{
		sarch_t		found=0;

		if (smpMap->entries[i].cpuId == CpuStream::bspCpuId)
			{ continue; };

		for (uarch_t j=0; j<numaMap->nCpuEntries; j++)
		{
			if (smpMap->entries[i].cpuId
				== numaMap->cpuEntries[j].cpuId)
			{
				found = 1;
				break;
			};
		};

		// Skip those CPUs which appear in both maps.
		if (!found)
		{
			getStream(smpMap->entries[i].cpuId)->powerManager
				.bootWaitForCpuToPowerOn();
		};
	};
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
	if (numaMap != NULL && numaMap->nCpuEntries > 0)
	{
		bootParseNumaMap(numaMap);
		bootConfirmNumaCpusBooted(numaMap);
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
			printf(CC"Filtering out NUMA CPUs.\n");
			bootParseNumaMap(numaMap, smpMap);
			bootConfirmNumaCpusBooted(numaMap, smpMap);
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
			bootConfirmSmpCpusBooted(smpMap);
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

	return ERROR_SUCCESS;
}
#endif

#if __SCALING__ >= SCALING_SMP
void CpuTrib::bootParseSmpMap(sZkcmSmpMap *smpMap)
{
	error_t		err;

	for (uarch_t i=0; i<smpMap->nEntries; i++)
	{
		if (smpMap->entries[i].cpuId == CpuStream::bspCpuId)
			{ continue; };

		err = bootCpuNotification(
#if __SCALING__ >= SCALING_CC_NUMA
			CHIPSET_NUMA_SHBANKID,
#endif
			smpMap->entries[i].cpuId,
			smpMap->entries[i].cpuAcpiId);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR CPUTRIB"bootParseSmpMap: "
				"Failed to power on CPU Stream "
				"for CPU %d.",
				smpMap->entries[i].cpuId);
		};

		getStream(smpMap->entries[i].cpuId)
			->powerManager.bootPowerOn(0);
	};
}

void CpuTrib::bootConfirmSmpCpusBooted(sZkcmSmpMap *smpMap)
{
	for (uarch_t i=0; i<smpMap->nEntries; i++)
	{
		if (smpMap->entries[i].cpuId != CpuStream::bspCpuId)
		{
			getStream(smpMap->entries[i].cpuId)->powerManager
				.bootWaitForCpuToPowerOn();
		};
	};
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


#if __SCALING__ >= SCALING_CC_NUMA
error_t CpuTrib::spawnStream(numaBankId_t bid, cpu_t cid, ubit32 cpuAcpiId)
#else
error_t CpuTrib::spawnStream(cpu_t cid, ubit32 cpuAcpiId)
#endif
{
	error_t		ret;
#if __SCALING__ >= SCALING_CC_NUMA
	NumaCpuBank	*ncb;
#endif
	CpuStream	*cs;

#if __SCALING__ >= SCALING_CC_NUMA
	if (bid == NUMABANKID_INVALID || cid == CPUID_INVALID) {
#else
	if (cid == CPUID_INVALID) {
#endif
		return ERROR_INVALID_ARG_VAL;
	};

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
#if __SCALING__ >= SCALING_CC_NUMA
		cs = new (processTrib.__kgetStream()->memoryStream.memAlloc(
			PAGING_BYTES_TO_PAGES(sizeof(CpuStream)),
			MEMALLOC_NO_FAKEMAP))
				CpuStream(bid, cid, cpuAcpiId);
#else
		cs = new (processTrib.__kgetStream()->memoryStream.*memAlloc(
			PAGING_BYTES_TO_PAGES(sizeof(CpuStream)),
			MEMALLOC_NO_FAKEMAP))
				CpuStream(cid, cpuAcpiId);
#endif
	}
	else
	{
#if __SCALING__ >= SCALING_CC_NUMA
		cs = new (&bspCpu) CpuStream(bid, cid, cpuAcpiId);
#else
		cs = new (&bspCpu) CpuStream(cid, cpuAcpiId);
#endif
	};

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

