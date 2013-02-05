#ifndef _PRIO_QUEUE_H
	#define _PRIO_QUEUE_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>
	#include <__kclasses/slamCache.h>
	#include <__kclasses/cachePool.h>
	#include <__kclasses/debugPipe.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/taskTrib/prio.h>

// Make sure that the insert options match the scheduling attributes.
#define PRIOQUEUE			"Prio-Q: "
#define PRIOQUEUE_INSERT_FRONT		SCHEDOPTS_STICKY

template <class T>
class prioQueueC
{
public:
	prioQueueC(ubit16 nPriorities)
	:
	nodeCache(__KNULL), nPrios(nPriorities), queues(__KNULL)
	{
		headQueue.rsrc = -1;
	}

	error_t initialize(void);

	~prioQueueC(void)
	{
		headQueue.rsrc = -1;
		delete[] queues;
		cachePool.destroyCache(nodeCache);
	}

public:
	error_t insert(T *item, ubit16 prio, ubit32 opt=0);
	T *pop(void);
	void remove(T *item, ubit16 prio);

	void dump(void);

private:
	template <class T2>
	class queueC
	{
	friend class prioQueueC;
	public:
		queueC(ubit16 prio, slamCacheC *cache)
		:
		prio(prio), nodeCache(cache)
		{}

		error_t initialize(void) { return ERROR_SUCCESS; }

		~queueC(void)
		{
			queueNodeS	*tmp, *curr;

			q.lock.acquire();

			for (curr = q.rsrc.head; curr != __KNULL; )
			{
				tmp = curr;
				curr = curr->next;
				nodeCache->free(tmp);
			};

			q.lock.release();
		}

	public:
		error_t insert(T2 *item, ubit32 opt);
		T2 *pop(void);
		void remove(T2 *item);

		inline uarch_t getNItems(void)
		{
			uarch_t		ret;

			q.lock.acquire();
			ret = q.rsrc.nItems;
			q.lock.release();

			return ret;
		}

		void dump(void);

	private:
		ubit16 prio;
		struct queueNodeS
		{
			T2		*item;
			queueNodeS	*next;
		};

		struct queueStateS
		{
			queueStateS(void)
			:
			nItems(0), head(__KNULL), tail(__KNULL)
			{}

			uarch_t		nItems;
			queueNodeS	*head, *tail;
		};

		sharedResourceGroupC<waitLockC, queueStateS>	q;
		slamCacheC		*nodeCache;
	};

	slamCacheC	*nodeCache;
	// Locking isn't needed; state is only modified once at instance init.
	ubit16		nPrios;
	queueC<T>	*queues;
	// Q number of the first queue with items in it. -1 if all Qs empty.
	sharedResourceGroupC<waitLockC, sbit16>	headQueue;
};


/**	Template definition and inline methods for prioQueueC.
 *****************************************************************************/

template <class T>
error_t prioQueueC<T>::initialize(void)
{
	error_t		ret;

	// Allocate a node Cache:
	nodeCache = cachePool.createCache(
		sizeof(class queueC<T>::queueNodeS));

	if (nodeCache == __KNULL) { return ERROR_MEMORY_NOMEM; };

	// Allocate internal array.
	queues = (queueC<T> *) new ubit8[sizeof(queueC<T>) * nPrios];
	if (queues == __KNULL)
	{
		cachePool.destroyCache(nodeCache);
		return ERROR_MEMORY_NOMEM;
	};

	for (uarch_t i=0; i<nPrios; i++)
	{
		// Re-construct the item using placement new.
		new (&queues[i]) queueC<T>(i, nodeCache);
		ret = queues[i].initialize();
		if (ret != ERROR_SUCCESS)
		{
			delete[] queues;
			cachePool.destroyCache(nodeCache);
			return ret;
		};
	};

	return ERROR_SUCCESS;
}

template <class T>
inline error_t prioQueueC<T>::insert(T *item, ubit16 prio, ubit32 opt)
{
	error_t		ret;

	if (prio >= nPrios) { return ERROR_INVALID_ARG_VAL; };

	ret = queues[prio].insert(item, opt);
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	headQueue.lock.acquire();

	if (headQueue.rsrc == -1 || prio < headQueue.rsrc) {
		headQueue.rsrc = prio;
	};

	headQueue.lock.release();
	return ERROR_SUCCESS;
}

template <class T>
T *prioQueueC<T>::pop(void)
{
	ubit16		qId;
	T		*ret;

	headQueue.lock.acquire();

	if (headQueue.rsrc == -1)
	{
		headQueue.lock.release();
		return __KNULL;
	};

	qId = headQueue.rsrc;

	headQueue.lock.release();
	ret = queues[qId].pop();

	if (queues[qId].getNItems() == 0)
	{
		for (ubit16 i=0; i<nPrios; i++)
		{
			// Skip the one we already know is empty.
			if (i == qId) { continue; };
			if (queues[i].getNItems() > 0)
			{
				headQueue.lock.acquire();
				headQueue.rsrc = i;
				headQueue.lock.release();

				return ret;
			};
		};

		headQueue.lock.acquire();
		headQueue.rsrc = -1;
		headQueue.lock.release();

		return ret;
	};

	return ret;
}

