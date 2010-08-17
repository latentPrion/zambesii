#define DEBUG_ON(__con)				\
	if (__con) \
	{ \
		asm volatile ( \
			"pushl	%ecx \n\t " \
			"movl	$0, %ecx \n\t" \
			"1: \n\t" \
			"cmp	$1, %ecx \n\t" \
			"jne	1b \n\t" \
			"popl	%ecx \n\t"); \
	}

