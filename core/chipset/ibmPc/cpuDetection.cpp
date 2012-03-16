
#include <debug.h>

#include <__ksymbols.h>
#include <arch/arch.h>
#include <arch/memory.h>
#include <arch/io.h>
#include <arch/x8632/cpuid.h>
#include <chipset/memoryAreas.h>
#include <chipset/zkcm/cpuDetection.h>
#include <platform/cpu.h>
#include <asm/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/libacpi.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <kernel/common/cpuTrib/cpuStream.h>
#include <__kthreads/__kcpuPowerOn.h>
#include "cpuDetection.h"
#include "i8259a.h"
#include "zkcmIbmPcState.h"


#define SMPINFO		CPUMOD"SMP: "
#define CPUMOD		"CPU Mod: "


error_t ibmPc_cpuMod_initialize(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_cpuMod_shutdown(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_cpuMod_suspend(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_cpuMod_restore(void)
{
	return ERROR_SUCCESS;
}

zkcmNumaMapS *ibmPc_cm_rGnm(void)
{
	zkcmNumaMapS		*ret;
	acpi_rsdtS		*rsdt;
	acpi_rSratS		*srat;
	acpi_rSratCpuS		*cpuEntry;
	void			*handle, *handle2, *context;
	ubit32			nEntries=0, i=0;

	// Caller has determined that there is an RSDT present on the chipset.
	if (acpi::mapRsdt() != ERROR_SUCCESS)
	{
		__kprintf(NOTICE CPUMOD"getNumaMap(): Failed to map in the "
			"RSDT.\n");

		return __KNULL;
	};

	rsdt = acpi::getRsdt();

	// Get the SRAT (all if multiple).
	context = handle = __KNULL;
	srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);
	while (srat != __KNULL)
	{
		// For each SRAT (if multiple), get the LAPIC entries.
		handle2 = __KNULL;
		cpuEntry = acpiRSrat::getNextCpuEntry(srat, &handle2);
		for (; cpuEntry != __KNULL;
			cpuEntry = acpiRSrat::getNextCpuEntry(srat, &handle2))
		{
			// Count the number of entries there are in all.
			nEntries++;
		};

		acpiRsdt::destroySdt((acpi_sdtS *)srat);
		srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);
	};

	__kprintf(NOTICE CPUMOD"getNumaMap(): %d LAPIC SRAT entries.\n",
		nEntries);

	// If there are no NUMA CPUs, return.
	if (nEntries == 0) { return __KNULL; };

	// Now we know how many entries exist. Allocate map and reparse.
	ret = new zkcmNumaMapS;
	if (ret == __KNULL)
	{
		__kprintf(ERROR CPUMOD"getNumaMap(): Failed to allocate room "
			"for CPU NUMA map, or 0 entries found.\n");

		return __KNULL;
	};

	ret->cpuEntries = new numaCpuMapEntryS[nEntries];
	if (ret->cpuEntries == __KNULL)
	{
		delete ret;
		__kprintf(ERROR CPUMOD"getNumaMap(): Failed to alloc "
			"NUMA CPU map entries.\n");

		return __KNULL;
	};

	// Reparse entries and create generic kernel CPU map.
	context = handle = __KNULL;
	srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);
	while (srat != __KNULL)
	{
		handle2 = __KNULL;
		cpuEntry = acpiRSrat::getNextCpuEntry(srat, &handle2);
		for (; cpuEntry != __KNULL;
			cpuEntry = acpiRSrat::getNextCpuEntry(srat, &handle2),
			i++)
		{
			/**	FIXME
			 * For this case, we don't know what the correct ACPI
			 * ID is. We'd like to assume of course, that the ACPI
			 * ID is the same as the LAPIC ID. This is often not
			 * the case.
			 *
			 * The most effective solution may be to cross check
			 * the SRAT entries with the MADT entries and try
			 * to determine if the CPU is also in the MADT. Then we
			 * can extract its ACPI ID from there.
			 **/
			ret->cpuEntries[i].cpuId = cpuEntry->lapicId;
			ret->cpuEntries[i].cpuAcpiId = cpuEntry->lapicId;
			ret->cpuEntries[i].bankId =
				cpuEntry->domain0
				| (cpuEntry->domain1 & 0xFFFFFF00);

			ret->cpuEntries[i].flags = 0;
			if (__KFLAG_TEST(
				cpuEntry->flags, ACPI_SRAT_CPU_FLAGS_ENABLED))
			{
				__KFLAG_SET(
					ret->cpuEntries[i].flags,
					NUMACPUMAP_FLAGS_ONLINE);
			};
		};

		acpiRsdt::destroySdt((acpi_sdtS *)srat);
		srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);
	};

	ret->nCpuEntries = nEntries;
	return ret;
}

