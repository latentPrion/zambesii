
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/zasyncStream.h>
#include <kernel/common/process.h>
#include <__kthreads/__korientation.h>


namespace {

static sbit8 waitForResponse_syncDispatcher(MessageStream::sHeader *msg)
{
	__korientationMainDispatchOne(msg);
	return 0;
}

}

static error_t waitForResponse(
	MessageStreamCb *callerCb,
	TimerTrib::sEventProcessor::sControlMsg::commandE expectedCommand,
	const char *operation
	)
{
	error_t ret;
	MessageStream::sHeader *msg = NULL;
	MessageStream::Filter filter(
		0,
		MSGSTREAM_SUBSYSTEM_TIMERTRIB_EVENT_PROCESSOR,
		expectedCommand, 0, 0, callerCb,
		MessageStream::Filter::FLAG_SUBSYSTEM
		| MessageStream::Filter::FLAG_FUNCTION
		| MessageStream::Filter::FLAG_PRIVATE_DATA);

	ret = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
		->messageStream.pullAndDispatchUntil(
			&msg, 0, &filter, &waitForResponse_syncDispatcher);
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR TIMERTRIB"%s: "
			"Failed to pullAndDispatchUntil (ret=%d).\n",
			operation, ret);
		return ret;
	}

	ret = msg->error;
	delete msg;
	return ret;
}

error_t TimerTrib::sEventProcessor::sendShutdownMessage(
	sbit8 doSynchronously, MessageStreamCb *callerCb
	)
{
	error_t ret;

	if (callerCb == NULL) { return ERROR_INVALID_ARG_VAL; };

	Thread *currThread = cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentThread();
	
	sControlMsg     controlMsg(sControlMsg::EXIT_THREAD, NULL);
	ret = currThread->parent->zasyncStream.send(
		this->tid, 
		&controlMsg, 
		sizeof(controlMsg),
		ipc::METHOD_BUFFER, 
		0, callerCb);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR TIMERTRIB"shutdownMessage: "
			"Failed to send shutdown message (ret=%d).\n", ret);
	}

	if (!doSynchronously) { return ret; }

	return waitForResponse(
		callerCb, sControlMsg::EXIT_THREAD, "shutdownMessage");
}

error_t TimerTrib::sEventProcessor::enableWaitingOnQueue(
	TimerQueue *queue,
	sbit8 doSynchronously, MessageStreamCb *callerCb
	)
{
	error_t ret;

	if (queue == NULL || callerCb == NULL)
		{ return ERROR_INVALID_ARG_VAL; };

	if (!queue->isLatched()) { return ERROR_UNINITIALIZED; };

	Thread *currThread = cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentThread();

	sControlMsg     controlMsg(sControlMsg::QUEUE_LATCHED, queue);
	ret = currThread->parent->zasyncStream.send(
		this->tid, 
		&controlMsg, 
		sizeof(controlMsg),
		ipc::METHOD_BUFFER, 
		0, callerCb);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR TIMERTRIB"enableWaitingOnQueue: "
			"Failed to send message (ret=%d).\n", ret);
	}

	if (!doSynchronously) { return ret; }

	return waitForResponse(
		callerCb, sControlMsg::QUEUE_LATCHED,
		"enableWaitingOnQueue");
}

error_t TimerTrib::sEventProcessor::disableWaitingOnQueue(
	TimerQueue *queue,
	sbit8 doSynchronously, MessageStreamCb *callerCb
	)
{
	error_t ret;

	if (queue == NULL || callerCb == NULL)
		{ return ERROR_INVALID_ARG_VAL; };

	if (!queue->isLatched()) { return ERROR_UNINITIALIZED; };

	Thread *currThread = cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentThread();

	sControlMsg     controlMsg(sControlMsg::QUEUE_UNLATCHED, queue);
	ret = currThread->parent->zasyncStream.send(
		this->tid, 
		&controlMsg, 
		sizeof(controlMsg),
		ipc::METHOD_BUFFER, 
		0, callerCb);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR TIMERTRIB"disableWaitingOnQueue: "
			"Failed to send message (ret=%d).\n", ret);
	}

	if (!doSynchronously) { return ret; }

	return waitForResponse(
		callerCb, sControlMsg::QUEUE_UNLATCHED,
		"disableWaitingOnQueue");
}

void TimerTrib::sendQMessage(void)
{
	sZkcmTimerEvent		*irqEvent;

	irqEvent = period10ms.getDevice()->allocateIrqEvent();
	irqEvent->device = period10ms.getDevice();
	irqEvent->latchedStream = &processTrib.__kgetStream()
		->floodplainnStream;
	getCurrentTime(&irqEvent->irqStamp.time);
	getCurrentDate(&irqEvent->irqStamp.date);

	period10ms.getDevice()->getEventQueue()->addItem(irqEvent);
}

void TimerTrib::sEventProcessor::dump(void)
{
	const ubit8	nWaitSlots = sizeof(waitSlots) / sizeof(waitSlots[0]);

	printf(NOTICE TIMERTRIB"Event DQer thread: %d wait slots:\n",
		nWaitSlots);

	for (ubit8 i=0; i<nWaitSlots; i++)
	{
		printf(NOTICE TIMERTRIB"waitSlot[%d]: timerQueue %p, "
			"eventQueue %p.\n",
			i, waitSlots[i].timerQueue,
			waitSlots[i].eventQueue);

		if (waitSlots[i].eventQueue != NULL)
			{ waitSlots[i].eventQueue->dump(); };
		if (waitSlots[i].timerQueue != NULL)
			{ waitSlots[i].timerQueue->dump(); };
	};
}
