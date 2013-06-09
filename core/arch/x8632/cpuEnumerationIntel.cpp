
#include <arch/x8632/cpuid.h>
#include <arch/x8632/cpuEnumeration.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


#define INTELENUM		"Intel CPU Enum: "

#define x86CPUENUM_SIGNATURE_GET_TYPE(_sig)		(((_sig) >> 12) & 0x3)
#define x86CPUENUM_SIGNATURE_GET_FULL_FAMILY(_sig)		\
	((((_sig) >> 20) & 0xFF) + (((_sig) >> 8) & 0xF))

#define x86CPUENUM_SIGNATURE_GET_EXT_MODEL(_sig)	(((_sig) >> 16) & 0xF)
#define x86CPUENUM_SIGNATURE_GET_LOW_MODEL(_sig)	(((_sig) >> 4) & 0xF)
#define x86CPUENUM_SIGNATURE_GET_STEPPING(_sig)		((_sig) & 0xF)

#define x86CPUENUM_SIGNATURE_TYPE_OEM			0x0
#define x86CPUENUM_SIGNATURE_TYPE_OVERDRIVE		0x1
#define x86CPUENUM_SIGNATURE_TYPE_DUALPROCESSOR		0x2

static utf8Char		*unknownCpu = CC"Unknown Intel x86 based CPU";

/**	EXPLANATION:
 * These arrays are for the CPUID CPU signature feature; Each index gives the 
 * name of a CPU which would correspond with that family and model combination.
 * Some of the entries are "NULL". This is because those entries were undefined
 * in the intel CPUID specification.
 *
 * I hope the kernel never encounters such a CPU, but if it does I can print
 * an error message and warn the user that their CPU may be a dud, and then
 * probably just use the name of the CPU that preceded, or followed it.
 **/
utf8Char	*oemFamily4Signatures[] =
{
	CC"Intel 486 DX",
	CC"Intel 486 DX",
	CC"Intel 486 SX",
	CC"Intel 486 DX2",
	CC"Intel 486 SL",
	CC"Intel SX2",
	__KNULL,
	CC"Intel Enhanced DX2",
	CC"Intel DX4 series"
};

utf8Char	*oemFamily5Signatures[] =
{
	__KNULL,
	CC"Intel Pentium series",
	CC"Intel Pentium series",
	__KNULL,
	CC"Intel Pentium CPU with MMX"
};

utf8Char	*oemFamily6Signatures[] =
{
	__KNULL,
	CC"Intel Pentium Pro",
	__KNULL,
	CC"Intel Pentium 2",
	__KNULL,
	CC"Intel Pentium 2 Xeon",
	CC"Intel Celeron",
	CC"Intel Pentium 3",
	CC"Intel Pentium 3+",
	CC"Intel Pentium M",
	CC"Intel Pentium 3 Xeon",
	CC"Intel Pentium 3+",
	__KNULL,
	CC"Intel Pentium M",
	CC"Intel Core Solo/Duo series",
	CC"Intel Core2 Solo/Duo series",

	// Model > 0xF && extended model == 0x1
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,
	CC"Intel EP80579 series",
	CC"Intel Celeron (Model 0x16)",
	CC"Intel Core2 Extreme",
	__KNULL,
	__KNULL,
	CC"Intel Core i7",
	__KNULL,
	CC"Intel Atom",
	CC"Intel Xeon",
	CC"Intel Core i5/i7 or Intel Core i5/i7 mobile",

	// Model > 0xF && Extended model == 0x2
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,
	CC"Intel Core i3",
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,
	CC"Intel Core i7",
	__KNULL,
	CC"Intel Core i7",
	__KNULL,
	CC"Intel Xeon MP",
	CC"Intel Xeon MP"
};

utf8Char	*oemFamily15Signatures[] =
{
	CC"Intel Pentium 4 / Intel Xeon",
	CC"Intel Pentium 4 / Intel Xeon / Intel Xeon MP",
	CC"Mobile Intel Pentium 4",
	CC"Intel Pentium 4 / Intel Celeron D",
	CC"Intel Pentium 4 Extreme Edition",
	__KNULL,
	CC"Intel Pentium D"
};

utf8Char	*overdriveFamily4Signatures[] =
{
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,
	__KNULL,
	CC"Intel DX4 Overdrive"
};

utf8Char	*overdriveFamily5Signatures[] =
{
	__KNULL,
	CC"Pentium (i586) Overdrive",
	CC"Pentium (i586) Overdrive",
	CC"Pentium (i486) Overdrive",
	CC"Pentium Overdrive with MMX"
};

