
#include <arch/x8632/debug.h>
#include <arch/x8632/cpuFlags.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>


// Static data member.
uarch_t debug::debuggerC::flags = 0;

void debug::enableDebugExtensions(void)
{
	asm volatile(
		"pushl	%eax\n\t \
		movl	%cr4, %eax\n\t \
		orl	$0x8, %eax\n\t \
		movl	%eax, %cr4\n\t \
		popl	%eax\n\t");

	__KFLAG_SET(debuggerC::flags, DEBUG_FLAGS_ENABLED);
}

void debug::disableDebugExtensions(void)
{
	asm volatile(
		"pushl	%eax\n\t \
		movl	%cr4, %eax\n\t \
		andl	$0xFFFFFFF7, %eax\n\t \
		movl	%eax, %cr4\n\t \
		popl	%eax\n\t");

	__KFLAG_UNSET(debuggerC::flags, DEBUG_FLAGS_ENABLED);
}

sarch_t debug::debugExtensionsEnabled(void)
{
	return __KFLAG_TEST(debuggerC::flags, DEBUG_FLAGS_ENABLED);
}

static const ubit8 rwxShiftTable[4] = { 16, 20, 24, 28 };
static const ubit8 opSizeShiftTable[4] = { 18, 22, 26, 30 };

static void __attribute__((noinline)) setRwxBits(ubit8 breakpoint, ubit8 rwx)
{
	ubit32		dr7Value;

	/**	EXPLANATION:
	 * Converts the rwx flags into flags as specified by the Intel manuals
	 * for DR7.RW<n>.
	 **/
	if (!__KFLAG_TEST(rwx, DEBUG_BREAKPOINT_READ))
	{
		switch (rwx)
		{
		case DEBUG_BREAKPOINT_WRITE: rwx = 0x1; break;
		case DEBUG_BREAKPOINT_EXEC: rwx = 0x0; break;
		// We don't support I/O breakpoints right now.
		default:
			printf(ERROR"x86 Debug: Invalid breakpoint "
				"condition 0x%x for bp %d.\n"
				"\tDisabling breakpoint.\n",
				rwx, breakpoint);

			debug::debuggerC	debugger;
			debugger.disableBreakpoint(breakpoint);
			return;
		};
	}
	else
	{
		/* Asking for #DB on read actually sets the CPU to #DB on read
		 * and write, because the arch doesn't support read-only
		 * breakpoint triggering.
		 **/
		rwx = 0x3;
	};

	// "rwx" now contains the value to write to DR7.RW<n>. Shift and write.
	dr7Value = rwx << rwxShiftTable[breakpoint];
	asm volatile(
		"movl	%%dr7, %%eax\n\t \
		orl	%0, %%eax\n\t \
		movl	%%eax, %%dr7\n\t"
		:
		: "r" (dr7Value)
		: "%eax");
}

static void __attribute__((noinline)) setOpSize(ubit8 breakpoint, ubit8 opSize)
{
	ubit32		dr7Value;

	if ((opSize > 4 && opSize != 8) || opSize == 3)
	{
		printf(ERROR"x86 Debug: Invalid opsize %d for bp %d. "
			"Disabling breakpoint.\n",
			opSize, breakpoint);

		debug::debuggerC	debugger;
		debugger.disableBreakpoint(breakpoint);
		return;
	};

	switch (opSize)
	{
	case 8: dr7Value = 0x2; break;
	case 4: dr7Value = 0x3; break;
	case 2: dr7Value = 0x1; break;
	default: dr7Value = 0; break;
	};

	dr7Value = opSize << opSizeShiftTable[breakpoint];
	asm volatile(
		"movl	%%dr7, %%eax\n\t \
		orl	%0, %%eax\n\t \
		movl	%%eax, %%dr7\n\t"
		:
		: "r" (dr7Value)
		: "%eax");
}

static void setBreakpoint0(void *vaddr, ubit8 opSize, ubit8 rwx)
{
	asm volatile(
		"movl	%0, %%dr0\n\t"
		:
		: "r" (vaddr));

	setRwxBits(0, rwx);
	setOpSize(0, opSize);
}

static void setBreakpoint1(void *vaddr, ubit8 opSize, ubit8 rwx)
{
	asm volatile(
		"movl	%0, %%dr1\n\t"
		:
		: "r" (vaddr));

	setRwxBits(1, rwx);
	setOpSize(1, opSize);
}

static void setBreakpoint2(void *vaddr, ubit8 opSize, ubit8 rwx)
{
	asm volatile(
		"movl	%0, %%dr2\n\t"
		:
		: "r" (vaddr));

	setRwxBits(2, rwx);
	setOpSize(2, opSize);
}

static void setBreakpoint3(void *vaddr, ubit8 opSize, ubit8 rwx)
{
	asm volatile(
		"movl	%0, %%dr3\n\t"
		:
		: "r" (vaddr));

	setRwxBits(3, rwx);
	setOpSize(3, opSize);
}

static void (*const breakpointFuncs[])(void *, ubit8, ubit8) =
{
	&setBreakpoint0, &setBreakpoint1, &setBreakpoint2, &setBreakpoint3
};

void debug::debuggerC::setBreakpoint(
	ubit8 breakpoint, void *addr, ubit8 opSize, ubit8 rwx
	)
{
	if (breakpoint > 3)
	{
		printf(ERROR"x86 Debug: Invalid breakpoint %d.\n",
			breakpoint);

		return;
	};

	(*breakpointFuncs[breakpoint])(addr, opSize, rwx);
}

void debug::debuggerC::enableBreakpoint(ubit8 breakpoint)
{
	/**	EXPLANATION:
	 * Sets the G<n> flag to globally enable the breakpoint. No support for
	 * local breakpoints planned.
	 **/
	if (breakpoint > 3)
	{
		printf(ERROR"x86 Debug: Invalid breakpoint %d.\n",
			breakpoint);

		return;
	};

	breakpoint = breakpoint * 2 + 1;
	breakpoint = 1 << breakpoint;

	asm volatile(
		"movl	%%dr7, %%eax\n\t \
		orb	%0, %%al\n\t \
		movl	%%eax, %%dr7\n\t"
		:
		: "r" (breakpoint)
		: "%eax");
}

void debug::debuggerC::disableBreakpoint(ubit8 breakpoint)
{
	/**	EXPLANATION:
	 * Unsets the G<n> flag to globally disable the breakpoint. No support
	 * for local breakpoints planned.
	 **/
	if (breakpoint > 3)
	{
		printf(ERROR"x86 Debug: Invalid breakpoint %d.\n",
			breakpoint);

		return;
	};

	breakpoint = breakpoint * 2 + 1;
	breakpoint = 1 << breakpoint;
	breakpoint = ~breakpoint;

	asm volatile(
		"movl	%%dr7, %%eax\n\t \
		andb	%0, %%al\n\t \
		movl	%%eax, %%dr7\n\t"
		:
		: "r" (breakpoint)
		: "%eax");
}

sarch_t debug::debuggerC::breakpointEnabled(ubit8 breakpoint)
{
	if (breakpoint > 3)
	{
		printf(ERROR"x86 Debug: Invalid breakpoint %d.\n",
			breakpoint);

		return 0;
	};

	breakpoint = breakpoint * 2 + 1;
	breakpoint = 1 << breakpoint;

	ubit32		dr7;
	asm volatile(
		"movl	%%dr7, %0\n\t"
		: "=r" (dr7));

	return !!(dr7 & breakpoint);
}

