
#include <kernel/common/taskTrib/taskStream.h>


error_t taskStreamC::schedule(taskS *task)
{
	

void taskTribC::updateCapacity(ubit8 action, uarch_t val)
{
	switch (action)
	{
	case PROCESSTRIB_UPDATE_ADD:
		capacity += val;
		return;

	case PROCESSTRIB_UPDATE_SUBTRACT:
		capacity -= val;
		return;

	case PROCESSTRIB_UPDATE_SET:
		capacity = val;
		return;

	default: return;
	};
}

void taskTribC::updateLoad(ubit8 action, uarch_t val)
{
	switch (action)
	{
	case PROCESSTRIB_UPDATE_ADD:
		load += val;
		return;

	case PROCESSTRIB_UPDATE_SUBTRACT:
		load -= val;
		return;

	case PROCESSTRIB_UPDATE_SET:
		load = val;
		return;

	default: return;
	};
}

