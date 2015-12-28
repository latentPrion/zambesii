
#include <in_asm.h>
#include <assembler.h>
#include <arch/paging.h>
#include <chipset/memory.h>


.extern __korientationInit, __korientationStack
.extern __kpagingLevel0Tables
.extern x8632GdtPtr

ASM_SECTION(.__ksetupText)

ASM_GLOBAL_FUNCTION(_start)
	cli

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

xchg %bx, %bx
	/* Jump to the virtual offset now. */
	movl $_startVirtual, %ecx
	jmp *%ecx
ASM_END_FUNCTION(_start)

ASM_SECTION(.text)

ASM_LOCAL_FUNCTION(_startVirtual)
	/* Load the stack pointer for the BSP power thread. */
	movl $bspCpuPowerStack, %esp
	movl (%esp), %esp
	movl $PAGING_BASE_SIZE, %ecx
	lea (%esp, %ecx, CHIPSET_MEMORY___KSTACK_NPAGES), %esp

	/* Clear the CPU flags */
	pushl $0
	popfl

	/* Load the GDT */
	lgdt 	(x8632GdtPtr)
	movw	$0x10, %cx
	movw	%cx, %ds
	movw	%cx, %es
	movw	%cx, %fs
	movw	%cx, %gs
	movw	%cx, %ss
	ljmp	$0x08, $.loadAX

.loadAX:
	/* This is a security trick. We push the address of an infinite loop,
	 * and if the kernel manages to return from the orientation thread
	 * somehow, we manipulate the return address to point to this infinite
	 * loop code.
	 **/
	pushl $.infiniteLoop

	/* Push the (possibly) multiboot flags. */
	pushl %ebx
	pushl %eax

	call main

	addl $0x8, %esp
ret

.infiniteLoop:
	cli
	hlt
	jmp .infiniteLoop
ASM_END_FUNCTION(_startVirtual)

