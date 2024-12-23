#ifndef _ZKCM_IBM_PC_STATE_H
	#define _ZKCM_IBM_PC_STATE_H

	#include <arch/paddr_t.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/smpTypes.h>

/**	NOTE:
 * Try not to include this file in headers. Include it only in source files.
 **/

#define SMPSTATE_UNIPROCESSOR	0x0
#define SMPSTATE_SMP		0x1

struct sIbmPcChipsetState
{
	sIbmPcChipsetState(void)
	:
	lapicPaddr(0)
	{}

	struct sBspInfo
	{
		sBspInfo(void)
		:
		bspIdRequestedAlready(0), bspId(CPUID_INVALID)
		{}

		sarch_t		bspIdRequestedAlready;
		cpu_t		bspId;
	} bspInfo;

	struct sSmpInfo
	{
		sSmpInfo(void)
		:
		chipsetState(SMPSTATE_UNIPROCESSOR),
		chipsetOriginalState(SMPSTATE_UNIPROCESSOR)
		{}

		// Whether SMP (Symmetric I/O) or non-SMP mode.
		ubit8		chipsetState;
		// Set the board back to Virt. wire or PIC mode at shutdown.
		ubit8		chipsetOriginalState;
	} smpInfo;

	paddr_t		lapicPaddr;
};

extern sIbmPcChipsetState	ibmPcState;

#endif

