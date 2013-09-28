/*
 * File: udi_inst_attr.h
 *
 * Public definitions for UDI Instance Attribute Management.
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
 */

#ifndef _UDI_CORE_INST_ATTR_H
#define _UDI_CORE_INST_ATTR_H

#ifndef _UDI_H_INSIDE
#error "udi_inst_attr.h must not be #included directly; use udi.h instead."
#endif

typedef udi_ubit8_t  udi_instance_attr_type_t;

/*
 * Instance Attribute data types.
 */
#define	UDI_ATTR_NONE		0
#define	UDI_ATTR_STRING		1
#define UDI_ATTR_ARRAY8		2
#define UDI_ATTR_UBIT32		3
#define	UDI_ATTR_BOOLEAN	4
#define	UDI_ATTR_FILE		5

#define UDI_MAX_ATTR_NAMELEN	32
#define UDI_MAX_ATTR_SIZE	64

/*
 * Enumeration instance attribute list.
 */
typedef struct {
	char attr_name[UDI_MAX_ATTR_NAMELEN];
	udi_ubit8_t attr_value[UDI_MAX_ATTR_SIZE];
	udi_ubit8_t attr_length;
	udi_instance_attr_type_t attr_type;
} udi_instance_attr_list_t;

/*
 * Macros
 */
#define UDI_INSTANCE_ATTR_DELETE(callback, gcb, attr_name) \
		udi_instance_attr_set(callback, gcb, attr_name, \
				 0, 0, UDI_ATTR_NONE);

#define UDI_ATTR32_INIT(v) \
		{ (v) & 0xff, ((v) >> 8) & 0xff,  \
			((v) >> 16) & 0xff, ((v) >> 24) & 0xff }

#define UDI_ATTR32_SET(aval, v) \
		{ udi_ubit32_t vtmp = (v); \
		  (aval)[0] = (vtmp) & 0xff; \
		  (aval)[1] = ((vtmp) >> 8) & 0xff; \
		  (aval)[2] = ((vtmp) >> 16) & 0xff; \
		  (aval)[3] = ((vtmp) >> 24) & 0xff; }

#define UDI_ATTR32_GET(aval) \
		((udi_ubit8_t)(aval)[0] | ((udi_ubit8_t)(aval)[1] << 8) | \
		((udi_ubit8_t)(aval)[2] << 16) | ((udi_ubit8_t)(aval)[3] << 24))

/*
 * Callback types.
 */
typedef void udi_instance_attr_get_call_t(
			udi_cb_t *gcb,
			udi_instance_attr_type_t attr_type,
			udi_size_t actual_length);
typedef void udi_instance_attr_set_call_t(
			udi_cb_t *gcb,
			udi_status_t status);

/*
 * Function prototypes for instance attribute management
 */
void udi_instance_attr_get(
			udi_instance_attr_get_call_t *callback,
			udi_cb_t *gcb,
			const char *attr_name,
			udi_ubit32_t child_id,
			void *attr_value,
			udi_size_t attr_length);
void udi_instance_attr_set(
			udi_instance_attr_set_call_t *callback,
			udi_cb_t *gcb,
			const char *attr_name,
			udi_ubit32_t child_id,
			const void *attr_value,
			udi_size_t attr_length,
			udi_ubit8_t attr_type);

#endif /* _UDI_CORE_INST_ATTR_H */
