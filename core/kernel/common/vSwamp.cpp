
#include <arch/paging.h>
#include <__kstdlib/__kcxxlib/cstring>
#include <__kclasses/debugPipe.h>
#include <kernel/common/vSwamp.h>

vSwampC::vSwampC(void)
{
}

vSwampC::vSwampC(void *startAddr, uarch_t swampSize, holeMapS *holeMap)
{
	initialize(startAddr, swampSize, holeMap);
}

void vSwampC::dump(void)
{
	for (uarch_t i=0; i<VSWAMP_NSWAMPS; i++)
	{
		__kdebug.printf(NOTICE"vSwamp: Swamp %d: base: %p, size: %X, ",
			i, swamps[i].baseAddr, swamps[i].size);

		swamps[i].ptrs.lock.acquire();

		if (swamps[i].ptrs.rsrc.head == __KNULL)
		{
			__kdebug.printf((utf8Char *)"invalid.\n");
			swamps[i].ptrs.lock.release();
			continue;
		};
		__kdebug.printf((utf8Char *)"valid\n");

		for (swampInfoNodeC *tmp = swamps[i].ptrs.rsrc.head;
			tmp != __KNULL; tmp = tmp->next)
		{
			__kdebug.printf(NOTICE"\tNode: baseAddr %p, nPages %p"
				"\n", tmp->startAddr, tmp->nPages);
		};

		swamps[i].ptrs.lock.release();
	};
}

//Calculate the specs for, and initialize, the swamp.
error_t vSwampC::initialize(
	void *baseAddr, uarch_t swampSize, holeMapS *holeMap
	)
{
	memset(this, 0, sizeof(*this));

	// FIXME: Proof-check these calculations for userspace process spawning.
	swamps[0].baseAddr = PAGING_BASE_ALIGN_FORWARD(
		reinterpret_cast<uarch_t>( baseAddr ));
	if (holeMap != __KNULL)
	{
		swamps[0].size = PAGING_BASE_ALIGN_TRUNCATED(
			holeMap->baseAddr - swamps[0].baseAddr);
		swamps[1].baseAddr = PAGING_BASE_ALIGN_FORWARD(
			holeMap->baseAddr + holeMap->size);
		swamps[1].size = PAGING_BASE_ALIGN_TRUNCATED(
			(reinterpret_cast<uarch_t>( baseAddr ) + swampSize) -
			swamps[1].baseAddr);

		// Spawn the init swamp node for swamp 1.
		swamps[1].ptrs.rsrc.head = swamps[1].ptrs.rsrc.tail =
			&initSwampNodes[1];
		initSwampNodes[1].prev = initSwampNodes[1].next = __KNULL;
		initSwampNodes[1].startAddr = swamps[1].baseAddr;
		initSwampNodes[1].nPages = PAGING_BYTES_TO_PAGES(
			swamps[1].size);
	}
	else
	{
		swamps[0].size = PAGING_BASE_ALIGN_TRUNCATED(swampSize);
		swamps[1].baseAddr = swamps[1].size = __KNULL;
	};
	// Spawn the init node for swamp 0.
	swamps[0].ptrs.rsrc.head = swamps[0].ptrs.rsrc.tail =
		&initSwampNodes[0];
	initSwampNodes[0].prev = initSwampNodes[0].next = __KNULL;
	initSwampNodes[0].startAddr = swamps[0].baseAddr;
	initSwampNodes[0].nPages = PAGING_BYTES_TO_PAGES(
		swamps[0].size);

	// Technically, a swamp cannot fail to initialize.
	return ERROR_SUCCESS;
}

vSwampC::~vSwampC(void)
{
}

/**	FIXME:
 * Find a way to get finer grained locking in here.
 **/
void *vSwampC::getPages(uarch_t nPages)
{
	void			*ret;
	uarch_t			i;

	for (i=0; i<VSWAMP_NSWAMPS; i++)
	{
		swamps[i].ptrs.lock.acquire();

		for (swampInfoNodeC *currentNode = swamps[i].ptrs.rsrc.head;
			currentNode != __KNULL;
			currentNode = currentNode->next)
		{
			if (currentNode->nPages >= nPages)
			{
				// Modify the swamp info node to show the alloc.
				currentNode->nPages -= nPages;
				ret = reinterpret_cast<void *>(
					currentNode->startAddr );

				currentNode->startAddr +=
					nPages * PAGING_BASE_SIZE;

				// If all pages on node are exhausted delete it.
				if (currentNode->nPages == 0)
				{
					// If node to be deleted is the head:
					if (currentNode ==
						swamps[i].ptrs.rsrc.head)
					{
						swamps[i].ptrs.rsrc.head =
							currentNode->next;
					};
					// If node being deleted is the tail:
					if (currentNode ==
						swamps[i].ptrs.rsrc.tail)
					{
						swamps[i].ptrs.rsrc.tail =
							currentNode->prev;
					};

					/* If this is the init node, it just 
					 * won't be freed.
					 **/
					deleteSwampNode(currentNode);
				};

				// Return the vmem.
				swamps[i].ptrs.lock.release();
				return ret;
			};
		};
		swamps[i].ptrs.lock.release();
	};
	return __KNULL;
}

