
#include <arch/x8632/cpuid.h>
#include <arch/x8632/gdt.h>
#include <arch/x8632/cpuEnumeration.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <commonlibs/libacpi/libacpi.h>
#include <kernel/common/panic.h>
#include <kernel/common/cpuTrib/cpuStream.h>
#include <kernel/common/cpuTrib/chipsetSmpMode.h>
#include <__kthreads/__kcpuPowerOn.h>


struct x86ManufacturerEntryS
{
	utf8Char	*name;
	status_t	(*func)(void);
};

// XXX: Always ensure that this array is terminated with a null entry.
static x86ManufacturerEntryS		x86Manufacturers[] =
{
	{CC"GenuineIntel", &x86CpuEnumeration::intel},
	{CC"AuthenticAMD", &x86CpuEnumeration::amd},
	{CC"AMDisbetter!", &x86CpuEnumeration::amd},
	{__KNULL, __KNULL}
/*	{CC"CyrixInstead"},
	{CC"TransmetaCPU"},
	{CC"CentaurHauls"},
	{CC"GenuineTMx86"},
	{CC"CentaurHauls"},
	{CC"NexGenDriven"},
	{CC"UMC UMC UMC "},
	{CC"SiS SiS SiS "},
	{CC"Geode by NSC"},
	{CC"RiseRiseRise"} */
};


void cpuStreamC::baseInit(void)
{
	// Load DR0 with a pointer to this CPU's CPU Stream.
	asm volatile (
		"pushl	%%eax \n\t \
		movl	%%dr7, %%eax \n\t \
		andl	$0xFFFFFE00, %%eax \n\t \
		movl	%%eax, %%dr7 \n\t \
		popl	%%eax \n\t \
		\
		movl	%0, %%dr0 \n\t"
		:
		: "r" (this));

	if (!__KFLAG_TEST(flags, CPUSTREAM_FLAGS_BSP))
	{
		// Load kernel's main GDT:
		asm volatile ("lgdt	(x8632GdtPtr)");
		// Load the kernel's IDT:
		asm volatile ("lidt	(x8632IdtPtr)");
	};

	// Load the __kcpuPowerOnThread into the currentTask holder.
	taskStream.currentTask = &__kcpuPowerOnThread;
}

// Parses a MADT for NMI entries to be used to setup the LAPIC LINT inputs.
static void lintParseMadtForEntries(acpi_rMadtS *rmadt, cpuStreamC *caller)
{
	void			*handle;
	acpi_rMadtLapicNmiS	*nmiEntry;

	handle = __KNULL;
	nmiEntry = acpiRMadt::getNextLapicNmiEntry(rmadt, &handle);
	while (nmiEntry != __KNULL)
	{
		// If this entry pertains to this CPU:
		if (nmiEntry->acpiLapicId == caller->cpuAcpiId
			|| nmiEntry->acpiLapicId == ACPI_MADT_NMI_LAPICID_ALL)
		{
			__kprintf(NOTICE CPUSTREAM"%d: ACPI NMI: lint %d.\n",
				caller->cpuId, nmiEntry->lapicLint);

			x86Lapic::lintSetup(
				nmiEntry->lapicLint,
				x86LAPIC_LINT_TYPE_NMI,
				x86Lapic::lintConvertAcpiFlags(
					nmiEntry->flags),
				0);

			x86Lapic::lintEnable(nmiEntry->lapicLint);
		};

		nmiEntry = acpiRMadt::getNextLapicNmiEntry(rmadt, &handle);
	};
}

