/*
 * File: udi_pio.h
 *
 * UDI Programmed I/O Public Definitions
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

#ifndef _UDI_PHYSIO_PIO_H
#define _UDI_PHYSIO_PIO_H

#ifndef _UDI_PHYSIO_H_INSIDE
#error "udi_pio.h must not be #included directly; use udi_physio.h instead."
#endif

typedef _UDI_HANDLE	udi_pio_handle_t;
#define UDI_NULL_PIO_HANDLE	((udi_pio_handle_t)NULL)
/* PIO Handle Layout Element Type Code */
#define UDI_DL_PIO_HANDLE_T	200

/*
 * Programmed I/O (PIO) constraint attribute flags (pio_attributes).
 */
#define	UDI_PIO_STRICTORDER	(1U<<0)
#define UDI_PIO_UNORDERED_OK	(1U<<1)
#define UDI_PIO_MERGING_OK	(1U<<2)
#define UDI_PIO_LOADCACHING_OK	(1U<<3)
#define UDI_PIO_STORECACHING_OK	(1U<<4)
#define UDI_PIO_BIG_ENDIAN	(1U<<5)
#define UDI_PIO_LITTLE_ENDIAN	(1U<<6)
#define UDI_PIO_NEVERSWAP	(1U<<7)
#define UDI_PIO_UNALIGNED	(1U<<8)

/*
 * PIO Class A addressing modes in pio_op in udi_pio_trans_t.
 */
#define _UDI_PIO_ENCODE_ADDRESS_MODE(x) (x << 3)
#define UDI_PIO_DIRECT  _UDI_PIO_ENCODE_ADDRESS_MODE(0)
#define UDI_PIO_SCRATCH _UDI_PIO_ENCODE_ADDRESS_MODE(1)
#define UDI_PIO_BUF     _UDI_PIO_ENCODE_ADDRESS_MODE(2)
#define UDI_PIO_MEM     _UDI_PIO_ENCODE_ADDRESS_MODE(3)

/*
 * PIO Class A opcodes.
 */
#define _UDI_PIO_ENCODE_CLASS_A(x) (udi_ubit8_t) (x << 5)
#define UDI_PIO_IN	_UDI_PIO_ENCODE_CLASS_A(0)
#define UDI_PIO_OUT	_UDI_PIO_ENCODE_CLASS_A(1)
#define UDI_PIO_LOAD	_UDI_PIO_ENCODE_CLASS_A(2)
#define UDI_PIO_STORE	_UDI_PIO_ENCODE_CLASS_A(3)

/*
 * PIO Class B opcodes.
 */
#define _UDI_PIO_ENCODE_CLASS_B(x) (udi_ubit8_t) (0x80 | (x) << 3)
#define UDI_PIO_LOAD_IMM	_UDI_PIO_ENCODE_CLASS_B( 0)
#define UDI_PIO_CSKIP		_UDI_PIO_ENCODE_CLASS_B( 1)
#define UDI_PIO_IN_IND		_UDI_PIO_ENCODE_CLASS_B( 2)
#define UDI_PIO_OUT_IND		_UDI_PIO_ENCODE_CLASS_B( 3)
#define UDI_PIO_SHIFT_LEFT	_UDI_PIO_ENCODE_CLASS_B( 4)
#define UDI_PIO_SHIFT_RIGHT	_UDI_PIO_ENCODE_CLASS_B( 5)
#define UDI_PIO_AND		_UDI_PIO_ENCODE_CLASS_B( 6)
#define UDI_PIO_AND_IMM		_UDI_PIO_ENCODE_CLASS_B( 7)
#define UDI_PIO_OR		_UDI_PIO_ENCODE_CLASS_B( 8)
#define UDI_PIO_OR_IMM		_UDI_PIO_ENCODE_CLASS_B( 9)
#define UDI_PIO_XOR		_UDI_PIO_ENCODE_CLASS_B(10)
#define UDI_PIO_ADD		_UDI_PIO_ENCODE_CLASS_B(11)
#define UDI_PIO_ADD_IMM		_UDI_PIO_ENCODE_CLASS_B(12)
#define UDI_PIO_SUB		_UDI_PIO_ENCODE_CLASS_B(13)

/*
 * PIO Class C opcodes.
 */
#define _UDI_PIO_ENCODE_CLASS_C(x) (udi_ubit8_t) (0xf0 | (x))
#define UDI_PIO_BRANCH		_UDI_PIO_ENCODE_CLASS_C( 0)
#define UDI_PIO_LABEL		_UDI_PIO_ENCODE_CLASS_C( 1)
#define UDI_PIO_REP_IN_IND	_UDI_PIO_ENCODE_CLASS_C( 2)
#define UDI_PIO_REP_OUT_IND	_UDI_PIO_ENCODE_CLASS_C( 3)
#define UDI_PIO_DELAY		_UDI_PIO_ENCODE_CLASS_C( 4)
#define UDI_PIO_BARRIER		_UDI_PIO_ENCODE_CLASS_C( 5)
#define UDI_PIO_SYNC		_UDI_PIO_ENCODE_CLASS_C( 6)
#define UDI_PIO_SYNC_OUT	_UDI_PIO_ENCODE_CLASS_C( 7)
#define UDI_PIO_DEBUG		_UDI_PIO_ENCODE_CLASS_C( 8)
#define UDI_PIO_END		_UDI_PIO_ENCODE_CLASS_C(14)
#define UDI_PIO_END_IMM		_UDI_PIO_ENCODE_CLASS_C(15)

