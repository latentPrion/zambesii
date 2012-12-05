#ifndef ___K_ERROR_H
	#define ___K_ERROR_H

	#define ERROR_SUCCESS				0
	#define ERROR_GENERAL				-1
	#define ERROR_UNKNOWN				-2
	#define ERROR_CRITICAL				-10
	#define ERROR_FATAL				-11
	#define ERROR_INVALID_ARG_VAL			-3
	#define ERROR_INVALID_ARG			-4
	#define ERROR_MEMORY_NOMEM			-5
	#define ERROR_MEMORY_VIRTUAL_PAGEMAP		-6
	#define ERROR_MEMORY_NOMEM_VIRTUAL		-7
	#define ERROR_MEMORY_NOMEM_PHYSICAL		-8
	// Hopefully we should never need to use this.
	#define ERROR_MEMORY_NOMEM_IN_CONSTRUCTOR	-9
	#define ERROR_UNIMPLEMENTED			-12
	#define ERROR_RESOURCE_BUSY			-13
	#define ERROR_UNAUTHORIZED			-14
	#define ERROR_RESOURCE_UNAVAILABLE		-15
	#define ERROR_NOT_SUPPORTED			-16

	#define UNIMPLEMENTED(__funcName)		\
		__kprintf(ERROR"Unimplemented: %s on line %d of file %s.\n", \
			__funcName, __LINE__, __FILE__);

#endif

