
#include <arch/x8632/cpuid.h>
#include <arch/x8632/cpuEnumeration.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


#define AMDENUM		"AMD Cpu Enum: "

#define AMDENUM_SIG_GET_FAMILY(_sig)			\
	(AMDENUM_SIG_GET_FAMILY_LOW(_sig)+AMDENUM_SIG_GET_FAMILY_HIGH(_sig))

#define AMDENUM_SIG_GET_FAMILY_HIGH(_sig)		(((_sig) >> 20) & 0xFF)
#define AMDENUM_SIG_GET_FAMILY_LOW(_sig)		(((_sig) >> 8) & 0xF)

#define AMDENUM_SIG_GET_MODEL(_sig)			\
	((EMDENUM_SIG_GET_MODEL_HIGH(_sig)<<4)|AMDENUM_SIG_GET_MODEL_LOW(_sig))

#define AMDENUM_SIG_GET_MODEL_HIGH(_sig)		(((_sig) >> 16) & 0xF)
#define AMDENUM_SIG_GET_MODEL_LOW(_sig)			(((_sig) >> 4) & 0xF)

#define AMDENUM_SIG_GET_REV(_sig)			((_sig) & 0xF)

/*
static utf8Char		*amdFamily0FCpus[] =
{
	__KNULL
};

static utf8Char		*amdFamily10Cpus[] =
{
	__KNULL
};

static utf8Char		*amdFamily11Cpus[] =
{
	__KNULL
};

static utf8Char		*amdFamily12Cpus[] =
{
	__KNULL
};

static utf8Char		*amdFamily14Cpus[] =
{
	__KNULL
};

static utf8Char		*amdFamily15Cpus[] =
{
	__KNULL
};
*/

static utf8Char		*unknownCpu = CC"Unknown AMD Processor";

static status_t amdBrandStringEnum(void)
{
	ubit32		eax, ebx, ecx, edx, cpuIdIndex=0x80000002;
	utf8Char	*brandString;

	brandString = new utf8Char[48];
	if (brandString == __KNULL)
	{
		__kprintf(ERROR AMDENUM"Unable to alloc mem brandstring.\n");
		return CPUENUM_CPU_MODEL_UNKNOWN;
	};

	for (ubit32 i=0; i<3; i++, cpuIdIndex++)
	{
		execCpuid(cpuIdIndex, &eax, &ebx, &ecx, &edx);
		strncpy8(&brandString[i * 16], CC(&eax), 4);
		strncpy8(&brandString[(i + 16) + 4], CC(&ebx), 4);
		strncpy8(&brandString[(i + 16) + 8], CC(&ecx), 4);
		strncpy8(&brandString[(i + 16) + 12], CC(&edx), 4);
	};

	cpuTrib.getCurrentCpuStream()
		->cpuFeatures.archFeatures.cpuNameNSpaces = 0;

	for (ubit32 i=0; brandString[i] == ' '; i++)
	{
		cpuTrib.getCurrentCpuStream()
			->cpuFeatures.archFeatures.cpuNameNSpaces++;
	};

	cpuTrib.getCurrentCpuStream()->cpuFeatures.cpuName = &brandString[
		cpuTrib.getCurrentCpuStream()
			->cpuFeatures.archFeatures.cpuNameNSpaces];

	return ERROR_SUCCESS;
}

static status_t amdSignatureEnum(void)
{
	ubit32		eax, ebx, ecx, edx;

	execCpuid(0x1, &eax, &ebx, &ecx, &edx);

	if (AMDENUM_SIG_GET_FAMILY_LOW(eax) < 0xF)
	{
		// Use only low values. Reusing ebx and ecx to save stack space.
		ebx = AMDENUM_SIG_GET_FAMILY_LOW(eax);
		ecx = AMDENUM_SIG_GET_MODEL_LOW(eax);
	}
	else
	{
		// Use full values with extended bits.
		ebx = AMDENUM_SIG_GET_FAMILY(eax);
		ecx = AMDENUM_SIG_GET_FAMILY(ebx);
	};

	edx = AMDENUM_SIG_GET_REV(eax);
	// For now just display the values you've got. Debug on real hardware.
	__kprintf(NOTICE AMDENUM"Family low: 0x%x.\nTaken family value: 0x%x."
		"Taken model value: 0x%x.\nTaken revision: 0x%x.\n",
		AMDENUM_SIG_GET_FAMILY_LOW(eax),
		ebx, ecx, edx);

	cpuTrib.getCurrentCpuStream()->cpuFeatures.cpuName = unknownCpu;
	return ERROR_SUCCESS;
}

status_t x86CpuEnumeration::amd(void)
{
	ubit32		eax, ebx, ecx, edx;

	// Check for brand string.
	execCpuid(0x80000000, &eax, &ebx, &ecx, &edx);
	if (eax >= 0x80000004)
	{
		__kprintf(NOTICE AMDENUM"CPUID brand string supported.\n");
		if (amdBrandStringEnum() == ERROR_SUCCESS) {
__kprintf(NOTICE AMDENUM"CPU identified as %s.\n", cpuTrib.getCurrentCpuStream()->cpuFeatures.cpuName);
			return ERROR_SUCCESS;
		};
	};

	// Check for signature feature support.
	execCpuid(0x1, &eax, &ebx, &ecx, &edx);
	if (eax >= 0x1)
	{
		__kprintf(NOTICE AMDENUM"CPU signature supported.\n");
		if (amdSignatureEnum() == ERROR_SUCCESS) {
__kprintf(NOTICE AMDENUM"CPU identified as %s.\n", cpuTrib.getCurrentCpuStream()->cpuFeatures.cpuName);
			return ERROR_SUCCESS;
		};
	};

__kprintf(NOTICE AMDENUM"Support for AMD Brand ID and Signature CPUID identification is currently unimplemented.\n");
	cpuTrib.getCurrentCpuStream()->cpuFeatures.cpuName = unknownCpu;
	return CPUENUM_CPU_MODEL_UNKNOWN;
}

