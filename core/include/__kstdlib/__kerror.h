#ifndef ___K_ERROR_H
	#define ___K_ERROR_H

	#define ERROR_SUCCESS				(0)
	#define ERROR_GENERAL				(-1)
	#define ERROR_UNKNOWN				(-2)
	#define ERROR_CRITICAL				(-3)
	#define ERROR_FATAL				(-4)

	#define ERROR_INVALID_ARG			(-10)
	#define ERROR_INVALID_ARG_VAL			(-11)
	#define ERROR_INVALID_OPERATION			(-12)
	#define ERROR_INVALID_RESOURCE_NAME		(-13)
	#define ERROR_INVALID_FORMAT			(-14)

	#define ERROR_MEMORY_NOMEM			(-20)
	#define ERROR_MEMORY_NOMEM_VIRTUAL		(-21)
	#define ERROR_MEMORY_NOMEM_PHYSICAL		(-22)
	#define ERROR_MEMORY_VIRTUAL_PAGEMAP		(-23)
	// We should never need to use this.
	#define ERROR_MEMORY_NOMEM_IN_CONSTRUCTOR	(-24)

	#define ERROR_UNIMPLEMENTED			(-30)
	#define ERROR_UNSUPPORTED			(-31)
	#define ERROR_UNINITIALIZED			(-32)
	#define ERROR_UNAUTHORIZED			(-33)

	#define ERROR_RESOURCE_BUSY			(-40)
	#define ERROR_RESOURCE_UNAVAILABLE		(-41)
	#define ERROR_RESOURCE_EXHAUSTED		(-42)

	#define ERROR_WOULD_BLOCK			(-50)
	#define ERROR_NOT_FOUND				(-51)
	#define ERROR_LIMIT_OVERFLOWED			(-52)

	#define UNIMPLEMENTED(__funcName)		\
		printf(ERROR"Unimplemented: %s on line %d of file %s.\n", \
			__funcName, __LINE__, __FILE__);

#endif

