
#include <scaling.h>
#include <chipset/cpus.h>
#include <chipset/pkg/chipsetPackage.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <__kthreads/__korientation.h>


// A loop through all the entries of the particular map to find the highest val.
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


static cpu_t		highestCpuId=0;
#if defined(CHIPSET_CPU_NUMA_GENERATE_SHBANK) \
	|| defined(CHIPSET_CPU_NUMA_SHBANKID)
static numaBankId_t	highestBankId=CHIPSET_CPU_NUMA_SHBANKID;
#else
static numaBankId_t	highestBankId=0;
#endif

cpuTribC::cpuTribC(void)
{
}

error_t cpuTribC::initialize2(void)
{
	error_t			ret;
	chipsetSmpMapS		*smpMap=__KNULL;
	chipsetNumaMapS		*numaMap=__KNULL;
	cpuModS			*cpuMod;
#if (__SCALING__ == SCALING_SMP) || defined(CHIPSET_CPU_NUMA_GENERATE_SHBANK)
	sarch_t			found;
#endif
	numaCpuBankC		*ncb;

	/**	EXPLANATION:
	 * Several abstractions need to be initialized for the CPU Tributary
	 * to work as it should. These are the internal BMP of all CPUs,
	 * and the per-bank BMPs of CPUs on each bank.
	 *
	 * The kernel will first initialize the "SMP" BMP, which simply has one
	 * bit set for each present CPU, regardless of its NUMA locality.
	 *
	 * Next, for each bank in the NUMA map, the kernel will generate a
	 * numaCpuBankC object which is (for now) nothing more than a BMP of
	 * all CPUs on that bank.
	 *
	 * After that, if CHIPSET_CPU_NUMA_GENERATE_SHBANK is defined, the
	 * the kernel will run through each entry in the SMP map (if present)
	 * and for each CPU in the SMP map that doesn't appear in the NUMA map,
	 * the kernel will add that CPU to the Shared Bank to be treated as a
	 * CPU without any known NUMA locality.
	 *
	 *	NOTES:
	 * ^ In the event that no NUMA map is found (on a NUMA kernel build) or
	 *   that the kernel is an SMP build, all CPUs will be added on the
	 *   shared bank.
	 *
	 * ^ If a NUMA map is found, but no SMP map is found, shbank will not be
	 *   generated.
	 *
	 * ^ On an SMP build, the NUMA map is ignored.
	 * ^ On uni-processor build, both the SMP and NUMA maps are ignored and
	 *   a single CPU is assumed regardless of presence of other CPUs.
	 **/
#if __SCALING__ >= SCALING_SMP
	if (chipsetPkg.cpus != __KNULL)
	{
		if ((*chipsetPkg.cpus->initialize)() == ERROR_SUCCESS)
		{
			cpuMod = chipsetPkg.cpus;

#if __SCALING__ >= SCALING_CC_NUMA
			numaMap = (*cpuMod->getNumaMap)();
#endif
			smpMap = (*cpuMod->getSmpMap)();
		}
		else
		{
			__kprintf(ERROR CPUTRIB"CPU Info mod present, but "
				"initialization failed.\n");

			goto fallbackToUp;
		};
	}
	else
	{
		__kprintf(ERROR CPUTRIB"No CPU Info mod present.\n");
		goto fallbackToUp;
	};
#endif

#if __SCALING__ >= SCALING_SMP
	/* First find out the highest CPU ID number and the highest NUMA bank
	 * ID number currently available on the chipset.
	 **/
#if __SCALING__ >= SCALING_CC_NUMA
	if (numaMap != __KNULL)
	{
		// Expands to a macro which performs a loop.
		getHighestId(
			highestCpuId, numaMap, cpuEntries, cpuId,
			numaMap->nCpuEntries);
	};
#endif
	if (smpMap != __KNULL)
	{
		getHighestId(
			highestCpuId, smpMap, entries, cpuId,
			smpMap->nEntries);
	};

	if (numaMap != __KNULL)
	{
		getHighestId(
			highestBankId, numaMap, cpuEntries, bankId,
			numaMap->nCpuEntries);
	};

	__kprintf(NOTICE"Highest cpu id: %d.\n", highestCpuId);
	__kprintf(NOTICE"Highest bank ID: %d.\n", highestBankId);

	/* Next, initialize the "all available CPUs", "all online CPUs" and
	 * "all available NUMA banks" BMPs with a number of bits equal to the
	 * highest CPU and NUMA bank IDs on the chipset.
	 **/
	ret = onlineCpus.initialize(highestCpuId + 1);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR CPUTRIB"Failed to init() online CPU bmp with "
			"%d as the highest detected CPU Id.\n",
			highestCpuId);

		return ret;
	};

	ret = availableCpus.initialize(highestCpuId + 1);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR CPUTRIB"Failed to init() available CPU bmp "
			"with %d as the higest CPU id.\n",
			highestCpuId);

		return ret;
	};

	ret = availableBanks.initialize(highestBankId + 1);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR CPUTRIB"Failed to init() available bank bmp "
			"with %d as the higest bank id.\n",
			highestCpuId);

		return ret;
	};

