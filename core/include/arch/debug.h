#include <arch/arch_include.h>
#include ARCH_INCLUDE(debug.h)

#ifndef _ARCH_DEBUG_COMMON_H
	#define _ARCH_DEBUG_COMMON_H

namespace debug
{
	struct stackDescriptorS
	{
		void		*start, *eof;
		uarch_t		nBytes;
	};

	void getCurrentStackInfo(stackDescriptorS *desc);
	void printStackTrace(void *startFrame, stackDescriptorS *desc);
	void printStackArguments(void *stackFrame, void *stackPointer);
}

#endif
