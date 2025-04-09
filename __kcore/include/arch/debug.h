#include <arch/arch_include.h>
#include ARCH_INCLUDE(debug.h)


#ifndef _ARCH_DEBUG_COMMON_H
	#define _ARCH_DEBUG_COMMON_H

#include <config.h>
#include <kernel/common/sharedResourceGroup.h>
#include <kernel/common/waitLock.h>

namespace debug
{
	struct sStackDescriptor
	{
		void dump(void);

		void		*start, *eof;
		uarch_t		nBytes;
	};

	void getCurrentStackInfo(sStackDescriptor *desc);
#ifdef CONFIG_DEBUG_LOCKS
	void printStackTraceToBuffer(
		SharedResourceGroup<WaitLock, utf8Char *> *buff,
		uarch_t bufferSize,
		void *startFrame, sStackDescriptor *stack);
#endif
	void printStackTrace(void *startFrame, sStackDescriptor *stack);
	void printStackArguments(void *stackFrame, void *stackPointer);
}

#endif
