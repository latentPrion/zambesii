
#include <chipset/dedicatedMemoryRegions.h>
#include <__kclasses/debugPipe.h>


void dumpChipsetDedicatedMemoryRegions(void)
{
	printf(NOTICE"Chipset dedicated memory regions:\n");
	for (uarch_t i=0; i<dedicatedMemoryRegionMap->nEntries; i++)
	{
		printf(NOTICE"Region %d: base %p, size %p.\n",
			i, dedicatedMemoryRegionMap->entries[i].baseAddr,
			dedicatedMemoryRegionMap->entries[i].size);
	}
}
