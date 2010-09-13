
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/pageAttributes.h>
#include <kernel/common/firmwareTrib/rivMemoryApi.h>
#include <kernel/common/firmwareTrib/rivDebugApi.h>
#include "acpi.h"


/**	EXPLANATION:
 * Will map low memory into the kernel's address space (again, even though
 * the firmware code in x86Emu does the same), and then scan for ACPI RSDP, and
 * from there look for SRAT stuff, parse it, and return a structure to the
 * kernel.
 **/

#define IBMPC_NMAP_LOWMEM_NPAGES		256

static uarch_t		lowmem;
static uarch_t		ebda;
static struct rsdpS	*rsdp;

static struct numaMemMapEntryS	tmapEntries[] =
{
	{
		0xC0800000,
		0x800000,
		12,
		0
	},
	{
		0xC0000000,
		0x800000,
		12,
		0
	},
	{
		0x1000000,
		0x100000,
		3,
		0
	},
	{
		0xDFFC0000,
		0x7000,
		3,
		0
	},
	{
		0x80C0F000,
		0x3000,
		5,
		0
	}
};

// Test map for debugging.
static struct chipsetNumaMapS	tmap =
{
	tmapEntries,
	5,
	0
};

struct chipsetNumaMapS *ibmPc_mi_getNumaMap(void)
{
	(void)tmap;

	return __KNULL;
}

struct chipsetNumaMapS *ibmPc_mi_getNumaMapRemoved(void)
{
	status_t	status;
	char		rsdpver[9];
	uarch_t		i;

	lowmem = (uarch_t)__kvaddrSpaceStream_getPages(
		IBMPC_NMAP_LOWMEM_NPAGES);

	if (lowmem == __KNULL) {
		return __KNULL;
	};

	// Now map low memory into the page range allocated.
	status = walkerPageRanger_mapInc(
		(void *)lowmem, 0x0, IBMPC_NMAP_LOWMEM_NPAGES,
		PAGEATTRIB_WRITE | PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (status < IBMPC_NMAP_LOWMEM_NPAGES)
	{
		__kvaddrSpaceStream_releasePages(
			(void *)lowmem, IBMPC_NMAP_LOWMEM_NPAGES);

		rivPrintf(NOTICE IBMPCNMAP"Unable to map %d frames at 0x%X. No "
			"NUMA map returned.\n",
			IBMPC_NMAP_LOWMEM_NPAGES, lowmem);

		return __KNULL;
	};

	// At this point we have low memory mapped at 'lowmem'.
	ebda = lowmem + 0x80000;
	for (rsdp = 0; ebda < (lowmem + 0x100000); ebda += 0x10)
	{
		if (strncmp((char *)ebda, "RSD PTR ", 8) == 0)
		{
			rivPrintf(NOTICE IBMPCNMAP"Found RSDP at 0x%X.\n",
				ebda - lowmem);

			rsdp = (struct rsdpS *)ebda;
			break;
		};
	};

	if (rsdp == __KNULL)
	{
		__kvaddrSpaceStream_releasePages(
			(void *)lowmem, IBMPC_NMAP_LOWMEM_NPAGES);

		rivPrintf(NOTICE IBMPCNMAP"Failed to locate RSDP.\n");

		return __KNULL;
	};

	for (i=0; i<8; i++) {
		rsdpver[i] = rsdp->sig[i];
	};
	rsdpver[8] = '\0';
	rivPrintf(NOTICE IBMPCNMAP"Print out string found for RSDP: ");
	rivPrintf((utf8Char *)rsdpver);
	rivPrintf((utf8Char *)"\n");

	rivPrintf(NOTICE IBMPCNMAP"RSDP: rsdtPaddr 0x%X, rsdtLen 0x%X, "
		"extSDTPaddr 0x%X_0x%X.\n",
		rsdp->rsdtPaddr, rsdp->rsdtLength, rsdp->xsdtPaddr[1],
		rsdp->xsdtPaddr[0]);

	ibmPc_mi_findSrat((paddr_t)rsdp->rsdtPaddr);
}

