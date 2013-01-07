#include <scaling.h>
#include <chipset/cpus.h>
#include <chipset/zkcm/zkcmCore.h>
#include <arch/cpuControl.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <__kthreads/__korientation.h>
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

#if __SCALING__ >= SMP
static cpu_t		highestCpuId=0;
#endif
#if __SCALING__ >= SCALING_CC_NUMA
	#ifdef CHIPSET_CPU_NUMA_GENERATE_SHBANK
static numaBankId_t	highestBankId=CHIPSET_CPU_NUMA_SHBANKID;
	#else
static numaBankId_t	highestBankId=0;
	#endif
#endif

cpuTribC::cpuTribC(void)
{
	bspId = 0;
	_usingChipsetSmpMode = 0;
}

cpuTribC::~cpuTribC(void)
{
}

error_t cpuTribC::fallbackToUpMode(cpu_t bspId, ubit32 bspAcpiId)
{
	error_t		ret;

	__kprintf(WARNING"The kernel is falling back to Uniprocessor mode; %s."
		"\n",
		((_usingChipsetSmpMode == 1) ?
			"\n\thowever, hotplug of new CPUs is allowed"
			:"\n\tthis chipset is not safe for multiprocessing"));

#if __SCALING__ >= SCALING_CC_NUMA
	ret = cpuTrib.spawnStream(CHIPSET_CPU_NUMA_SHBANKID, bspId, bspAcpiId);
#else
	ret = cpuTrib.spawnStream(bspId, bspAcpiId);
#endif

	if (ret != ERROR_SUCCESS)
	{
		__kprintf(FATAL CPUTRIB"Failed to spawn BSP stream when "
			"falling back to single-cpu mode.\n");

		return ERROR_FATAL;
	};

	return ERROR_SUCCESS;
}

error_t cpuTribC::initialize2(void)
{
	error_t			ret;
#if __SCALING__ >= SCALING_CC_NUMA
	zkcmNumaMapS		*numaMap;
#endif
#if __SCALING__ == SCALING_SMP || defined(CHIPSET_CPU_NUMA_GENERATE_SHBANK)
	zkcmSmpMapS		*smpMap;
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
	// Ensure that the CPU info mod is ready for use.
	ret = zkcmCore.cpuDetection.initialize();
	if (ret != ERROR_SUCCESS) {
		__kprintf(ERROR CPUTRIB"initialize2: CPU mod init failed.\n");
	};

#if __SCALING__ >= SCALING_SMP
	bspId = zkcmCore.cpuDetection.getBspId();
#else
	bspId = 0;
#endif

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
			__kprintf(ERROR CPUTRIB"initialize2: Chipset failed "
				"to safely transition to SMP mode.\n"
				"\tProceeding in uniprocessor mode.\n");
		}
		else {
			_usingChipsetSmpMode = 1;
		};
	}
	else
	{
		__kprintf(ERROR CPUTRIB"initialize2:\n"
			"\tIMPORTANT: Your kernel was compiled as a multi-cpu\n"
			"\tbuild kernel, but your chipset reports that it is\n"
			"\tnot safe to use multi-cpu processing on it.\n"
			"\t\tIf your board is a new board, this may indicate\n"
			"\tthat it is flawed or defective. If it is an old\n"
			"\tboard, it may just mean that your kernel was built\n"
			"\twith multi-cpu features that your board simply can't\n"
			"\thandle.\n");

		// Small delay.
		for (uarch_t i=0; i<500000; i++) { cpuControl::subZero(); };
		return fallbackToUpMode(bspId, bspId);
	};
	/* On MP build it's beneficial to pre-determine the size of the BMPs
	 * so that when CPUs are being spawned the BMPs aren't constantly
	 * resized.
	 **/
#if __SCALING__ >= SCALING_CC_NUMA
	numaMap = zkcmCore.cpuDetection.getNumaMap();
	if (numaMap != __KNULL && numaMap->nCpuEntries > 0)
	{
		getHighestId(
			highestBankId, numaMap, cpuEntries,
			bankId, numaMap->nCpuEntries);

		getHighestId(
			highestCpuId, numaMap, cpuEntries,
			cpuId, numaMap->nCpuEntries);
	};
	if (availableBanks.initialize(highestBankId + 1) != ERROR_SUCCESS) {
		panic(CC""CPUTRIB"AvailableBanks BMP initialize() failed.\n");
	};
