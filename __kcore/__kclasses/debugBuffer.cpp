
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

static ubit8	buffMem[PAGING_BASE_SIZE * DEBUGBUFFER_INIT_NPAGES];

DebugBuffer::DebugBuffer(void)
:
buff(CC"DebugBuffer buff")
{
	DebugBuffer::sBuffPage		*tmp;

	buff.rsrc.head = buff.rsrc.cur = new (buffMem) sBuffPage;
	buff.rsrc.index = 0;

	tmp = buff.rsrc.head;
	for (uarch_t i=1; i<DEBUGBUFFER_INIT_NPAGES; i++, tmp = tmp->next) {
		tmp->next = new (&buffMem[i * PAGING_BASE_SIZE]) sBuffPage;
	};

	buff.rsrc.buffNPages = DEBUGBUFFER_INIT_NPAGES;
}

error_t DebugBuffer::initialize(void)
{
	return ERROR_SUCCESS;
}

error_t DebugBuffer::shutdown(void)
{
	DebugBuffer::sBuffPage		*tmp;

	for (;FOREVER;)
	{
		buff.lock.acquire();

		// Manually bound-checking here allows for finer grained locking.
		if (buff.rsrc.head == NULL)
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

error_t DebugBuffer::suspend(void)
{
	return ERROR_SUCCESS;
}

error_t DebugBuffer::awake(void)
{
	return ERROR_SUCCESS;
}

void *DebugBuffer::lock(void)
{
	buff.lock.acquire();
	return buff.rsrc.head;
}

// This expects the caller to call lock() before beforehand, and unlock() after.
utf8Char *DebugBuffer::extract(void **handle, uarch_t *len)
{
	utf8Char	*ret;

	if (handle == NULL || *handle == NULL || len == 0) {
		return NULL;
	};

	if (*reinterpret_cast<DebugBuffer::sBuffPage **>( handle )
		!= buff.rsrc.cur)
	{
		*len = DEBUGBUFFER_PAGE_NCHARS;
		ret = &(reinterpret_cast<sBuffPage *>( *handle )->data[0]);
		*handle = reinterpret_cast<sBuffPage *>( *handle )->next;
	}
	else
	{
		*len = buff.rsrc.index;
		ret = &(reinterpret_cast<sBuffPage *>( *handle )->data[0]);

		// Don't allow the caller to advance any further.
		*handle = NULL;
	};

	return ret;
}

void DebugBuffer::unlock(void)
{
	buff.lock.release();
}

void DebugBuffer::write(utf8Char *str, uarch_t buffLen)
{
	buff.lock.acquire();

	if (buff.rsrc.head == NULL)
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

		if (buff.rsrc.cur == NULL)
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
DebugBuffer::sBuffPage *DebugBuffer::scrollBuff(
	uarch_t *index, uarch_t buffLen
	)
{
	DebugBuffer::sBuffPage		*tmp1, *tmp2;
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

	for (; tmp2 != NULL; )
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

