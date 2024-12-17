#ifndef _SET_JMP_H
	#define _SET_JMP_H

	#include <__kstdlib/__ktypes.h>

typedef struct c_jmp_buf
{
	uarch_t		eip;
	uarch_t		esp, ebp;
	uarch_t		ebx, ecx, edx;
	uarch_t		esi, edi;
} jmp_buf;

#ifdef __cplusplus
extern "C" {
#endif

int setjmp(jmp_buf *state);
void longjmp(jmp_buf *state, int ret);

#ifdef __cplusplus
}
#endif

#endif