zkcmNumaMapS *ibmPc_cpuMod_getNumaMap(void)
{
	/**	EXPLANATION:
	 * For the IBM-PC, a NUMA map of all CPUs is essentially obtained by
	 * asking ACPI's SRAT for its layout.
	 *
	 * We just call on the kernel's libacpi and then return the map. As
	 * simple as that.
	 **/
	if (acpi::findRsdp() != ERROR_SUCCESS) { return __KNULL; };

// Use XSDT on everything but 32-bit, since XSDT has 64-bit physical addresses.
#ifndef __32_BIT__
	if (acpi::testForXsdt())
	{
		ret = ibmPc_cm_xGnm();
		if (ret != __KNULL) { return ret; };
	};
#endif
	if (acpi::testForRsdt())
	{
		__kprintf(NOTICE CPUMOD"getNumaMap(): Falling back to RSDT.\n");
		return ibmPc_cm_rGnm();
	};

	return __KNULL;
}

zkcmSmpMapS *ibmPc_cpuMod_getSmpMap(void)
{
	x86_mpCfgCpuS	*mpCpu;
	void		*handle, *handle2, *context;
	uarch_t		pos=0, nEntries, i;
	zkcmSmpMapS	*ret;
	acpi_rsdtS	*rsdt;
	acpi_rMadtS	*madt;
	acpi_rMadtCpuS	*madtCpu;

	/**	NOTES:
	 *  This function will return a structure describing all CPUs at boot,
	 * regardless of NUMA bank membership, and regardless of whether or not
	 * that CPU is enabled, bad, or was already described in the NUMA Map.
	 *
	 * This SMP map is used to generate shared bank CPUs (CPUs which were
	 * not mentioned in the NUMA Map, yet are mentioned here), and to tell
	 * the kernel which CPUs are bad, and in an "un-enabled", or unusable
	 * state, though not bad.
	 *
	 *	EXPLANATION:
	 * This function will use the Intel x86 MP Specification structures to
	 * enumerate CPUs, and build the SMP map, combining it with the info
	 * returned from the NUMA map.
	 *
	 * This function depends on the kernel libx86mp.
	 **/
	// First try using ACPI. Trust ACPI over MP tables.
	acpi::findRsdp();
	// acpi::initializeCache();
	if (acpi::findRsdp() != ERROR_SUCCESS)
	{
		__kprintf(NOTICE SMPINFO"getSmpMap: No ACPI. Trying MP.\n");
		goto tryMpTables;
	};

	if (!acpi::testForRsdt())
	{
		__kprintf(WARNING SMPINFO"getSmpMap: ACPI, but no RSDT.\n");
		if (acpi::testForXsdt())
		{
			__kprintf(WARNING SMPINFO"getSmpMap: XSDT found. "
				"Consider using 64-bit build for XSDT.\n");
		}
		goto tryMpTables;
	};

	if (acpi::mapRsdt() != ERROR_SUCCESS)
	{
		__kprintf(NOTICE SMPINFO"getSmpMap: ACPI: Failed to map RSDT."
			"\n");

		goto tryMpTables;
	};

	// Now look for an MADT.
	rsdt = acpi::getRsdt();
	context = handle = __KNULL;
	nEntries = 0;
	madt = acpiRsdt::getNextMadt(rsdt, &context, &handle);
	while (madt != __KNULL)
	{
		handle2 = __KNULL;
		for (madtCpu = acpiRMadt::getNextCpuEntry(madt, &handle2);
			madtCpu != __KNULL;
			madtCpu = acpiRMadt::getNextCpuEntry(madt, &handle2))
		{
			if (__KFLAG_TEST(
				madtCpu->flags, ACPI_MADT_CPU_FLAGS_ENABLED))
			{
				nEntries++;
			};
		};

		acpiRsdt::destroySdt((acpi_sdtS *)madt);
		madt = acpiRsdt::getNextMadt(rsdt, &context, &handle);
	};

	__kprintf(NOTICE SMPINFO"getSmpMap: ACPI: %d valid CPU entries.\n",
		nEntries);

	ret = new zkcmSmpMapS;
	if (ret == __KNULL)
	{
		__kprintf(ERROR SMPINFO"getSmpMap: Failed to alloc SMP map.\n");
		return __KNULL;
	};

	ret->entries = new zkcmSmpMapEntryS[nEntries];
	if (ret->entries == __KNULL)
	{
		delete ret;
		__kprintf(ERROR SMPINFO"getSmpMap: Failed to alloc SMP map "
			"entries.\n");

		return __KNULL;
	};

	i=0;
	context = handle = __KNULL;
	madt = acpiRsdt::getNextMadt(rsdt, &context, &handle);
	while (madt != __KNULL)
	{
		handle2 = __KNULL;
		for (madtCpu = acpiRMadt::getNextCpuEntry(madt, &handle2);
			madtCpu != __KNULL;
			madtCpu = acpiRMadt::getNextCpuEntry(madt, &handle2))
		{
			if (__KFLAG_TEST(
				madtCpu->flags, ACPI_MADT_CPU_FLAGS_ENABLED))
			{
				ret->entries[i].cpuId = madtCpu->lapicId;
				ret->entries[i].cpuAcpiId
					= madtCpu->acpiLapicId;

				ret->entries[i].flags = 0;
				i++;
				continue;
			};
			__kprintf(NOTICE SMPINFO"getSmpMap: ACPI: Skipping "
				"un-enabled CPU %d, ACPI ID %d.\n",
				madtCpu->lapicId, madtCpu->acpiLapicId);
		};

		acpiRsdt::destroySdt((acpi_sdtS *)madt);
		madt = acpiRsdt::getNextMadt(rsdt, &context, &handle);
	};

	ret->nEntries = i;
	return ret;

tryMpTables:

	__kprintf(NOTICE SMPINFO"getSmpMap: Falling back to MP tables.\n");
	x86Mp::initializeCache();
	if (!x86Mp::mpFpFound())
	{
		if (x86Mp::findMpFp() == __KNULL)
		{
			__kprintf(NOTICE SMPINFO"getSmpMap: No MP tables.\n");
			return __KNULL;
		};

		if (x86Mp::mapMpConfigTable() == __KNULL)
		{
			__kprintf(ERROR SMPINFO"getSmpMap: Unable to map MP "
				"Config tables.\n");

			return __KNULL;
		};

		// Parse x86 MP table info and discover how many CPUs there are.
		nEntries = 0;
		handle = __KNULL;
		mpCpu = x86Mp::getNextCpuEntry(&pos, &handle);
		for (; mpCpu != __KNULL;
			mpCpu = x86Mp::getNextCpuEntry(&pos, &handle))
		{
			if (__KFLAG_TEST(
				mpCpu->flags, x86_MPCFG_CPU_FLAGS_ENABLED))
			{
				nEntries++;
			};
		};

		__kprintf(NOTICE SMPINFO"getSmpMap: %d valid CPU entries in MP "
			"config tables.\n", nEntries);

		ret = new zkcmSmpMapS;
		if (ret == __KNULL)
		{
			__kprintf(ERROR SMPINFO"getSmpMap: Failed to alloc SMP "
				"Map.\n");

			return __KNULL;
		};

		ret->entries = new zkcmSmpMapEntryS[nEntries];
		if (ret->entries == __KNULL)
		{
			__kprintf(ERROR SMPINFO"getSmpMap: Failed to alloc SMP "
				"map entries.\n");

			delete ret;
			return __KNULL;
		};

		// Iterate one more time and fill in SMP map.
		pos = 0;
		handle = __KNULL;
		mpCpu = x86Mp::getNextCpuEntry(&pos, &handle);
		for (i=0; mpCpu != __KNULL;
			mpCpu = x86Mp::getNextCpuEntry(&pos, &handle))
		{
			if (!__KFLAG_TEST(
				mpCpu->flags, x86_MPCFG_CPU_FLAGS_ENABLED))
			{
				__KFLAG_SET(
					ret->entries[i].flags,
					ZKCM_SMPMAP_FLAGS_BADCPU);

				ret->entries[i].cpuId = mpCpu->lapicId;

				if (__KFLAG_TEST(
					mpCpu->flags, x86_MPCFG_CPU_FLAGS_BSP))
				{
					__KFLAG_SET(
						ret->entries[i].flags,
						ZKCM_SMPMAP_FLAGS_BSP);
				};
				continue;
			};
			__kprintf(NOTICE SMPINFO"getSmpMap: Skipping un-enabled"
				" CPU %d in MP tables.\n",
				mpCpu->lapicId);

		};

		ret->nEntries = i;
		return ret;
	};

	return __KNULL;
}

