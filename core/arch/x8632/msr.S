
#include <in_asm.h>
#include <assembler.h>


/* extern "C" void rdmsr(ubit32 idx, ubit32 *high, ubit32 *low) */
ASM_GLOBAL_FUNCTION(rdmsr)
	/* ESP+0 = ret, ESP+4 = idx, ESP+8=high */
	pushl	%ecx
	pushl	%edx
	pushl	%eax

	/* ESP+12 = ret, ESP+16 = idx, ESP+20 = *high */
	movl	16(%esp), %ecx
	rdmsr
	movl	20(%esp), %ecx
	movl	%edx, (%ecx)
	movl	24(%esp), %ecx
	movl	%eax, (%ecx)

	popl	%eax
	popl	%edx
	popl	%ecx
ret
ASM_END_FUNCTION(rdmsr)

/* extern "C" void wrmsr(ubit32 idx, ubit32 high, ubit32 low) */
ASM_GLOBAL_FUNCTION(wrmsr)
	pushl	%ecx
	pushl	%edx
	pushl	%eax

	/* ESP+12 = ret, ESP+16=idx, ESP+20=high, etc */
	movl	16(%esp), %ecx
	movl	20(%esp), %edx
	movl	24(%esp), %eax
	wrmsr

	popl	%eax
	popl	%edx
	popl	%ecx
ret
ASM_END_FUNCTION(wrmsr)
