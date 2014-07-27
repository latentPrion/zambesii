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
class PrioQueue
{
public:
	PrioQueue(ubit16 nPriorities)
	:
	Nodeache(NULL), nPrios(nPriorities), queues(NULL)
	{
		headQueue.rsrc = -1;
	}

	error_t initialize(void);

	~PrioQueue(void)
	{
		headQueue.rsrc = -1;
		delete[] queues;
		cachePool.destroyCache(Nodeache);
	}

public:
	error_t insert(T *item, ubit16 prio, ubit32 opt=0);
	T *pop(void);
	void remove(T *item, ubit16 prio);

	void dump(void);

private:
	template <class T2>
	class Queue
	{
	friend class PrioQueue;
	public:
		// fine.
		Queue(ubit16 prio, SlamCache *cache)
		:
		prio(prio), Nodeache(cache)
		{}

		// fine.
		error_t initialize(void) { return ERROR_SUCCESS; }

		// fine.
		~Queue(void)
		{
			sQueueNode	*tmp, *curr;

			q.lock.acquire();

			for (curr = q.rsrc.head; curr != NULL; )
			{
				tmp = curr;
				curr = curr->next;
				Nodeache->free(tmp);
			};

			q.lock.release();
		}

	public:
		error_t insert(T2 *item, ubit32 opt);
		T2 *pop(void);
		void remove(T2 *item);

		// fine.
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
		struct sQueueNode
		{
			T2		*item;
			sQueueNode	*next;
		};

		struct sQueueState
		{
			sQueueState(void)
			:
			nItems(0), head(NULL), tail(NULL)
			{}

			uarch_t		nItems;
			sQueueNode	*head, *tail;
		};

		ubit16 prio;
		SharedResourceGroup<WaitLock, sQueueState>	q;
		SlamCache		*Nodeache;
	};

	SlamCache	*Nodeache;
	// Locking isn't needed; state is only modified once at instance init.
	ubit16		nPrios;
	Queue<T>	*queues;
	// Q number of the first queue with items in it. -1 if all Qs empty.
	SharedResourceGroup<WaitLock, sbit16>	headQueue;
};


/**	Template definition and inline methods for PrioQueue.
 *****************************************************************************/

// fine.
template <class T>
error_t PrioQueue<T>::initialize(void)
{
	error_t		ret;

	// Allocate a node Cache:
	Nodeache = cachePool.createCache(
		sizeof(class Queue<T>::sQueueNode));

	if (Nodeache == NULL) { return ERROR_MEMORY_NOMEM; };

	// Allocate internal array.
	queues = (Queue<T> *) new ubit8[sizeof(Queue<T>) * nPrios];
	if (queues == NULL)
	{
		cachePool.destroyCache(Nodeache);
		return ERROR_MEMORY_NOMEM;
	};

	for (uarch_t i=0; i<nPrios; i++)
	{
		// Re-construct the item using placement new.
		new (&queues[i]) Queue<T>(i, Nodeache);
		ret = queues[i].initialize();
		if (ret != ERROR_SUCCESS)
		{
			delete[] queues;
			cachePool.destroyCache(Nodeache);
			return ret;
		};
	};

	return ERROR_SUCCESS;
}

// Safe.
template <class T>
inline error_t PrioQueue<T>::insert(T *item, ubit16 prio, ubit32 opt)
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

// fine.
template <class T>
T *PrioQueue<T>::pop(void)
{
	ubit16		qId;
	T		*ret;

	headQueue.lock.acquire();

	if (headQueue.rsrc == -1)
	{
		headQueue.lock.release();
		return NULL;
	};

	qId = headQueue.rsrc;
	ret = queues[qId].pop();

	if (queues[qId].getNItems() == 0)
	{
		for (ubit16 i=0; i<nPrios; i++)
		{
			// Skip the one we already know is empty.
			if (i == qId) { continue; };
			if (queues[i].getNItems() > 0)
			{
				headQueue.rsrc = i;
				headQueue.lock.release();

				return ret;
			};
		};

		headQueue.rsrc = -1;
		headQueue.lock.release();

		return ret;
	};

	headQueue.lock.release();
	return ret;
}

// safe.
template <class T>
inline void PrioQueue<T>::remove(T *item, ubit16 prio)
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
inline void PrioQueue<T>::dump(void)
{
	printf(NOTICE PRIOQUEUE"%d prios, cache @0x%p, first valid q %d: "
		"dumping.\n",
		nPrios, Nodeache, headQueue.rsrc);

	for (ubit16 i=0; i<nPrios; i++) {
		queues[i].dump();
	};
}

/** Template definition and methods for PrioQueue::Queue.
 ******************************************************************************/
// safe.
template <class T> template <class T2>
error_t PrioQueue<T>::Queue<T2>::insert(T2 *item, ubit32 opt)
{
	sQueueNode	*tmp;

	tmp = (sQueueNode *)Nodeache->allocate();
	if (tmp == NULL) { return ERROR_MEMORY_NOMEM; };
	tmp->item = item;

	q.lock.acquire();

	if (FLAG_TEST(opt, PRIOQUEUE_INSERT_FRONT))
	{
		// safe.
		tmp->next = q.rsrc.head;
		q.rsrc.head = tmp;
		if (q.rsrc.tail == NULL) {
			q.rsrc.tail = tmp;
		};
	}
	else
	{
		// safe.
		tmp->next = NULL;
		if (q.rsrc.tail != NULL) {
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

// safe.
template <class T> template <class T2>
T2 *PrioQueue<T>::Queue<T2>::pop(void)
{
	sQueueNode	*tmp;
	T2		*ret=NULL;

	q.lock.acquire();

	tmp = q.rsrc.head;
	if (q.rsrc.head == q.rsrc.tail) {
		q.rsrc.tail = NULL;
	};

	if (q.rsrc.head != NULL)
	{
		q.rsrc.head = q.rsrc.head->next;
		q.rsrc.nItems--;
	};

	q.lock.release();

	if (tmp != NULL) {
		ret = tmp->item;
	};

	Nodeache->free(tmp);
	return ret;
}

// safe.
template <class T> template <class T2>
void PrioQueue<T>::Queue<T2>::remove(T2 *item)
{
	sQueueNode	*tmp, *prev=NULL;

	q.lock.acquire();

	for (tmp=q.rsrc.head; tmp != NULL; prev = tmp, tmp = tmp->next)
	{
		// Skip until we find the item.
		if (tmp->item != item) { continue; };

		if (prev == NULL) {
			q.rsrc.head = tmp->next;
		} else {
			prev->next = tmp->next;
		};

		if (tmp->next == NULL) {
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
void PrioQueue<T>::Queue<T2>::dump(void)
{
	sQueueNode	*tmp;
	ubit8		flipFlop=0;

	q.lock.acquire();

	printf(CC"\tQueue %d, head 0x%p, tail 0x%p, %d items:\n%s",
		prio, q.rsrc.head, q.rsrc.tail, q.rsrc.nItems,
		(q.rsrc.head == NULL) ? "" : "\t");

	for (tmp = q.rsrc.head; tmp != NULL; tmp = tmp->next, flipFlop++)
	{
		if (flipFlop > 5)
		{
			printf(CC"\n\t");
			flipFlop = 0;
		};

		printf(CC"0x%p ", tmp->item);
	};

	q.lock.release();
	if (flipFlop != 0) { printf(CC"\n"); };
}

#endif

