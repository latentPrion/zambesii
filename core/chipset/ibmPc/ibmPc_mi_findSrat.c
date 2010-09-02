
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/firmwareTrib/rivMemoryApi.h>
#include <kernel/common/firmwareTrib/rivDebugApi.h>
#include "acpi.h"


#define SDT_PERMAP_NPAGES	256
#define RSDT_MAP_NPAGES		32

static struct rsdtS		*rsdt;
static struct sdtHeaderS	*currSdt;

struct sratS *ibmPc_mi_findSrat(ubit32 sdtPaddr)
{
	status_t	status;
	ubit32		tmp[8];
	uarch_t		i, j, offset, nSdts;
	char		tblType[5];

	offset = sdtPaddr & 0xFFF;
	sdtPaddr >>= 12;
	sdtPaddr <<= 12;

	// Okay...found the RSDP, and have the SDTP. Map in 1MB from the SDTP.
	rsdt = (struct rsdtS *)__kvaddrSpaceStream_getPages(RSDT_MAP_NPAGES);
	if (rsdt == __KNULL)
	{
		rivPrintf(NOTICE IBMPCNMAP"Not enough vmem to map RSDT.\n");
		return __KNULL;
	};

	// Map vmem to sdtPaddr.
	status = walkerPageRanger_mapInc(
#ifdef CONFIG_ARCH_x86_32_PAE
		rsdt, paddr_t(0, sdtPaddr), RSDT_MAP_NPAGES,
#else
		rsdt, sdtPaddr, RSDT_MAP_NPAGES,
#endif
		PAGEATTRIB_WRITE | PAGEATTRIB_PRESENT | PAGEATTRIB_SUPERVISOR);

	if (status < RSDT_MAP_NPAGES)
	{
		__kvaddrSpaceStream_releasePages(rsdt, RSDT_MAP_NPAGES);
		rivPrintf(NOTICE IBMPCNMAP"WPR failed to map %d pages for RSDT."
			"\n", RSDT_MAP_NPAGES);

		return __KNULL;
	};

	rsdt = (struct rsdtS *)((uarch_t)rsdt + offset);

	rivPrintf(NOTICE IBMPCNMAP"Mapped RSDT paddr to v 0x%X.\n", rsdt);

	if (strncmp(rsdt->header.sig, "RSDT", 4) != 0)
	{
		rivPrintf(NOTICE IBMPCNMAP"Mapped RSDT, but table header != "
			"\"RSDT\". Exiting.\n");

		goto releaseRsdtVmem;
	};

	nSdts = (rsdt->header.tableLength - sizeof(struct sdtHeaderS))
		/ sizeof(ubit32);

	rivPrintf(NOTICE IBMPCNMAP"RSDT signature \"RSDT\" verified, RSDT "
		"length field: 0x%X, num SDTs: %d.\n",
		rsdt->header.tableLength, nSdts);

	currSdt = (struct sdtHeaderS *)__kvaddrSpaceStream_getPages(
		SDT_PERMAP_NPAGES);

	if (currSdt == __KNULL)
	{
		rivPrintf(NOTICE IBMPCNMAP"Unable to allocate %d pages of "
			"kernel vaddrspace to point to sdts. Exiting.\n",
			SDT_PERMAP_NPAGES);

		goto releaseRsdtVmem;
	};

	// Now that we have the RSDT mapped in, let's try to find an SRAT.
	rivPrintf(NOTICE IBMPCNMAP"Attempting to map each SDT and read sig.\n");
	
	for (i=0; i<nSdts; i++)
	{
		// Get the sdtPaddr.
		sdtPaddr = *(ubit32 *)((uarch_t)&rsdt->entries + (i * 4));
		offset = sdtPaddr & 0xFFF;
		sdtPaddr >>= 12;
		sdtPaddr <<= 12;

		status = walkerPageRanger_mapInc(
			currSdt, sdtPaddr, SDT_PERMAP_NPAGES,
			PAGEATTRIB_WRITE | PAGEATTRIB_PRESENT
			| PAGEATTRIB_SUPERVISOR);

		if (status < SDT_PERMAP_NPAGES)
		{
			rivPrintf(NOTICE IBMPCNMAP"Failed to map SDT %d.\n", i);
			goto releaseGsdtVmem;
		};

		// Mapped. Add offset back on.
		currSdt = (struct sdtHeaderS *)((uarch_t)currSdt + offset);

		// Read and printf header!
		for (j=0; j<4; j++) {
			tblType[j] = currSdt->sig[j];
		};
		tblType[4] = '\0';

		rivPrintf(NOTICE IBMPCNMAP"SDT %d: at paddr 0x%X, Sig: ",
			i, sdtPaddr + offset);

		rivPrintf((utf8Char *)tblType);
		rivPrintf((utf8Char *)"\n");
	};

releaseGsdtVmem:
	__kvaddrSpaceStream_releasePages(currSdt, SDT_PERMAP_NPAGES);

releaseRsdtVmem:
	__kvaddrSpaceStream_releasePages(rsdt, RSDT_MAP_NPAGES);
	return __KNULL;
}

