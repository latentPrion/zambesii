
#include <arch/x8632/cpuid.h>
#include <arch/x8632/gdt.h>
#include <arch/x8632/cpuEnumeration.h>
#include <arch/cpuControl.h>
#include <arch/debug.h>
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libx86mp/libx86mp.h>
#include <commonlibs/libacpi/libacpi.h>
#include <kernel/common/panic.h>
#include <kernel/common/cpuTrib/cpuStream.h>
#include <kernel/common/cpuTrib/chipsetSmpMode.h>
#include <kernel/common/processTrib/processTrib.h>
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
	{NULL, NULL}
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

void cpuStream::baseInit(void)
{
	debug::disableDebugExtensions();

	// Load DR0 with a pointer to this CPU's CPU Stream.
	asm volatile (
		"pushl	%%eax \n\t \
		movl	%%dr7, %%eax \n\t \
		andl	$0xFFFFFC00, %%eax \n\t \
		movl	%%eax, %%dr7 \n\t \
		popl	%%eax \n\t \
		\
		movl	%0, %%dr0 \n\t"
		:
		: "r" (this));

	if (!FLAG_TEST(flags, CPUSTREAM_FLAGS_BSP))
	{
		// Load kernel's main GDT:
		asm volatile ("lgdt	(x8632GdtPtr)");
		// Load the kernel's IDT:
		asm volatile ("lidt	(x8632IdtPtr)");

		// Load the __kcpuPowerOnThread into the currentTask holder.
		taskStream.currentTask = &__kcpuPowerOnThread;
	};
}

error_t cpuStream::bind(void)
{
	error_t		ret;

	// Init LAPIC (if SMP mode). Check for LAPIC first in case it's the BSP.
	if (lapic.cpuHasLapic())
	{
		ret = lapic.setupLapic();
		if (ret != ERROR_SUCCESS)
		{
			printf(ERROR CPUSTREAM"%d: bind: LAPIC init failed."
				"\n", cpuId);

			return ret;
		};
	};

	cpuTrib.availableCpus.setSingle(cpuId);

	// Open the floodgates.
	cpuControl::enableInterrupts();

	// Enumerate CPU and features.
	enumerate();
	printf(NOTICE CPUSTREAM"%d: CPU model detected as %s.\n",
		cpuId, cpuFeatures.cpuModel);

	return ERROR_SUCCESS;
}

status_t cpuStream::enumerate(void)
{
	ubit32			eax, ebx, ecx, edx;
	utf8Char		cpuSignature[13];

	// Find out which brand of x86 CPU this is.
	execCpuid(0x0, &eax, &ebx, &ecx, &edx);
	strncpy8(&cpuSignature[0], reinterpret_cast<utf8Char *>( &ebx ), 4);
	strncpy8(&cpuSignature[4], reinterpret_cast<utf8Char *>( &edx ), 4);
	strncpy8(&cpuSignature[8], reinterpret_cast<utf8Char *>( &ecx ), 4);
	cpuSignature[12] = '\0';

	for (uarch_t i=0; x86Manufacturers[i].name != NULL; i++)
	{
		if (!strcmp8(cpuSignature, x86Manufacturers[i].name)) {
			return (*x86Manufacturers[i].func)();
		};
	};

	printf(ERROR CPUSTREAM"%d: Unable to determine CPU brand.\n", cpuId);
	return CPUENUM_UNSUPPORTED_CPU;
}

