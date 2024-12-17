#ifndef _ZBZ_LIB_ACPI_MAIN_TABLES_H
	#define _ZBZ_LIB_ACPI_MAIN_TABLES_H

	#include <__kstdlib/__ktypes.h>
	#include "baseTables.h"

#define ACPI_SRAT_GET_FIRST_ENTRY(_srat)		\
	((void *)((uarch_t)(_srat) + sizeof(*(_srat))))

namespace acpiR
{
	struct sSrat
	{
		struct acpi::sSdt	hdr;
		ubit8			reserved[12];
	};

	#define ACPI_MADT_FLAGS_PC8259_COMPAT		(1<<0)
	#define ACPI_MADT_GET_FIRST_ENTRY(_madt)		\
		((void *)((uarch_t)(_madt) + sizeof(*(_madt))))

	struct sMadt
	{
		struct acpi::sSdt	hdr;
		ubit32			lapicPaddr;
		ubit32			flags;
	};

	struct sFadt
	{
		struct acpi::sSdt	hdr;
	/*	ubit32		facsPaddr;
		ubit32		dsdtPaddr;
		ubit8		rsvd1;
		ubit8		preferredPmProfile;
		ubit16		sciIntVector;

		// SMI command port and valid commands to send to it.
		ubit32		smiCmd;
		ubit8		acpiEnable;
		ubit8		acpiDisable;
		ubit8		s4BiosReq;
		ubit8		cpuPowerCtrlRelinquish;

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

		ubit16		c2LatencyUs;
		ubit16		c3LatencyUs;
		ubit16		wbinvdFlushSize;
		ubit16		wbinvdFlushStride;

		ubit8		cpuDutyOffset;
		ubit8		cpuDutyWidth;

		ubit8		cmosDayAlarm;
		ubit8		cmosMonthAlarm;
	*/
		ubit8		padding[108];
		ubit8		cmosCentury;
	} __attribute__((packed));
}

#endif