#if __SCALING__ >= SCALING_CC_NUMA
	/* Next parse the NUMA map, and for every entry, see if we already have
	 * a managing numaCpuBankC object for the bank that the entry is
	 * related to.
	 *
	 * If not of course, it's created on the fly in this next section.
	 **/
	if (numaMap != __KNULL)
	{
		for (ubit32 i=0; i<numaMap->nCpuEntries; i++)
		{
			if (getBank(numaMap->cpuEntries[i].bankId) == __KNULL)
			{
				// Create a bank for it if one doesn't exist.
				ret = createBank(numaMap->cpuEntries[i].bankId);
				if (ret != ERROR_SUCCESS)
				{
					__kprintf(ERROR CPUTRIB"createBank(%d) "
						"failed returning %d.\n",
						numaMap->cpuEntries[i].bankId,
						ret);

					continue;
				};

				__kprintf(NOTICE"Created bank with id %d.\n",
					numaMap->cpuEntries[i].bankId);
			};
		};
	};

	__kprintf(NOTICE"Finished creating and initializing numa banks.\n");

	// Next, for every entry, create a new CPU stream for the related CPU.
	if (numaMap != __KNULL)
	{
		for (ubit32 i=0; i<numaMap->nCpuEntries; i++)
		{
			// Make sure the bank for stream being created exists.
			ncb = getBank(numaMap->cpuEntries[i].bankId);
			if (ncb == __KNULL)
			{
				__kprintf(WARNING"Bank %d found in NUMA map, "
					"but no bank object was created for it"
					"\n", numaMap->cpuEntries[i].bankId);

				continue;
			};

			ret = spawnStream(
				numaMap->cpuEntries[i].bankId,
				numaMap->cpuEntries[i].cpuId);

			if (ret != ERROR_SUCCESS)
			{
				__kprintf(ERROR CPUTRIB"Failed to spawn CPU "
					"Stream for CPU ID %d on bank %d.\n",
					numaMap->cpuEntries[i].cpuId,
					numaMap->cpuEntries[i].bankId);

				continue;
			};
		};
	};
#endif

#endif

#if (__SCALING__ == SCALING_SMP) || defined(CHIPSET_CPU_NUMA_GENERATE_SHBANK)
	/* Next, if the supervisor configured the kernel to build with a
	 * shared bank for all CPUs that exist, but are not on an identifiable
	 * bank (kernel didn't find a bank associated with its entry), we now
	 * do the processing necessary to fulfil that directive.
	 **/
	__kprintf(NOTICE"About to create shbank.\n");
	ret = createBank(CHIPSET_CPU_NUMA_SHBANKID);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR CPUTRIB"Failed to generate shbank.\n");

#if __SCALING__ >= SCALING_CC_NUMA
		/* If the shared bank could not be generated, and there is no
		 * NUMA map, then we have no CPU to use except (we assume)
		 * the one we're running on. Only option is to fall back to
		 * UP mode.
		 **/
		if (numaMap == __KNULL || numaMap->nCpuEntries == 0) {
			goto fallbackToUp;
		}
		else
		{
			/**	NOTE:
			 * Reasoning here is that we already have the NUMA banks
			 * done and everything, so we can just set their bits,
			 * wake them up, and then move on without a shared bank.
			 *
			 * In other words, we already generated all the other
			 * banks above, and shared bank is the only one that
			 * failed. So we can ignore shbank and use the CPUs on
			 * the other banks.
			 **/
			__kprintf(NOTICE CPUTRIB"Waking CPUs anyway.\n");
			goto wakeCpus;
		};
#else
		goto fallbackToUp;