#endif
#endif


#if __SCALING__ == SCALING_SMP || defined(CHIPSET_CPU_NUMA_GENERATE_SHBANK)
	smpMap = zkcmCore.cpuDetection.getSmpMap();
	if (smpMap != __KNULL && smpMap->nEntries > 0)
	{
		getHighestId(
			highestCpuId, smpMap, entries, cpuId, smpMap->nEntries);
	};

	if (availableCpus.initialize(highestCpuId + 1) != ERROR_SUCCESS) {
		panic(CC""CPUTRIB"Failed to initialize() availableCpus bmp.\n");
	};

	if (onlineCpus.initialize(highestCpuId + 1) != ERROR_SUCCESS) {
		panic(CC""CPUTRIB"Failed to initialize() onlineCpus bmp.\n");
	};
#endif

#if __SCALING__ >= SCALING_CC_NUMA
	if (numaInit() != ERROR_SUCCESS) {
		return fallbackToUpMode(bspId, bspId);
	};
#endif
#if __SCALING__ == SCALING_SMP
	if (smpInit() != ERROR_SUCCESS) {
		return fallbackToUpMode(bspId, bspId);
	};
#endif
#if __SCALING__ == SCALING_UNIPROCESSOR
	return uniProcessorInit();
#else
	return ERROR_SUCCESS;
#endif
}

#if __SCALING__ >= SCALING_CC_NUMA
error_t cpuTribC::numaInit(void)
{
	error_t			ret;
	zkcmNumaMapS		*numaMap;
#ifdef CHIPSET_CPU_NUMA_GENERATE_SHBANK
	zkcmSmpMapS		*smpMap;
	ubit8			found;
#endif

	/**	EXPLANATION:
	 * Use the chipset's NUMA CPU map and spawn streams for each CPU. Each
	 * CPU is also powered on as soon as it is given a stream.
	 *
	 * The chipset CPU info module should have been initialized outside
	 * by the calling code. Should be able to just use the module from here
	 * on.
	 **/

	// Return non success to force caller to execute fallbackToUpMode().
	if (!usingChipsetSmpMode()) { return ERROR_GENERAL; };

	numaMap = zkcmCore.cpuDetection.getNumaMap();
	if (numaMap != __KNULL && numaMap->nCpuEntries > 0)
	{
		for (uarch_t i=0; i<numaMap->nCpuEntries; i++)
		{
			ret = spawnStream(
				numaMap->cpuEntries[i].bankId,
				numaMap->cpuEntries[i].cpuId,
				numaMap->cpuEntries[i].cpuAcpiId);

			if (ret == ERROR_SUCCESS)
			{
				if (numaMap->cpuEntries[i].cpuId == bspId) {
					continue;
				};

				getStream(numaMap->cpuEntries[i].cpuId)
					->powerControl(CPUSTREAM_POWER_ON, 0);
			}
			else
			{
				__kprintf(ERROR CPUTRIB"numaInit: Failed to "
					"spawn stream for CPU %d.\n",
					numaMap->cpuEntries[i].cpuId);
			};
		};

		return ERROR_SUCCESS;
	}
	else
	{
		__kprintf(WARNING CPUTRIB"numaInit: NUMA build, but CPU mod "
#ifndef CHIPSET_CPU_NUMA_GENERATE_SHBANK
			"reports no NUMA CPUs, and no shbank configured.\n");
#else
			"reports no NUMA CPUs.\n");
#endif
	};

