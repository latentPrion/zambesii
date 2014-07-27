
#include <arch/walkerPageRanger.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/process.h>
#include <kernel/common/panic.h>
#include <kernel/common/messageStream.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>


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
		printf(FATAL"asyncCleanup: Failed to send msg from 0x%x to "
			"0x%x, with error %s because %s.\n",
			msg->sourceId, msg->targetId, strerror(msg->error),
			strerror(ret));
	};
}

Syscallback *newSyscallback(syscallbackDataF *fn, void *data)
{
	return new (asyncContextCache->allocate()) Syscallback(fn, data);
}

Syscallback *newSyscallback(syscallbackFuncF *fn, void (*func)())
{
	return new (asyncContextCache->allocate()) Syscallback(fn, func);
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
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTaskId();

	return ret;
}

inline static sarch_t is__kvaddrspaceProcess(processId_t pid)
{
	if (PROCID_PROCESS(pid) == __KPROCESSID
		|| PROCID_PROCESS(pid) == 0)
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

	currTid = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTaskId();
	currProcess = cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentTask()->parent;

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
		(uintptr_t)header->foreignVaddr, void *);

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
				.getCurrentTask()->parent->getVaddrSpaceStream()
				->vaddrSpace,
			buffer, &p, sourceMappingNPages, &f);
	};

	delete header;
}

MessageStream::sHeader::sHeader(
	processId_t targetPid, ubit16 subsystem, ubit16 function,
	uarch_t size, uarch_t flags, void *privateData)
:
privateData(privateData),
subsystem(subsystem), flags(0), function(function), size(size)
{
	sourceId = determineSourceThreadId(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask(),
		&this->flags);

	targetId = determineTargetThreadId(
		targetPid, sourceId, flags, &this->flags);

	if (size > sizeof(sIterator))
	{
		printf(FATAL MSGSTREAM"sHeader con: size %d exceeds "
			"sizeof(sIterator) (%d)\n"
			"\tCaller 0x%p: subsystem %d, function %d, from T0x%x "
			"to T0x%x\n",
			size, sizeof(sIterator),
			__builtin_return_address(0),
			subsystem, function, sourceId, targetId);

		panic(ERROR_LIMIT_OVERFLOWED);
	};
}

processId_t MessageStream::determineSourceThreadId(
	Task *caller, ubit16 *flags
	)
{
	if (caller->getType() == task::PER_CPU)
	{
		FLAG_SET(*flags, MSGSTREAM_FLAGS_CPU_SOURCE);
		return cpuTrib.getCurrentCpuStream()->cpuId;
	}
	else { return ((Thread *)caller)->getFullId(); };
}

processId_t MessageStream::determineTargetThreadId(
	processId_t targetId, processId_t sourceId,
	uarch_t callerFlags, ubit16 *messageFlags
	)
{
	// If target thread is a CPU:
	if (FLAG_TEST(callerFlags, MSGSTREAM_FLAGS_CPU_TARGET))
	{
		FLAG_SET(*messageFlags, MSGSTREAM_FLAGS_CPU_TARGET);
		return targetId;
	};

	if (targetId == 0) { return sourceId; };
	return targetId;
}

error_t MessageStream::enqueueOnThread(
	processId_t targetStreamId, MessageStream::sHeader *header
	)
{
	MessageStream	*targetStream;

	if (FLAG_TEST(header->flags, MSGSTREAM_FLAGS_CPU_TARGET))
	{
		cpuStream		*cs;

		/* Dealing with an asynchronous response to an API call from a
		 * per-CPU thread.
		 **/
		cs = cpuTrib.getStream((cpu_t)targetStreamId);
		if (cs == NULL) { return ERROR_INVALID_RESOURCE_NAME; };

		targetStream = &cs->getTaskContext()->messageStream;
	}
	else
	{
		ProcessStream		*targetProcess;
		Thread			*targetThread;

		/* Dealing with an asynchronous response to an API call from a
		 * normal unique-context thread.
		 **/
		targetProcess = processTrib.getStream(targetStreamId);
		if (targetProcess == NULL) {
			return ERROR_INVALID_RESOURCE_NAME;
		};

		targetThread = targetProcess->getThread(targetStreamId);
		if (targetThread == NULL) {
			return ERROR_INVALID_RESOURCE_NAME;
		};

		targetStream = &targetThread->getTaskContext()->messageStream;
	};

	return targetStream->enqueue(header->subsystem, header);
}