utf8Char	*overdriveFamily6Signatures[] =
{
	__KNULL,
	__KNULL,
	__KNULL,
	CC"Intel Pentium 2 Overdrive"
};

utf8Char	*brandIds[] =
{
	__KNULL,
	CC"Intel Celeron",
	CC"Intel Pentium 3",
	CC"Intel Pentium 3 Xeon",
	CC"Intel Pentium 3",
	__KNULL,
	CC"Mobile Intel Pentium 3 Processor-M",
	CC"Mobile Intel Celeron",
	CC"Intel Pentium 4",
	CC"Intel Pentium 4",
	CC"Intel Celeron",
	CC"Intel Xeon",
	CC"Intel Xeon MP",
	__KNULL,
	CC"Mobile Intel Pentium 4 Processor-M",
	CC"Mobile Intel Celeron",
	__KNULL,
	CC"Mobile Genuine Intel",
	CC"Intel Celeron M",
	CC"Mobile Intel Celeron",
	CC"Intel Celeron",
	CC"Mobile Genuine Intel",
	CC"Intel Pentium M",
	CC"Mobile Intel Celeron"
};

static status_t intelSignatureLookup(utf8Char **cpuNames, ubit32 sigLowModel)
{
	if (cpuNames[sigLowModel] == __KNULL)
	{
		__kprintf(ERROR INTELENUM"%d: CPU name unclear.\n",
			cpuTrib.getCurrentCpuStream()->cpuId);

		strcpy8(
			cpuTrib.getCurrentCpuStream()->cpuFeatures.cpuModel,
			unknownCpu);

		return CPUENUM_CPU_MODEL_UNCLEAR;
	};

	strcpy8(
		cpuTrib.getCurrentCpuStream()->cpuFeatures.cpuModel,
		cpuNames[sigLowModel]);

	return ERROR_SUCCESS;
}

static status_t intelSignatureEnum(ubit32 sig)
{
	ubit32		sigType, sigFamily, sigLowModel, sigExtModel;

	/**	EXPLANATION:
	 * Attempt to identify the CPU using the CPU signature returned in EAX
	 * after the call to CPUID[1].
	 **/
	sigType = x86CPUENUM_SIGNATURE_GET_TYPE(sig);
	sigFamily = x86CPUENUM_SIGNATURE_GET_FULL_FAMILY(sig);
	sigLowModel = x86CPUENUM_SIGNATURE_GET_LOW_MODEL(sig);
	sigExtModel = x86CPUENUM_SIGNATURE_GET_EXT_MODEL(sig);

	switch (sigType)
	{
	case x86CPUENUM_SIGNATURE_TYPE_OEM:
		switch (sigFamily)
		{
		case 0x4:
			if (sigExtModel != 0) return CPUENUM_CPU_MODEL_UNKNOWN;
			return intelSignatureLookup(
				oemFamily4Signatures, sigLowModel);

		case 0x5:
			if (sigExtModel != 0) return CPUENUM_CPU_MODEL_UNKNOWN;
			return intelSignatureLookup(
				oemFamily5Signatures, sigLowModel);
			
		case 0x6:
			switch (sigExtModel)
			{
			case 0x0:
				return intelSignatureLookup(
					oemFamily6Signatures, sigLowModel);

			case 0x1:
				return intelSignatureLookup(
					&oemFamily6Signatures[16], sigLowModel);

			case 0x2:
				return intelSignatureLookup(
					&oemFamily6Signatures[31], sigLowModel);

			default: return CPUENUM_CPU_MODEL_UNKNOWN;
			};

		case 0xF:
			if (sigExtModel != 0) return CPUENUM_CPU_MODEL_UNKNOWN;
			return intelSignatureLookup(
				oemFamily15Signatures, sigLowModel);

		default: return CPUENUM_CPU_MODEL_UNKNOWN;
		};

	case x86CPUENUM_SIGNATURE_TYPE_OVERDRIVE:
		switch (sigFamily)
		{
		case 0x4:
			if (sigExtModel != 0) return CPUENUM_CPU_MODEL_UNKNOWN;
			return intelSignatureLookup(
				overdriveFamily4Signatures, sigLowModel);

		case 0x5:
			if (sigExtModel != 0) return CPUENUM_CPU_MODEL_UNKNOWN;
			return intelSignatureLookup(
				overdriveFamily5Signatures, sigLowModel);

		case 0x6:
			if (sigExtModel != 0) return CPUENUM_CPU_MODEL_UNKNOWN;
			return intelSignatureLookup(
				overdriveFamily6Signatures, sigLowModel);

		default: return CPUENUM_CPU_MODEL_UNKNOWN;
		};
	default: return CPUENUM_CPU_MODEL_UNKNOWN;
	};

	return ERROR_SUCCESS;
}