#if defined(CHIPSET_CPU_NUMA_GENERATE_SHBANK)				\
	&& defined(CHIPSET_CPU_NUMA_SHBANKID)
	smpMap = zkcmCore.cpuDetection.getSmpMap();
	if (smpMap != __KNULL && smpMap->nEntries > 0)
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
		__kprintf(NOTICE CPUTRIB"numaInit: Using shared CPU bank; ");

		// Case 1 from above.
		if (numaMap != __KNULL && numaMap->nCpuEntries > 0)
		{
			// Filter out the CPUs which need to be in shared bank.
			__kprintf(CC"Filtering out NUMA CPUs.\n");
			for (uarch_t i=0; i<smpMap->nEntries; i++)
			{
				found = 0;
				for (uarch_t j=0; j<numaMap->nCpuEntries; j++)
				{
					if (numaMap->cpuEntries[j].cpuId
						== smpMap->entries[i].cpuId)
					{
						found = 1;
						break;
					};
				};

				// If the CPU is not an extra, don't process it.
				if (found) { continue; };

				ret = spawnStream(
					CHIPSET_CPU_NUMA_SHBANKID,
					smpMap->entries[i].cpuId,
					smpMap->entries[i].cpuId);

				if (ret == ERROR_SUCCESS)
				{
					if (smpMap->entries[i].cpuId == bspId) {
						continue;
					};

					getStream(smpMap->entries[i].cpuId)
						->powerControl(
							CPUSTREAM_POWER_ON, 0);
				}
				else
				{
					__kprintf(ERROR CPUTRIB"numaInit: "
						"Failed to spawn CPU Stream "
						"%d.\n",
						smpMap->entries[i].cpuId);

					continue;
				};
			};

			return ERROR_SUCCESS;
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
			__kprintf(CC"No NUMA map, all CPUs on shared bank (SMP "
				"operation).\n");

			for (uarch_t i=0; i<smpMap->nEntries; i++)
			{
				ret = spawnStream(
					CHIPSET_CPU_NUMA_SHBANKID,
					smpMap->entries[i].cpuId,
					smpMap->entries[i].cpuAcpiId);

				if (ret == ERROR_SUCCESS)
				{
					if (smpMap->entries[i].cpuId == bspId) {
						continue;
					};

					getStream(smpMap->entries[i].cpuId)
						->powerControl(
							CPUSTREAM_POWER_ON, 0);
				}
				else
				{
					__kprintf(ERROR CPUTRIB"numaInit: "
						"Failed to spawn CPU Stream "
						"for CPU %d.",
						smpMap->entries[i].cpuId);
				};
			};

			return ERROR_SUCCESS;
		};
	}
	else
	{
		// No SMP map. If also no NUMA map, then assume single CPU.
		if (numaMap == __KNULL || numaMap->nCpuEntries == 0)
		{
			__kprintf(WARNING CPUTRIB"numaInit: Falling back to "
				"uniprocessor mode.\n");

			return ERROR_CRITICAL;
		};
	};
#endif

	return ERROR_CRITICAL;
}
#endif

#if __SCALING__ == SCALING_SMP
error_t cpuTribC::smpInit(void)
{
	error_t			ret;
	zkcmSmpMapS		*smpMap;

	/**	EXPLANATION:
	 * Quite simple: look for an SMP map, and if none exists, return an
	 * error to indicate that the kernel should fallback to uniprocessor
	 * mode.
	 **/

	// Return non success to force caller to execute fallbackToUpMode().
	if (!usingChipsetSmpMode()) { return ERROR_GENERAL; };
	
	smpMap = zkcmCore.cpuDetection.getSmpMap();
	if (smpMap != __KNULL && smpMap->nEntries > 0)
	{
		for (uarch_t i=0; i<smpMap->nEntries; i++)
		{
			ret = spawnStream(
				smpMap->entries[i].cpuId,
				smpMap->entries[i].cpuAcpiId);

			if (ret == ERROR_SUCCESS)
			{
				if (smpMap->entries[i].cpuId == bspId) {
					continue;
				};

				getStream(smpMap->entries[i].cpuId)
					->powerControl(CPUSTREAM_POWER_ON, 0);
			}
			else
			{
				__kprintf(ERROR CPUTRIB"smpInit: Failed to "
					"spawn CPU Stream for CPU %d.\n",
					smpMap->entries[i].cpuId);
			};
		};
	}
	else
	{
		// Fallback to uniprocessor mode if no SMP map.
		__kprintf(WARNING CPUTRIB"smpInit: SMP build, but CPU mod "
			"reports no SMP CPUs.\n");

		return ERROR_CRITICAL;
	};

	return ERROR_SUCCESS;
}
#endif

#if __SCALING__ == SCALING_UNIPROCESSOR
error_t cpuTribC::uniProcessorInit(void)
{
	/**	EXPLANATION:
	 * Base setup for a UP build. Nothing very interesting. It just makes
	 * sure that if getStream(foo) is called, the BSP will always be
	 * returned, no matter what argument is passed.
	 **/
	/* TODO: Check if you should call spawnStream() and initialize() on the
	 * BSP on a non-MP build.
	 **/
	cpu = getCurrentCpuStream();
	return ERROR_SUCCESS;
}
#endif

