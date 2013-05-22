
#include <__ksymbols.h>
#include <arch/arch.h>
#include <arch/memory.h>
#include <arch/io.h>
#include <chipset/memoryAreas.h>
#include <chipset/zkcm/zkcmCore.h>
#include <platform/cpu.h>
#include <arch/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/libacpi.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <kernel/common/panic.h>
#include <kernel/common/cpuTrib/cpuStream.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <__kthreads/__kcpuPowerOn.h>

#include "i8254.h"
#include "i8259a.h"
#include "zkcmIbmPcState.h"


#define SMPINFO		CPUMOD"SMP: "
#define CPUMOD		"CPU Mod: "


error_t zkcmCpuDetectionModC::initialize(void)
{
	return ERROR_SUCCESS;
}

error_t zkcmCpuDetectionModC::shutdown(void)
{
	return ERROR_SUCCESS;
}

error_t zkcmCpuDetectionModC::suspend(void)
{
	return ERROR_SUCCESS;
}

error_t zkcmCpuDetectionModC::restore(void)
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

zkcmNumaMapS *zkcmCpuDetectionModC::getNumaMap(void)
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

zkcmSmpMapS *zkcmCpuDetectionModC::getSmpMap(void)
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

sarch_t zkcmCpuDetectionModC::checkSmpSanity(void)
{
	acpi_rsdtS		*rsdt;
	acpi_rMadtS		*madt;
	x86_mpCfgIrqSourceS	*mpCfgIrqSource;
	void			*context, *handle, *handle2;
	uarch_t			pos;
	sarch_t			haveMpTableIrqSources=0, haveAcpiIrqSources=0,
				haveMpTableIoApics=0, haveAcpiIoApics=0;
	x86LapicC		*lapic;

	/**	EXPLANATION:
	 * This function is supposed to be called during the kernel's initial
	 * CPU detection stretch. It does the most basic checks to ensure
	 * that the kernel can successfully run SMP on the chipset.
	 *
	 * These checks include:
	 *	* Check for presence of a LAPIC on the BSP.
	 *	* Check for the presence of at least one IO-APIC.
	 *	* Check for the presence of either the ACPI IRQ routing
	 *	  information, or the MP tables' equivalent. Without this
	 *	  bus-device-pin mapping for use with the IO-APICs, we will have
	 *	  no idea how IRQs are wired from devices to the IO-APICs.
	 *
	 *	  For ISA devices, if there is no such information, we can rely
	 *	  on there being a 1:1 mapping between the ISA i8259 pins and
	 *	  the first 16 IO-APIC pins, but beyond that, for devices on
	 *	  any other bus, we have zero information.
	 **/
	lapic = &cpuTrib.getCurrentCpuStream()->lapic;
	if (!lapic->cpuHasLapic())
	{
		__kprintf(ERROR CPUMOD"checkSmpSanity(): BSP has no LAPIC.\n");
		return 0;
	};

	/**	EXPLANATION:
	 * We also check for the existence of either the ACPI MADT, or the MP
	 * tables, and print a warning to the log if neither exists, but return
	 * positive still.
	 *
	 * The chipset may be hot-plug enabled, with only one CPU plugged in
	 * during boot.
	 **/
	acpi::initializeCache();
	if (acpi::findRsdp() != ERROR_SUCCESS)
	{
		__kprintf(WARNING CPUMOD"checkSmpSanity(): No ACPI tables.\n");
		goto checkForMpTables;
	};

#if (!defined(__32_BIT__)) || (defined(CONFIG_ARCH_x86_32_PAE))
	// XSDT code here, fallback to RSDT if XSDT gives problems.
#endif
	if (acpi::testForRsdt())
	{
		if (acpi::mapRsdt() != ERROR_SUCCESS)
		{
			__kprintf(WARNING CPUMOD"checkSmpSanity: Failed to map "
				"RSDT.\n");

			goto checkForMpTables;
		};

		// Now check for the presence of an MADT, and warn if none.
		rsdt = acpi::getRsdt();
		handle = context = __KNULL;
		madt = acpiRsdt::getNextMadt(rsdt, &context, &handle);
		// Warn if no MADT.
		if (madt == __KNULL) {
			__kprintf(WARNING CPUMOD"checkSmpSanity: No MADT.\n");
		}
		else
		{
			// Check for at least one IO-APIC entry in the MADT.
			handle2 = __KNULL;
			if (acpiRMadt::getNextIoApicEntry(madt, &handle2)
				!= __KNULL)
			{
				haveAcpiIoApics = 1;
			}
			else
			{
				__kprintf(WARNING CPUMOD"checkSmpSanity: No "
					"MADT IO-APIC Entries.\n");
			};
		};

		acpiRsdt::destroySdt(reinterpret_cast<acpi_sdtS *>( madt ));
		acpiRsdt::destroyContext(&context);
	};

checkForMpTables:
	x86Mp::initializeCache();
	if (x86Mp::findMpFp() == __KNULL) {
		__kprintf(WARNING CPUMOD"checkSmpSanity(): No MP tables.\n");
	};

	// Now check for at least one MP table platform interrupt source entry.
	handle = __KNULL;
	pos = 0;
	mpCfgIrqSource = x86Mp::getNextIrqSourceEntry(&pos, &handle);
	if (mpCfgIrqSource != __KNULL) {
		haveMpTableIrqSources = 1;
	}
	else
	{
		__kprintf(WARNING CPUMOD"checkSmpSanity: No MP table IRQ "
			"source entries.\n");
	};

	if (x86Mp::getNextIoApicEntry(&pos, &handle) != __KNULL) {
		haveMpTableIoApics = 1;
	}
	else
	{
		__kprintf(WARNING CPUMOD"checkSmpSanity: No MP Table IO-APIC "
			"entries.\n");
	};

	/**	TODO:
	 * When we port ACPICA, check for the chipset IRQ routing info.
	 * For now since we don't have ACPICA parsing, if we didn't find an MP
	 * table with IRQ source information, we will have to settle for
	 * warning the user, and continuing in spite of that.
	 *
	 * Ideally, we should return false and prevent the kernel from switching
	 * the chipset to SMP mode, but for the sake of being able to make
	 * progress in development until we have ACPICA, we allow the kernel to
	 * continue on with switching to SMP mode.
	 **/
	if (!haveMpTableIrqSources && !haveAcpiIrqSources)
	{
		__kprintf(ERROR CPUMOD"checkSmpSanity: No MP table IRQ "
			"sources, and no ACPICA ported yet.\n");

		// Should eventually "return 0" here when ACPICA is ported.
	};

	if (!haveMpTableIoApics && !haveAcpiIoApics)
	{
		__kprintf(ERROR CPUMOD"checkSmpSanity: Anomally:\n\t"
			"Your kernel was configured for multiprocessor "
			"operation\n\tbut it has no IO-APICs. The kernel must "
			"refuse MP operation\n\ton this board.\n");

		return 0;
	};

	return 1;
}

