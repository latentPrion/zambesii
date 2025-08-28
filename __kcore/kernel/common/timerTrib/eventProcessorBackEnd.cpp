
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/zasyncStream.h>
#include <kernel/common/messageStream.h>


sarch_t TimerTrib::sEventProcessor::getFreeWaitSlot(ubit8 *ret)
{
	for (*ret=0; *ret<6; *ret += 1)
	{
		if (waitSlots[*ret].eventQueue == NULL) {
			return 1;
		};
	};

	return 0;
}

void TimerTrib::sEventProcessor::releaseWaitSlotFor(TimerQueue *timerQueue)
{
	for (ubit8 i=0; i<6; i++)
	{
		if (waitSlots[i].timerQueue == timerQueue) {
			releaseWaitSlot(i);
		};
	};
}

void TimerTrib::sEventProcessor::processQueueLatchedMessage(sControlMsg *msg)
{
	ubit8		slot;

	// Wait on the specified queue.
	if (!getFreeWaitSlot(&slot))
	{
		printf(ERROR TIMERTRIB"event DQer: failed to wait on "
			"timer Q %dus: no free slots.\n",
			msg->timerQueue->getNativePeriod() / 1000);

		return;
	};

	waitSlots[slot].timerQueue = msg->timerQueue;
	waitSlots[slot].eventQueue = msg->timerQueue
		->getDevice()->getEventQueue();

	waitSlots[slot].eventQueue->setWaitingThread(
		timerTrib.eventProcessor.task);

	printf(NOTICE TIMERTRIB"event DQer: Waiting on timerQueue %dus.\n\t"
		"Allocated to slot %d.\n",
		waitSlots[slot].timerQueue->getNativePeriod() / 1000, slot);
}

void TimerTrib::sEventProcessor::processQueueUnlatchedMessage(sControlMsg *msg)
{
	// Stop waiting on the specified queue.
	releaseWaitSlotFor(msg->timerQueue);
	printf(NOTICE TIMERTRIB"event DQer: no longer waiting on timerQueue "
		"%dus.\n",
		msg->timerQueue->getNativePeriod() / 1000);
}

void TimerTrib::sEventProcessor::processExitMessage(sControlMsg *)
{
	printf(WARNING TIMERTRIB"event DQer: Got EXIT_THREAD message.\n");
	/*UNIMPLEMENTED(
		"TimerTrib::"
		"sEventProcessor::processExitMessage");*/
}

void TimerTrib::sEventProcessor::handleZAsyncStreamMsg(
	MessageStream::sHeader *msg
	)
{
	error_t			err;
	sControlMsg 		controlMsg;

	Thread *self = cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	if (msg->subsystem != MSGSTREAM_SUBSYSTEM_ZASYNC)
	{
		printf(ERROR TIMERTRIB"event DQer thread: invalid message "
			"subsystem %d.\n",
			msg->subsystem);

		delete msg;
		return;
	}

	ZAsyncStream::sZAsyncMsg *asyncMsg = reinterpret_cast<
		ZAsyncStream::sZAsyncMsg *>(msg);

	// Receive the message
	err = self->parent->zasyncStream.receive(
		asyncMsg->dataHandle, &controlMsg, 0);

	if (err != ERROR_SUCCESS)
	{
		printf(ERROR TIMERTRIB"event DQer thread: zasyncStream::"
			"receive() failed from TID %x.\n",
			asyncMsg->header.sourceId);

		delete msg;
		return;
	}

	switch (controlMsg.command)
	{
	case sEventProcessor::sControlMsg::EXIT_THREAD:
		timerTrib.eventProcessor.processExitMessage(&controlMsg);
		break;

	case sEventProcessor::sControlMsg::QUEUE_LATCHED:
		timerTrib.eventProcessor.processQueueLatchedMessage(
				&controlMsg);
		break;

	case sEventProcessor::sControlMsg::QUEUE_UNLATCHED:
		timerTrib.eventProcessor.processQueueUnlatchedMessage(
				&controlMsg);
		break;

	default:
		printf(NOTICE TIMERTRIB"event DQer thread: invalid message "
			"type %d.\n",
			controlMsg.command);

		break;
	};

	// Send acknowledgment
	self->parent->zasyncStream.acknowledge(
		asyncMsg->dataHandle, &controlMsg,
		msg->privateData);

	// Reuse the message using placement new.
	new (msg) MessageStream::sHeader(
		asyncMsg->header.sourceId,
		MSGSTREAM_SUBSYSTEM_TIMERTRIB_EVENT_PROCESSOR,
		controlMsg.command,
		sizeof(MessageStream::sHeader), 0,
		msg->privateData);

	msg->error = ERROR_SUCCESS;
	MessageStream::enqueueOnThread(
		msg->targetId, msg);
}

