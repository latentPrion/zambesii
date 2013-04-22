
#include <chipset/zkcm/timerDevice.h>
#include <kernel/common/floodplainn/floodplainnStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


error_t zkcmTimerDeviceC::initialize(void)
{
	irqEventCache = cachePool.createCache(sizeof(zkcmTimerEventS));
	if (irqEventCache == __KNULL)
	{
		__kprintf(WARNING"ZKCM-Timer: Creating obj cache "
			"failed for device \"%s\".\n",
			getBaseDevice()->shortName);

		return ERROR_MEMORY_NOMEM;
	};

	return irqEventQueue.initialize();
}

error_t zkcmTimerDeviceC::latch(class floodplainnStreamC *stream)
{
	state.lock.acquire();

	if (__KFLAG_TEST(
		state.rsrc.flags,
		ZKCM_TIMERDEV_STATE_FLAGS_LATCHED))
	{
		state.lock.release();
		return ERROR_RESOURCE_BUSY;
	};

	state.rsrc.latchedStream = stream;
	__KFLAG_SET(
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
	if (PROCID_PROCESS(currCpu->taskStream.currentTask->id)
		== PROCID_PROCESS(state.rsrc.latchedStream->id))
	{
		state.lock.acquire();

		__KFLAG_UNSET(
			state.rsrc.flags,
			ZKCM_TIMERDEV_STATE_FLAGS_LATCHED);

		state.lock.release();
	};
}

sarch_t zkcmTimerDeviceC::getLatchState(class floodplainnStreamC **latchedStream)
{
	state.lock.acquire();

	if (__KFLAG_TEST(
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
		->taskStream.currentTask;

	// Replace with floodplain binding check.
	if (getLatchState(&stream)
		&& PROCID_PROCESS(stream->id)
			== PROCID_PROCESS(currTask->id))
	{
		return 1;
	} else {
		return 0;
	};
}