cpu_t zkcmCpuDetectionModC::getBspId(void)
{
	x86LapicC	*lapic;

	lapic = &cpuTrib.getCurrentCpuStream()->lapic;
	if (!ibmPcState.bspInfo.bspIdRequestedAlready)
	{
		// We expect to be executing on the BSP, or this won't work.
		if (!lapic->cpuHasLapic())
		{
			__kprintf(NOTICE CPUMOD"getBspId: BSP CPU has no "
				"LAPIC. Assigning fake ID of 0.\n");

			ibmPcState.bspInfo.bspId = (cpu_t)0;
			ibmPcState.bspInfo.bspIdRequestedAlready = 1;
			return ibmPcState.bspInfo.bspId;
		};

		if (!x86LapicC::lapicMemIsMapped())
		{
			// Not safe, but detectPaddr() never returns error.
			if (x86LapicC::detectPaddr() != ERROR_SUCCESS) {
				return 0;
			};

			if (x86LapicC::mapLapicMem() != ERROR_SUCCESS)
			{
				panic(ERROR CPUMOD"getBspId: Failed to map LAPIC "
					"into kernel vaddrspace.\n");
			};
		};

		ibmPcState.bspInfo.bspId = lapic->read32(
			x86LAPIC_REG_LAPICID);

		ibmPcState.bspInfo.bspId >>= 24;
		ibmPcState.bspInfo.bspIdRequestedAlready = 1;
	};

	return ibmPcState.bspInfo.bspId;
}

error_t zkcmCpuDetectionModC::setSmpMode(void)
{
	error_t		ret;
	ubit8		*lowmem;
	void		*srcAddr, *destAddr;
	uarch_t		copySize, cpuFlags;
	x86_mpFpS	*mpFp;
	ubit8		i8254WasEnabled=0, i8254WasSoftEnabled=0;

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

	x86IoApic::initializeCache();
	if (!x86IoApic::ioApicsAreDetected())
	{
		ret = x86IoApic::detectIoApics();
		if (ret != ERROR_SUCCESS) { return ret; };
	};

	/* The i8254 is the only device which should be operational right
	 * now. Disable it to force it to forget its __kpin assignment, then
	 * mask all of the i8259 PIC pins.
	 **/
	if (i8254Pit.isEnabled()) { i8254WasEnabled = 1; };
	if (i8254Pit.isSoftEnabled()) { i8254WasSoftEnabled = 1; };
	__kprintf(NOTICE CPUMOD"setSmpMode: i8254: wasEnabled=%d, "
		"wasSoftEnabled=%d.\n",
		i8254WasEnabled, i8254WasSoftEnabled);

	i8254Pit.setSmpModeSwitchFlag(
		cpuTrib.getCurrentCpuStream()
			->taskStream.getCurrentTask()->getFullId());

	__kprintf(NOTICE CPUMOD"setSmpMode: Waiting for devices to disable.\n");
	if (i8254WasEnabled)
	{
		i8254Pit.disable();
		taskTrib.block();
	};

	i8254Pit.unsetSmpModeSwitchFlag();

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

		// Enable Symm. I/O mode using IMCR.
		cpuControl::safeDisableInterrupts(&cpuFlags);
		io::write8(0x22, 0x70);
		io::write8(0x23, 0x1);
		cpuControl::safeEnableInterrupts(cpuFlags);
		__kprintf(NOTICE CPUMOD"setSmpMode: chipset was in PIC "
			"mode.\n");
	}
	else
	{
		ibmPcState.smpInfo.chipsetOriginalState = SMPSTATE_SMP;
		__kprintf(NOTICE CPUMOD"setSmpMode: chipset was in Virtual "
			"wire mode.\n");
	};

	ibmPcState.smpInfo.chipsetState = SMPSTATE_SMP;
	assert_fatal(
		zkcmCore.irqControl.bpm.loadBusPinMappings(CC"isa")
			== ERROR_SUCCESS);

	__kprintf(NOTICE CPUMOD"setSmpMode: Re-enabling devices.\n");
	if (i8254WasEnabled)
	{
		ret = i8254Pit.enable();
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR CPUMOD"setSmpMode: i8254 enable() "
				"failed.\n");

			return ret;
		};

		if (!i8254WasSoftEnabled) { i8254Pit.softDisable(); };
	};

	zkcmCore.irqControl.chipsetEventNotification(
		IRQCTL_EVENT_SMP_MODE_SWITCH, 0);

	return ERROR_SUCCESS;
}