/* Args:
 * __pb = pointer to the bitmapC object to be checked.
 * __n = the bit number which the bitmap should be able to hold. For example,
 *	 if the bit number is 32, then the BMP will be checked for 33 bit
 *	 capacity or higher, since 0-31 = 32 bits, and bit 32 would be the 33rd
 *	 bit.
 * __ret = pointer to variable to return the error code from the operation in.
 * __fn = The name of the function this macro was called inside.
 * __bn = the human recognizible name of the bitmapC instance being checked.
 *
 * The latter two are used to print out a more useful error message should an
 * error occur.
 **/
#define CHECK_AND_RESIZE_BMP(__pb,__n,__ret,__fn,__bn)			\
	do { \
	*(__ret) = ERROR_SUCCESS; \
	if ((__n) > (signed)(__pb)->getNBits() - 1) \
	{ \
		*(__ret) = (__pb)->resizeTo((__n) + 1); \
		if (*(__ret) != ERROR_SUCCESS) \
		{ \
			__kprintf(ERROR CPUTRIB"%s: resize failed on %s with " \
				"required capacity = %d.\n", \
				__fn, __bn, __n); \
		}; \
	}; \
	} while (0);

#if __SCALING__ >= SCALING_CC_NUMA
error_t cpuTribC::spawnStream(numaBankId_t bid, cpu_t cid, ubit32 cpuAcpiId)
#else
error_t cpuTribC::spawnStream(cpu_t cid, ubit32 cpuAcpiId)
#endif
{
	error_t		ret;
#if __SCALING__ >= SCALING_CC_NUMA
	numaCpuBankC	*ncb;
#endif
	cpuStreamC	*cs;


#if __SCALING__ >= SCALING_CC_NUMA
	/**	EXPLANATION:
	 * The NUMA case requires that the CPU be assigned to its holding bank.
	 * The extra NUMA code is really just dealing with that.
	 **/
	if ((ncb = getBank(bid)) == __KNULL)
	{
		if ((ret = createBank(bid)) != ERROR_SUCCESS)
		{
			__kprintf(ERROR CPUTRIB"spawnStream(%d, %d, %d): "
				"Failed to create bank.\n",
				bid, cid, cpuAcpiId);

			return __KNULL;
		};
		ncb = getBank(bid);
	};

	CHECK_AND_RESIZE_BMP(
		&ncb->cpus, cid, &ret,
		"spawnStream", "resident bank");

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
	if (cid != bspId) {
#if __SCALING__ >= SCALING_CC_NUMA
		cs = new (memoryTrib.__kmemoryStream.memAlloc(
			PAGING_BYTES_TO_PAGES(sizeof(cpuStreamC)),
			MEMALLOC_NO_FAKEMAP))
				cpuStreamC(bid, cid, cpuAcpiId);
#else
		cs = new (memoryTrib.__kmemoryStream.*memAlloc(
			PAGING_BYTES_TO_PAGES(sizeof(cpuStreamC)),
			MEMALLOC_NO_FAKEMAP))
				cpuStreamC(cid, cpuAcpiId);
#endif
	}
	else
	{
		cs = &bspCpu;
#if __SCALING__ >= SCALING_CC_NUMA
		cs->bankId = bid;
#endif
		cs->cpuAcpiId = cpuAcpiId;
		cs->initialize();
	};

	if (cs == __KNULL) { return ERROR_MEMORY_NOMEM; };

	if ((ret = cpuStreams.addItem(cid, cs)) != ERROR_SUCCESS)
	{
#if __SCALING__ >= SCALING_CC_NUMA
		__kprintf(ERROR CPUTRIB"spawnStream(%d, %d, %d): Failed to add "
			"new CPU stream to list.\n",
			bid, cid, cpuAcpiId);
#else
		__kprintf(ERROR CPUTRIB"spawnStream(%d, %d): Failed to add "
			"new CPU stream to list.\n",
			cid, cpuAcpiId);
#endif		
	};

#if __SCALING__ >= SCALING_CC_NUMA
	__kprintf(NOTICE CPUTRIB"spawnStream(%d,%d,%d): Success.\n",
		bid, cid, cpuAcpiId);
#else
	__kprintf(NOTICE CPUTRIB"spawnStream(%d,%d): Success.\n",
		cid, cpuAcpiId);
#endif
	__kupdateAffinity(cid, CPUTRIB___KUPDATEAFFINITY_ADD);

	return ERROR_SUCCESS;
}