static error_t initializeLapic(cpuStreamC *caller)
{
	uarch_t				pos=0;
	void				*handle, *context;
	x86_mpCfgLocalIrqSourceS	*lintEntry;
	acpi_rsdtS			*rsdt;
	acpi_rMadtS			*rmadt;

	/**	EXPLANATION:
	 * Basically, ensure that the LAPIC is in a sane, predefined state
	 * so that it can handle interrupts, exceptions, IPIs, NMIs, and provide
	 * a scheduler timer.
	 *
	 * For the LAPIC's LINT inputs, we search BOTH tables (MP and ACPI)
	 * simply because I have found real hardware cases where there is an
	 * MP config table that says that there are extINT inputs on LAPICs.
	 * At the same time, ACPI doesn't give information about extINT, and
	 * only tells you about NMI. So if we just take ACPI only and ignore
	 * MP tables if ACPI was found, we may be missing out on the extINT
	 * programming information.
	 *
	 * Technically though, extINT doesn't matter when the chipset is in
	 * SMP mode: Symm. IO mode uses the IO APICs and not any extINT
	 * compatible PIC, so an extINT should never hit any CPU. Regardless,
	 * there's no harm in being thorough.
	 *
	 *	NOTE:
	 * Intel manual, Vol3A, Section 10.4.7.2: State of the LAPIC while it is
	 * temporarily disabled:
	 *	* LAPIC will respond normally to INIT, NMI, SMI and SIPI.
	 *	* Pending INTs will require handling and are held until cleared.
	 *	* The LAPIC can still be used safely to issue IPIs.
	 *	* Attempts to set or modify the LVT registers will be ignored.
	 *	* LAPIC continues to monitor the bus to keep synchronized.
	 *
	 * In particular, the 3rd and 4th ones are of interest: The BSP CPU
	 * sends out IPIs to APs in the order that the APs appear in the MP/ACPI
	 * tables. Even though the ACPI/MP tables require that the BSP appear
	 * as the first entry, there is no guarantee this will happen, so the
	 * BSP may send out IPIs before it itself is given a chance to execute
	 * this code which sets up its LAPIC.
	 *
	 * According to the 3rd provision then, this is still safe, even if the
	 * BSP's LAPIC was in soft-disable mode. The 4th point is of interest
	 * simply because it implies that we should explicitly ensure that the
	 * LAPIC is soft-enabled before trying to set up the rest of the LAPIC
	 * operating state.
	 **/
	
	x86Lapic::initializeCache();
	if (!x86Lapic::lapicMemIsMapped())
	{
		panic(FATAL CPUSTREAM"%d: setupLapic(): Somehow the LAPIC "
			"lib's cached vaddr mapping was corrupted.\n",
			caller->cpuId);
	};

	x86Lapic::setupSpuriousVector(0xFF);
	// See above, explicitly enable LAPIC before fiddling with LVT regs.
	x86Lapic::softEnable();
	// Use vector 0xFF as the LAPIC error vector.
	x86Lapic::setupLvtError(0xFE);
	x86Lapic::ipi::installHandler();

	// This is called only when the CPU is ready to take IPIs.
	caller->interCpuMessager.initialize();
if (__KFLAG_TEST(caller->flags, CPUSTREAM_FLAGS_BSP)) { for (__kprintf(NOTICE CPUSTREAM"%d: Reached HLT.\n", caller->cpuId);;) { asm volatile("hlt\n\t"); }; };

	// First print out the LAPIC Int assignment entries.
	x86Mp::initializeCache();
	if (x86Mp::findMpFp() != __KNULL)
	{
		if (x86Mp::mapMpConfigTable() != __KNULL)
		{
			lintEntry = x86Mp::getNextLocalIrqSourceEntry(
				&pos, &handle);

			while (lintEntry != __KNULL)
			{
				// If the entry pertains to this CPU:
				if (lintEntry->destLapicId == caller->cpuId
					|| lintEntry->destLapicId
						== x86_MPCFG_LIRQSRC_DEST_ALL)
				{
					__kprintf(NOTICE CPUSTREAM"%d: MPCFG: "
						"lint %d, type %d.\n",
						caller->cpuId,
						lintEntry->destLapicLint,
						lintEntry->intType);

					x86Lapic::lintSetup(
						lintEntry->destLapicLint,
						x86Lapic::lintConvertMpCfgType(
							lintEntry->intType),
						x86Lapic::lintConvertMpCfgFlags(
							lintEntry->flags),
						0);

					x86Lapic::lintEnable(
						lintEntry->destLapicLint);
				};

				lintEntry = x86Mp::getNextLocalIrqSourceEntry(
					&pos, &handle);
			};
		}
		else {
			__kprintf(ERROR"Failed to map in MP CFG table.\n");
		};
	};

	acpi::initializeCache();
	if (acpi::findRsdp() == ERROR_SUCCESS)
	{
		if (acpi::mapRsdt() == ERROR_SUCCESS)
		{
			rsdt = acpi::getRsdt();

			context = handle = __KNULL;
			rmadt = acpiRsdt::getNextMadt(rsdt, &context, &handle);
			while (rmadt != __KNULL)
			{
				lintParseMadtForEntries(rmadt, caller);

				acpiRsdt::destroySdt((acpi_sdtS *)rmadt);
				rmadt = acpiRsdt::getNextMadt(
					rsdt, &context, &handle);
			};
		}
		else
		{
			__kprintf(ERROR CPUSTREAM"%d: Unable to map RSDT.\n",
				caller->cpuId);
		};
	};

	/**	NOTE:
	 * At this point, the LINT pins on the CPU should be set up. Now we set
	 * up the LVT entries, calibrate the timer, setup vectors for spurious
	 * irq, LAPIC error, IPI and LAPIC timer.
	 **/
	return ERROR_SUCCESS;
}

error_t cpuStreamC::initialize(void)
{
	// Init LAPIC (if SMP mode).
	if (usingChipsetSmpMode()) {
		initializeLapic(this);
	};

#if 0
	// Enumerate CPU and features.
	enumerate();
	__kprintf(NOTICE CPUSTREAM"%d: CPU model detected as %s.\n",
		cpuId, cpuFeatures.cpuName);
#endif

	__kprintf(NOTICE CPUSTREAM"%d: Initialize() complete.\n", cpuId);
	return ERROR_SUCCESS;
}

status_t cpuStreamC::enumerate(void)
{
	ubit32			eax, ebx, ecx, edx;
	utf8Char		cpuSignature[13];

	// Find out which brand of x86 CPU this is.
	execCpuid(0x0, &eax, &ebx, &ecx, &edx);
	strncpy8(&cpuSignature[0], reinterpret_cast<utf8Char *>( &ebx ), 4);
	strncpy8(&cpuSignature[4], reinterpret_cast<utf8Char *>( &edx ), 4);
	strncpy8(&cpuSignature[8], reinterpret_cast<utf8Char *>( &ecx ), 4);
	cpuSignature[12] = '\0';

	for (uarch_t i=0; x86Manufacturers[i].name != __KNULL; i++)
	{
		if (!strcmp8(cpuSignature, x86Manufacturers[i].name)) {
			return (*x86Manufacturers[i].func)();
		};
	};

	__kprintf(ERROR CPUSTREAM"%d: Unable to determine CPU brand.\n", cpuId);
	return CPUENUM_UNSUPPORTED_CPU;
}

