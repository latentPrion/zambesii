#ifndef _ARCH_x86_32_TASK_CONTEXT_H
	#define _ARCH_x86_32_TASK_CONTEXT_H

	#include <__kstdlib/__ktypes.h>

struct taskContextS
{
	uarch_t		ds_es;
	uarch_t		fs_gs;
};

#endif

