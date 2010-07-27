
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugBuffer.h>
#include <kernel/common/memoryTrib/memoryTrib.h>

/**	EXPLANATION:
 * The list of buffer pages works like this:
 *
 * New pages are added to the end of the list and the tail pointer is advanced
 * to point to the new page. When memory is low and the buffer must be scrolled,
 * the list is traversed from the head, copying all data up by enough codepoints
 * to hold the current request.
 **/

error_t debugBufferC::initialize(void)
{
	buff.rsrc.tail = new ((memoryTrib.__kmemoryStream.*
		memoryTrib.__kmemoryStream.memAlloc)(1))
			debugBufferC::buffPageS;

	if (buff.rsrc.tail == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	buff.rsrc.tail->next = __KNULL;
	buff.rsrc.cur = buff.rsrc.head = buff.rsrc.tail;
	buff.rsrc.index = 0;
	buff.rsrc.buffNPages = 1;

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

		memoryTrib.__kmemoryStream.memFree(tmp);
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

void debugBufferC::read(unicodePoint *str, uarch_t buffLen)
{
	buff.lock.acquire();

	for (; buffLen; buffLen--, str++)
	{
		if (buff.rsrc.index >= DEBUGBUFFER_PAGE_NCHARS)
		{
			buff.rsrc.cur = buff.rsrc.cur->next;
			buff.rsrc.index = 0;
		};

		if (buff.rsrc.cur == __KNULL)
		{
			buff.rsrc.tail->next = new (
				(memoryTrib.__kmemoryStream
					.*memoryTrib.__kmemoryStream.memAlloc)(
						1)) debugBufferC::buffPageS;

			if (buff.rsrc.tail->next != __KNULL)
			{
				buff.rsrc.cur = buff.rsrc.tail =
					buff.rsrc.tail->next;

				buff.rsrc.index = 0;
				buff.rsrc.buffNPages++;
			}
			else
			{
				buff.rsrc.cur = scrollBuff(
					&buff.rsrc.index,
					buffLen);
			};
		};

		buff.rsrc.cur->data[buff.rsrc.index] = *str;
	};

	buff.lock.release();
}

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

	// At this point, the whole buffer is copied buffLen up. Return.
	*index = bound;
	return tmp1;
}

