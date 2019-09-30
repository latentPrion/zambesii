#ifndef _ARCH_x86_32_TASK_CONTEXT_H
	#define _ARCH_x86_32_TASK_CONTEXT_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string.h>
	#include <kernel/common/execDomain.h>
	#include <kernel/common/memoryTrib/vaddrSpaceStream.h>

class __attribute__((packed)) RegisterContext
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
		eip = (uintptr_t)entryPoint;
	}

	void dump(void);

public:
	ubit16		es;		// 2
	ubit16		ds;		// 4
	ubit16		gs;		// 6
	ubit16		fs;		// 8
	ubit32		edi, esi;	// 16
	ubit32		ebp, dummyEsp;	// 24
	ubit32		ebx, edx;	// 32
	ubit32		ecx, eax;	// 40
	ubit32		vectorNo;	// 44
	// Error code pushed by the CPU for certain exceptions.
	ubit32		errorCode;	// 48
	ubit32		eip, cs;	// 56
	ubit32		eflags;		// 60
	ubit32		esp, ss;	// 68
};

#endif