void cpuTribC::destroyStream(cpu_t cid)
{
#if __SCALING__ >= SCALING_CC_NUMA
	numaCpuBankC		*ncb;
#endif
	cpuStreamC		*cs;

	/**	NOTE:
	 * This will probably never be called on most machines. Called on CPU
	 * unplug/physical removal.
	 **/
	cs = getStream(cid);
	if (cs == __KNULL) { return; };

	availableCpus.unsetSingle(cid);

	// Now remove it from the list of CPUs and free the mem.
	cpuStreams.removeItem(cid);
	__kupdateAffinity(cid, CPUTRIB___KUPDATEAFFINITY_REMOVE);
	cs->~cpuStreamC();
	memoryTrib.__kmemoryStream.memFree(cs);

#if __SCALING__ >= SCALING_CC_NUMA
	ncb = getBank(cs->bankId);
	if (ncb != __KNULL) {
		ncb->cpus.unsetSingle(cid);
	};
#endif
}

#if __SCALING__ >= SCALING_CC_NUMA
error_t cpuTribC::createBank(numaBankId_t bankId)
{
	error_t			err;
	numaCpuBankC		*ncb;

	if ((ncb = getBank(bankId)) != __KNULL) {
		return ERROR_SUCCESS;
	};

	CHECK_AND_RESIZE_BMP(
		&availableBanks, bankId, &err,
		"createBank", "available banks");

	if (err != ERROR_SUCCESS) { return err; };

	ncb = new (memoryTrib.__kmemoryStream.memAlloc(
		PAGING_BYTES_TO_PAGES(sizeof(numaCpuBankC)),
		MEMALLOC_NO_FAKEMAP))
			numaCpuBankC;

	if (ncb == __KNULL) { return ERROR_MEMORY_NOMEM; };

	err = cpuBanks.addItem(bankId, ncb);
	if (err != ERROR_SUCCESS)
	{
		__kprintf(ERROR CPUTRIB"createBank(%d): Failed to add to list."
			"\n", bankId);

		ncb->~numaCpuBankC();
		memoryTrib.__kmemoryStream.memFree(ncb);
		return err;
	}

	availableBanks.setSingle(bankId);
	__kprintf(NOTICE CPUTRIB"createBank(%d): Successful.\n", bankId);
	return ERROR_SUCCESS;
}
#endif

#if __SCALING__ >= SCALING_CC_NUMA
void cpuTribC::destroyBank(numaBankId_t bid)
{
	numaCpuBankC		*ncb;

	ncb = getBank(bid);
	if (ncb == __KNULL) { return; };

	availableBanks.unsetSingle(bid);
	cpuBanks.removeItem(bid);
	ncb->~numaCpuBankC();
	memoryTrib.__kmemoryStream.memFree(ncb);
}
#endif

error_t cpuTribC::__kupdateAffinity(cpu_t cid, ubit8 action)
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
			&__korientationThread.cpuAffinity, cid, &ret,
			"__kupdateAffinity", "__korientation CPU affinity");

		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR CPUTRIB"__korientation unable to use "
				"CPU %d.\n",
				cid);
		}
		else {
			__korientationThread.cpuAffinity.setSingle(cid);
		};

		CHECK_AND_RESIZE_BMP(
			&__kcpuPowerOnThread.cpuAffinity, cid, &ret,
			"__kupdateAffinity", "__kCPU Power on CPU affinity");

		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR CPUTRIB"__kcpuPowerOn unable to use "
				"CPU %d.\n",
				cid);
		}
		else {
			__kcpuPowerOnThread.cpuAffinity.setSingle(cid);
		};

		return ERROR_SUCCESS;

	case CPUTRIB___KUPDATEAFFINITY_REMOVE:
		__korientationThread.cpuAffinity.unsetSingle(cid);
		__kcpuPowerOnThread.cpuAffinity.unsetSingle(cid);
		return ERROR_SUCCESS;

	default: return ERROR_INVALID_ARG_VAL;
	};

	__kprintf(ERROR CPUTRIB"__kupdateAffinity: Reached unreachable "
		"point.\n");

	return ERROR_UNKNOWN;
}

