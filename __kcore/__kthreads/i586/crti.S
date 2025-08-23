#include <assembler.h>

.section .init
ASM_GLOBAL_FUNCTION(_init)
	pushl	%ebp
	movl	%esp, %ebp

.section .fini
ASM_GLOBAL_FUNCTION(_fini)
	pushl	%ebp
	movl	%esp, %ebp