template <class T>
inline void prioQueueC<T>::remove(T *item, ubit16 prio)
{
	queues[prio].remove(item);

	headQueue.lock.acquire();
	if (prio == headQueue.rsrc && queues[prio].getNItems() == 0)
	{
		for (ubit16 i=0; i<nPrios; i++)
		{
			// Skip the one we know is empty.
			if (i == prio) { continue; };

			if (queues[i].getNItems() > 0)
			{
				// Set the un-empty one as the new head queue.
				headQueue.rsrc = i;

				headQueue.lock.release();
				return;
			};
		};

		headQueue.rsrc = -1;
	};

	headQueue.lock.release();
}

template <class T>
inline void prioQueueC<T>::dump(void)
{
	__kprintf(NOTICE PRIOQUEUE"%d prios, cache @0x%p, first valid q %d: "
		"dumping.\n",
		nPrios, nodeCache, headQueue.rsrc);

	for (ubit16 i=0; i<nPrios; i++) {
		queues[i].dump();
	};
}

/** Template definition and methods for prioQueueC::queueC.
 ******************************************************************************/

template <class T> template <class T2>
error_t prioQueueC<T>::queueC<T2>::insert(T2 *item, ubit32 opt)
{
	queueNodeS	*tmp;

	tmp = (queueNodeS *)nodeCache->allocate();
	if (tmp == __KNULL) { return ERROR_MEMORY_NOMEM; };
	tmp->item = item;

	q.lock.acquire();

	if (__KFLAG_TEST(opt, PRIOQUEUE_INSERT_FRONT))
	{
		tmp->next = q.rsrc.head;
		q.rsrc.head = tmp;
		if (q.rsrc.tail == __KNULL) {
			q.rsrc.tail = tmp;
		};
	}
	else
	{
		tmp->next = __KNULL;
		if (q.rsrc.tail != __KNULL) {
			q.rsrc.tail->next = tmp;
		} else {
			q.rsrc.head = tmp;
		};

		q.rsrc.tail = tmp;
	}

	q.rsrc.nItems++;
	q.lock.release();
	return ERROR_SUCCESS;
}

template <class T> template <class T2>
T2 *prioQueueC<T>::queueC<T2>::pop(void)
{
	queueNodeS	*tmp;
	T2		*ret=__KNULL;

	q.lock.acquire();

	tmp = q.rsrc.head;
	if (q.rsrc.head == q.rsrc.tail) {
		q.rsrc.tail = __KNULL;
	};

	if (q.rsrc.head != __KNULL)
	{
		q.rsrc.head = q.rsrc.head->next;
		q.rsrc.nItems--;
	};

	q.lock.release();

	if (tmp != __KNULL) {
		ret = tmp->item;
	};

	nodeCache->free(tmp);
	return ret;
}

template <class T> template <class T2>
void prioQueueC<T>::queueC<T2>::remove(T2 *item)
{
	queueNodeS	*tmp, *prev=__KNULL;

	q.lock.acquire();

	for (tmp=q.rsrc.head; tmp != __KNULL; prev = tmp, tmp = tmp->next)
	{
		// Skip until we find the item.
		if (tmp->item != item) { continue; };

		if (prev == __KNULL) {
			q.rsrc.head = tmp->next;
		} else {
			prev->next = tmp->next;
		};

		if (tmp->next == __KNULL) {
			q.rsrc.tail = prev;
		};

		q.rsrc.nItems--;

		q.lock.release();
		delete tmp;
		return;
	};

	// If we reached here, the item didn't exist in this queue.
	q.lock.release();
}

template <class T> template <class T2>
void prioQueueC<T>::queueC<T2>::dump(void)
{
	queueNodeS	*tmp;
	ubit8		flipFlop=0;

	q.lock.acquire();

	__kprintf(CC"\tQueue %d, head 0x%p, tail 0x%p, %d items:\n%s",
		prio, q.rsrc.head, q.rsrc.tail, q.rsrc.nItems,
		(q.rsrc.head == __KNULL) ? "" : "\t");

	for (tmp = q.rsrc.head; tmp != __KNULL; tmp = tmp->next, flipFlop++)
	{
		if (flipFlop > 5)
		{
			__kprintf(CC"\n\t");
			flipFlop = 0;
		};

		__kprintf(CC"0x%p (tid 0x%x) ", tmp->item, ((taskC *)tmp->item)->id);
	};

	q.lock.release();
	if (flipFlop != 0) { __kprintf(CC"\n"); };
}

#endif

