#ifndef _ZBZ_LIBACPI_FACP_H
	#define _ZBZ_LIBACPI_FACP_H

	#include <__kstdlib/__ktypes.h>

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
};

#endif

