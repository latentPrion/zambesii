#ifndef __KTYPES_H
	#define __KTYPES_H

	#include <arch/arch.h>
	#include <__kstdlib/compiler/stdint.h>
	#include <__kstdlib/__kstddef.h>

	#define ERROR_SUCCESS				0
	#define ERROR_GENERAL				-1
	#define ERROR_UNKNOWN				-2
	#define ERROR_CRITICAL				-10
	#define ERROR_FATAL				-11
	#define ERROR_INVALID_ARG_VAL			-3
	#define ERROR_INVALID_ARG			-4
	#define ERROR_MEMORY_NOMEM			-5
	#define ERROR_MEMORY___KSPACE_NOMEM		-6
	#define ERROR_MEMORY_NOMEM_VIRTUAL		-7
	#define ERROR_MEMORY_NOMEM_PHYSICAL		-8
	// Hopefully we should never need to use this.
	#define ERROR_MEMORY_NOMEM_IN_CONSTRUCTOR	-9

#ifndef __ASM__

namespace __ktypes
{
	typedef int8_t		sbit8;
	typedef int16_t		sbit16;
	typedef int32_t		sbit32;
	typedef int64_t		sbit64;
	
	typedef uint8_t		ubit8;
	typedef uint16_t	ubit16;
	typedef uint32_t	ubit32;
	typedef uint64_t	ubit64;
	
	//Define Zambezii's integral types.
#ifdef __64_BIT__
	typedef uint64_t	uarch_t;
	typedef int64_t		sarch_t;
#elif defined (__32_BIT__)
	typedef uint32_t	uarch_t;
	typedef int32_t		sarch_t;
#endif

	typedef sarch_t		error_t;
	typedef sarch_t		status_t;

	typedef ubit8		utf8Char;
	typedef ubit16		utf16Char;
}

using namespace __ktypes;

#endif /* !defined( __ASM__ ) */

#endif

