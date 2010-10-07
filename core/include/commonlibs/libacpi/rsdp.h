/**	LICENSE
 * File: rsdp.h
 * This file is part of Zambezii
 *
 * Copyright (C) 2010 - Reaven Grey
 *
 * You can redistribute and/or modify Zambezii under the terms of the
 * Zambezii License v1.0 as seen in LICENSE-zbz-1.0 in the root
 * folder of this source or object code distribution of
 * Zambezii, or the root folder of an official repository
 * of the Zambezii Kernel project where that found in the
 * latter repository overrides any other version of this License
 * found elsewhere.
 *
 * Zambezii is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Zambezii License v1.0 for more details.
 *
 * You should have received a copy of the Zambezii License
 * along with Zambezii; if not, see the root folder of any
 * repository of the Zambezii Kernel project for an official
 * version.
 */
#ifndef _ACPI_TABLES_H
	#define _ACPI_TABLES_H

	#include <__kstdlib/__ktypes.h>


#define ACPI_PTR_INC_BY(_ptr,_type)			\
	(void *)(((uarch_t)_ptr) + sizeof(_type))

#define ACPI_RSDP_XSDTPADDR(_rsdp)	(*(ubit64 *)_rsdp->xsdtPaddr)
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


#define ACPI_RSDT_GET_FIRST_ENTRY(_rsdt)		\
	(paddr_t)((uarch_t)_rsdt + sizeof(acpi_sdtS))

#define ACPI_RSDT_GET_NENTRIES(_rsdt)			\
	((_rsdt->hdr.tableLength - sizeof(acpi_sdtS)) / 4)

struct acpi_rsdtS
{
	struct acpi_sdtS	hdr;
	// N 4-byte physical pointers come below here.
};


#define ACPI_XSDT_GET_NENTRIES(_xsdt)			\
	((_xsdt->hdr.tableLength - sizeof(acpi_sdtS)) / 8)

struct acpi_xsdtS
{
	struct acpi_sdtS	hdr;
	// N 8-byte physical pointers from here on.
};


struct acpi_fadtS
{
	struct acpi_sdtS	hdr;
	ubit32		facsPaddr;
	ubit32		dsdtPaddr;
	ubit8		rsvd1;
	ubit8		preferredPmProfile;
	ubit16		sciIntVector;

	// SMI command port and valid commands to send to it.
	ubit32		smiCmd;
	ubit8		acpiEnable;
	ubit8		acpiDisable;
	ubit8		s4BiosReq;
	ubit8		cpuStateCtrl;

	ubit32		pm1aEvent;
	ubit32		pm1bEvent;
	ubit32		pm1aCtrl;
	ubit32		pm1bCtrl;

	ubit32		pm2Ctrl;
	ubit32		pmTimer;

	ubit32		gpEvent0;
	ubit32		gpEvent1;

	ubit8		pm1EventLen;
	ubit8		pm1CtrlLen;
	ubit8		pm2CtrlLen;
	ubit8		pmTimerLen;

	ubit8		gpEvent0Len;
	ubit8		gpEvent1Len;
	ubit8		gpEvent1Base;
	ubit8		cstCtrl;

	
#endif

