#ifndef __KTYPES_H
	#define __KTYPES_H

	#include <arch/arch.h>
	#include <__kstdlib/__kerror.h>
	#include <__kstdlib/compiler/stdint.h>
	#include <__kstdlib/__kclib/stddef.h>
	#include <__kstdlib/__kcxxCast.h>

#ifndef __ASM__

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

// Zambezii supports unicode in the form of UTF-8.
typedef ubit8		utf8Char;
typedef ubit16		utf16Char;
typedef ubit32		unicodePoint;

#endif /* !defined( __ASM__ ) */

#endif

