#ifndef ___K_MEMORY_STREAM_METHODS_H
	#define ___K_MEMORY_STREAM_METHODS_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * In order to lessen the number of circular dependencies in the kernel, these
 * wrapper functions around the kernel's Memory Stream instance, have been
 * implemented. Their sole purpose is to allow circumnavigation of the need to
 * include memoryStream.h where it causes an #include circular chaotic hell.
 **/

void *__kmemoryStream_memRealloc(
	void *oldmem, uarch_t oldNBytes, uarch_t newNBytes,
	uarch_t flags=0);

#endif

