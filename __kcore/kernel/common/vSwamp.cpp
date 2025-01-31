
#include <arch/paging.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/vSwamp.h>


error_t VSwamp::initialize(void)
{
	// Ensure the size doesn't overflow the vaddrspace.
	if ((void *)((uintptr_t)baseAddr + size) < baseAddr) {
		return ERROR_FATAL;
	};

	initSwampNode.baseAddr = baseAddr;
	initSwampNode.nPages = PAGING_BYTES_TO_PAGES(size);
	initSwampNode.prev = initSwampNode.next = NULL;

	state.rsrc.head = state.rsrc.tail = &initSwampNode;
	return swampNodeList.initialize();
}
void VSwamp::dump(void)
{
	printf(NOTICE"vSwamp: base: %p, size: %x, ", baseAddr, size);

	state.lock.acquire();

	if (state.rsrc.head == NULL)
	{
		printf(CC"invalid.\n");
		state.lock.release();
		return;
	};

	printf(CC"valid\n");

	for (SwampInfoNode *tmp = state.rsrc.head;
		tmp != NULL;
		tmp = tmp->next)
	{
		printf(CC"\tNode (v%p): baseAddr %p, nPages %x.\n",
			tmp, tmp->baseAddr, tmp->nPages);
	};

	state.lock.release();
}

void *VSwamp::getPages(uarch_t nPages, ubit32)
{
	void		*ret;

	state.lock.acquire();
	for (SwampInfoNode *currentNode = state.rsrc.head;
		currentNode != NULL;
		currentNode = currentNode->next)
	{
		if (currentNode->nPages >= nPages)
		{
			// Modify the swamp info node to show the alloc.
			currentNode->nPages -= nPages;
			ret = currentNode->baseAddr;

			currentNode->baseAddr =
				(void *)((uarch_t)currentNode->baseAddr
					+ (nPages * PAGING_BASE_SIZE));

			// If all pages on node are exhausted delete it.
			if (currentNode->nPages == 0)
			{
				// If node to be deleted is the head:
				if (currentNode == state.rsrc.head) {
					state.rsrc.head = currentNode->next;
				};

				// If node being deleted is the tail:
				if (currentNode == state.rsrc.tail) {
					state.rsrc.tail = currentNode->prev;
				};

				/* If this is the init node, it just
				 * won't be freed.
				 **/
				deleteSwampNode(currentNode);
			};

			// Return the vmem.
			state.lock.release();
			return ret;
		};
	};

	state.lock.release();
	return NULL;
}

void VSwamp::releasePages(void *vaddr, uarch_t nPages)
{
	SwampInfoNode	*insertionNode;
	status_t	status;

	if (vaddr == NULL
		|| reinterpret_cast<uarch_t>( vaddr ) & PAGING_BASE_MASK_LOW
		|| nPages == 0)
	{
		printf(WARNING OPTS(NOLOG)
			"vSwamp: ReleasePages with non-page "
			"aligned vaddr %p.\n", vaddr);

		return;
	};

	state.lock.acquire();

	insertionNode = findInsertionNode(vaddr, &status);

	// If the data structure is empty (all the process' vmem was exhausted):
	if (insertionNode == NULL)
	{

		state.lock.release();
		return;
	};

	if (status == VSWAMP_INSERT_BEFORE)
	{
		if ((void *)((uarch_t)vaddr + (nPages << PAGING_BASE_SHIFT))
			== insertionNode->baseAddr)
		{
			SwampInfoNode		*prevNode;

			// We can free directly to the existing insertion node.
			insertionNode->baseAddr = vaddr;
			insertionNode->nPages += nPages;

			if (insertionNode->prev == NULL) { goto out; };

			/* Check to see if this free also allows us to
			 * defragment the swamp even more by concatenating the
			 * preceding node as well.
			 **/
			prevNode = insertionNode->prev;

			if ((void *)((uarch_t)prevNode->baseAddr
				+ (prevNode->nPages * PAGING_BASE_SIZE))
				== insertionNode->baseAddr)
			{
				// Join the two nodes.
				insertionNode->baseAddr = prevNode->baseAddr;
				insertionNode->nPages += prevNode->nPages;

				// Ensure we keep head valid.
				if (prevNode == state.rsrc.head) {
					state.rsrc.head = insertionNode;
				};

				deleteSwampNode(prevNode);
			};
		}
		else
		{
			SwampInfoNode		*newNode;

			// Couldn't free to an existing swamp info node.
			newNode = getNewSwampNode(
				vaddr, nPages,
				insertionNode->prev, insertionNode);

			if (newNode == NULL)
			{
				// This is essentially a virtual memory leak.
				goto out;
			};

			insertionNode->prev = newNode;
			if (newNode->prev) {
				newNode->prev->next = newNode;
			}
			else
			{
				// Again, ensure we keep head valid.
				state.rsrc.head = newNode;
			};
		};
	}
	else
	{
		// Inserting at end of swamp, or list is empty.
		if (state.rsrc.head == NULL)
		{
			// List is empty; re-use the init-node.
			initSwampNode.baseAddr = vaddr;
			initSwampNode.nPages = nPages;
			initSwampNode.prev = initSwampNode.next = NULL;

			state.rsrc.head = state.rsrc.tail = &initSwampNode;
		}
		else
		{
			// Allocate a new node and append it.
			insertionNode->next = getNewSwampNode(
				vaddr, nPages, insertionNode, NULL
			);

			state.rsrc.tail = insertionNode->next;
		};
	};

out:
	state.lock.release();
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
SwampInfoNode *VSwamp::findInsertionNode(void *vaddr, status_t *status)
{
	SwampInfoNode		*altNode=NULL;

	for (SwampInfoNode *currentNode = state.rsrc.head;
		currentNode != NULL;
		currentNode = currentNode->next)
	{
		// If the current node is the insertion point:
		if (vaddr < currentNode->baseAddr)
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
void VSwamp::deleteSwampNode(SwampInfoNode *node)
{
	if (node ==NULL) {
		return;
	};

	if (node->next) {
		node->next->prev = node->prev;
	};
	if (node->prev) {
		node->prev->next = node->next;
	};

	// Don't attempt to free the initial placement-memory node.
	if (node == &initSwampNode) { return; };

	swampNodeList.free(node);
}

SwampInfoNode *VSwamp::getNewSwampNode(
	void *startAddr, uarch_t nPages, SwampInfoNode *prev,
	SwampInfoNode *next
	)
{
	SwampInfoNode		*ret;
	/* SwampInfoNode		tmp(startAddr, nPages, prev, next); */

	// Allocate a new node from the equalizer.
	/* ret = swampNodeList.addEntry(&tmp); */
	ret = new (swampNodeList.allocate()) SwampInfoNode(
		startAddr, nPages, prev, next);

	if (ret == NULL) {
		return NULL;
	};

	/* // Initialize it.
	ret->baseAddr = startAddr;
	ret->nPages = nPages;
	ret->prev = prev;
	ret->next = next; */

	return ret;
}

