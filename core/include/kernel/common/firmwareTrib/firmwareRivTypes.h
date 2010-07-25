#ifndef _FIRMWARE_RIVULET_TYPES_H
	#define _FIRMWARE_RIVULET_TYPES_H

	#include <arch/arch.h>
	#include <__kstdlib/__kerror.h>
	#include <__kstdlib/compiler/stdint.h>

/**	EXPLANATION:
 * It's important that we provide a set of sure-sized types for the rivulets,
 * so that we don't have "int" and "long" and stuff like that appearing in the
 * kernel source, even if it's in the firmware rivulets.
 **/

#define __KNULL		(0)

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

#endif