void vSwampC::releasePages(void *vaddr, uarch_t nPages)
{
	uarch_t		s=0;
	swampInfoNodeC	*insertionNode, *prevNode;
	status_t	status;

	if (vaddr == __KNULL 
		|| reinterpret_cast<uarch_t>( vaddr ) & PAGING_BASE_MASK_LOW)
	{
		return;
	};

	// Find out which swamp we're interested in:
	if (swamps[1].baseAddr != __KNULL
		&& reinterpret_cast<uarch_t>( vaddr ) >= swamps[1].baseAddr)
	{
		s = 1;
	};

	swamps[s].ptrs.lock.acquire();

	insertionNode = findInsertionNode(&swamps[s], vaddr, &status);
	if (status == VSWAMP_INSERT_BEFORE)
	{
		if ((reinterpret_cast<uarch_t>( vaddr )
			+ (nPages * PAGING_BASE_SIZE))
			== insertionNode->startAddr)
		{
			// We can free directly to an existing node.
			insertionNode->startAddr -= nPages * PAGING_BASE_SIZE;
			insertionNode->nPages += nPages;

			/* Check to see if this free also allows us to
			 * defragment the swamp even more by joining up other
			 * swamp nodes iteratively.
			 **/
			while (insertionNode->prev != __KNULL)
			{
				prevNode = insertionNode->prev;
				if ((prevNode->startAddr
					+ (prevNode->nPages
					* PAGING_BASE_SIZE))
					== insertionNode->startAddr)
				{
					// Join the two nodes.
					insertionNode->startAddr =
						prevNode->startAddr;
					insertionNode->nPages +=
						prevNode->nPages;

					// Ensure we keep head valid.
					if (prevNode ==
						swamps[s].ptrs.rsrc.head)
					{
						swamps[s].ptrs.rsrc.head =
							insertionNode;
					};
					deleteSwampNode(prevNode);
				}
				else {
					break;
				};
			};

			goto out;
		}
		else
		{
			// Couldn't free to an existing swamp info node.
			prevNode = getNewSwampNode(
				vaddr, nPages, 
				insertionNode->prev, insertionNode);

			if (prevNode == __KNULL)
			{
				// This is essentially a virtual memory leak.
				goto out;
			};

			insertionNode->prev = prevNode;
			if (prevNode->prev) {
				prevNode->prev->next = prevNode;
			};

			// Again, ensure we keep head valid.
			if (insertionNode == swamps[s].ptrs.rsrc.head) {
				swamps[s].ptrs.rsrc.head = prevNode;
			};

			goto out;
		};
	}
	else
	{
		// Inserting at end of swamp, or swamp empty.
		if (swamps[s].ptrs.rsrc.head == __KNULL)
		{
			swamps[s].ptrs.rsrc.head = getNewSwampNode(
				vaddr, nPages, __KNULL, __KNULL
			);
			goto out;
		}
		else
		{
			insertionNode->next = getNewSwampNode(
				vaddr, nPages, insertionNode, __KNULL
			);
			swamps[s].ptrs.rsrc.tail = insertionNode->next;

			goto out;
		};
	};

out:
	swamps[s].ptrs.lock.release();
	return;
}

//You must already hold the lock before calling the next function below.
/**	EXPLANATION:
 * This function will run through all the swamp nodes until it find an insertion
 * point for the node which must be created.
 *
 * For any allocation, A, which is to be freed, the vaddr could be freed to an
 * existing swamp info node, or need to have a swamp info node created for it.
 *
 * In the case of an info node needing to be created, we need to know whether
 * it must be created before the insertion node which was returned, or after it.
 *
 * Thus the status_t *status argument, which returns a status code telling us
 * whether to insert before or after. In the case of an insert-before, there is
 * no problem since an insert-before cannot imply an empty swamp or an insertion
 * at the end of the list.
 *
 * If we get an insert-after, it could mean one of two things: the swamp was
 * completely empty and this bit of virtual memory we're currently freeing will
 * re-vitalize it, or the chunk of virtual memory we're freeing will create a
 * new swamp info node at the end of the current swamp info node list.
 *
 * Both conditions have their associated handling requirements.
 **/
swampInfoNodeC *vSwampC::findInsertionNode(
	swampS *swamp, void *vaddr, status_t *status
	)
{
	swampInfoNodeC		*altNode=0;

	for (swampInfoNodeC *currentNode = swamp->ptrs.rsrc.head;
		currentNode != __KNULL;
		currentNode = currentNode->next)
	{
		// If the current node is the insertion point:
		if (reinterpret_cast<uarch_t>( vaddr ) < currentNode->startAddr)
		{
			*status = VSWAMP_INSERT_BEFORE;
			return currentNode;
		};
		altNode = currentNode;
	};

	*status = VSWAMP_INSERT_AFTER;
	return altNode;
}

//You must already hold the lock before calling this.
void vSwampC::deleteSwampNode(swampInfoNodeC *node)
{
	if (node ==__KNULL) {
		return;
	};

	if (node->next) {
		node->next->prev = node->prev;
	};
	if (node->prev) {
		node->prev->next = node->next;
	};

	swampNodeList.removeEntry(node);
}

swampInfoNodeC *vSwampC::getNewSwampNode(
	void *startAddr, uarch_t nPages, swampInfoNodeC *prev,
	swampInfoNodeC *next
	)
{
	swampInfoNodeC		*ret;
	swampInfoNodeC		tmp =
	{
		reinterpret_cast<uarch_t>( startAddr ),
		nPages,
		prev, next
	};

	// Allocate a new node from the equalizer.
	ret = swampNodeList.addEntry(&tmp);
	if (ret == __KNULL) {
		return __KNULL;
	};

	// Initialize it.
	ret->startAddr = reinterpret_cast<uarch_t>( startAddr );
	ret->nPages = nPages;
	ret->prev = prev; ret->next = next;

	return ret;
}

