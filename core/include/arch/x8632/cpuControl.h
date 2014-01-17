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
	inline void safeEnableInterrupts(uarch_t f);
	inline void safeDisableInterrupts(uarch_t *f);
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

inline void cpuControl::safeEnableInterrupts(uarch_t f)
{
	// Restore CPU flags, then call enableInterrupts().
	asm volatile (
		"pushl	%%eax \n\t \
		movl	%0, %%eax \n\t \
		pushl	%%eax \n\t \
		popfl \n\t \
		popl	%%eax \n\t"
		:
		: "r" (f));
}

inline void cpuControl::safeDisableInterrupts(uarch_t *f)
{
	asm volatile (
		"pushl	%%eax \n\t \
		pushl	%%ebx \n\t \
		movl	%0, %%eax \n\t \
		pushfl \n\t \
		popl	%%ebx \n\t \
		movl	%%ebx, (%%eax) \n\t \
		popl	%%ebx \n\t \
		popl	%%eax \n\t"
		:
		: "r" (f));

	disableInterrupts();
}
		
inline sarch_t cpuControl::interruptsEnabled(void)
{
	uarch_t		flags;

	asm volatile (
		"pushfl \n\t \
		popl %0"
		:"=g" (flags)
		);
	return FLAG_TEST(flags, x8632_CPUFLAGS_IF);
}

#endif