#endif
	};

	__kprintf(NOTICE"Created and initialized SHBANK.\n");

	/* If there is a NUMA map, run through, and for each CPU in the SMP map
	 * that does not exist in the NUMA map, add it to the shared bank.
	 *
	 * If there is no NUMA map, add all CPUs to the shared bank.
	 **/
	__kprintf(NOTICE"The following cpus are in both smp and numa maps.\n");
	if (smpMap != __KNULL && smpMap->nEntries != 0
		&& numaMap != __KNULL && numaMap->nCpuEntries != 0)
	{
		/* For each entry in the SMP map, check to see if you find
		 * an equivalent CPU ID in the NUMA Map. If not, then add the
		 * CPU to the shared bank.
		 **/
		for (ubit32 i=0; i<smpMap->nEntries; i++)
		{
			found = 0;
			for (ubit32 j=0; j<numaMap->nCpuEntries; j++)
			{
				if (numaMap->cpuEntries[j].cpuId
					== smpMap->entries[i].cpuId)
				{
					found = 1;
					break;
				};
			};

			if (found)
			{
				__kprintf(CC"%d ", smpMap->entries[i].cpuId);
				continue;
			};

			ret = spawnStream(
				CHIPSET_CPU_NUMA_SHBANKID,
				smpMap->entries[i].cpuId);

			if (ret != ERROR_SUCCESS)
			{
				__kprintf(ERROR CPUTRIB"Failed to spawn CPU "
					"Stream for Id %d on shbank.\n",
					smpMap->entries[i].cpuId);
			};
		};
	}
	else if ((smpMap != __KNULL) && (numaMap == __KNULL))
	{
		/* If there's an SMP map, but no NUMA map, set everything into
		 * shbank.
		 **/
		for (uarch_t i=0; i<smpMap->nEntries; i++)
		{
			spawnStream(
				CHIPSET_CPU_NUMA_SHBANKID,
				smpMap->entries[i].cpuId);
		};
	}
	else {
		goto fallbackToUp;
	};
	__kprintf(CC"\n");
#endif

	// Don't forget to wake up the CPUs.
wakeCpus:
	return ERROR_SUCCESS;

fallbackToUp:
#if __SCALING__ > SCALING_UNI_PROCESSOR
	__kprintf(WARNING CPUTRIB"CPU detection fell through to uni-processor "
		"mode. Assuming single CPU.\n");
#endif

#if __SCALING__ >= SCALING_SMP
	ncb = getBank(CHIPSET_CPU_NUMA_SHBANKID);
	if (ncb == __KNULL)
	{
		// Try to create it again for the sake of it.
		ret = createBank(CHIPSET_CPU_NUMA_SHBANKID);
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR CPUTRIB"Failed to create ShBank on a "
				"non-UP build, while attempting to fall back "
				"to UP mode. No CPU management in place.\n\n"

				"Kernel forced to halt orientation.\n");

			return ERROR_FATAL;
		}
		else {
			availableBanks.setSingle(CHIPSET_CPU_NUMA_SHBANKID);
		};
	};

	// Set a single bit for CPU 0, our UP mode single CPU.
	ncb->cpus.setSingle(0);
	availableCpus.setSingle(0);
	onlineCpus.setSingle(0);
#else
	cpu = getCurrentCpuStream();
#endif
	return ERROR_SUCCESS;
}
	

cpuTribC::~cpuTribC(void)
{
}

/* Args:
 * __pb = pointer to the bitmapC object to be checked.
 * __n = the bit number which the bitmap should be able to hold. For example,
 *	 if the bit number is 32, then the BMP will be checked for 33 bit
 *	 capacity or higher, since 0-31 = 32 bits, and bit 32 would be the 33rd
 *	 bit.
 * __ret = variable to return the error code from the operation in.
 * __fn = The name of the function this macro was called inside.
 * __bn = the human recognizible name of the bitmapC instance being checked.
 *
 * The latter two are used to print out a more useful error message should an
 * error occur.
 **/
#define CHECK_AND_RESIZE_BMP(__pb,__n,__ret,__fn,__bn)			\
	if ((__n) > (signed)(__pb)->getNBits() - 1) \
	{ \
		*(__ret) = (__pb)->resizeTo((__n) + 1); \
		if (*(__ret) != ERROR_SUCCESS) \
		{ \
			__kprintf(ERROR CPUTRIB"%s: resize failed on %s with " \
				"required capacity = %d.\n", \
				__fn, __bn, __n); \
		}; \
	}

