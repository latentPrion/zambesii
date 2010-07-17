#include <in_asm.h>

.global __korientationEntry
.extern __korientationMain, __korientationStack
.extern __kpagingLevel0Tables

.section .__ksetupText

.type __korientationEntry, @function
__korientationEntry:
	cli
	xchg %bx,%bx
	jmp .
	/* Enable Paging, then move on from there. */
#ifdef CONFIG_ARCH_X86_32_PAE
	/* Code to enable the PAE, etc here. */
#else
	movl $__kpagingLevel0Tables, %ecx
	movl %ecx, %cr3
#endif
	movl %cr0, %ecx
	orl $0x80000000, %ecx
	movl %ecx, %cr0

	/* Jump to the virtual offset now. */
	movl $.virtualStart, %ecx
	jmp *%ecx

.balign 0x4
.section .__korientationText

.virtualStart:
	/* Load the stack pointer for the Orientation thread. */
	movl $__korientationStack+4096, %esp
	/* Clear the CPU flags */
	pushl $0
	popfl

	/* This is a security trick. We push the address of an infinite loop,
	 * and if the kernel manages to return from the orientation thread
	 * somehow, we manipulate the return address to point to this infinite
	 * loop code.
	 **/
	pushl $.infiniteLoop

	/* Push the (possibly) multiboot flags. */
	pushl %ebx
	pushl %eax
	call __korientationMain

	addl $0x8, %esp
ret

.infiniteLoop:
	cli
	hlt
	jmp .infiniteLoop

