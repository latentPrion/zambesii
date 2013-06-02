
#include <__kstdlib/utf16.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kclasses/debugBuffer.h>
#include <kernel/common/processTrib/processTrib.h>

/**	EXPLANATION:
 * The list of buffer pages works like this:
 *
 * New pages are added to the end of the list and the tail pointer is advanced
 * to point to the new page. When memory is low and the buffer must be scrolled,
 * the list is traversed from the head, copying all data up by enough codepoints
 * to hold the current request.
 **/

debugBufferC::debugBufferC(void)
{
	buff.rsrc.head = buff.rsrc.cur = buff.rsrc.tail = __KNULL;
	buff.rsrc.index = buff.rsrc.buffNPages = 0;
}

#ifdef CONFIG_DEBUGPIPE_STATIC
static ubit8	buffMem[PAGING_BASE_SIZE * (DEBUGBUFFER_INIT_NPAGES-1)];
#endif

error_t debugBufferC::initialize(void)
{
	debugBufferC::buffPageS		*mem, *mem2;
	uarch_t				pageCount = 0;

#ifndef CONFIG_DEBUGPIPE_STATIC
	mem = new (
		processTrib.__kgetStream()->memoryStream.memAlloc(
			1, MEMALLOC_NO_FAKEMAP))
		debugBufferC::buffPageS;
#else
	mem = new (buffMem) debugBufferC::buffPageS;
#endif

	if (mem == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	buff.lock.acquire();

	buff.rsrc.cur = buff.rsrc.tail = buff.rsrc.head = mem;
	buff.rsrc.buffNPages = 1;
	buff.rsrc.index = 0;

	buff.lock.release();

	mem2 = mem;
	for (uarch_t i=0; i<DEBUGBUFFER_INIT_NPAGES-1; i++)
	{
#ifndef CONFIG_DEBUGPIPE_STATIC
		mem2->next = new (processTrib.__kgetStream()->memoryStream.memAlloc(
			1, MEMALLOC_NO_FAKEMAP))
				debugBufferC::buffPageS;
#else
		mem2->next = new (&buffMem[i * PAGING_BASE_SIZE]) buffPageS;
#endif

		if (mem2->next == __KNULL) {
			break;
		};
		mem2 = mem2->next;
		pageCount++;
	};

	buff.lock.acquire();

	buff.rsrc.tail = mem2;
	buff.rsrc.buffNPages += pageCount;

	buff.lock.release();

	return ERROR_SUCCESS;
}

error_t debugBufferC::shutdown(void)
{
	debugBufferC::buffPageS		*tmp;

	for (;;)
	{
		buff.lock.acquire();

		// Manually bound-checking here allows for finer grained locking.
		if (buff.rsrc.head == __KNULL)
		{
			buff.lock.release();
			break;
		};

		tmp = buff.rsrc.head;
		buff.rsrc.head = buff.rsrc.head->next;

		buff.lock.release();

		processTrib.__kgetStream()->memoryStream.memFree(tmp);
	};
	return ERROR_SUCCESS;
}

error_t debugBufferC::suspend(void)
{
	return ERROR_SUCCESS;
}

error_t debugBufferC::awake(void)
{
	return ERROR_SUCCESS;
}

void *debugBufferC::lock(void)
{
	buff.lock.acquire();
	return buff.rsrc.head;		
}

// This expects the caller to call lock() before beforehand, and unlock() after.
utf8Char *debugBufferC::extract(void **handle, uarch_t *len)
{
	utf8Char	*ret;

	if (handle == __KNULL || *handle == __KNULL || len == 0) {
		return __KNULL;
	};

	if (*reinterpret_cast<debugBufferC::buffPageS **>( handle )
		!= buff.rsrc.cur)
	{
		*len = DEBUGBUFFER_PAGE_NCHARS;
		ret = &(reinterpret_cast<buffPageS *>( *handle )->data[0]);
		*handle = reinterpret_cast<buffPageS *>( *handle )->next;
	}
	else
	{
		*len = buff.rsrc.index;
		ret = &(reinterpret_cast<buffPageS *>( *handle )->data[0]);

		// Don't allow the caller to advance any further.
		*handle = __KNULL;
	};

	return ret;
}

void debugBufferC::unlock(void)
{
	buff.lock.release();
}

void debugBufferC::syphon(utf8Char *str, uarch_t buffLen)
{
	buff.lock.acquire();

	if (buff.rsrc.head == __KNULL)
	{
		buff.lock.release();
		return;
	};

	for ( ; buffLen > 0; buffLen--, str++)
	{
		if (buff.rsrc.index >= DEBUGBUFFER_PAGE_NCHARS)
		{
			buff.rsrc.cur = buff.rsrc.cur->next;
			buff.rsrc.index = 0;
		};

		if (buff.rsrc.cur == __KNULL)
		{
			buff.rsrc.cur = scrollBuff(
				&buff.rsrc.index,
				buffLen);
		}

		buff.rsrc.cur->data[buff.rsrc.index] = *str;
		buff.rsrc.index++;
	};

	buff.lock.release();
}

// Expects the lock to be held when called.
debugBufferC::buffPageS *debugBufferC::scrollBuff(
	uarch_t *index, uarch_t buffLen
	)
{
	debugBufferC::buffPageS		*tmp1, *tmp2;
	uarch_t				bound, nWholePages, nCharsExcess;

	if (buffLen >= buff.rsrc.buffNPages * DEBUGBUFFER_PAGE_NCHARS)
	{
		*index = 0;
		return buff.rsrc.head;
	};

	tmp2 = tmp1 = buff.rsrc.head;
	nWholePages = buffLen / DEBUGBUFFER_PAGE_NCHARS;
	nCharsExcess = buffLen % DEBUGBUFFER_PAGE_NCHARS;

	for (; nWholePages; nWholePages--) {
		tmp2 = tmp2->next;
	};

	bound = DEBUGBUFFER_PAGE_NCHARS - nCharsExcess;
	for (uarch_t i=0; i<bound; i++) {
		tmp1->data[i] = tmp2->data[i + nCharsExcess];
	};
	tmp2 = tmp2->next;

	for (; tmp2 != __KNULL; )
	{
		for (uarch_t i=bound, j=0; i<DEBUGBUFFER_PAGE_NCHARS;
			i++, j++)
		{
			tmp1->data[i] = tmp2->data[j];
		};
		tmp1 = tmp1->next;

		for (uarch_t i=0; i<bound; i++) {
			tmp1->data[i] = tmp2->data[i + nCharsExcess];
		};
		tmp2 = tmp2->next;
	};

	// At this point, the whole buffer is copied 'buffLen' up. Return.
	*index = bound;
	return tmp1;
}

