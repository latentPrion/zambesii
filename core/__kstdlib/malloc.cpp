
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kclib/stdlib.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/memReservoir.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/process.h>
#include <kernel/common/panic.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


void pointer::vmemUnmapAndFree(
	void *vaddr, uarch_t nPages, status_t nMapped,
	vaddrSpaceStreamC *vasStream
	)
{
	if (vasStream == NULL)
	{
		vasStream = cpuTrib.getCurrentCpuStream()->taskStream
			.getCurrentTask()->parent->getVaddrSpaceStream();
	};

	if (nMapped > 0)
	{
		paddr_t			p;
		uarch_t			f;

		walkerPageRanger::unmap(
			&vasStream->vaddrSpace, vaddr, &p, nPages, &f);
	};

	vasStream->releasePages(vaddr, nPages);
}

void pointer::streamFree(void *vaddr, memoryStreamC *memStream)
{
	if (memStream == NULL)
	{
		memStream = &cpuTrib.getCurrentCpuStream()->taskStream
			.getCurrentTask()->parent->memoryStream;
	};

	memStream->memFree(vaddr);
}

void *malloc(uarch_t nBytes)
{
	return memReservoir.allocate(nBytes);
}

void *calloc(uarch_t objSize, uarch_t nObjs)
{
	return memReservoir.allocate(objSize * nObjs);
}

void free(void *mem)
{
	if (mem == NULL) { return; };
	memReservoir.free(mem);
}

