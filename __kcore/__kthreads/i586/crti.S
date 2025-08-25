#include <assembler.h>

.section .init
#if 1
ASM_GLOBAL_FUNCTION(_init)
	pushl	%ebp
	movl	%esp, %ebp
#endif

.section .fini
#if 1
ASM_GLOBAL_FUNCTION(_fini)
	pushl	%ebp
	movl	%esp, %ebp
#endif
