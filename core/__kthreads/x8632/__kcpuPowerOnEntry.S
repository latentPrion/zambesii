
#include <in_asm.h>
#include <assembler.h>

#define TMPSTACK_SIZE		512

/**	EXPLANATION:
 * This section is copied by the kernel into lower memory, at 0xA000. The APs
 * begin their execution here. Obviously the section is also linked virtually 
 * to the address in question.
 **/
ASM_SECTION(.__kcpuPowerOnText)

ASM_GLOBAL_FUNCTION(__kcpuPowerOnEntry)

	.code16

	/* Interrupts are assumed to be disabled on the AP at entry, but still.
	 **/
	cli

	movw	$0x0, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* Going to jump into pmode. */
	lgdt	(__kcpuPowerOnGdtPtr)

	movl	%cr0, %ebx
	orl	$0x1, %ebx
	movl	%ebx, %cr0

	ljmp	$0x8, $.next

.next:

	.code32

	movw	$0x10, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* Enable the MMU immediately. */
#ifdef CONFIG_ARCH_x86_32_PAE
	/* Code to enable PAE etc here. Not implemented. */
#else
	movl	$__kpagingLevel0Tables, %ecx
	movl	%ecx, %cr3
#endif
	movl	%cr0, %ecx
	orl	$0x80000000, %ecx
	movl	%ecx, %cr0
	hlt

	/* Jump into kernel vaddrspace. Force a non-relative jump. */
	movl	$__kcpuPowerOnVirtualStart, %ecx
	jmp	*%ecx
ASM_END_FUNCTION(__kcpuPowerOnEntry)

/* Switch back to the kernel's .text section, virtually linked to 0xC0000000. */
.section .text

ASM_LOCAL_FUNCTION(__kcpuPowerOnVirtualStart)
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
	call	__kcpuPowerOnHll
ASM_END_FUNCTION(__kcpuPowerOnVirtualStart)


/** EXPLANATION:
 * The .__kcpuPowerOnData section is placed (at the time of writing) at 0xA000
 * in pmem. It holds useful static data such as the temporary GDT used to switch
 * CPUs to protected mode, etc.
 **/
ASM_SECTION(.__kcpuPowerOnData)
.balign 0x4
.global		__kcpuPowerOnStackLock, __kcpuPowerOnSelf

/**	NOTE:
 * This lock serializes accesses to the temporary stack used to get the CPU on
 * its feet.
 *
 * The new CPU does its best to load its own kernel sleep stack and release the
 * temporary stack ASAP so it doesn't block other CPUs that are booting.
 **/
__kcpuPowerOnStackLock:
	.long	0
__kcpuPowerOnSelf:
	.long	0

/* Temporary GDT which allows the AP to see all of pmem...or at least the 1st
 * 4GB of it.
 **/
__kcpuPowerOnGdt:
	.word	0, 0
	.byte	0, 0, 0, 0

	.word	0xFFFF, 0x0
	.byte	0x0
	.byte	0x9A
	.byte	0xCF
	.byte	0x0

	.word	0xFFFF, 0x0
	.byte	0x0
	.byte	0x92
	.byte	0xCF
	.byte	0x0
__kcpuPowerOnGdtEnd:

__kcpuPowerOnGdtPtr:
	.word	__kcpuPowerOnGdtEnd - __kcpuPowerOnGdt - 1
	.long	__kcpuPowerOnGdt

.balign(0x4)
tmpStack:
	.space	TMPSTACK_SIZE

