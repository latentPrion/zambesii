#ifndef ___KORIENTATION_H
	#define ___KORIENTATION_H

	#include <multiboot.h>
	#include <arch/paging.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/task.h>

#define DO_OR_DIE(__ret)		\
	if (__ret != ERROR_SUCCESS) { \
		for (;;){}; \
	}

extern taskS		__korientationThread;
extern "C" void __korientationMain(ubit32 mbMagic, multibootDataS *mbData);
extern ubit8		*__korientationStack;

#endif

