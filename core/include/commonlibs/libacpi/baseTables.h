#ifndef _ZBZ_LIB_ACPI_BASE_TABLES_H
	#define _ZBZ_LIB_ACPI_BASE_TABLES_H

	#include <__kstdlib/__ktypes.h>

#define ACPI_TABLE_GET_ENDADDR(_t)			\
	((void *)(((uarch_t)(_t)) + ((_t)->hdr.tableLength) - 1))

#define ACPI_TABLE_GET_FIRST_ENTRY(_tbl)		\
	(void *)((uarch_t)(_tbl) + sizeof(*(_tbl)))

#define ACPI_PTR_INC_BY(_ptr,_amt)			\
	(void *)(((uarch_t)(_ptr)) + (_amt))

namespace acpi
{
	#define ACPI_RSDP_XSDTPADDR(_rsdp)	(*(ubit64 *)(_rsdp)->xsdtPaddr)
	#define ACPI_RSDP_SIG			CC"RSD PTR "
	#define ACPI_RSDP_SIGLEN		8

	struct sRsdp
	{
		utf8Char	sig[ACPI_RSDP_SIGLEN];
		ubit8		checksum;
		utf8Char	oemId[6];
		ubit8		revision;

		ubit32		rsdtPaddr;
		// Size of *this* table in bytes beginning at offset 0.
		ubit32		tableLength;
		ubit32		xsdtPaddr[2];
		ubit32		extendedChecksum;
	};


	#define ACPI_SDT_SIG_APIC		CC"APIC"
	#define ACPI_SDT_SIG_DSDT		CC"DSDT"
	#define ACPI_SDT_SIG_ECDT		CC"ECDT"
	#define ACPI_SDT_SIG_FACP		CC"FACP"
	#define ACPI_SDT_SIG_FACS		CC"FACS"
	#define ACPI_SDT_SIG_PSDT		CC"PSDT"
	#define ACPI_SDT_SIG_RSDT		CC"RSDT"
	#define ACPI_SDT_SIG_SBST		CC"SBST"
	#define ACPI_SDT_SIG_SLIT		CC"SLIT"
	#define ACPI_SDT_SIG_SRAT		CC"SRAT"
	#define ACPI_SDT_SIG_SSDT		CC"SSDT"
	#define ACPI_SDT_SIG_XSDT		CC"XSDT"
	#define ACPI_SDT_SIG_BOOT		CC"BOOT"
	#define ACPI_SDT_SIG_CPEP		CC"CPEP"
	#define ACPI_SDT_SIG_DBGP		CC"DBGP"
	#define ACPI_SDT_SIG_ETDT		CC"ETDT"
	#define ACPI_SDT_SIG_HPET		CC"HPET"
	#define ACPI_SDT_SIG_MCFG		CC"MCFG"
	#define ACPI_SDT_SIG_SPCR		CC"SPCR"
	#define ACPI_SDT_SIG_SPMI		CC"SPMI"
	#define ACPI_SDT_SIG_TCPA		CC"TCPA"
	#define ACPI_SDT_SIG_WDRT		CC"WDRT"

	struct sSdt
	{
		utf8Char	sig[4];
		// Size of the *entire* table, i.e: this header + it's attached to.
		ubit32		tableLength;
		ubit8		revision;
		ubit8		checksum;
		utf8Char	oemId[6];
		utf8Char	oemTableId[8];
		ubit32		oemRevision;
		ubit32		creatorId;
		ubit32		creatorRevision;
	};


	#define ACPI_RSDT_GET_NENTRIES(_rsdt)			\
		(((_rsdt)->hdr.tableLength - sizeof(sSdt)) / 4)

	struct sRsdt
	{
		struct sSdt	hdr;
		// N 4-byte physical pointers come below here.
	};


	#define ACPI_XSDT_GET_NENTRIES(_xsdt)			\
		(((_xsdt)->hdr.tableLength - sizeof(sSdt)) / 8)

	struct sXsdt
	{
		struct sSdt	hdr;
		// N 8-byte physical pointers from here on.
	};
}

#endif

