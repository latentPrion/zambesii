#ifndef _SUPPORT_RIVULET_API_H
	#define _SUPPORT_RIVULET_API_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/pageAttributes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**	EXPLANATION:
 * In order to make sure that the code in the firmware rivulets can access the
 * kernel's API, we have to provide a "C" linkage interface to allow for the
 * C code to call our C++ functions.
 *
 * In here, we present our current API for memory management (and tie it
 * statically to the kernel's own memory stream) in a C linked interface.
 **/

// Virtual Memory management.
void *__kvaddrSpaceStream_getPages(uarch_t nPages);
void __kvaddrSpaceStream_releasePages(void *vaddr, uarch_t nPages);

/* Physical Memory management.
 *
 * Since I cannot think of a good way to support extended physical addresses
 * on for example, PAE x86-32, this API call will allocate as normal,
 * but transparently to the calling rivulet, it will check to see if the
 * return physical address is greater than the arch's normal physical address
 * limit.
 *
 * If it is, the API call will immediately free the physical memory and return
 * to the calling rivulet as if the allocation had failed.
 *
 * Hence the reason it's safe to use uarch_t as the 'paddr' argument's type.
 **/
error_t numaTrib_contiguousGetFrames(uarch_t nFrames, uarch_t *paddr);
status_t numaTrib_fragmentedGetFrames(uarch_t nFrames, uarch_t *paddr);
void numaTrib_releaseFrames(uarch_t paddr, uarch_t nFrames);

// Kernel Memory Stream interface.
void *__kmemoryStream_memAlloc(uarch_t nPages, uarch_t opts);
void __kmemoryStream_memFree(void *vaddr);

// Memory Tributary interface. Use sparingly. Prefer the __kmemoryStream* API.
void *memoryTrib_rawMemAlloc(uarch_t nPages);
void memoryTrib_rawMemFree(void *vaddr, uarch_t nPages);

// Walker Page Ranger interface. The kernel vaddrSpace object is implied.
status_t walkerPageRanger_mapInc(
	void *vaddr, uarch_t paddr, uarch_t nPages, uarch_t flags);

status_t walkerPageRanger_mapNoInc(
	void *vaddr, uarch_t paddr, uarch_t nPages, uarch_t flags);

void walkerPageRanger_remapInc(
	void *vaddr, uarch_t paddr, uarch_t nPages, ubit8 op, uarch_t flags);

void walkerPageRanger_remapNoInc(
	void *vaddr, uarch_t paddr, uarch_t nPages, ubit8 op, uarch_t flags);

void walkerPageRanger_setAttributes(
	void *vaddr, uarch_t nPages, ubit8 op, uarch_t flags);

status_t walkerPageRanger_lookup(
	void *vaddr, uarch_t *paddr, uarch_t *flags);

status_t walkerPageRanger_unmap(
	void *vaddr, uarch_t *paddr, uarch_t nPages, uarch_t *flags);

#ifdef __cplusplus
}
#endif

#endif

