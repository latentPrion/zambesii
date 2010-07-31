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
 * The buffer is dynamically sized, and will simply grow as the kernel's debug
 * output grows. If ever the buffer asks the kernel for more memory and
 * gets a failed allocation, it will move all data in the buffer up and out of
 * the buffer as necessary to facilitate the new message.
 **/

#define DEBUGBUFFER_PAGE_NCHARS				\
	(PAGING_BASE_SIZE - sizeof(void *)) / sizeof(unicodePoint)

class debugBufferC
{
struct buffPageS;
public:
	error_t initialize(void);
	error_t shutdown(void);
	error_t suspend(void);
	error_t awake(void);

public:
	// String of raw, expanded unicode characters.
	void read(unicodePoint *str, uarch_t buffLen);
	// Calling 'clear' is the same as flushing all pages.
	void clear(void);

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
	struct buffPtrStateS {
		buffPageS	*head, *tail, *cur;
		uarch_t		index, buffNPages;
	};
	sharedResourceGroupC<waitLockC, buffPtrStateS>	buff;
};

#endif

