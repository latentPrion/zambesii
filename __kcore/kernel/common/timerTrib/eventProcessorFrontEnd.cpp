
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/zasyncStream.h>
#include <kernel/common/process.h>


static error_t waitForResponse(
	MessageStreamCb *callerCb, const char *operation
	)
{
	error_t ret;
	MessageStream::sHeader *msg = NULL;
	MessageStream::Filter filter(
		0, 0, 0, 0, 0, callerCb,
		MessageStream::Filter::FLAG_PRIVATE_DATA);

	// Block until the event processor thread has processed the message.
	ret = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
		->messageStream.pull(&msg, 0, &filter);

	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR TIMERTRIB"%s: "
			"Failed to pull message (ret=%d).\n", operation, ret);
	}

	return msg->error;
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

	return waitForResponse(callerCb, "shutdownMessage");
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

	return waitForResponse(callerCb, "enableWaitingOnQueue");
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

	return waitForResponse(callerCb, "disableWaitingOnQueue");
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
