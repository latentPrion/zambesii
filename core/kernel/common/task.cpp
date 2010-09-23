
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/task.h>

error_t task::initialize(taskS *t)
{
	t->context = new taskContextS;
	if (t == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	return ERROR_SUCCESS;
}

