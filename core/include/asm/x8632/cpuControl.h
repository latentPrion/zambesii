#ifndef _ASM_CPU_CONTROL_H
	#define _ASM_CPU_CONTROL_H

	#include <arch/x8632/cpuFlags.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kflagManipulation.h>

namespace cpuControl
{
	inline void halt(void);
	inline void jump(void *vaddr);
	inline void subZero(void);
	inline void enableInterrupts(void);
	inline void disableInterrupts(void);
	inline sarch_t interruptsEnabled(void);
}


/**	Inline Methods
 *******************************************************************/

inline void cpuControl::halt(void)
{
	asm volatile ("hlt\n\t");
}

inline void cpuControl::jump(void *vaddr)
{
	asm volatile (
		"jmp %0\n\t"
		: "=r" (vaddr)
	);
}

inline void cpuControl::subZero(void)
{
	asm volatile ("pause\n\t");
}

inline void cpuControl::enableInterrupts(void)
{
	asm volatile ("sti\n\t");
}

inline void cpuControl::disableInterrupts(void)
{
	asm volatile ("cli\n\t");
}

inline sarch_t cpuControl::interruptsEnabled(void)
{
	uarch_t		flags;

	asm volatile (
		"pushfl \n\t \
		popl %0"
		:"=g" (flags)
		);
	return __KFLAG_TEST(flags, CPUFLAGS_IF);
}

#endif

