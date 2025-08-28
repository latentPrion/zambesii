
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/callback.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/process.h>
#include <kernel/common/panic.h>
#include <kernel/common/messageStream.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


#include <debug.h>
AsyncResponse::~AsyncResponse(void)
{
	MessageStream::sHeader		*msg;
	error_t				ret;

	msg = (MessageStream::sHeader *)msgHeader;

	if (msg == NULL) { return; };
	msg->error = err;
	ret = MessageStream::enqueueOnThread(msg->targetId, msg);
	if (ret != ERROR_SUCCESS)
	{
		printf(FATAL"asyncCleanup: Failed to send msg from %x to "
			"%x, with error %s because %s.\n",
			msg->sourceId, msg->targetId, strerror(msg->error),
			strerror(ret));
	};
}

ipc::sDataHeader *ipc::createDataHeader(
	void *data, uarch_t nBytes, methodE method
	)
{
	sDataHeader		*ret;

	if (method == METHOD_BUFFER) {
		ret = new (new ubit8[sizeof(*ret) + nBytes]) sDataHeader;
	} else {
		ret = new sDataHeader;
	};

	if (ret == NULL) { return NULL; };

	ret->method = method;
	ret->nBytes = nBytes;
	if (method == METHOD_BUFFER)
	{
		// For a kernel buffered copy, just set state and memcpy.
		ret->foreignVaddr = &ret[1];
		memcpy(ret->foreignVaddr, data, nBytes);
		return ret;
	};

	ret->foreignVaddr = data;
	ret->foreignTid =
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
			->getFullId();

	return ret;
}

inline static sarch_t is__kvaddrspaceProcess(processId_t pid)
{
	if (PROCID_PROCESS(pid) == __KPROCESSID
		|| PROCID_PROCESS(pid) == CPU_PROCESSID)
		{ return 1; };

	return 0;
}

#if 0
static error_t createSharedMapping(
	VaddrSpaceStream *targetVas, void *targetVaddrStart,
	VaddrSpaceStream *sourceVas, void *sourceVaddrStart,
	uarch_t nPages)
{
	for (uarch_t pageCounter=0; pageCounter < nPages;
		pageCounter++,
		sourceVaddrStart = (void *)((uintptr_t)sourceVaddrStart
			+ PAGING_BASE_SIZE),
		targetVaddrStart = (void *)((uintptr_t)targetVaddrStart
			+ PAGING_BASE_SIZE))
	{
		paddr_t		p;
		uarch_t		f;
		status_t	nMapped;

		walkerPageRanger::lookup(
			&sourceVas->vaddrSpace, sourceVaddrStart, &p, &f);

		nMapped = walkerPageRanger::mapInc(
			&targetVas->vaddrSpace, targetVaddrStart, p, 1, f);

		if (nMapped < 1) { return ERROR_MEMORY_VIRTUAL_PAGEMAP; };
	};

	return ERROR_SUCCESS;
}
#endif

static sarch_t vaddrRangeIsLoose(
	VaddrSpaceStream *vas, void *vaddr, uarch_t nPages
	)
{
	for (uarch_t i=0; i<nPages;
		i++,
		vaddr = (void *)((uintptr_t)vaddr + PAGING_BASE_SIZE))
	{
		paddr_t		p;
		uarch_t		f;

		if (walkerPageRanger::lookup(&vas->vaddrSpace, vaddr, &p, &f)
			!= WPRANGER_STATUS_UNMAPPED)
		{
			return 0;
		};
	};

	return 1;
}