error_t MessageStream::pullFrom(
	ubit16 subsystemQueue, MessageStream::sIterator *callback,
	ubit32 flags
	)
{
	MessageStream::sHeader	*tmp;

	if (callback == NULL) { return ERROR_INVALID_ARG; };
	if (subsystemQueue > MSGSTREAM_SUBSYSTEM_MAXVAL)
		{ return ERROR_INVALID_ARG_VAL; };

	for (;;)
	{
		pendingSubsystems.lock();

		if (pendingSubsystems.test(subsystemQueue))
		{
			tmp = queues[subsystemQueue].popFromHead();
			if (queues[subsystemQueue].getNItems() == 0) {
				pendingSubsystems.unset(subsystemQueue);
			};

			pendingSubsystems.unlock();

			memcpy(callback, tmp, tmp->size);
			delete tmp;
			return ERROR_SUCCESS;
		};

		if (FLAG_TEST(flags, ZCALLBACK_PULL_FLAGS_DONT_BLOCK))
		{
			pendingSubsystems.unlock();
			return ERROR_WOULD_BLOCK;
		};

		Lock::sOperationDescriptor	unlockDescriptor(
			&pendingSubsystems.bmp.lock,
			Lock::sOperationDescriptor::WAIT);

		taskTrib.block(&unlockDescriptor);
	};
}

error_t MessageStream::pull(
	MessageStream::sIterator *callback, ubit32 flags
	)
{
	MessageStream::sHeader	*tmp;

	if (callback == NULL) { return ERROR_INVALID_ARG; };

	for (;;)
	{
		pendingSubsystems.lock();

		for (ubit16 i=0; i<MSGSTREAM_SUBSYSTEM_MAXVAL + 1; i++)
		{
			if (pendingSubsystems.test(i))
			{
				tmp = queues[i].popFromHead();
				if (queues[i].getNItems() == 0) {
					pendingSubsystems.unset(i);
				};

				pendingSubsystems.unlock();

				// Very useful checks here for sanity.
				assert_fatal(tmp->size >= sizeof(*tmp));
				assert_fatal(
					tmp->size <= sizeof(MessageStream::sIterator));

				memcpy(callback, tmp, tmp->size);

				delete tmp;
				return ERROR_SUCCESS;
			};
		};

		if (FLAG_TEST(flags, ZCALLBACK_PULL_FLAGS_DONT_BLOCK))
		{
			pendingSubsystems.unlock();
			return ERROR_WOULD_BLOCK;
		};

		Lock::sOperationDescriptor	unlockDescriptor(
			&pendingSubsystems.bmp.lock,
			Lock::sOperationDescriptor::WAIT);

		taskTrib.block(&unlockDescriptor);
	};
}

error_t MessageStream::postMessage(
	processId_t tid, ubit16 userQId, ubit16 messageNo, void *data
	)
{
	MessageStream::sHeader		*message;
	processId_t			currTid;

	currTid = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTaskId();

	if (userQId > MSGSTREAM_USERQ_MAXVAL)
		{ return ERROR_LIMIT_OVERFLOWED; };

	if (PROCID_PROCESS(tid) != PROCID_PROCESS(currTid))
		{ return ERROR_UNAUTHORIZED; };

	message = new MessageStream::sHeader(
		tid, MSGSTREAM_USERQ(userQId), messageNo,
		sizeof(*message), 0, data);

	if (message == NULL) { return ERROR_MEMORY_NOMEM; };

	return enqueueOnThread(tid, message);
}

error_t	MessageStream::enqueue(ubit16 queueId, MessageStream::sHeader *callback)
{
	error_t		ret;

	if (callback == NULL) { return ERROR_INVALID_ARG; };
	if (queueId > MSGSTREAM_SUBSYSTEM_MAXVAL) {
		return ERROR_INVALID_ARG_VAL;
	};

	if (callback->size > sizeof(sIterator))
	{
		printf(FATAL MSGSTREAM"sHeader con: size %d exceeds "
			"sizeof(sIterator) (%d)\n"
			"\tCaller 0x%p: subsystem %d, function %d, from T0x%x "
			"to T0x%x\n",
			callback->size, sizeof(sIterator),
			__builtin_return_address(0),
			callback->subsystem, callback->function,
			callback->sourceId, callback->targetId);

		panic(ERROR_LIMIT_OVERFLOWED);
	};

	/**	TODO:
	 * Think about this type of situation and determine whether or not it's
	 * safe to execute within the critical section with interrupts enabled
	 * on the local CPU. Most likely not though. Low priority as well
	 * since this is mostly a throughput optimization and not a
	 * functionality tweak.
	 **/
	pendingSubsystems.lock();

	ret = queues[queueId].addItem(callback);
	if (ret == ERROR_SUCCESS)
	{
		pendingSubsystems.set(queueId);
		/* Unblock the thread. This may be a normal thread, or a per-cpu
		 * thread. In the case of it being a normal thread, no extra
		 * work is required: just unblock() it.
		 *
		 * If it's a per-CPU thread, we need to unblock it /on the
		 * target CPU/.
		 **/
		if (parent->contextType == task::PER_CPU) {
			taskTrib.unblock(parent->parent.cpu);
		} else {
			taskTrib.unblock(parent->parent.thread);
		};
	};

	pendingSubsystems.unlock();
	return ret;
}