sarch_t ibmPc_cpuMod_checkSmpSanity(void)
{
	uarch_t		eax, ebx, ecx, edx;

	/**	EXPLANATION:
	 * This function is supposed to be called during the kernel's initial
	 * CPU detection stretch. It does the most basic checks to ensure
	 * that the kernel can successfully run SMP on the chipset.
	 *
	 * These checks include:
	 *	* Check for presence of a LAPIC on the BSP.
	 **/
	// This check will work for both x86-32 and x86-64.
	execCpuid(1, &eax, &ebx, &ecx, &edx);
	if (!(edx & (1 << 9)))
	{
		__kprintf(ERROR CPUMOD"checkSmpSanity(): CPUID[1].EDX[9] LAPIC "
			"check failed. EDX: %x.\n",
			edx);

		return 0;
	};

	return 1;
}

cpu_t ibmPc_cpuMod_getBspId(void)
{
	x86_mpCfgS		*cfgTable;
	acpi_rsdtS		*rsdt;
	acpi_rMadtS		*madt;
	void			*handle, *context;
	paddr_t			tmp;

	/* At some point I'll rewrite this function: it's very unsightly.
	 **/
	if (!ibmPcState.bspInfo.bspIdRequestedAlready)
	{
		/* Run a local CPU ID read. Need libLapic. Will have to parse
		 * APIC/MP tables to determine the LAPIC paddr, then use
		 * liblapic to read the current CPUID.
		 **/
		x86Mp::initializeCache();
		if (!x86Mp::mpConfigTableIsMapped())
		{
			x86Mp::findMpFp();
			if (!x86Mp::mpFpFound()) {
				goto tryAcpi;
			};

			if (x86Mp::mapMpConfigTable() == __KNULL) {
				goto tryAcpi;
			};
		};
		cfgTable = x86Mp::getMpCfg();
		ibmPcState.lapicPaddr = cfgTable->lapicPaddr;
		goto initLibLapic;

tryAcpi:
		acpi::initializeCache();
#ifndef __32_BIT__
		// If not 32 bit, use XSDT.
#else
		// 32 bit uses RSDT only.
		if (acpi::findRsdp() != ERROR_SUCCESS || !acpi::testForRsdt()
			|| acpi::mapRsdt() != ERROR_SUCCESS)
		{
			goto useDefaultPaddr;
		};

		rsdt = acpi::getRsdt();
		context = handle = __KNULL;
		madt = acpiRsdt::getNextMadt(rsdt, &context, &handle);
		if (madt == __KNULL) { goto useDefaultPaddr; };
		ibmPcState.lapicPaddr = (paddr_t)madt->lapicPaddr;

		acpiRsdt::destroyContext(&context);
		acpiRsdt::destroySdt(reinterpret_cast<acpi_sdtS *>( madt ));
#endif
		goto initLibLapic;

useDefaultPaddr:
		__kprintf(NOTICE CPUMOD"getBspId(): Using default paddr.\n");
		ibmPcState.lapicPaddr = 0xFEE00000;

initLibLapic:
		__kprintf(NOTICE CPUMOD"getBspId(): LAPIC paddr = 0x%P.\n",
			ibmPcState.lapicPaddr);

		x86Lapic::initializeCache();
		if (!x86Lapic::getPaddr(&tmp)) {
			x86Lapic::setPaddr(ibmPcState.lapicPaddr);
		};

		if (x86Lapic::mapLapicMem() != ERROR_SUCCESS)
		{
			__kprintf(WARNING CPUMOD"getBspId(): Failed to map "
				"lapic into kernel vaddrspace. Unable to get "
				"true BSP id.\n");

			return 0;
		};

		ibmPcState.bspInfo.bspId = x86Lapic::read32(
			x86LAPIC_REG_LAPICID);

		ibmPcState.bspInfo.bspIdRequestedAlready = 1;
	};

	ibmPcState.bspInfo.bspId >>= 24;
	return ibmPcState.bspInfo.bspId;
}