error_t ipc::dispatchDataHeader(sDataHeader *header, void *buffer)
{
	ProcessStream	*currProcess, *sourceProcess;
	void		*targetMapping, *sourceMapping;
	uarch_t		sourceMappingNPages;
	processId_t	currTid;

	if (header->method == METHOD_BUFFER)
	{
		memcpy(buffer, header->foreignVaddr, header->nBytes);
		return ERROR_SUCCESS;
	};

	// targetMapping = shared mapping to be created within target process.
	targetMapping = NULL;
	// Calculate offsets and number of pages to be mapped.
	sourceMapping =
		(void *)((uintptr_t)header->foreignVaddr
			& PAGING_BASE_MASK_HIGH);

	sourceMappingNPages =
		PAGING_BYTES_TO_PAGES(header->nBytes);

	if ((uintptr_t)header->foreignVaddr & PAGING_BASE_MASK_LOW)
		{ sourceMappingNPages++; };

	if (header->nBytes & PAGING_BASE_MASK_LOW)
		{ sourceMappingNPages++; };

	sourceProcess = processTrib.getStream(header->foreignTid);
	if (sourceProcess == NULL) { return ERROR_NOT_FOUND; };

	currTid = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
		->getFullId();

	currProcess = cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentThread()->parent;

	if (header->method == METHOD_MAP_AND_COPY)
	{
		/**	EXPLANATION:
		 * For MAP_AND_COPY, the caller just passes the address of a
		 * buffer area s/he wishes for the kernel to copy the data into.
		 * The kernel takes on the burden of creating the temporary
		 * shared mapping to the source process, and tearing it down
		 * afterward.
		 *
		 **	NOTES:
		 * If both processes actually live in the same address
		 * space, we don't need to map the source memory range into the
		 * target process' addrspace, because the source data is already
		 * accessible. We need only copy, and then we can return.
		 **/
		if ((is__kvaddrspaceProcess(header->foreignTid)
				&& is__kvaddrspaceProcess(currTid))
			|| (currProcess->getVaddrSpaceStream()
				== sourceProcess->getVaddrSpaceStream()))
		{
			memcpy(buffer, header->foreignVaddr, header->nBytes);
			return ERROR_SUCCESS;
		};
	};

	if (header->method == ipc::METHOD_MAP_AND_READ)
	{
		/**	EXPLANATION:
		 * For MAP_AND_READ, the caller passes a range of loose pages
		 * when calling receive(). That range is passed here as the
		 * variable "buffer".
		 *
		 * 1. Check the vaddr range provided with the vaddrSpaceStream
		 *    for the receiving process to ensure that it has been
		 *    previously handed out by the kernel.
		 * 2. Check the vaddr range provided to ensure that it is
		 *    indeed loose (unmapped) vmem.
		 * 3. Map the page range from the source process into the
		 *    receiving process' addrspace.
		 * 4. Return and wait for the release() call.
		 *
		 **	FIXME:
		 * Make sure to check to see whether or not the target process'
		 * loose-page range has been previously handed out by the
		 * process' VaddrSpace Stream (step 1).
		 **/
		targetMapping = buffer;

		// The target vmem *must* be loose (unmapped) vmem.
		if (!vaddrRangeIsLoose(
			currProcess->getVaddrSpaceStream(),
			targetMapping, sourceMappingNPages))
		{ return ERROR_INVALID_OPERATION; };
	};

	// Allocate vmem region to map to foreign vmem region.
	targetMapping = walkerPageRanger::createSharedMappingTo(
		&sourceProcess->getVaddrSpaceStream()->vaddrSpace,
		sourceMapping, sourceMappingNPages,
		PAGEATTRIB_PRESENT
		| ((currProcess->execDomain == PROCESS_EXECDOMAIN_KERNEL)
			? PAGEATTRIB_SUPERVISOR : 0),
		targetMapping);

	if (targetMapping == NULL)
	{
		printf(FATAL"ipc::dispatchDataHeader: Failed to create shared "
			"mapping.\n");

		return ERROR_UNKNOWN;
	};

	// Adjust the vaddr to re-add the offset.
	targetMapping = WPRANGER_ADJUST_VADDR(
		targetMapping,
		paddr_t((uintptr_t)header->foreignVaddr), void *);

	// For MAP_AND_READ, we're now done.
	if (header->method == METHOD_MAP_AND_READ) { return ERROR_SUCCESS; };

	/* Else it was MAP_AND_COPY between two processes in different
	 * vaddrspaces. So now we must copy, then unmap.
	 **/
	paddr_t		p;
	uarch_t		f;

	memcpy(buffer, targetMapping, header->nBytes);
	walkerPageRanger::unmap(
		&currProcess->getVaddrSpaceStream()->vaddrSpace,
		targetMapping, &p, sourceMappingNPages, &f);

	currProcess->getVaddrSpaceStream()->releasePages(
		targetMapping, sourceMappingNPages);

	return ERROR_SUCCESS;
}

