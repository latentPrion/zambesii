.section .init
	movl	%ebp, %esp
	popl	%ebp
	ret

.section .fini
	movl	%ebp, %esp
	popl	%ebp
	ret
