
#include <config.h>
#include <arch/walkerPageRanger.h>
#include <kernel/common/numaTrib/numaTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/firmwareTrib/firmwareSupportRivApi.h>

void *__kvaddrSpaceStream_getPages(uarch_t nPages)
{
	return (memoryTrib.__kmemoryStream.vaddrSpaceStream
		.*memoryTrib.__kmemoryStream.vaddrSpaceStream.getPages)(nPages);
}

void __kvaddrSpaceStream_releasePages(void *vaddr, uarch_t nPages)
{
	memoryTrib.__kmemoryStream.vaddrSpaceStream.releasePages(vaddr, nPages);
}

error_t numaTrib_contiguousGetFrames(uarch_t nFrames, uarch_t *paddr)
{
	error_t		ret;

	ret = numaTrib.contiguousGetFrames(nFrames, paddr);
#ifdef CONFIG_ARCH_x86_32_PAE
	if ((ret == ERROR_SUCESS) && (*paddr > 0xFFFFFFFF))
	{
		numaTrib.releaseFrames(*paddr, nFrames);
		ret = ERROR_MEMORY_NOMEM_PHYSICAL;
	};
#endif
	return ret;
}

status_t numaTrib_fragmentedGetFrames(uarch_t nFrames, uarch_t *paddr)
{
	status_t		ret;

	ret = numaTrib.fragmentedGetFrames(nFrames, paddr);
#ifdef CONFIG_ARCH_x86_32_PAE
	if ((ret > 0) && (*paddr > 0xFFFFFFFF))
	{
		numaTrib.releaseFrames(*paddr, nFrames);
		ret = 0;
	};
#endif
	return ret;
}

void numaTrib_releaseFrames(uarch_t paddr, uarch_t nFrames)
{
	numaTrib.releaseFrames(paddr, nFrames);
}

void *__kmemoryStream_memAlloc(uarch_t nPages)
{
	return (memoryTrib.__kmemoryStream
		.*memoryTrib.__kmemoryStream.memAlloc)(nPages);
}

void __kmemoryStream_memFree(void *vaddr)
{
	memoryTrib.__kmemoryStream.memFree(vaddr);
}

void *memoryTrib_rawMemAlloc(uarch_t nPages)
{
	return memoryTrib.rawMemAlloc(nPages);
}

void memoryTrib_rawMemFree(void *vaddr, uarch_t nPages)
{
	memoryTrib.rawMemFree(vaddr, nPages);
}

// Walker Page Ranger interface. The kernel vaddrSpace object is implied.
status_t walkerPageRanger_mapInc(
	void *vaddr, uarch_t paddr, uarch_t nPages, uarch_t flags
	)
{
	return walkerPageRanger::mapInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		vaddr, paddr, nPages, flags);
}

status_t walkerPageRanger_mapNoInc(
	void *vaddr, uarch_t paddr, uarch_t nPages, uarch_t flags
	)
{
	return walkerPageRanger::mapNoInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		vaddr, paddr, nPages, flags);
}

void walkerPageRanger_remapInc(
	void *vaddr, uarch_t paddr, uarch_t nPages, ubit8 op, uarch_t flags
	)
{
	walkerPageRanger::remapInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		vaddr, paddr, nPages, op, flags);
}

void walkerPageRanger_remapNoInc(
	void *vaddr, uarch_t paddr, uarch_t nPages, ubit8 op, uarch_t flags
	)
{
	walkerPageRanger::remapInc(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		vaddr, paddr, nPages, op, flags);
}

void walkerPageRanger_setAttributes(
	void *vaddr, uarch_t nPages, ubit8 op, uarch_t flags
	)
{
	walkerPageRanger::setAttributes(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		vaddr, nPages, op, flags);
}

status_t walkerPageRanger_lookup(
	void *vaddr, uarch_t *paddr, uarch_t *flags
	)
{
	return walkerPageRanger::lookup(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		vaddr, paddr, flags);
}

status_t walkerPageRanger_unmap(
	void *vaddr, uarch_t *paddr, uarch_t nPages, uarch_t *flags
	)
{
	return walkerPageRanger::unmap(
		&memoryTrib.__kmemoryStream.vaddrSpaceStream.vaddrSpace,
		vaddr, paddr, nPages, flags);
}

