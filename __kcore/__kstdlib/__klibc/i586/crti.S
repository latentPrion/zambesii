.section .init
.global __init
.type __init, @function
__init:
	pushl	%ebp
	movl	%esp, %ebp

.section .fini
.global __fini
.type __fini, @function
__fini:
	pushl	%ebp
	movl	%esp, %ebp
