#ifndef ___K_ERROR_H
	#define ___K_ERROR_H

	#define ERROR_SUCCESS				((error_t)0)
	#define ERROR_GENERAL				((error_t)-1)
	#define ERROR_UNKNOWN				((error_t)-2)
	#define ERROR_CRITICAL				((error_t)-3)
	#define ERROR_FATAL				((error_t)-4)
	#define ERROR_DUPLICATE				((error_t)-5)
	#define ERROR_NON_CONFORMANT			((error_t)-6)

	#define ERROR_INVALID_ARG			((error_t)-10)
	#define ERROR_INVALID_ARG_VAL			((error_t)-11)
	#define ERROR_INVALID_OPERATION			((error_t)-12)
	#define ERROR_INVALID_RESOURCE_NAME		((error_t)-13)
	#define ERROR_INVALID_FORMAT			((error_t)-14)
	#define ERROR_INVALID_RESOURCE_HANDLE		((error_t)-15)
	#define ERROR_INVALID_RESOURCE_ID		((error_t)-16)
	#define ERROR_INVALID_STATE			((error_t)-17)

	#define ERROR_MEMORY_NOMEM			((error_t)-20)
	#define ERROR_MEMORY_NOMEM_VIRTUAL		((error_t)-21)
	#define ERROR_MEMORY_NOMEM_PHYSICAL		((error_t)-22)
	#define ERROR_MEMORY_VIRTUAL_PAGEMAP		((error_t)-23)
	// We should never need to use this.
	#define ERROR_MEMORY_NOMEM_IN_CONSTRUCTOR	((error_t)-24)

	#define ERROR_UNIMPLEMENTED			((error_t)-30)
	#define ERROR_UNSUPPORTED			((error_t)-31)
	#define ERROR_UNINITIALIZED			((error_t)-32)
	#define ERROR_UNAUTHORIZED			((error_t)-33)
	#define ERROR_INITIALIZATION_FAILURE		((error_t)-34)

	#define ERROR_RESOURCE_BUSY			((error_t)-40)
	#define ERROR_RESOURCE_UNAVAILABLE		((error_t)-41)
	#define ERROR_RESOURCE_EXHAUSTED		((error_t)-42)

	#define ERROR_WOULD_BLOCK			((error_t)-50)
	#define ERROR_NOT_FOUND				((error_t)-51)
	#define ERROR_LIMIT_OVERFLOWED			((error_t)-52)
	#define ERROR_NO_MATCH				((error_t)-53)

	#define UNIMPLEMENTED(__funcName)		\
		printf(ERROR"Unimplemented: %s on line %d of file %s.\n", \
			__funcName, __LINE__, __FILE__);

#endif