void ipc::destroyDataHeader(sDataHeader *header, void *buffer)
{
	/**	EXPLANATION:
	 * For BUFFER, we don't have to do anything.
	 * For MAP_AND_COPY, we don't have to do anything.
	 * For MAP_AND_READ, we have to unmap the loose page range.
	 **/
	if (header->method == METHOD_MAP_AND_READ)
	{
		paddr_t		p;
		uarch_t		f, sourceMappingNPages;

		sourceMappingNPages =
			PAGING_BYTES_TO_PAGES(header->nBytes);

		if ((uintptr_t)header->foreignVaddr & PAGING_BASE_MASK_LOW)
			{ sourceMappingNPages++; };

		if (header->nBytes & PAGING_BASE_MASK_LOW)
			{ sourceMappingNPages++; };

		walkerPageRanger::unmap(
			&cpuTrib.getCurrentCpuStream()->taskStream
				.getCurrentThread()->parent
				->getVaddrSpaceStream()->vaddrSpace,
			buffer, &p, sourceMappingNPages, &f);
	};

	delete header;
}

sbit8 MessageStream::Filter::compare(const sHeader *hdr) const
{
	if ((comparisonFlags & FLAG_SOURCE_ID) &&
		hdr->sourceId != criteria.sourceId) {
		return false;
	}
	if ((comparisonFlags & FLAG_TARGET_ID) &&
		hdr->targetId != criteria.targetId) {
		return false;
	}
	if ((comparisonFlags & FLAG_PRIVATE_DATA) &&
		hdr->privateData != criteria.privateData) {
		return false;
	}
	if ((comparisonFlags & FLAG_SUBSYSTEM) &&
		(hdr->subsystem > MSGSTREAM_SUBSYSTEM_MAXVAL ||
		hdr->subsystem != criteria.subsystem)) {
		return false;
	}
	if ((comparisonFlags & FLAG_FUNCTION) &&
		hdr->function != criteria.function) {
		return false;
	}
	if ((comparisonFlags & FLAG_SIZE) &&
		hdr->size != criteria.size) {
		return false;
	}
	if ((comparisonFlags & FLAG_ERROR) &&
		hdr->error != criteria.error) {
		return false;
	}
	if ((comparisonFlags & FLAG_FLAGS) &&
		hdr->flags != criteria.flags) {
		return false;
	}
	return true;
}

MessageStream::sHeader::sHeader(
	processId_t targetPid, ubit16 subsystem, ubit16 function,
	uarch_t size, uarch_t flags, void *privateData)
:
privateData(privateData),
subsystem(subsystem), flags(0), function(function), size(size)
{
	sourceId = determineSourceThreadId(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread(),
		&this->flags);

	targetId = determineTargetThreadId(
		targetPid, sourceId, flags, &this->flags);

#if 0
	if (size > sizeof(sIterator))
	{
		printf(FATAL MSGSTREAM"sHeader con: size %d exceeds "
			"sizeof(sHeader) (%d)\n"
			"\tCaller %p: subsystem %d, function %d, from T%x "
			"to T%x\n",
			size, sizeof(sHeader),
			__builtin_return_address(0),
			subsystem, function, sourceId, targetId);

		panic(ERROR_LIMIT_OVERFLOWED);
	};
#endif
}

void MessageStream::dump(void)
{
	printf(NOTICE MSGSTREAM"@%p: Dumping.\n", this);
	state.rsrc.pendingSubsystems.dump();
	for (uarch_t i=0; i<MSGSTREAM_SUBSYSTEM_MAXVAL + 1; i++) {
		state.rsrc.queues[i].dump();
	}
}

processId_t MessageStream::determineSourceThreadId(Thread *caller, ubit16 *)
{
	return caller->getFullId();
}

processId_t MessageStream::determineTargetThreadId(
	processId_t targetId, processId_t sourceId,
	uarch_t callerFlags, ubit16 *messageFlags
	)
{
	(void)sourceId;
	(void)callerFlags;
	(void)messageFlags;

	return targetId;
}

error_t MessageStream::enqueueOnThread(
	processId_t targetStreamId, MessageStream::sHeader *header
	)
{
	Thread			*targetThread;

	targetThread = processTrib.getThread(targetStreamId);
	if (targetThread == NULL) {
		return ERROR_INVALID_RESOURCE_NAME;
	};

	return targetThread->messageStream.enqueue(header->subsystem, header);
}

