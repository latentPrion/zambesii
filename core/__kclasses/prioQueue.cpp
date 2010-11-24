
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/prioQueue.h>


prioQueueC::prioQueueC(ubit16 nPriorities)
:
nPrios(nPriorities)
{
	nodeCache = __KNULL;
	q.rsrc.prios = __KNULL;
	q.rsrc.head = __KNULL;
}

prioQueueC::~prioQueueC(void)
{
}

error_t prioQueueC::initialize(void)
{
	nodeCache = cachePool.createCache(sizeof(prioQueueNodeS));
	if (nodeCache == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	q.rsrc.prios = new prioQueueNodeS*[nPrios];
	if (q.rsrc.prios == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	memset8(q.rsrc.prios, 0, sizeof(*q.rsrc.prios) * nPrios);
	return ERROR_SUCCESS;
}

error_t prioQueueC::insert(void *item, ubit16 prio, ubit32 opt)
{
	sbit16		greaterPrio, lesserPrio;
	prioQueueNodeS	*newNode, *adjacentNode;

	if (item == __KNULL || prio >= nPrios) {
		return ERROR_INVALID_ARG_VAL;
	};

	newNode = new (nodeCache->allocate()) prioQueueNodeS;
	if (newNode == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	newNode->item = item;
	newNode->prio = prio;

	q.lock.acquire();

	greaterPrio = getNextGreaterPrio(prio);
	lesserPrio = getNextLesserPrio(prio);
	if (__KFLAG_TEST(opt, PRIOQUEUE_INSERT_INFRONT))
	{
		if (greaterPrio == -1) { q.rsrc.head = newNode;	}
		else
		{
			getLastNodeIn(q.rsrc.prios[greaterPrio], greaterPrio)
				->next = newNode;
		};

		if (q.rsrc.prios[prio] == __KNULL)
		{
			if (lesserPrio == -1) { newNode->next = __KNULL; }
			else {
				newNode->next = q.rsrc.prios[lesserPrio];
			};
		}
		else {
			newNode->next = q.rsrc.prios[prio];
		};
		q.rsrc.prios[prio] = newNode;
	}
	else
	{
		adjacentNode = getLastNodeIn(q.rsrc.prios[prio], prio);
		// If the target list is empty:
		if (adjacentNode == __KNULL)
		{
			// Check to see if we're actually creating a new head.
			if (greaterPrio == -1) {
				q.rsrc.head = newNode;
			}
			else
			{
				/* Else point the last node of the next greater
				 * prio's item list to the new node.
				 **/
				getLastNodeIn(
					q.rsrc.prios[greaterPrio], greaterPrio)
					->next = newNode;
			};
			// And finally point target prio list to new node.
			q.rsrc.prios[prio] = newNode;
		}
		else {
			adjacentNode->next = newNode;
		};

		// Point new node's 'next' to next lesser prio's first node.
		if (lesserPrio == -1) { newNode->next = __KNULL; }
		else {
			newNode->next = q.rsrc.prios[lesserPrio];
		};
	};
				
	q.lock.release();
	return ERROR_SUCCESS;
}

void *prioQueueC::pop(void)
{
	prioQueueNodeS	*retNode;
	void		*ret;
	ubit16		prio;

	/* This whole function just tries to advance the "head" pointer one
	 * place forward.
	 **/

	q.lock.acquire();

	retNode = q.rsrc.head;
	if (retNode == __KNULL)
	{
		q.lock.release();
		return __KNULL;
	};

	// Get the item's priority.
	prio = retNode->prio;
	if ((retNode->next != __KNULL) && (retNode->next->prio == prio)) {
		q.rsrc.prios[prio] = retNode->next;
	}
	else {
		q.rsrc.prios[prio] = __KNULL;
	};
	// Advance the head pointer.
	q.rsrc.head = retNode->next;

	q.lock.release();

	// Free the queue node.
	ret = retNode->item;
	nodeCache->free(retNode);
	return ret;
}

void prioQueueC::remove(void *item, ubit16 prio)
{
	prioQueueNodeS	*curNode, *prevNode;
	sbit16		lesserPrio, greaterPrio;

	if (prio >= nPrios) { return; };

	prevNode = __KNULL;

	q.lock.acquire();

	lesserPrio = getNextLesserPrio(prio);
	greaterPrio = getNextGreaterPrio(prio);
	curNode = q.rsrc.prios[prio];

	for (; (curNode != __KNULL) && (curNode->prio == prio); )
	{
		// If we've found the item:
		if (curNode->item == item)
		{
			if (prevNode == __KNULL)
			{
				if (greaterPrio != -1)
				{
					getLastNodeIn(
						q.rsrc.prios[greaterPrio],
						greaterPrio)->next
							= curNode->next;
				}
				else {
					q.rsrc.head = curNode->next;
				};
				q.rsrc.prios[prio] = curNode->next;
			}
			else {
				prevNode->next = curNode->next;
			};

			// Now free the node and exit.
			q.lock.release();

			nodeCache->free(curNode);
			return;
		};

		prevNode = curNode;
		curNode = curNode->next;
	};

	// The list on 'prio' was empty, or the item wasn't found on it.
	q.lock.release();
}

// Lock is expected to be held before calling this.
prioQueueC::prioQueueNodeS *prioQueueC::getLastNodeIn(
	prioQueueNodeS *list, ubit16 listPrio
	)
{
	if (list == __KNULL) {
		return __KNULL;
	};

	for (; (list->next != __KNULL) && (list->next->prio == listPrio);
		list = list->next)
	{};
	return list;
}

// Lock must be held before calling this.
sbit16 prioQueueC::getNextGreaterPrio(ubit16 prio)
{
	for (sbit16 ret = static_cast<sbit16>( prio + 1 );
		ret < static_cast<sbit16>( nPrios );
		ret++)
	{
		if (q.rsrc.prios[ret] != __KNULL) {
			return ret;
		};
	};

	return -1;
}

sbit16 prioQueueC::getNextLesserPrio(ubit16 prio)
{
	for (sbit16 ret = (static_cast<sbit16>( prio ) - 1);
		ret >= 0;
		ret--)
	{
		if (q.rsrc.prios[ret] != __KNULL) {
			return ret;
		};
	};

	return -1;
}

