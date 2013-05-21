#ifndef ___K_ERROR_H
	#define ___K_ERROR_H

	#define ERROR_SUCCESS				(0)
	#define ERROR_GENERAL				(-1)
	#define ERROR_UNKNOWN				(-2)
	#define ERROR_CRITICAL				(-3)
	#define ERROR_FATAL				(-4)

	#define ERROR_INVALID_ARG			(-9)
	#define ERROR_INVALID_ARG_VAL			(-10)
	#define ERROR_INVALID_OPERATION			(-11)
	#define ERROR_INVALID_RESOURCE_NAME		(-12)

	#define ERROR_MEMORY_NOMEM			(-17)
	#define ERROR_MEMORY_NOMEM_VIRTUAL		(-18)
	#define ERROR_MEMORY_NOMEM_PHYSICAL		(-19)
	#define ERROR_MEMORY_VIRTUAL_PAGEMAP		(-20)
	// We should never need to use this.
	#define ERROR_MEMORY_NOMEM_IN_CONSTRUCTOR	(-21)

	#define ERROR_UNIMPLEMENTED			(-25)
	#define ERROR_UNSUPPORTED			(-26)
	#define ERROR_UNINITIALIZED			(-27)
	#define ERROR_UNAUTHORIZED			(-28)

	#define ERROR_RESOURCE_BUSY			(-41)
	#define ERROR_RESOURCE_UNAVAILABLE		(-42)
	#define ERROR_RESOURCE_EXHAUSTED		(-43)

	#define ERROR_WOULD_BLOCK			(-49)
	#define ERROR_NOT_FOUND				(-50)
	#define ERROR_LIMIT_OVERFLOWED			(-51)

	#define UNIMPLEMENTED(__funcName)		\
		__kprintf(ERROR"Unimplemented: %s on line %d of file %s.\n", \
			__funcName, __LINE__, __FILE__);

#endif

