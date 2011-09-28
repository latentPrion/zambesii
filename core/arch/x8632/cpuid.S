
#include <in_asm.h>
#include <assembler.h>


ASM_GLOBAL_FUNCTION(execCpuid)
	/* void execCpuid(
	 *	uarch_t num,
	 *	uarch_t *eax, uarch_t *ebx, uarch_t *ecx, uarch_t *edx);
	 **/
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	movl	20(%esp), %eax
	cpuid

	pushl	%ebx
	movl	28(%esp), %ebx
	movl	%eax, (%ebx)
	popl	%ebx
	movl	28(%esp), %eax
	movl	%ebx, (%eax)
	movl	32(%esp), %eax
	movl	%ecx, (%eax)
	movl	36(%esp), %eax
	movl	%edx, (%eax)

	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
ret

.loop:	jmp	.

ASM_END_FUNCTION(execCpuid)

ASM_GLOBAL_FUNCTION(x8632_verifyCpuIsI486OrHigher)
	pushl	%ebx

	/* Try to change the ID flag in EFLAGS to see if CPUID is supported. */
	pushfl
	popl	%eax
	movl	$1, %ebx
	shl	$21, %ebx
	orl	%ebx, %eax
	pushl	%eax
	popfl
	
	/* Check to see if it got changed. */
	pushfl
	popl	%eax
	andl	%ebx, %eax
	popl	%ebx
	cmpl	$0, %eax
	je	.failed

	/*  CPU is at least i486, since it has CPUID. */
	ret

.failed:
	movl	$0, %eax
	ret
ASM_END_FUNCTION(x8632_verifyCpuIsI486OrHigher)