static error_t ibmPc_cpuMod_setSmpMode(void)
{
	error_t		ret;
	ubit8		*lowmem;
	void		*srcAddr, *destAddr;
	uarch_t		copySize, cpuFlags;
	x86_mpFpS	*mpFp;

	/**	EXPLANATION:
	 * This function will enable Symmetric I/O mode on the IBM-PC chipset.
	 * By extension, it also sets the CMOS warm reset mode and the BIOS
	 * reset vector.
	 *
	 * Everything done here is taken from the MP specification and Intel
	 * manuals, with cross-examination from Linux source just in case.
	 *
	 * Naturally, to access lowmem, we use the chipset's memoryAreas
	 * manager.
	 *
	 * NOTE:
	 * * I don't think you need to enable Symm. I/O mode to use the LAPICs.
	 **/
	ret = chipsetMemAreas::mapArea(CHIPSET_MEMAREA_LOWMEM);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR CPUMOD"setSmpMode(): Failed to map lowmem.\n");
		return ret;
	};

	lowmem = static_cast<ubit8 *>(
		chipsetMemAreas::getArea(CHIPSET_MEMAREA_LOWMEM) );

	// Change the warm reset vector.
	*reinterpret_cast<uarch_t **>( &lowmem[(0x40 << 4) + 0x67] ) =
		&__kcpuPowerOnTextStart;

	// TODO: Set CMOS reset operation to "warm reset".
	cpuControl::safeDisableInterrupts(&cpuFlags);
	io::write8(0x70, 0x0F);
	io::write8(0x71, 0x0A);
	cpuControl::safeEnableInterrupts(cpuFlags);

	/* Next, we need to copy the kernel's .__kcpuPowerOn[Text/Data] stuff to
	 * lowmem. Memcpy() ftw...
	 **/
	srcAddr = (void *)(((uarch_t)&__kcpuPowerOnTextStart
		- x8632_IBMPC_POWERON_PADDR_BASE)
		+ ARCH_MEMORY___KLOAD_VADDR_BASE);

	destAddr = &lowmem[(uarch_t)&__kcpuPowerOnTextStart];
	copySize = (uarch_t)&__kcpuPowerOnTextEnd
		- (uarch_t)&__kcpuPowerOnTextStart;

	__kprintf(NOTICE CPUMOD"setSmpMode: Copy CPU wakeup code: 0x%p "
		"to 0x%p; %d B.\n",
		srcAddr, destAddr, copySize);

	memcpy8(destAddr, srcAddr, copySize);

	srcAddr = (void *)(((uarch_t)&__kcpuPowerOnDataStart
		- x8632_IBMPC_POWERON_PADDR_BASE)
		+ ARCH_MEMORY___KLOAD_VADDR_BASE);

	destAddr = &lowmem[(uarch_t)&__kcpuPowerOnDataStart];
	copySize = (uarch_t)&__kcpuPowerOnDataEnd
		- (uarch_t)&__kcpuPowerOnDataStart;

	__kprintf(NOTICE CPUMOD"setSmpMode: Copy CPU wakeup data: 0x%p "
		"to 0x%p; %d B.\n",
		srcAddr, destAddr, copySize);

	memcpy8(destAddr, srcAddr, copySize);

	// Mask all ISA IRQs at the PIC:
	ibmPc_i8259a_maskAll();

	/** EXPLANATION
	 * Next, we parse the MP tables to see if the chipset has the IMCR
	 * implemented. If so, we turn on Symm. I/O mode.
	 *
	 * If there are no MP tables, the chipset is assumed to be in virtual
	 * wire mode.
	 **/
	mpFp = x86Mp::findMpFp();
	if (mpFp != __KNULL
		&& __KFLAG_TEST(mpFp->features[1], x86_MPFP_FEAT1_FLAG_PICMODE))
	{
		ibmPcState.smpInfo.chipsetOriginalState = SMPSTATE_UNIPROCESSOR;

		cpuControl::safeDisableInterrupts(&cpuFlags);
		io::write8(0x22, 0x70);
		io::write8(0x23, 0x1);
		cpuControl::safeEnableInterrupts(cpuFlags);
		__kprintf(NOTICE CPUMOD"setSmpMode: chipset was in PIC mode.\n");
	}
	else
	{
		ibmPcState.smpInfo.chipsetOriginalState = SMPSTATE_SMP;
		__kprintf(NOTICE CPUMOD"setSmpMode: CPU Mod: chipset was in "
			"Virtual wire mode.\n");
	};
	ibmPcState.smpInfo.chipsetState = SMPSTATE_SMP;
	x86IoApic::initializeCache();
	if (!x86IoApic::ioApicsAreDetected())
	{
		ret = x86IoApic::detectIoApics();
		if (ret != ERROR_SUCCESS) { return ret; };
	};

	return ERROR_SUCCESS;
}

