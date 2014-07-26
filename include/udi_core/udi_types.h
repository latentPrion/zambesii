/*
 * File: udi_types.h
 *
 * Basic type definitions for UDI.
 *
 * UDI drivers must not directly #include this file. It is included as
 * part of udi.h.
 *
 * $Copyright udi_reference:
 * 
 * 
 *    Copyright (c) 1995-2001; Compaq Computer Corporation; Hewlett-Packard
 *    Company; Interphase Corporation; The Santa Cruz Operation, Inc;
 *    Software Technologies Group, Inc; and Sun Microsystems, Inc
 *    (collectively, the "Copyright Holders").  All rights reserved.
 * 
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the conditions are met:
 * 
 *            Redistributions of source code must retain the above
 *            copyright notice, this list of conditions and the following
 *            disclaimer.
 * 
 *            Redistributions in binary form must reproduce the above
 *            copyright notice, this list of conditions and the following
 *            disclaimers in the documentation and/or other materials
 *            provided with the distribution.
 * 
 *            Neither the name of Project UDI nor the names of its
 *            contributors may be used to endorse or promote products
 *            derived from this software without specific prior written
 *            permission.
 * 
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *    "AS IS," AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *    HOLDERS OR ANY CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 *    DAMAGE.
 * 
 *    THIS SOFTWARE IS BASED ON SOURCE CODE PROVIDED AS A SAMPLE REFERENCE
 *    IMPLEMENTATION FOR VERSION 1.01 OF THE UDI CORE SPECIFICATION AND/OR
 *    RELATED UDI SPECIFICATIONS. USE OF THIS SOFTWARE DOES NOT IN AND OF
 *    ITSELF CONSTITUTE CONFORMANCE WITH THIS OR ANY OTHER VERSION OF ANY
 *    UDI SPECIFICATION.
 * 
 * 
 * $
 * 
 * udi_types.h contains the only public definitions for UDI drivers that
 * may need to be provided differently on different classes of platforms.
 * The definitions below for the fixed-length types will only work with
 * ILP32, LP64 and P64 data models. Once the ISO C fixed-length types
 * are defined (in the C9X rev of the ISO C standard), this udi_types.h
 * can be made even more portable by defining the UDI types in terms of
 * the ISO C fixed-length types.
 * 
 * However, the definitions of the abstract types can still be different
 * on different platforms.  Typically these definitions will vary based
 * on processor data word size and platform constraints, so it's expected
 * that most 32-bit platforms could all have the same definitions for
 * the abstract types; similarly for 64-bit systems.
 *
 * Given the above it would appear likely that we can provide a single
 * udi_types.h file that works correctly for most 32-bit and 64-bit
 * platforms.  Other size platforms, or specialized platforms, may need
 * to create their own udi_types.h.
 * 
 * Binary Portability
 * -------------------
 * UDI drivers are by definition completely source-code portable and
 * are binary portable within a given ABI (Architected Binary Interface).
 * An ABI is determined by several factors: the ISA (Instruction Set 
 * Architecture), endianness, data model (e.g. 32- vs. 64-bit), data
 * structure alignments and sizes, calling conventions, and the UDI API
 * definitions. By defining data type sizes, this file is providing
 * some aspects of an ABI binding, whether defacto or corresponding to
 * an actual ABI specification.
 */

#ifndef _UDI_CORE_TYPES_H
#define _UDI_CORE_TYPES_H

#ifndef _UDI_H_INSIDE
#error "udi_types.h must not be #included directly; use udi.h instead."
#endif

#include <config.h>
#include <stdint.h>
#include <stddef.h>

/*
 * Basic types.
 */
typedef uint8_t		udi_ubit8_t;	/* unsigned 8-bit: 0.. 2^8-1 */
typedef int8_t		udi_sbit8_t;	/* signed 8-bit: -2^7..2^7-1 */
typedef uint16_t	udi_ubit16_t;	/* unsigned 16-bit: 0..2^16-1 */
typedef int16_t		udi_sbit16_t;	/* signed 16-bit: -2^15 .. 2^15-1 */
typedef uint32_t	udi_ubit32_t;	/* unsigned 32-bit: 0..2^32-1 */
typedef int32_t		udi_sbit32_t;	/* signed 32-bit: -2^31..2^31-1 */

