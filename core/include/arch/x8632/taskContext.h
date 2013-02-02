#ifndef _ARCH_x86_32_TASK_CONTEXT_H
	#define _ARCH_x86_32_TASK_CONTEXT_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string.h>
	#include <kernel/common/execDomain.h>
	#include <kernel/common/memoryTrib/vaddrSpaceStream.h>

class taskC;

struct taskContextC
{
public:
	taskContextC(ubit8 execDomain);
	error_t initialize(void) { return ERROR_SUCCESS; };

	void setStacks(ubit8 execDomain, void *stack0, void *stack1);
	void setEntryPoint(void (*entryPoint)(void *))
	{
		eip = (ubit32)entryPoint;
	}

public:
	ubit16		es;
	ubit16		ds;
	ubit16		gs;
	ubit16		fs;
	ubit32		edi, esi;
	ubit32		ebp, dummyEsp;
	ubit32		ebx, edx;
	ubit32		ecx, eax;
	ubit32		vectorNo;
	// Error code pushed by the CPU for certain exceptions.
	ubit32		errorCode;
	ubit32		eip, cs;
	ubit32		eflags;
	ubit32		esp, ss;
};

#endif

