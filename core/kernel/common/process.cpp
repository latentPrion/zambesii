
#include <chipset/chipset.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/process.h>


error_t process::initialize(processS *process)
{
	process->nextTaskId.initialize(CHIPSET_MAX_NTASKS - 1);
	return ERROR_SUCCESS;
}