static status_t intelBrandIdEnum(ubit32 brandId)
{
	if (brandIds[brandId] == __KNULL)
	{
		__kprintf(ERROR INTELENUM"Invalid CPU brand ID %d.\n", brandId);
		return CPUENUM_CPU_MODEL_UNKNOWN;
	};

	strcpy8(
		cpuTrib.getCurrentCpuStream()->cpuFeatures.cpuModel,
		brandIds[brandId]);

	return ERROR_SUCCESS;
}

status_t intelBrandStringEnum(void)
{
	ubit32		eax, ebx, ecx, edx, cpuIdIndex=0x80000002;
	utf8Char	*brandString;

	brandString = new utf8Char[49];
	if (brandString == __KNULL)
	{
		__kprintf(ERROR INTELENUM"Unable to allocate room for brand "
			"string.\n");

		return CPUENUM_CPU_MODEL_UNKNOWN;
	};

	brandString[48] = '\0';
	// Call CPUID and get moving.
	for (ubit32 i=0; i<3; i++, cpuIdIndex++)
	{
		execCpuid(cpuIdIndex, &eax, &ebx, &ecx, &edx);
		strncpy8(&brandString[i * 16], CC(&eax), 4);
		strncpy8(&brandString[(i * 16) + 4], CC(&ebx), 4);
		strncpy8(&brandString[(i * 16) + 8], CC(&ecx), 4);
		strncpy8(&brandString[(i * 16) + 12], CC(&edx), 4);
	};

	cpuTrib.getCurrentCpuStream()
		->cpuFeatures.archFeatures.cpuNameNSpaces = 0;

	for (ubit32 i=0; brandString[i] == ' '; i++)
	{
		cpuTrib.getCurrentCpuStream()
			->cpuFeatures.archFeatures.cpuNameNSpaces++;
	};

	strcpy8(
		cpuTrib.getCurrentCpuStream()->cpuFeatures.cpuModel,
		&brandString[cpuTrib.getCurrentCpuStream()
			->cpuFeatures.archFeatures.cpuNameNSpaces]);

	delete brandString;
	return ERROR_SUCCESS;
}

status_t x86CpuEnumeration::intel(void)
{
	status_t	ret;
	ubit32		eax, ebx, ecx, edx, brandId;
	ubit32		sig;

	// Kernel's x86 port supports i486+.
	if (!x8632_verifyCpuIsI486OrHigher())
	{
		__kprintf(ERROR INTELENUM"%d: This CPU is i386 or lower.\n",
			cpuTrib.getCurrentCpuStream()->cpuId);

		return CPUSTREAM_ENUMERATE_UNSUPPORTED_CPU;
	};

	// Check to see if brand string identification is available.
	execCpuid(0x8000000, &eax, &ebx, &ecx, &edx);
	if (eax >= 0x80000004)
	{
		__kprintf(NOTICE INTELENUM"%d: CPUID[0x8000000] brand string "
			"supported.\n",
			cpuTrib.getCurrentCpuStream()->cpuId);

		if (intelBrandStringEnum() == ERROR_SUCCESS) {
			return ERROR_SUCCESS;
		};
	};

	// Else check for brand ID identification.
	execCpuid(0x1, &eax, &ebx, &ecx, &edx);
	sig = eax;
	if ((brandId = ebx & 0x7F) != 0)
	{
		__kprintf(NOTICE INTELENUM"%d: CPUID[1] brand ID supported.\n",
			cpuTrib.getCurrentCpuStream()->cpuId);

		if (intelBrandIdEnum(brandId) == ERROR_SUCCESS) {
			return ERROR_SUCCESS;
		};
	};

	// Try enumerating with CPUID[1].EAX CPU signature.
	ret = intelSignatureEnum(sig);
	if (ret != ERROR_SUCCESS)
	{
		// Place the manual old style enum code here later.
		__kprintf(ERROR INTELENUM"%d: CPU model unknown.\n",
			cpuTrib.getCurrentCpuStream()->cpuId);

		strcpy8(
			cpuTrib.getCurrentCpuStream()->cpuFeatures.cpuModel,
			unknownCpu);

		return ret;
	};
	return ERROR_SUCCESS;
}