error_t cpuTribC::createBank(numaBankId_t id)
{
	error_t		ret;
	numaCpuBankC	*ncb;

	/**	EXPLANATION:
	 * Creates a new numaCpuBankC object, and adds it to the CPU Trib's
	 * unified list of CPU banks, then sets the new bank's bit in the
	 * CPU Trib's "availableBanks" BMP.
	 **/

	// First make sure that the bitmap can hold the new bank's bit.
	CHECK_AND_RESIZE_BMP(&availableBanks, id, &ret,
		"createBank", "available banks");

	if (ret != ERROR_SUCCESS) { return ret; };

	// Allocate the new CPU bank object.
	ncb = new (
		(memoryTrib.__kmemoryStream
			.*memoryTrib.__kmemoryStream.memAlloc)(
				PAGING_BYTES_TO_PAGES(sizeof(numaCpuBankC)),
				MEMALLOC_NO_FAKEMAP))
		numaCpuBankC;

	if (ncb == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	// Add the newly initialized bank to the bank list.
	ret = cpuBanks.addItem(id, ncb);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR CPUTRIB"Failed to add CPU bank with ID %d to "
			"global list.",
			id);

		// Call destructor.
		ncb->~numaCpuBankC();
		memoryTrib.__kmemoryStream.memFree(ncb);
		return ret;
	};

	availableBanks.setSingle(id);
	return ret;
}

void cpuTribC::destroyBank(numaBankId_t id)
{
	numaCpuBankC		*ncb;

	ncb = static_cast<numaCpuBankC *>( cpuBanks.getItem(id) );
	ncb->~numaCpuBankC();

	memoryTrib.__kmemoryStream.memFree(ncb);

	cpuBanks.removeItem(id);
	availableBanks.unsetSingle(id);
}

error_t cpuTribC::spawnStream(numaBankId_t bid, cpu_t cid)
{
	cpuStreamC	*cs;
	numaCpuBankC	*ncb;
	error_t		ret;

	/**	EXPLANATION:
	 * Called when a new logical CPU is detected as being interted on the
	 * board and physically present. This may be at boot during the CPU
	 * enumeration stage, or at runtime for a hot-plug detected CPU.
	 *
	 * The sequence is as follows:
	 *	* Create a new cpuStreamC object.
	 *	* Set its bit on the bank it belongs to, and in the CPU Trib's
	 *	  unified list of all available CPUs.
	 *
	 * * The caller should no have to check to see whether or not the bank
	 *   to which the new CPU pertains has been created.
	 **/
	/* Make sure the available CPUs bmp, onlineCpus bmp and the BMP of
	 * CPUs on the containing bank all have enough bits to hold the new
	 * CPU's bit.
	 **/
	CHECK_AND_RESIZE_BMP(&availableCpus, cid, &ret,
		"spawnStream", "available CPUs");

	if (ret != ERROR_SUCCESS) { return ret; };

	CHECK_AND_RESIZE_BMP(&onlineCpus, cid, &ret,
		"spawnStream", "online CPUs");

	if (ret != ERROR_SUCCESS) { return ret; };

	if ((ncb = getBank(bid)) == __KNULL)
	{
		ret = createBank(bid);
		if (ret != ERROR_SUCCESS) {
			return ret;
		};

		ncb = getBank(bid);
	};

	CHECK_AND_RESIZE_BMP(&ncb->cpus, cid, &ret,
		"spawnStream", "resident bank");

	if (ret != ERROR_SUCCESS) { return ret; };

	cs = new cpuStreamC(bid, cid);
	if (cs == __KNULL) { return ERROR_MEMORY_NOMEM; };

	cpuStreams.addItem(cid, cs);

	ncb->cpus.setSingle(cid);
	availableCpus.setSingle(cid);
	return ERROR_SUCCESS;
}

void cpuTribC::destroyStream(cpu_t cpuId)
{
	cpuStreamC		*cs;
	numaCpuBankC		*ncb;

	cs = static_cast<cpuStreamC *>( cpuStreams.getItem(cpuId) );
	if (cs == __KNULL) { return; };

	availableCpus.unsetSingle(cpuId);

	ncb = getBank(cs->bankId);
	if (ncb != __KNULL) {
		ncb->cpus.unsetSingle(cpuId);
	};
	delete cs;
	cpuStreams.removeItem(cpuId);
}

