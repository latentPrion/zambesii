
#include <chipset/zkcm/timerDevice.h>
#include <kernel/common/process.h>
#include <kernel/common/floodplainn/floodplainnStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


error_t ZkcmTimerDevice::initialize(void)
{
	irqEventCache = cachePool.createCache(sizeof(sZkcmTimerEvent));
	if (irqEventCache == NULL)
	{
		printf(WARNING"ZKCM-Timer: Creating obj cache "
			"failed for device \"%s\".\n",
			getBaseDevice()->shortName);

		return ERROR_MEMORY_NOMEM;
	};

	return irqEventQueue.initialize();
}

error_t ZkcmTimerDevice::shutdown(void)
{
	if (irqEventCache != NULL) {
		cachePool.destroyCache(irqEventCache);
	}

	return ERROR_SUCCESS;
}

error_t ZkcmTimerDevice::latch(class FloodplainnStream *stream)
{
	state.lock.acquire();

	if (FLAG_TEST(
		state.rsrc.flags,
		ZKCM_TIMERDEV_STATE_FLAGS_LATCHED))
	{
		state.lock.release();
		return ERROR_RESOURCE_BUSY;
	};

	state.rsrc.latchedStream = stream;
	FLAG_SET(
		state.rsrc.flags, ZKCM_TIMERDEV_STATE_FLAGS_LATCHED);

	state.lock.release();
	return ERROR_SUCCESS;
}

void ZkcmTimerDevice::unlatch(void)
{
	CpuStream	*currCpu;

	// If it's not the owning process, deny the attempt.
	currCpu = cpuTrib.getCurrentCpuStream();

	// This condition needs to check the floodplain binding.
	if (currCpu->taskStream.getCurrentThread()->parent->id
		== state.rsrc.latchedStream->id)
	{
		state.lock.acquire();

		FLAG_UNSET(
			state.rsrc.flags,
			ZKCM_TIMERDEV_STATE_FLAGS_LATCHED);

		state.lock.release();
	};
}

sarch_t ZkcmTimerDevice::getLatchState(class FloodplainnStream **latchedStream)
{
	state.lock.acquire();

	if (FLAG_TEST(
		state.rsrc.flags, ZKCM_TIMERDEV_STATE_FLAGS_LATCHED))
	{
		*latchedStream = state.rsrc.latchedStream;

		state.lock.release();
		return 1;
	};

	state.lock.release();
	return 0;
}

sarch_t ZkcmTimerDevice::validateCallerIsLatched(void)
{
	FloodplainnStream	*stream;
	Thread			*currThread;

	currThread = cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentThread();

	// Replace with floodplain binding check.
	if (getLatchState(&stream) && (stream->id == currThread->parent->id)) {
		return 1;
	} else {
		return 0;
	};
}

