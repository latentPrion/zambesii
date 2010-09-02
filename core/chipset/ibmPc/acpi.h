#ifndef _IBM_PC_NUMA_ACPI_H
	#define _IBM_PC_NUMA_ACPI_H

	#include <chipset/numaMap.h>
	#include <__kstdlib/__ktypes.h>

#define IBMPCNMAP

struct rsdpS
{
	ubit8		sig[8];
	ubit8		checksum;
	ubit8		oemId[6];
	ubit8		revision;
	ubit32		rsdtPaddr;
	ubit32		rsdtLength;
	ubit32		xsdtPaddr[2];
	ubit8		extChecksum;
	ubit8		reserved[3];
} __attribute__((packed));

struct sdtHeaderS
{
	char		sig[4];
	ubit32		tableLength;
	ubit8		revision;
	ubit8		checksum;
	ubit8		oemId[6];
	ubit8		oemtableId[8];
	ubit32		oemRevision;
	ubit32		creatorId;
	ubit32		creatorRevision;
};

struct rsdtS
{
	struct sdtHeaderS	header;
	ubit32			entries;
};

struct xsdtS
{
	struct sdtHeaderS	header;
	ubit32			entries[][2];
};

struct sratS
{
	struct sdtHeaderS	header;
	ubit8			reserved[12];
	ubit8			sratType;
};

struct sratCpuS
{
	ubit8		sratType;
	ubit8		length;
	ubit8		domain0;
	ubit8		cpuId;
	ubit32		flags;
	ubit8		sapic;
	// OR into a 32 bit integer with domain0 for bankId.
	ubit8		domain1[3];
	ubit8		reserved[4];
};

struct sratMemS
{
	ubit8		sratType;
	ubit8		length;
	// Do two accesses on word boundaries to retrieve.
	ubit8		domain[4];
	ubit8		reserved[2];
	ubit32		baseLow;
	ubit32		baseHigh;
	ubit32		lengthLow;
	ubit32		lengthHigh;
	ubit32		reserved0;
	ubit32		flags;
	ubit8		reserved1[8];
};

struct chipsetNumaMapS *ibmPc_mi_getNumaMap(void);
struct sratS *ibmPc_mi_findSrat(ubit32 sdtPaddr);

#endif

