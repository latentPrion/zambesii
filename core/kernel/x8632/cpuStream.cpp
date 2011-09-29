
#include <arch/x8632/cpuid.h>
#include <arch/x8632/gdt.h>
#include <arch/x8632/cpuEnumeration.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuStream.h>
#include <__kthreads/__kcpuPowerOn.h>


struct x86ManufacturerEntryS
{
	utf8Char	*name;
	status_t	(*func)(void);
};

// Terminate this array with a null entry.
x86ManufacturerEntryS		x86Manufacturers[] =
{
	{CC"GenuineIntel", &x86CpuEnumeration::intel},
	{CC"AuthenticAMD", &x86CpuEnumeration::amd},
	{CC"AMDisbetter!", &x86CpuEnumeration::amd},
	{__KNULL, __KNULL}
/*	{CC"CentaurHauls"},
	{CC"TransmetaCPU"},
	{CC"GenuineTMx86"},
	{CC"CyrixInstead"},
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

	// Load the __kcpuPowerOnThread into the currentTask holder.
	currentTask = &__kcpuPowerOnThread;
}

error_t cpuStreamC::initialize(void)
{
	if (!__KFLAG_TEST(flags, CPUSTREAM_FLAGS_BSP))
	{
		// Load kernel's main GDT:
		asm volatile ("lgdt	(x8632GdtPtr)");
		// Load the kernel's IDT:
		asm volatile ("lidt	(x8632IdtPtr)");
	};

	// Enumerate CPU and features.
	enumerate();
	// Init LAPIC (if SMP mode).
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

