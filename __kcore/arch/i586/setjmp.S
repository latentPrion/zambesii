
#include <in_asm.h>
#include <assembler.h>

ASM_GLOBAL_FUNCTION(longjmp)
	/* esp[0]=retEip, esp[1]=addr(jump_buf), esp[2]=retval. */
	movl	4(%esp), %eax
	movl	8(%esp), %ebx
	movl	4(%eax), %esp

	/* EBX on stack = retval. */
	pushl	%ebx

	movl	8(%eax), %ebp
	movl	12(%eax), %ebx
	movl	16(%eax), %ecx
	movl	20(%eax), %edx
	movl	24(%eax), %esi
	movl	28(%eax), %edi

	/* Restore the old return EIP value onto the stack since it magically
	 * gets trampled (I don't know either). FreeBSD does the same, so
	 * there's apparently some reason for this random crap.
	 */
	movl	(%eax), %eax
	movl	%eax, 4(%esp)
	/* Move the retval into EAX */
	popl	%eax
ret
ASM_END_FUNCTION(longjmp)

ASM_GLOBAL_FUNCTION(setjmp)
	/* esp[0]=returnAddr, esp[1]=address of jmp_buf */
	movl	4(%esp), %eax

	/* Begin moving reg state into the jump buff. */
	movl	%esp, 4(%eax)
	movl	%ebp, 8(%eax)

	pushl	%edx

	/* esp[0]=edx, esp[1]=returnAddr, esp[2]=addr of jmp_buf */
	movl	4(%esp), %edx
	movl	%edx, (%eax)	/* first member of jmp_buf = retEIP */

	popl	%edx

	/* esp[0]=returnEip, esp[1]=addr of jmp_buf. */
	movl	%ebx, 12(%eax)
	movl	%ecx, 16(%eax)
	movl	%edx, 20(%eax)
	movl	%esi, 24(%eax)
	movl	%edi, 28(%eax)
	xorl	%eax, %eax
ret
ASM_END_FUNCTION(setjmp)