error_t MessageStream::pullFrom(
	ubit16 subsystemQueue, MessageStream::sHeader **message,
	ubit32 flags, const MessageStream::Filter *filter
	)
{
	if (message == NULL) { return ERROR_INVALID_ARG; };
	if (subsystemQueue > MSGSTREAM_SUBSYSTEM_MAXVAL)
		{ return ERROR_INVALID_ARG_VAL; };

	// Create a filter that matches only the specified subsystem
	MessageStream::Filter subsystemFilter(
		0, subsystemQueue, 0, 0, 0, NULL, 0);

	if (filter != NULL) {
		memcpy(&subsystemFilter, filter, sizeof(MessageStream::Filter));
	}
	
	/* Override the subsystem setting with parameter regardless of what the
	 * filter object has set.
	 * I.e: subsystemQueue overrides filter->criteria.subsystem.
	 **/
	subsystemFilter.criteria.subsystem = subsystemQueue;
	subsystemFilter.comparisonFlags |=
		MessageStream::Filter::FLAG_SUBSYSTEM;

	return pull(message, flags, &subsystemFilter);
}

error_t MessageStream::pull(
	MessageStream::sHeader **message,
	ubit32 flags, const MessageStream::Filter *filter
	)
{
	MessageStream::sHeader	*tmp=NULL;
	MultipleReaderLock	*schedStateLock;

	if (message == NULL) { return ERROR_INVALID_ARG; };

	// If CALLER_SCHEDLOCKED is set, forcibly set DONT_BLOCK.
	if (FLAG_TEST(flags, ZCALLBACK_PULL_FLAGS_CALLER_SCHEDLOCKED) &&
		!FLAG_TEST(flags, ZCALLBACK_PULL_FLAGS_DONT_BLOCK))
	{
		flags |= ZCALLBACK_PULL_FLAGS_DONT_BLOCK;
		printf(WARNING MSGSTREAM"pull called with CALLER_SCHEDLOCKED "
			"but without DONT_BLOCK. Forcibly setting DONT_BLOCK.\n");
	}

	schedStateLock = &parent->schedState.lock;

	for (;FOREVER;)
	{
		/**	EXPLANATION:
		 * If the caller is passes in CALLER_SCHEDLOCKED, it means they
		 * plan to manually manage the dequeuing thread's schedState
		 * lock. They'll acquire it and free it themself, so we
		 * shouldn't acquire it here.
		 **/
		if (FLAG_TEST(flags, ZCALLBACK_PULL_FLAGS_CALLER_SCHEDLOCKED))
			{ schedStateLock = NULL; }

		state.lock.acquire();
		MultipleReaderLock::ScopedWriteGuard schedStateGuard(
			schedStateLock);

		state.lock.giveOwnershipOfLocalIrqsTo(schedStateLock);

		for (ubit16 i=0; i<MSGSTREAM_SUBSYSTEM_MAXVAL + 1; i++)
		{
			if (!state.rsrc.pendingSubsystems.test(i))
				{ continue; };

			if (filter != NULL)
			{
				if (filter->comparisonFlagIsSet(
					MessageStream::Filter::FLAG_SUBSYSTEM)
					&& i != filter->criteria.subsystem)
				{
					/* Return early if filter has subsystem
					 * set and the subsystem does not match.
					 **/
					continue;
				}

				/* Search all messages in the queue for a match.
				 * If we find one, remove it from the queue and
				 * return it to the caller.
				 **/
				for (MessageQueue::Iterator it =
					state.rsrc.queues[i].begin();
					it != state.rsrc.queues[i].end(); ++it)
				{
					if (!filter->compare(*it))
						{ continue; };

					tmp = *it;
					state.rsrc.queues[i].removeItem(
						*it,
						PTRDBLLIST_OP_FLAGS_UNLOCKED);

					break;
				}
			}
			else
			{
				tmp = state.rsrc.queues[i].popFromHead(
					PTRDBLLIST_OP_FLAGS_UNLOCKED);
			}

			if (state.rsrc.queues[i].unlocked_getNItems() == 0) {
				state.rsrc.pendingSubsystems.unset(i);
			}

			if (tmp == NULL) { continue; }

			/* This should only be unlocked inside of this nested
			 * loop if we've found a message that should be returned
			 * to the caller.
			 *
			 * If we don't find a message, we'll unlock it below
			 * in the outer loop.
			 **/
			state.lock.release();

			// Very useful checks here for sanity.
			assert_fatal(tmp->size >= sizeof(*tmp));
#if 0
			assert_fatal(
				tmp->size <= sizeof(MessageStream::sHeader));
#endif

			*message = tmp;
			return ERROR_SUCCESS;
		};

		state.lock.release();

		if (FLAG_TEST(flags, ZCALLBACK_PULL_FLAGS_DONT_BLOCK))
			{ return ERROR_WOULD_BLOCK; };

		taskTrib.block(&schedStateGuard);
	};
}

