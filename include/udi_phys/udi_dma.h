/*
 * File: udi_dma.h
 *
 * UDI DMA Public Definitions
 *
 * UDI drivers must not directly #include this file. It is included as
 * part of udi_physio.h.
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
 */

#ifndef _UDI_PHYSIO_DMA_H
#define _UDI_PHYSIO_DMA_H

#ifndef _UDI_PHYSIO_H_INSIDE
#error "udi_dma.h must not be #included directly; use udi_physio.h instead."
#endif

#define _UDI_OPAQUE_64	struct { \
				udi_ubit32_t first_32; \
				udi_ubit32_t second_32; \
			}

typedef _UDI_OPAQUE_64	udi_busaddr64_t;

/* DMA constraints handle layout element type code */
#define UDI_DL_DMA_CONSTRAINTS_T     201

/*
 * DMA constraints attribute properties.
 * These numbers must start above the last generic constraints attribute.
 */

/* DMA Convenience Attribute Codes */
#define UDI_DMA_ADDRESSABLE_BITS		100
#define UDI_DMA_ALIGNMENT_BITS			101

/* DMA Constraints on the Entire Transfer */
#define UDI_DMA_DATA_ADDRESSABLE_BITS		110
#define	UDI_DMA_NO_PARTIAL			111

/* DMA Constraints on the Scatter/Gather List */
#define	UDI_DMA_SCGTH_MAX_ELEMENTS		120
#define UDI_DMA_SCGTH_FORMAT			121
#define UDI_DMA_SCGTH_ENDIANNESS                122
#define UDI_DMA_SCGTH_ADDRESSABLE_BITS		123
#define UDI_DMA_SCGTH_MAX_SEGMENTS		124

/* DMA Constraints on the Scatter/Gather Segments */
#define UDI_DMA_SCGTH_ALIGNMENT_BITS		130
#define UDI_DMA_SCGTH_MAX_EL_PER_SEG		131
#define	UDI_DMA_SCGTH_PREFIX_BYTES		132

/* DMA Constraints on the Scatter/Gather Elements */
#define UDI_DMA_ELEMENT_ALIGNMENT_BITS	        140
#define UDI_DMA_ELEMENT_LENGTH_BITS		141
#define	UDI_DMA_ELEMENT_GRANULARITY_BITS	142

/* DMA Constraints for Special Addressing */
#define	UDI_DMA_ADDR_FIXED_BITS			150
#define	UDI_DMA_ADDR_FIXED_TYPE			151
#define	UDI_DMA_ADDR_FIXED_VALUE_LO		152
#define	UDI_DMA_ADDR_FIXED_VALUE_HI		153

/* DMA Constraints on DMA Access Behavior */
#define UDI_DMA_SEQUENTIAL			160
#define UDI_DMA_SLOP_IN_BITS			161
#define UDI_DMA_SLOP_OUT_BITS			162
#define UDI_DMA_SLOP_OUT_EXTRA			163
#define UDI_DMA_SLOP_BARRIER_BITS		164

/*
 * Values for UDI_DMA_SCGTH_ENDIANNESS
 */
#define UDI_DMA_BIG_ENDIAN      (1U<<5)
#define UDI_DMA_LITTLE_ENDIAN   (1U<<6)

/*
 * Values for UDI_DMA_ADDR_FIXED_TYPE.
 */
#define UDI_DMA_FIXED_ELEMENT	                1
#define UDI_DMA_FIXED_LIST	                2
#define UDI_DMA_FIXED_VALUE	                3

/*
 * Defines for flags for udi_dma_prepare, udi_dma_buf_map, udi_dma_buf_unmap,
 *	udi_dma_sync and udi_dma_mem_alloc. UDI_MEM_NOZERO (1U<<0) is
 *	defined elsewhere.
 */
#define UDI_DMA_ANY_ENDIAN	(1U<<0)
#define	UDI_DMA_OUT	(1U<<2)
#define UDI_DMA_IN	(1U<<3)
#define UDI_DMA_REWIND	(1U<<4)	/* Valid for udi_dma_buf_map only */
#define UDI_DMA_NEVERSWAP	(1U<<7)

/*
 * Scatter/gather extension flag; used in either the block_length field
 *	of a udi_scgth_element_32_t or the el_reserved field of a
 *	udi_scgth_element_64_t.
 */
#define	UDI_SCGTH_EXT	0x80000000