typedef udi_ubit8_t	udi_boolean_t;	/* 0=False; 1..2^8-1=True */
#undef FALSE
#undef TRUE
#define FALSE 0
#define TRUE  1

/*
 * Abstract types (defined in terms of basic types).
 * The sizes below are not guaranteed by the core spec, but match most ABIs.
 * Note, the udi_size_t definition, in particular, is known to change
 * size between the various 32-bit ABIs (like IA32) and the 64-bit ABIs 
 * (like IA64 and UltraSparc.)   At this writing, all the 64-bit targets
 * of interest use L.*64 model, so this definition does correctly resolve
 * for both sizes.   A more flexible scheme for addressing this has been
 * proposed.
 */
typedef size_t		udi_size_t;
typedef udi_ubit8_t	udi_index_t;

/*
 * Representation for Opaque Handle Types.
 */
#define _UDI_HANDLE	void *

/*
 * UDI-defined macros for working with handle types.
 */
#define UDI_HANDLE_IS_NULL(handle, handle_type) \
		((handle) == NULL)

#define UDI_HANDLE_ID(handle, handle_type) \
		(handle)

/*
 * opaque types
 */
typedef _UDI_HANDLE     	udi_buf_path_t;
#define UDI_NULL_BUF_PATH       ((udi_buf_path_t)NULL)

/*
 * udi_timestamp_t is a self-contained opaque type.
 */
typedef unsigned long	udi_timestamp_t;

/*
 * Standard ISO C NULL value.
 */
#ifndef NULL
#error "Please use a compiler that provides stddef.h."
#endif

/*
 * Standard ISO C offsetof().
 */
#ifndef offsetof
#define offsetof(s, m)	(udi_size_t)(&(((s *)0)->m))
#endif

/*
 * Standard ISO C Variable-Argument macros.
 *
 * This implementation gets these from the native system's stdarg.h.
 */
#include <stdarg.h>

/*
 * For elements smaller than sizeof(int), ISO C won't let us use
 * va_arg() directly, so we have to promote to an int type and then
 * cast to the real type we're interested in.
 *
 * For each of the data types we need to handle, map a macro based on
 * the corresponding "VA" symbol to either va_arg or _UDI_VA_ARG_INT.
 */
#define _UDI_VA_ARG_INT(ap, ty)		(ty) va_arg(ap, int)

/*
 * These can be assumed to be at most sizeof(int) on all platforms,
 * and therefore need to be promoted:
 */
#define _UDI_VA_UBIT8_T			_UDI_VA_ARG_INT
#define _UDI_VA_SBIT8_T			_UDI_VA_ARG_INT
#define _UDI_VA_UBIT16_T		_UDI_VA_ARG_INT
#define _UDI_VA_SBIT16_T		_UDI_VA_ARG_INT

/*
 * These might be smaller or larger than sizeof(int), but all platforms
 * currently supported have these at least sizeof(int):
 */
#define _UDI_VA_UBIT32_T		va_arg
#define _UDI_VA_SBIT32_T		va_arg

/* Pointer types don't get promoted: */
#define _UDI_VA_POINTER			va_arg

/* We expect UDI ABIs to use types that don't get promoted for handles: */
#define _UDI_VA_CHANNEL_T		va_arg
#define _UDI_VA_ORIGIN_T		va_arg
#define _UDI_VA_DMA_CONSTRAINTS_T	va_arg

/* These are guaranteed to be certain sizes by the UDI Core Specification: */
#define _UDI_VA_BOOLEAN_T		_UDI_VA_UBIT8_T
#define _UDI_VA_STATUS_T		_UDI_VA_UBIT32_T

/* These may vary by platform, but match the current udi_types.h sizes: */
#define _UDI_VA_SIZE_T			_UDI_VA_UBIT32_T
#define _UDI_VA_INDEX_T			_UDI_VA_UBIT8_T

/*
 * This macro does "the right thing" for UDI data types.
 * va_code is one of the above VA type codes without the leading underscore.
 */
#define UDI_VA_ARG(ap,type,va_code) \
		_##va_code(ap,type)

#endif /* _UDI_CORE_TYPES_H */

