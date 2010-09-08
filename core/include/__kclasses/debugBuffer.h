#ifndef _DEBUG_BUFFER_H
	#define _DEBUG_BUFFER_H

	#include <arch/paging.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

/**	EXPLANATION:
 * The Debug Pipe Buffer takes all the raw code points from the debugPipeC
 * object and stores them, uninterpreted, into itself.
 *
 * The idea behind this buffer is such that the kernel debug output can be kept
 * safe for later dumping, so any printing done while, say, the kernel has
 * untied the debug pipe from all hardware will all go into the buffer.
 *
 * The buffer is statically sized, and its initial size is set with
 * DEBUGBUFFER_INIT_NPAGES below. This initial buffer is resized after the
 * NUMA Tributary's initialize2() routine, which allows the kernel to take
 * full advantage of the chipset's memory.
 *
 * At that point, the kernel will allocate more memory, and link it to the
 * end of the buffer, effectively extending the buffer to allow for more
 * output.
 *
 * That is to say, this class has been written to be implicitly dynamically
 * sized. However, the dynamicity was pulled since it created a circular
 * dependency. So now it's generationally dynamically sized, in that it is
 * resized in stages throughout the kernel's initialization.
 **/

#define DEBUGBUFFER_PAGE_NCHARS				\
	(PAGING_BASE_SIZE - sizeof(void *)) / sizeof(unicodePoint)

#define DEBUGBUFFER_INIT_NPAGES			(8)

class debugBufferC
{
struct buffPageS;
public:
	debugBufferC(void);
	error_t initialize(void);
	error_t shutdown(void);
	error_t suspend(void);
	error_t awake(void);

public:
	// String of raw, expanded unicode characters.
	void syphon(unicodePoint *str, uarch_t buffLen);
	// Calling 'clear' will reset the buffer to the 1st page, index 0.
	void clear(void);
	// Calling 'flush' will deallocate all pages in the buffer.
	void flush(void);

	void *lock(void);
	unicodePoint *extract(void **handle, uarch_t *len);
	void unlock(void);

private:
	buffPageS *scrollBuff(uarch_t *index, uarch_t buffLen);

	struct buffPageS
	{
		buffPageS	*next;
		// This member should always be 32-bit aligned.
		unicodePoint	data[DEBUGBUFFER_PAGE_NCHARS];
	};
	struct buffPtrStateS
	{
		buffPageS	*head, *tail, *cur;
		uarch_t		index, buffNPages;
	};
	sharedResourceGroupC<waitLockC, buffPtrStateS>	buff;
};

#endif

