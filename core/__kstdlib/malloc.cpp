
#include <arch/walkerPageRanger.h>
#include <__kstdlib/cleanup.h>
#include <__kstdlib/__kclib/stdlib.h>
#include <__kclasses/memReservoir.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/process.h>
#include <kernel/common/panic.h>
#include <kernel/common/messageStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


asyncResponseC::~asyncResponseC(void)
{
	messageStreamC::headerS		*msg;
	error_t				ret;

	msg = (messageStreamC::headerS *)pointers[0].v;

	if (msg == NULL) { return; };
	msg->error = pointers[0].nPages;
	ret = messageStreamC::enqueueOnThread(msg->targetId, msg);
	if (ret != ERROR_SUCCESS)
	{
		printf(FATAL"asyncCleanup: Failed to send msg from 0x%x to "
			"0x%x, with error %s.\n",
			msg->sourceId, msg->targetId, strerror(msg->error));
	};
}

void cleanup::vmemFree(
	int idx, pointerS *pointers, int nPointers,
	vaddrSpaceStreamC *vasStream
	)
{
	if (idx >= nPointers)
	{
		panic(FATAL"cleanupC::vmemFree: Attempt to free invalid "
				"index.");
	};

	if (pointers[idx].v == NULL) { return; };

	if (vasStream == NULL)
	{
		vasStream = cpuTrib.getCurrentCpuStream()->taskStream
			.getCurrentTask()->parent->getVaddrSpaceStream();
	};
	vasStream->releasePages(pointers[idx].v, pointers[idx].nPages);
}

void cleanup::vmemUnmapAndFree(
	int idx, pointerS *pointers, int nPointers,
	vaddrSpaceStreamC *vasStream
	)
{
	if (idx >= nPointers)
	{
		panic(FATAL"cleanupC::vmemUnmapAndFree: Attempt to free "
			"invalid index.");
	};

	if (pointers[idx].v == NULL) { return; };

	if (vasStream == NULL)
	{
		vasStream = cpuTrib.getCurrentCpuStream()->taskStream
			.getCurrentTask()->parent->getVaddrSpaceStream();
	};

	paddr_t			p;
	uarch_t			f;

	walkerPageRanger::unmap(
		&vasStream->vaddrSpace,
		pointers[idx].v, &p, pointers[idx].nPages, &f);

	vmemFree(idx, pointers, nPointers, vasStream);
}

void cleanup::streamFree(
	int idx, pointerS *pointers, int nPointers,
	memoryStreamC *memStream
	)
{
	if (idx >= nPointers)
	{
		panic(FATAL"cleanupC::streamFree: Attempt to free invalid "
				"index.");
	};

	if (pointers[idx].v == NULL) { return; };

	if (memStream == NULL)
	{
		memStream = &cpuTrib.getCurrentCpuStream()->taskStream
			.getCurrentTask()->parent->memoryStream;
	};

	memStream->memFree(pointers[idx].v);
}

void *malloc(uarch_t nBytes)
{
	return memReservoir.allocate(nBytes);
}

void *realloc(void *mem, uarch_t nBytes)
{
	memReservoir.free(mem);
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

