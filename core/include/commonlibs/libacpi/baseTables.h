#ifndef _ZBZ_LIB_ACPI_BASE_TABLES_H
	#define _ZBZ_LIB_ACPI_BASE_TABLES_H

	#include <__kstdlib/__ktypes.h>

#define ACPI_TABLE_GET_ENDADDR(_t)			\
	((void *)(((uarch_t)(_t)) + ((_t)->hdr.tableLength) - 1))

#define ACPI_TABLE_GET_FIRST_ENTRY(_tbl)		\
	(void *)((uarch_t)(_tbl) + sizeof(*(_tbl)))

#define ACPI_PTR_INC_BY(_ptr,_amt)			\
	(void *)(((uarch_t)(_ptr)) + (_amt))

#define ACPI_RSDP_XSDTPADDR(_rsdp)	(*(ubit64 *)(_rsdp)->xsdtPaddr)
#define ACPI_RSDP_SIG			"RSD PTR "
#define ACPI_RSDP_SIGLEN		8

struct acpi_rsdpS
{
	char		sig[ACPI_RSDP_SIGLEN];
	ubit8		checksum;
	char		oemId[6];
	ubit8		revision;

	ubit32		rsdtPaddr;
	// Size of *this* table in bytes beginning at offset 0.
	ubit32		tableLength;
	ubit32		xsdtPaddr[2];
	ubit32		extendedChecksum;
};


#define ACPI_SDT_SIG_APIC		"APIC"
#define ACPI_SDT_SIG_DSDT		"DSDT"
#define ACPI_SDT_SIG_ECDT		"ECDT"
#define ACPI_SDT_SIG_FACP		"FACP"
#define ACPI_SDT_SIG_FACS		"FACS"
#define ACPI_SDT_SIG_PSDT		"PSDT"
#define ACPI_SDT_SIG_RSDT		"RSDT"
#define ACPI_SDT_SIG_SBST		"SBST"
#define ACPI_SDT_SIG_SLIT		"SLIT"
#define ACPI_SDT_SIG_SRAT		"SRAT"
#define ACPI_SDT_SIG_SSDT		"SSDT"
#define ACPI_SDT_SIG_XSDT		"XSDT"
#define ACPI_SDT_SIG_BOOT		"BOOT"
#define ACPI_SDT_SIG_CPEP		"CPEP"
#define ACPI_SDT_SIG_DBGP		"DBGP"
#define ACPI_SDT_SIG_ETDT		"ETDT"
#define ACPI_SDT_SIG_HPET		"HPET"
#define ACPI_SDT_SIG_MCFG		"MCFG"
#define ACPI_SDT_SIG_SPCR		"SPCR"
#define ACPI_SDT_SIG_SPMI		"SPMI"
#define ACPI_SDT_SIG_TCPA		"TCPA"
#define ACPI_SDT_SIG_WDRT		"WDRT"

struct acpi_sdtS
{
	char		sig[4];
	// Size of the *entire* table, i.e: this header + it's attached to.
	ubit32		tableLength;
	ubit8		revision;
	ubit8		checksum;
	char		oemId[6];
	char		oemTableId[8];
	ubit32		oemRevision;
	ubit32		creatorId;
	ubit32		creatorRevision;
};


#define ACPI_RSDT_GET_NENTRIES(_rsdt)			\
	(((_rsdt)->hdr.tableLength - sizeof(acpi_sdtS)) / 4)

struct acpi_rsdtS
{
	struct acpi_sdtS	hdr;
	// N 4-byte physical pointers come below here.
};


#define ACPI_XSDT_GET_NENTRIES(_xsdt)			\
	(((_xsdt)->hdr.tableLength - sizeof(acpi_sdtS)) / 8)

struct acpi_xsdtS
{
	struct acpi_sdtS	hdr;
	// N 8-byte physical pointers from here on.
};
	
#endif