error_t ibmPc_cpuMod_powerControl(cpu_t cpuId, ubit8 command, uarch_t)
{
	error_t		ret;
	ubit8		nTries, isNewerCpu=1;
	void		*handle=0;
	uarch_t		pos=0, sipiVector;
	x86_mpCfgCpuS	*cpu;

	/**	EXPLANATION:
	 * According to the MP specification, only newer LAPICs require the
	 * INIT-SIPI-SIPI sequence; older LAPICs will be sufficient with a
	 * single INIT.
	 *
	 * The kernel therefore postulates the following when waking CPUs:
	 *	1. The only way to know what type of LAPIC exists on a CPU
	 *	   without having already awakened that CPU is for the chipset
	 *	   to tell the kernel (through MP tables).
	 *	2. If there are no MP tables, then the chipset is new enough
	 *	   to not be using older CPUs.
	 *
	 * This means that if the kernel checks for MP tables and finds none,
	 * then it will assume a newer CPU is being awakened and send the
	 * INIT-SIPI-SIPI sequence. Otherwise, it will check the MP tables and
	 * see if the CPU in question has an integrated or external LAPIC and
	 * decide whether or not to use SIPI-SIPI after the INIT.
	 **/
	sipiVector = (((uarch_t)&__kcpuPowerOnTextStart) >> 12) & 0xFF;

	// Scan MP tables.
	x86Mp::initializeCache();
	if (!x86Mp::mpConfigTableIsMapped())
	{
		if (x86Mp::findMpFp())
		{
			if (!x86Mp::mapMpConfigTable())
			{
				__kprintf(WARNING CPUMOD"cpu_powerOn(%d,%d): "
					"Unable to map MP config table.\n",
					cpuId, command);

				goto skipMpTables;
			};
		}
		else
		{
			__kprintf(NOTICE CPUMOD"cpu_powerOn(%d,%d): No MP "
				"tables; Assuming newer CPU and INIT-SIPI-SIPI."
				"\n", cpuId, command);

			goto skipMpTables;
		};
	};

	/* If CPU is not found in SMP tables, assume it is being hotplugged and
	 * the system admin who is inserting it is knowledgeable and won't
	 * hotplug an older CPU. I don't even think older CPUs support hotplug,
	 * but I'm sure specific chipsets may be custom built for all kinds of
	 * things.
	 **/
	cpu = x86Mp::getNextCpuEntry(&pos, &handle);
	for (; cpu != __KNULL; cpu = x86Mp::getNextCpuEntry(&pos, &handle))
	{
		if (cpu->lapicId != cpuId){ continue; };

		// Check for on-chip APIC.
		if (!__KFLAG_TEST(cpu->featureFlags, (1<<9)))
		{
			isNewerCpu = 0;
			__kprintf(WARNING CPUMOD"cpu_powerOn(%d,%d): Old non-"
				"integrated LAPIC.\n",
				cpuId, command);
		}
	};

skipMpTables:
	x86Lapic::initializeCache(); 

	switch (command)
	{
	case CPUSTREAM_POWER_ON:
		if (ibmPcState.smpInfo.chipsetState == SMPSTATE_UNIPROCESSOR)
		{
			ret = ibmPc_cpuMod_setSmpMode();
			if (ret != ERROR_SUCCESS) { return ret; };
		};

		nTries = 3;
		do
		{
			// Init IPI: Always with vector = 0.
			ret = x86Lapic::ipi::sendPhysicalIpi(
				x86LAPIC_IPI_TYPE_INIT,
				0,
				x86LAPIC_IPI_SHORTDEST_NONE,
				cpuId);

			if (ret != ERROR_SUCCESS)
			{
				__kprintf(ERROR CPUMOD"POWER_ON CPU %d: INIT "
					"IPI timed out.\n",
					cpuId);
			};
		} while (ret != ERROR_SUCCESS && --nTries);

		for (ubit32 i=10000; i>0; i--) { cpuControl::subZero(); };

		/* The next two IPIs should only be executed if the LAPIC
		 * is version 1.x. Older LAPICs only need an INIT.
		 **/
		if (isNewerCpu)
		{
			nTries = 3;
			do
			{
				ret = x86Lapic::ipi::sendPhysicalIpi(
					x86LAPIC_IPI_TYPE_SIPI,
					sipiVector,
					x86LAPIC_IPI_SHORTDEST_NONE,
					cpuId);

				if (ret != ERROR_SUCCESS)
				{
					__kprintf(ERROR CPUMOD"POWER_ON CPU "
						"%d: SIPI0 timed out.\n",
						cpuId);
				};
			} while (ret != ERROR_SUCCESS && --nTries);

			for (ubit32 i=10000 * 2; i>0; i--) {
				cpuControl::subZero();
			};

			nTries = 3;
			do
			{
				ret = x86Lapic::ipi::sendPhysicalIpi(
					x86LAPIC_IPI_TYPE_SIPI,
					sipiVector,
					x86LAPIC_IPI_SHORTDEST_NONE,
					cpuId);

				if (ret != ERROR_SUCCESS)
				{
					__kprintf(ERROR CPUMOD"POWER_ON CPU "
						"%d: SIPI1 timed out.\n",
						cpuId);
				};
			} while (ret != ERROR_SUCCESS && --nTries);

			for (ubit32 i=10000 * 2; i>0; i--) {
				cpuControl::subZero();
			};
		};
		break;

	default: ret = ERROR_SUCCESS;
	};

	return ret;
}