/*
 * Condition codes for UDI_PIO_CSKIP.
 */
#define UDI_PIO_Z	      	0	/* reg == 0 */
#define UDI_PIO_NZ	     	1	/* reg != 0 */
#define UDI_PIO_NEG     	2	/* reg < 0 [signed] */
#define UDI_PIO_NNEG     	3	/* reg >= 0 [signed] */

/*
 * Convenience macro for encoding args to UDI_PIO_REP_{IN,OUT}_IND.
 */
#define UDI_PIO_REP_ARGS(mode,mem_reg,mem_stride,pio_reg,pio_stride,cnt_reg) \
          ((mode)|(mem_reg)|((mem_stride)<<5)| \
	  ((pio_reg)<<7)|((pio_stride)<<10)|\
	  ((cnt_reg)<<13))

/*
 * Mnemonic constants for trans_size values.
 */
#define UDI_PIO_1BYTE		0
#define UDI_PIO_2BYTE		1
#define UDI_PIO_4BYTE		2
#define UDI_PIO_8BYTE		3
#define UDI_PIO_16BYTE		4
#define UDI_PIO_32BYTE		5

/*
 * Mnemonic constants for selecting registers.
 */
#define UDI_PIO_R0		0
#define UDI_PIO_R1		1
#define UDI_PIO_R2		2
#define UDI_PIO_R3		3
#define UDI_PIO_R4		4
#define UDI_PIO_R5		5
#define UDI_PIO_R6		6
#define UDI_PIO_R7		7

/*
 * Mnemonic constants for tracing levels.
 */
#define UDI_PIO_TRACE_OPS_NONE	0
#define UDI_PIO_TRACE_OPS1	1
#define UDI_PIO_TRACE_OPS2	2
#define UDI_PIO_TRACE_OPS3	3
#define UDI_PIO_TRACE_REGS_NONE	(0U << 2)
#define UDI_PIO_TRACE_REGS1	(1U << 2)
#define UDI_PIO_TRACE_REGS2	(2U << 2)
#define UDI_PIO_TRACE_REGS3	(3U << 2)
#define UDI_PIO_TRACE_DEV_NONE	(0U << 4)
#define UDI_PIO_TRACE_DEV1	(1U << 4)
#define UDI_PIO_TRACE_DEV2	(2U << 4)
#define UDI_PIO_TRACE_DEV3	(3U << 4)

/*
 * PIO related structure definitions.
 */
typedef const struct _udi_pio_trans_s {
        udi_ubit8_t pio_op;
        udi_ubit8_t tran_size;
        udi_ubit16_t operand;
} udi_pio_trans_t;

/*
 * PIO related callback definitions.
 */
typedef void udi_pio_map_call_t(
			udi_cb_t *gcb,
			udi_pio_handle_t new_pio_handle);
typedef void udi_pio_trans_call_t(
			udi_cb_t *gcb,
			udi_buf_t *new_buf,
	                udi_status_t status,
	                udi_ubit16_t result);
typedef void udi_pio_probe_call_t(
			udi_cb_t *gcb,
			udi_status_t status);

/*
 * PIO related metalanguage function definitions.
 */
void udi_pio_map(
	udi_pio_map_call_t *callback,
	udi_cb_t *gcb,
	udi_ubit32_t regset_idx,
	udi_ubit32_t base_offset,
	udi_ubit32_t length,
       	udi_pio_trans_t *trans_list,
	udi_ubit16_t list_length,
	udi_ubit16_t pio_attributes,
	udi_ubit32_t pace,
	udi_index_t serialization_domain);

void udi_pio_unmap(
	udi_pio_handle_t pio_handle);

udi_ubit32_t udi_pio_atomic_sizes(
	udi_pio_handle_t pio_handle);

void udi_pio_abort_sequence(
	udi_pio_handle_t pio_handle,
	udi_size_t scratch_requirement);

void udi_pio_trans(
	udi_pio_trans_call_t *callback,
	udi_cb_t *gcb,
	udi_pio_handle_t pio_handle,
	udi_index_t start_label,
	udi_buf_t *buf,
	void *mem_ptr);

void udi_pio_probe(
	udi_pio_probe_call_t *callback,
	udi_cb_t *gcb,
	udi_pio_handle_t pio_handle,
	void *mem_ptr,
	udi_ubit32_t pio_offset,
	udi_ubit8_t trans_size,
	udi_ubit8_t direction);

#endif /* _UDI_PHYSIO_PIO_H */