error_t MessageStream::pullAndDispatchUntil(
	MessageStream::sHeader **message, ubit32 flags,
	const MessageStream::Filter *filter, DispatchFn *dispatchFn
	)
{
	error_t ret;
	
	for (;FOREVER;)
	{
		ret = pull(message, flags, filter);
		if (ret != ERROR_SUCCESS)
		{
			printf(ERROR MSGSTREAM"%d: pullAndDispatchUntil: "
				"pull() returned %d.\n",
				parent->getFullId(), ret);
			return ret;
		}

		if (dispatchFn(*message) != 0)
		{
			return ERROR_SUCCESS;
		}
	}
}

error_t MessageStream::postUserQMessage(
	processId_t tid, ubit16 userQId, ubit16 messageNo, void *data,
	void *privateData, error_t errorVal
	)
{
	MessageStream::sPostMsg		*message;
	processId_t			currTid;

	currTid = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
		->getFullId();

	if (userQId > MSGSTREAM_USERQ_MAXVAL)
		{ return ERROR_LIMIT_OVERFLOWED; };

	/**	NOTE:
	 * I thought about removing this restriction when we made CPUs have
	 * their own threads, but then CPUs shouldn't be using the postUserQMessage()
	 * facility to post messages to kernel threads, and vice-versa.
	 *
	 * postUserQMessage() when used by kernel threads should be used to post
	 * messages between *related* groups of kernel threads. CPUs should only
	 * use postUserQMessage() to post messages to *other* CPUs.
	 **/
	if (PROCID_PROCESS(tid) != PROCID_PROCESS(currTid))
		{ return ERROR_UNAUTHORIZED; };

	message = new MessageStream::sPostMsg(
		tid, MSGSTREAM_USERQ(userQId), messageNo,
		sizeof(*message), 0, privateData);

	if (message == NULL) { return ERROR_MEMORY_NOMEM; };
	message->set(data, errorVal);

	return enqueueOnThread(tid, &message->header);
}

error_t	MessageStream::enqueue(ubit16 queueId, MessageStream::sHeader *callback)
{
	error_t		ret;

	if (callback == NULL) { return ERROR_INVALID_ARG; };
	if (queueId > MSGSTREAM_SUBSYSTEM_MAXVAL) {
		return ERROR_INVALID_ARG_VAL;
	};

#if 0
	if (callback->size > sizeof(sHeader))
	{
		printf(FATAL MSGSTREAM"sHeader con: size %d exceeds "
			"sizeof(sHeader) (%d)\n"
			"\tCaller %p: subsystem %d, function %d, from T%x "
			"to T%x\n",
			callback->size, sizeof(sHeader),
			__builtin_return_address(0),
			callback->subsystem, callback->function,
			callback->sourceId, callback->targetId);

		panic(ERROR_LIMIT_OVERFLOWED);
	};
#endif

	/**	TODO:
	 * Think about this type of situation and determine whether or not it's
	 * safe to execute within the critical section with interrupts enabled
	 * on the local CPU. Most likely not though. Low priority as well
	 * since this is mostly a throughput optimization and not a
	 * functionality tweak.
	 **/
	state.lock.acquire();
	MultipleReaderLock::ScopedReadGuard 	threadSchedStateGuard(
		&parent->schedState.lock);

	state.lock.giveOwnershipOfLocalIrqsTo(&threadSchedStateGuard.flags);

	ret = state.rsrc.queues[queueId].addItem(
		callback, PTRDBLLIST_ADD_TAIL, PTRDBLLIST_OP_FLAGS_UNLOCKED);

	if (ret != ERROR_SUCCESS)
	{
		state.lock.release();
		return ret;
	}

	state.rsrc.pendingSubsystems.set(queueId);

	/**	FIXME:
	 * I suspect that we can release state.lock here, but I'm not sure how
	 * exactly pendingSubsystems is used inside of pull(), and whether
	 * unlocking here will cause lost wakeups. Examine this and see if we
	 * can unlock state.lock earlier up here.
	 *
	 * (We used to release state.lock after calling unblock() below).
	 **/
	state.lock.release();

	/**	EXPLANATION:
	 * Unblock the thread atomically with respect to this enqueue() call's
	 * queue operation.
	 *
	 * Unblock() will release the threadSchedStateGuard on our behalf after
	 * internally manipulating the sched queues. This has the effect of
	 * making the thread unblock() operation atomic with respect to this
	 * enqueue() call's queue operation.
	 **/
	taskTrib.unblock(parent, &threadSchedStateGuard);
	return ERROR_SUCCESS;
}

