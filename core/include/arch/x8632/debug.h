#ifndef _ARCH_x86_32_DEBUG_H
	#define _ARCH_x86_32_DEBUG_H

	#include <__kstdlib/__ktypes.h>

	#define DEBUG_BREAKPOINT_READ		(1<<0)
	#define DEBUG_BREAKPOINT_WRITE		(1<<1)
	#define DEBUG_BREAKPOINT_EXEC		(1<<2)

	#define DEBUG_FLAGS_ENABLED		(1<<4)
	#define DEBUG_FLAGS_BP0_ENABLED		(1<<0)
	#define DEBUG_FLAGS_BP1_ENABLED		(1<<1)
	#define DEBUG_FLAGS_BP2_ENABLED		(1<<2)
	#define DEBUG_FLAGS_BP3_ENABLED		(1<<3)

namespace debug
{
	void enableDebugExtensions(void);
	void disableDebugExtensions(void);
	sarch_t debugExtensionsEnabled(void);

	class debuggerC
	{
	friend void debug::enableDebugExtensions(void);
	friend void debug::disableDebugExtensions(void);
	friend sarch_t debug::debugExtensionsEnabled(void);

	public:
		void setBreakpoint(
			ubit8 breakpoint, void *addr, ubit8 opSize, ubit8 rwx);

		void enableBreakpoint(ubit8 breakpoint);
		void disableBreakpoint(ubit8 breakpoint);
		sarch_t breakpointEnabled(ubit8 breakpoint);

	private:
		static uarch_t	flags;
	};
}

#endif

	
