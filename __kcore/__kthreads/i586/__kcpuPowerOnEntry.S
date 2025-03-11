
#include <in_asm.h>
#include <assembler.h>
#include <commonlibs/libx86mp/lapic.h>

#define TMPSTACK_SIZE		512

/**	EXPLANATION:
 * This section is copied by the kernel into lower memory, at 0xA000. The APs
 * begin their execution here. Obviously the section is also linked virtually 
 * to the address in question.
 **/
ASM_SECTION(.__kcpuPowerOnText)

ASM_GLOBAL_FUNCTION(__kcpuPowerOnEntry)
	/* In real mode. */
	.code16

	/* Interrupts are assumed to be disabled on the AP at entry, but still.
	 **/
	cli

	/* Make sure real mode segment is set to seg 0. */
	ljmp	$0x0, $.realNext
.realNext:
	movw	$0x0, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* Load temporary GDT for pmode. */
	lgdt	(__kcpuPowerOnGdtPtr)

	movl	%cr0, %ebx
	orl	$0x1, %ebx
	movl	%ebx, %cr0

	ljmp	$0x8, $.next

.next:
	/* In protected mode now. */
	.code32

	/* Load segment selectors from temp GDT so that we can access all of
	 * linear memory. After they are loaded we will be able to load the
	 * kernel's main GDT since the CPU will now be able to see all 4GB of
	 * linear memory.
	 */
	movw	$0x10, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* Enable the MMU immediately. */
#ifdef CONFIG_ARCH_x86_32_PAE
	#error "PAE not implemented."
	/* Code to enable PAE etc here. Not implemented. */
#else
	movl	$__kpagingLevel0Tables, %ecx
	movl	%ecx, %cr3
#endif
	movl	%cr0, %ecx
	orl	$0x80000000, %ecx
	movl	%ecx, %cr0

	/* Jump into kernel vaddrspace. Force a non-relative jump. */
	movl	$__kcpuPowerOnVirtualStart, %ecx
	jmp	*%ecx
ASM_END_FUNCTION(__kcpuPowerOnEntry)

/* Switch back to the kernel's .text section, virtually linked to 0xC0000000. */
.section .text

ASM_LOCAL_FUNCTION(__kcpuPowerOnVirtualStart)
	/* Now load the kernel's main GDT and set AP's seg regs to selectors. */
	lgdt	(x8632GdtPtr)
	movw	$0x10, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	ljmp	$0x8, $.next2

.next2:
	/* Get the virtual address the kernel has mapped the LAPIC to.
	 **/
	movl	$__kcpuPowerOnLapicVaddr, %eax
	/* EBX = LAPIC base virtual address. */
	movl	(%eax), %ebx
	/* EBX = This CPU's LAPIC ID (LAPIC stores it at bit 24). */
	movl	x86LAPIC_REG_LAPICID(%ebx), %ebx
	shr	$24, %ebx

	/* EAX = __kcpuPowerStacks pointer array start address. */
	movl	$__kcpuPowerStacks, %eax
	/* ESP = power stack address for this CPU, minus stack arguments
	 * that the kernel prepared. The kernel already adjusted the address
	 * to account for them, on our behalf.
	 **/
	movl	(%eax,%ebx,4), %esp
	movl	%esp, %ebp

	/* Clear EFLAGS. */
	pushl	$0x0
	popfl

	/* Call into the C++ HLL code finally. */
	call	__kcpuPowerOnMain
ASM_END_FUNCTION(__kcpuPowerOnVirtualStart)

ASM_GLOBAL_FUNCTION(getRegs)
	/* void getRegs(RegisterContext *t);
	 * t(8) -> eip(4) | edi(0) ->
	 */
	pushl	%edi

	movl	8(%esp), %edi
	movw	%es, (%edi)
	movw	%ds, 2(%edi)
	movw	%gs, 4(%edi)
	movw	%fs, 6(%edi)

	pushl	%edx
	movl	4(%esp), %edx
	movl	%edx, 8(%edi)
	popl	%edx
	movl	%esi, 12(%edi)
	movl	%ebp, 16(%edi)
	movl	%esp, 20(%edi)
	movl	%ebx, 24(%edi)
	movl	%edx, 28(%edi)
	movl	%ecx, 32(%edi)
	movl	%eax, 36(%edi)
	movl	$0, 40(%edi)

	pushl	%ebx
	pushfl
	popl	%ebx
	movl	%ebx, 44(%edi)
	movl	8(%esp), %ebx
	movl	%ebx, 48(%edi)
	popl	%ebx
	movw	%cs, 52(%edi)
	movl	$0, 56(%edi)
	movl	%esp, 60(%edi)
	movw	%ss, 64(%edi)

	popl	%edi
ret
ASM_END_FUNCTION(getRegs)


/** EXPLANATION:
 * The .__kcpuPowerOnData section is placed (at the time of writing) at 0xA000
 * in pmem. It holds useful static data such as the temporary GDT used to switch
 * CPUs to protected mode, etc.
 **/
ASM_SECTION(.__kcpuPowerOnData)
.balign 0x4

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

