#ifndef _ASSEMBLER_H
	#define _ASSEMBLER_H

#define ASM_GLOBAL_FUNCTION(__sym)				\
	.globl __sym; \
	.type __sym, @function; \
	__sym: \

#define ASM_LOCAL_FUNCTION(__sym)				\
	.type __sym, @function; \
	__sym: \

#define ASM_END_FUNCTION(__sym)				\
	.size __sym, .-__sym; \

#define ASM_GLOBAL_DATA(__sym)				\
	.globl __sym; \
	.type __sym, @object; \
	__sym: \

#define ASM_LOCAL_DATA(__sym)				\
	.type __sym, @object; \
	__sym: \

#define ASM_END_DATA(__sym)					\
	.size __sym, .-__sym; \

#define ASM_SECTION(__secname)				\
	.section __secname,"a"; \

#endif

