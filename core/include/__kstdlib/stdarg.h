#ifndef _VARIADIC_ARGS_H
	#define _VARIADIC_ARGS_H

typedef	void* va_list;

#define va_start(vaContainer,firstArg)		\
	vaContainer = (void *)&firstArg

#define va_start_forward(vaContainer,firstArg)		\
	vaContainer = (void *)((uarch_t)&firstArg + sizeof(void *))

#define va_arg(vaContainer,type)		\
	*(type *)vaContainer; \
	vaContainer = (void *)((uarch_t)vaContainer + sizeof(void *))

#define va_copy(dest,src)			\
	dest = src

#define va_end(vaContainer)			\
	vaContainer = 0

#endif

