
#include <__kclasses/singleWaiterQueue.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/thread.h>
#include <kernel/common/taskTrib/taskTrib.h>


error_t SingleWaiterQueue::addItem(void *item)
{
	error_t		ret;

	/**	EXPLANATION:
	 * The enqueue and unblock() do not have to be atomically executed,
	 * unlike the check and block() inside of pop(). The reason is that
	 * even if the following happens:
	 *
	 *	(1) T1 => enqueue(item1)
	 *	(2) T1 => unblock(Tserver);
	 *	(3) T1 => enqueue(item2);
	 *	(4) Tserver => wakeup and pop(item1); process(item1);
	 *	(5) Tserver => wakeup and pop(item2); process(item2);
	 *	(6) T1 => unblock(Tserver);
	 *	(7) Tserver => wakeup and check and find nothing.
	 *
	 * When #7 happens, there's no risk because when Tserver finds nothing
	 * it'll just block itself again. I.e, it is fine for the server thread
	 * to be signaled and unblocked for no reason. It's not optimal, but
	 * the server thread can deal with it by checking to ensure that it
	 * actually got some data to process from the client.
	 */
	state.lock.acquire();

	if (state.rsrc.thread == NULL)
	{
		state.lock.release();

		printf(ERROR SWAITQ"addItem failed because no waiting thread "
			"has been set.\n");

		return ERROR_UNINITIALIZED;
	};

	// Thread's schedState lock prevents lost wakeups from race conditions.
	MultipleReaderLock::ScopedReadGuard	threadSchedStateGuard(
		&state.rsrc.thread->schedState.lock);

	ret = HeapDoubleList<void>::addItem(
		item, PTRDBLLIST_ADD_TAIL, PTRDBLLIST_OP_FLAGS_UNLOCKED);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR SWAITQ"addItem(%p) failed. Err %d (%s).\n",
			item, ret, strerror(ret));

		return ret;
	};

	/**	EXPLANATION:
	 * Atomically unblock the waiting thread with respect to this
	 * addItem() call's queue operation.
	 *
	 * TaskTrib::unblock() will release the threadSchedStateGuard on our
	 * behalf after internally manipulating the sched queues. This has the
	 * effect of making the thread unblock() operation atomic with respect to
	 * this addItem() call's queue operation.
	 */
	Lock::sOperationDescriptor	unlockDescriptor(
		&state.lock,
		Lock::sOperationDescriptor::WAIT);

	threadSchedStateGuard.releaseManagementAndUnlock();
	ret = taskTrib.unblock(reinterpret_cast<Thread *>(
		atomicAsm::read(&state.rsrc.thread)), &unlockDescriptor);

	if (ret != ERROR_SUCCESS)
		{ panic(ret, FATAL SWAITQ"Failed to unblock thread!"); }

	return ERROR_SUCCESS;
}

error_t SingleWaiterQueue::pop(void **item, uarch_t flags)
{
	Thread		*currThread;

	/**	EXPLANATION:
	 * The pop operation must atomically do both the check and the block.
	 * The block() call must be atomic with respect to the check because
	 * if the the check and the block() weren't atomic, then the following
	 * could happen:
	 *
	 *	(1) Tserver => check queue, and find that it's empty.
	 *	(2) T1 => enqueue(item1) and unblock(Tserver);
	 *	(3) Tserver => proceed to call block(), not having re-checked
	 * 	    queue.
	 *
	 * During step #2, when T1 calls unblock(Tserver), the kernel would
	 * treat the unblock(Tserver) call as a no-op since Tserver is still
	 * running and hasn't actually blocked itself as yet -- it will block
	 * itself in step #3.
	 *
	 * Because of this, when Tserver executes #3, it will lose the wakeup
	 * that it should have gotten during #2, and the newly enqueued item
	 * will never be taken out of the queue.
	 *
	 * Therefore the check and block() must be atomic.
	 */
	currThread = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	for (;FOREVER;)
	{
		// Prevent lost wakeups from race conditions.
		state.lock.acquire();

		if (state.rsrc.thread == NULL)
		{
			state.lock.release();

			printf(ERROR SWAITQ"pop failed because no waiting thread "
				"has been set.\n");

			return ERROR_UNINITIALIZED;
		};

		if (state.rsrc.thread != currThread)
		{
			state.lock.release();

			printf(ERROR SWAITQ"pop called by thread %x, but "
				"waiting thread is %x.\n",
				currThread->getFullId(),
				state.rsrc.thread->getFullId());

			return ERROR_UNINITIALIZED;
		};

		*item = HeapDoubleList<void>::popFromHead(
			PTRDBLLIST_OP_FLAGS_UNLOCKED);

		if (*item != NULL)
		{
			state.lock.release();
			return ERROR_SUCCESS;
		};

		if (FLAG_TEST(flags, SINGLEWAITERQ_POP_FLAGS_DONTBLOCK))
		{
			state.lock.release();
			return ERROR_WOULD_BLOCK;
		};

		Lock::sOperationDescriptor	unlockDescriptor(
			&state.lock,
			Lock::sOperationDescriptor::WAIT);

		taskTrib.block(&unlockDescriptor);
	};
}

error_t SingleWaiterQueue::setWaitingThread(Thread *newThread)
{
	if (newThread == NULL) { return ERROR_INVALID_ARG; };

	WaitLock::ScopedGuard			listGuard(&state.lock);

	// Only allow threads from the currently owning process to wait.
	if (state.rsrc.thread != NULL
		&& state.rsrc.thread->parent->id != newThread->parent->id)
	{
		listGuard.releaseManagementAndUnlock();

		printf(WARNING SWAITQ"Failed to allow task %x to "
			"wait.\n",
			newThread->getFullId());

		return ERROR_RESOURCE_BUSY;
	}

	state.rsrc.thread = newThread;
	return ERROR_SUCCESS;
}

