#ifndef _ARCH_x86_32_TASK_CONTEXT_H
	#define _ARCH_x86_32_TASK_CONTEXT_H

	#include <__kstdlib/__ktypes.h>

struct taskContextS
{
	ubit16		es;
	ubit16		ds;
	ubit16		gs;
	ubit16		fs;
	ubit32		edi, esi;
	ubit32		ebp, dummyEsp;
	ubit32		ebx, edx;
	ubit32		ecx, eax;
	ubit32		vectorNum;
	ubit32		flags;
	ubit32		eip, cs;
	ubit32		eflags;
	ubit32		esp, ss;
};

#endif