void TimerTrib::sEventProcessor::thread(void *)
{
	sarch_t				messagesWereFound;
	error_t				err;
	Thread				*self;
	MessageStream::sHeader		*msg = NULL;
	sControlMsg			controlMsg;

	// Tracing control for this function
	const bool enableTracing = true;

	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	// Enable connectionless receiving on this thread's ZAsyncStream
	self->parent->zasyncStream.listen(self->getFullId(), 1);

	self->sendAckToSpawner(ERROR_SUCCESS);

	printf(NOTICE TIMERTRIB"Event DQer thread has begun executing.\n");
	for (;FOREVER;)
	{
		messagesWereFound = 0;

		/**	EXPLANATION:
		 * Multiqueue pulling:
		 *
		 * Manually acquire our own schedState lock here to prevent
		 * any other thread from attempting to change our schedState
		 * while we are pulling messages.
		 *
		 * Then use the CALLER_SCHEDLOCKED flag to prevent pull() and
		 * other dequeue operations from calling taskTrib.block()
		 * automatically on our behalf. When we're done pulling, we'll
		 * manually call taskTrib.block() and have it unlock our own
		 * schedState lock.
		 **/
		MultipleReaderLock::ScopedWriteGuard guard(
			&self->schedState.lock);

		err = self->messageStream.pull(
			&msg,
			ZCALLBACK_PULL_FLAGS_CALLER_SCHEDLOCKED
			| ZCALLBACK_PULL_FLAGS_DONT_BLOCK);

		if (err == ERROR_SUCCESS)
		{
			messagesWereFound = 1;

			// Handle ZAsyncStream messages
			timerTrib.eventProcessor.handleZAsyncStreamMsg(msg);
			continue;
		}

		// Wait for the other queues here.
		sZkcmTimerEvent		*currIrqEvent;
		for (ubit8 i=0; i<6; i++)
		{
			// Skip blank slots.
			if (timerTrib.eventProcessor.waitSlots[i].eventQueue
				== NULL)
			{
				continue;
			};

			if (enableTracing) {
printf(CC"`1`");
			}
			err = timerTrib.eventProcessor.waitSlots[i].eventQueue
				->pop(
					(void **)&currIrqEvent,
					SINGLEWAITERQ_POP_FLAGS_CALLER_SCHEDLOCKED
					| SINGLEWAITERQ_POP_FLAGS_DONTBLOCK);

			if (err != ERROR_SUCCESS) {
				if (enableTracing) {
printf(CC"`2`");
				}
				continue;
			}

			messagesWereFound = 1;
			if (enableTracing) {
printf(CC"`3:QnItems=%d`",
	timerTrib.eventProcessor.waitSlots[i].timerQueue->requestQueue.getNItems());
			}

			// Dispatch the message here.
			timerTrib.eventProcessor.waitSlots[i].timerQueue
				->tick(currIrqEvent);

			timerTrib.eventProcessor.waitSlots[i].timerQueue
				->getDevice()->freeIrqEvent(
					currIrqEvent);
		};

		// If the loop ran to its end and there were no messages, block.
		if (messagesWereFound)
			{ continue; };

if (enableTracing) { printf(CC"`4`"); };

		taskTrib.block(&guard);
	};
}
