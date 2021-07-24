#ifndef _DEBUG_HELPER_H
	#define _DEBUG_HELPER_H

#include <__kstdlib/__ktypes.h>

#define DEBUG_ON(__con)				\
	if (__con) \
	{ \
		asm volatile("" ::: "memory"); \
		asm volatile ( \
			"pushl	%%ecx \n\t " \
			"movl	$0, %%ecx \n\t" \
			"1: \n\t" \
			"cmp	$1, %%ecx \n\t" \
			"jne	1b \n\t" \
			"popl	%%ecx \n\t" \
			::: "memory"); \
	}

extern int oo, pp, qq, rr;
extern char *pp0, *pp1, *pp2, *pp3;
int testFunction(...);

// GCC likes to warn me about this, and it gets really irritating.
void PRINTFON(int __cond, const utf8Char *__str, ...);

#endif

