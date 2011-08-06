
#include <in_asm.h>
#include <assembler.h>

/*
*edx
*ecx
*ebx
*eax
num
eip
---
eax
ebx
ecx
edx
*/
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

