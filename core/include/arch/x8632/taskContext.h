#ifndef _ARCH_x86_32_TASK_CONTEXT_H
	#define _ARCH_x86_32_TASK_CONTEXT_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string.h>
	#include <kernel/common/execDomain.h>

struct taskContextC
{
public:
	taskContextC(void)
	{
		memset(this, 0, sizeof(*this));
	}

	error_t initialize(void) { return ERROR_SUCCESS; }

	void load(void);

	void setStacks(ubit8 execDomain, void *stack0, void *stack1)
	{
		
		if (execDomain == PROCESS_EXECDOMAIN_KERNEL)
		{
			esp = (ubit32)stack0;
			cs = 0x08;
			ds = ss = 0x10;
		}
		else
		{
			esp = (ubit32)stack1;
			cs = 0x18;
			ds = ss = 0x20;
		};
	}
	
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

