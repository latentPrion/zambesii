
#include <in_asm.h>
#include <assembler.h>

#define TMPSTACK_SIZE		512

ASM_SECTION(.__ksetupText)

ASM_GLOBAL_FUNCTION(__kcpuPowerOnEntry)
	cli
#ifdef CONFIG_ARCH_x86_32_PAE
	/* Enable PAE etc here. */
#else
	movl	$__kpagingLevel0Tables, %ecx
	movl	%ecx, %cr3
#endif
	movl	%cr0, %ecx
	orl	$0x80000000, %ecx
	movl	%ecx, %cr0

	/* Jump into kernel vaddrspace. */
	movl	$virtualStart, %ecx
	jmp	*%ecx

ASM_END_FUNCTION(__kcpuPowerOnEntry)

.section .text

ASM_LOCAL_FUNCTION(virtualStart)
	/* Need to find a way to obtain the current CPU's sleep stack.
	 * This means we need to access an object on the heap...very difficult
	 * to do from within ASM.
	 *
	 * To avoid all of that, this thread provides a very small temporary
	 * stack for the waking CPU to use. This stack is protected by a very
	 * crude spinlock.
	 **/
	movl	$1, %eax
.again:
	xchg	%eax, (__kcpuPowerOnStackLock)
	cmp	$0, %eax
	jne	.again

	/* We now have the temporary stack, so we can enter the kernel's C++
	 * code.
	 **/
	movl	$tmpStack + TMPSTACK_SIZE, %esp
	call	__kcpuPowerOnMain
ASM_END_FUNCTION(virtualStart)

.global		__kcpuPowerOnStackLock, __kcpuPowerOnId
__kcpuPowerOnStackLock:
	.long	0
__kcpuPowerOnSelf:
	.long	0

tmpStack:
	.space	TMPSTACK_SIZE