/*
 * Defines for the scgth_format field.
 */
#define UDI_SCGTH_32	        (1U<<0)
#define	UDI_SCGTH_64	        (1U<<1)
#define UDI_SCGTH_DMA_MAPPED    (1U<<6)
#define UDI_SCGTH_DRIVER_MAPPED (1U<<7)

/*
 * DMA related structures.
 */
typedef struct {
	udi_size_t	max_legal_contig_alloc;
	udi_size_t	max_safe_contig_alloc;
	udi_size_t	cache_line_size;
} udi_dma_limits_t;

#define UDI_DMA_MIN_ALLOC_LIMIT	4000

typedef struct {
	udi_ubit32_t	block_busaddr;
	udi_ubit32_t	block_length;
} udi_scgth_element_32_t;

typedef struct {
	udi_busaddr64_t	block_busaddr;
	udi_ubit32_t	block_length;
	udi_ubit32_t	el_reserved;
} udi_scgth_element_64_t;

typedef struct {
	udi_ubit16_t	scgth_num_elements;
	udi_ubit8_t	scgth_format;
	udi_boolean_t	scgth_must_swap;
	union {
		udi_scgth_element_32_t *el32p;
		udi_scgth_element_64_t *el64p;
	} scgth_elements;
	union {
		udi_scgth_element_32_t el32;
		udi_scgth_element_64_t el64;
	} scgth_first_segment;
} udi_scgth_t;

/*
 * DMA related callback definitions.
 */
typedef void udi_dma_prepare_call_t(
			udi_cb_t *gcb,
			udi_dma_handle_t new_dma_handle);
typedef void udi_dma_buf_map_call_t(
			udi_cb_t *gcb,
			udi_scgth_t *scgth,
			udi_boolean_t complete,
			udi_status_t status);
typedef void udi_dma_mem_alloc_call_t(
			udi_cb_t *gcb,
			udi_dma_handle_t new_dma_handle,
                        void *mem_ptr,
			udi_size_t actual_gap,
			udi_boolean_t single_element,
			udi_scgth_t *scgth,
                        udi_boolean_t must_swap);
typedef void udi_dma_sync_call_t(
			udi_cb_t *gcb);
typedef void udi_dma_scgth_sync_call_t(
			udi_cb_t *gcb);
typedef void udi_dma_mem_to_buf_call_t(
			udi_cb_t *gcb,
			udi_buf_t *new_dst_buf);

/*
 * DMA related metalanguage function definitions.
 */
void udi_dma_limits(
		udi_dma_limits_t *dma_limits);
void udi_dma_prepare (
		udi_dma_prepare_call_t *callback,
		udi_cb_t *gcb,
		udi_dma_constraints_t constraints,
		udi_ubit8_t flags);
void udi_dma_buf_map(
		udi_dma_buf_map_call_t *callback,
		udi_cb_t *gcb,
		udi_dma_handle_t dma_handle,
		udi_buf_t *buf,
		udi_size_t offset,
		udi_size_t len,
		udi_ubit8_t flags);
udi_buf_t * udi_dma_buf_unmap(
		udi_dma_handle_t dma_handle,
		udi_size_t new_buf_size);
void udi_dma_mem_alloc(
		udi_dma_mem_alloc_call_t *callback,
		udi_cb_t *gcb,
		udi_dma_constraints_t constraints,
		udi_ubit8_t flags,
		udi_ubit16_t nelements,
		udi_size_t element_size,
		udi_size_t max_gap);
void udi_dma_sync(
		udi_dma_sync_call_t *callback,
		udi_cb_t *gcb,
		udi_dma_handle_t dma_handle,
		udi_size_t offset,
		udi_size_t length,
		udi_ubit8_t flags);
void udi_dma_scgth_sync(
		udi_dma_scgth_sync_call_t *callback,
		udi_cb_t *gcb,
		udi_dma_handle_t dma_handle);
void udi_dma_mem_barrier(
		udi_dma_handle_t dma_handle);
void udi_dma_free(
		udi_dma_handle_t dma_handle);
void udi_dma_mem_to_buf(
		udi_dma_mem_to_buf_call_t *callback,
		udi_cb_t *gcb,
		udi_dma_handle_t dma_handle,
		udi_size_t src_off,
		udi_size_t src_len,
		udi_buf_t *dst_buf);

#endif /* _UDI_PHYSIO_DMA_H */
