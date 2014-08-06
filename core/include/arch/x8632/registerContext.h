#ifndef _ARCH_x86_32_TASK_CONTEXT_H
	#define _ARCH_x86_32_TASK_CONTEXT_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string.h>
	#include <kernel/common/execDomain.h>
	#include <kernel/common/memoryTrib/vaddrSpaceStream.h>

class Thread;

class RegisterContext
{
public:
	RegisterContext(ubit8 execDomain);
	error_t initialize(void) { return ERROR_SUCCESS; };

	/**	EXPLANATION:
	 * "initialStackIndex" specifies which of the stacks to set the new
	 * thread's stack pointer register to.
	 *
	 * "0" for the kernel stack and "1" for the userspace stack.
	 **/
	void setStacks(void *stack0, void *stack1, ubit8 initialStackIndex);

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

