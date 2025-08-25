#include <assembler.h>

.section .init
#if 1
	movl	%ebp, %esp
	popl	%ebp
	ret
#endif

.section .fini
#if 1
	movl	%ebp, %esp
	popl	%ebp
	ret
#endif
