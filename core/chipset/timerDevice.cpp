
#include <chipset/zkcm/timerDevice.h>
#include <kernel/common/process.h>
#include <kernel/common/floodplainn/floodplainnStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


error_t zkcmTimerDeviceC::initialize(void)
{
	irqEventCache = cachePool.createCache(sizeof(zkcmTimerEventS));
	if (irqEventCache == NULL)
	{
		printf(WARNING"ZKCM-Timer: Creating obj cache "
			"failed for device \"%s\".\n",
			getBaseDevice()->shortName);

		return ERROR_MEMORY_NOMEM;
	};

	return irqEventQueue.initialize();
}

error_t zkcmTimerDeviceC::latch(class floodplainnStreamC *stream)
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

void zkcmTimerDeviceC::unlatch(void)
{
	cpuStreamC	*currCpu;

	// If it's not the owning process, deny the attempt.
	currCpu = cpuTrib.getCurrentCpuStream();

	// This condition needs to check the floodplain binding.
	if (currCpu->taskStream.getCurrentTask()->parent->id
		== state.rsrc.latchedStream->id)
	{
		state.lock.acquire();

		FLAG_UNSET(
			state.rsrc.flags,
			ZKCM_TIMERDEV_STATE_FLAGS_LATCHED);

		state.lock.release();
	};
}

sarch_t zkcmTimerDeviceC::getLatchState(class floodplainnStreamC **latchedStream)
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

sarch_t zkcmTimerDeviceC::validateCallerIsLatched(void)
{
	floodplainnStreamC	*stream;
	taskC			*currTask;

	currTask = cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentTask();

	// Replace with floodplain binding check.
	if (getLatchState(&stream) && (stream->id == currTask->parent->id)) {
		return 1;
	} else {
		return 0;
	};
}

