/*
 * File: udi_mei.h
 *
 * UDI MEI -- Metalanguage-to-Environment Interface
 *
 * Public definitions for interfaces accessible to metalanguage libraries.
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

#ifndef _UDI_CORE_MEI_H
#define _UDI_CORE_MEI_H

#ifndef _UDI_H_INSIDE
#error "udi_mei.h must not be #included directly; use udi.h instead."
#endif

/*
 * Generic types, constants.
 */

/* Maximum sizes for control block layouts */
#define UDI_MEI_MAX_VISIBLE_SIZE 2000
#define UDI_MEI_MAX_MARSHAL_SIZE 4000
/* Maximum scratch size in a cb */
#define UDI_MAX_SCRATCH 4000

/*
 * Prototype for Metalanguage direct call stub.
 */
typedef void udi_mei_direct_stub_t(
	udi_op_t *op,
	udi_cb_t *gcb,
	va_list arglist);

/*
 * Prototype for Metalanguage Back-End Stubs (unmarshalling routines)
 */
typedef void udi_mei_backend_stub_t(
	udi_op_t *op,
	udi_cb_t *gcb,
	void *marshal_space);

/*
 * op template structure
 */

typedef const struct _udi_mei_op_template {
	const char 			*op_name;
	udi_ubit8_t			op_category;
	udi_ubit8_t			op_flags;
	udi_index_t			meta_cb_num;
	udi_index_t			completion_ops_num;
	udi_index_t			completion_vec_idx;
	udi_index_t			exception_ops_num;
	udi_index_t			exception_vec_idx;
	udi_mei_direct_stub_t		*direct_stub;
	udi_mei_backend_stub_t		*backend_stub;
	udi_layout_t 			*visible_layout;
	udi_layout_t			*marshal_layout;
} udi_mei_op_template_t;

/* Values for op_category */

#define UDI_MEI_OPCAT_REQ 1
#define UDI_MEI_OPCAT_ACK 2
#define UDI_MEI_OPCAT_NAK 3
#define UDI_MEI_OPCAT_IND 4
#define UDI_MEI_OPCAT_RES 5
#define UDI_MEI_OPCAT_RDY 6

/* Values for op_flags */

#define UDI_MEI_OP_ABORTABLE		(1U<<0)
#define UDI_MEI_OP_RECOVERABLE		(1U<<1)
#define UDI_MEI_OP_STAT_CHANGE		(1U<<2)

/*
 * Metalanguage ops vector template
 */
typedef const struct _udi_mei_ops_vec_template {
	udi_index_t meta_ops_num;
	udi_ubit8_t relationship;
	udi_mei_op_template_t * op_template_list;
} udi_mei_ops_vec_template_t;

/* 
 * Metalanguage initialization structure
 */
typedef const struct _udi_mei_init {
	udi_mei_ops_vec_template_t * ops_vec_template_list;
} udi_mei_init_t;

/*
 * Interface Relationship values.
 *   Defines the relationship of a region using an interface to the region
 *   on the other end of the interface.
 */

#define UDI_MEI_REL_INITIATOR 		(1U<<0)
#define UDI_MEI_REL_BIND 		(1U<<1)
#define UDI_MEI_REL_EXTERNAL 		(1U<<2)
#define UDI_MEI_REL_INTERNAL 		(1U<<3)
#define UDI_MEI_REL_SINGLE 		(1U<<4)

/* 
 * Function prototypes for MEI environment services.
 */

void udi_mei_call(
	udi_cb_t *gcb,
	udi_mei_init_t *meta_info,
	udi_index_t meta_ops_num,
	udi_index_t vec_idx,
	...);

void udi_mei_driver_error(
	udi_cb_t *gcb,
	udi_mei_init_t *meta_info,
	udi_index_t meta_ops_num,
	udi_index_t vec_idx);


/*
 * ========================================================================
 * Macros for use in Metalanguage/Environment interface (MEI)
 * 
 * This describes how channel operations occur.
 * These are standard macros based on the MEI, which provide both
 * source and binary portability of the metalanguage libraries.
 * 
 * Each invocation of UDI_MEI_STUBS() generates three functions for
 * a channel operation.
 *   1) A frontend stub to implement the visible interface for the 
 *      caller of a channel op.  It generates a call to udi_mei_call
 *      with appropriate arguments.
 *   2) A direct-call stub to implement a call into the target driver 
 *      when the environment wishes to invoke it directly from the original 
 *      calling context.  This will be used if the channel end points are
 *      in the same domain, the target region is not busy, etc.
 *   3) A backend stub to dequeue the paramaters from the a control block
 *      that has been queued (presumably it was decided during the channel
 *      op that it was unsuitable as a direct call, has been queued, and 
 *      is now ready to be executed) and rebuild an appropriate argument
 *      list for the channel op that is now ready to be called.
 *     
 * Unfortunately, implementation of this is somewhat obscured by the absence
 * of vararg macros in C89.
 * 
 * Examples:
 *  void
 *  udi_scsi_io_req (SCSI Metalanguage):
 * 	The scsi_io_req operation has zero additional paramaters and
 * 	requires the use of a udi_scsi_io_cb_t for the channel op.  It
 * 	would be implemented as follows:
 * 
 * 		UDI_MEI_STUBS(udi_scsi_io_req, udi_scsi_io_cb_t,
 *		   0, (), (), (),
 *		   UDI_SCSI_HD_OPS_NUM, UDI_SCSI_IO_REQ)
 *
 * 	This will result in the generation of three functions,
 *		udi_scsi_io_req() - call udi_mei_call with metalanguage
 * 			ops num and ops vector index. If udi_mei_call
 * 			call udi_scsi_io_req_direct() at this time, it 
 * 			will marshal the arg list into the control block.
 *		udi_scsi_io_req_direct() 
 *		udi_scsi_io_req_backend()
 *
 *  void
 *  udi_devmgmt_req (Management Metalanguage):
 *	The udi_devmgmt_req channl operation has two extra paramaters
 *      and requires a udi_mgmt_cb_t for the channel operation.   It would
 *      be impelmented as follows.
 * 	
 * 		UDI_MEI_STUBS(udi_devmgmt_req, udi_mgmt_cb_t,
 *		   	2, (mgmt_op, parent_id),
 *			(udi_ubit8_t, udi_ubit8_t),
 *			(UDI_VA_UBIT8_T, UDI_VA_UBIT8_T),
 *			UDI_MGMT_DRV_OPS_NUM, UDI_DEVMGMT_REQ)
 *
 * 	This will result in the generation of three functions,
 *		udi_devmgmt_req() - call udi_mei_call with metalanguage
 * 			ops num and ops vector index passing mgmt_op 
 * 			and parent_id in an appropriate manner.  If 
 * 			udi_mei_call cannot call udi_scsi_req_direct() at
 * 			this time, it will marshal the arg list into the
 * 			control block.
 *		udi_devmgmt_req_direct() - calls the target channel operation
 *			function with an appropriate arg list.
 *		udi_devmgmt_req_backend() - pulls the marshalled arguments
 * 			out of the control block and executes the target 
 * 			channel operation function with an appropriate 
 * 			arg list.
 * ========================================================================
 */

/*
 * Apply the appropriate va_arg-ish macro to each of the arguments.
 */
#define _UDI_VA_ARGS_0() (void)arglist;
#define _UDI_VA_ARGS_1(a,va_a) \
					a arg1 = UDI_VA_ARG(arglist,a,va_a);
#define _UDI_VA_ARGS_2(a,b,va_a,va_b) \
					a arg1 = UDI_VA_ARG(arglist,a,va_a); \
					b arg2 = UDI_VA_ARG(arglist,b,va_b);
#define _UDI_VA_ARGS_3(a,b,c,va_a,va_b,va_c) \
					a arg1 = UDI_VA_ARG(arglist,a,va_a); \
					b arg2 = UDI_VA_ARG(arglist,b,va_b); \
					c arg3 = UDI_VA_ARG(arglist,c,va_c);
#define _UDI_VA_ARGS_4(a,b,c,d,va_a,va_b,va_c,va_d) \
					a arg1 = UDI_VA_ARG(arglist,a,va_a); \
					b arg2 = UDI_VA_ARG(arglist,b,va_b); \
					c arg3 = UDI_VA_ARG(arglist,c,va_c); \
					d arg4 = UDI_VA_ARG(arglist,d,va_d);
#define _UDI_VA_ARGS_5(a,b,c,d,e,va_a,va_b,va_c,va_d,va_e) \
					a arg1 = UDI_VA_ARG(arglist,a,va_a); \
					b arg2 = UDI_VA_ARG(arglist,b,va_b); \
					c arg3 = UDI_VA_ARG(arglist,c,va_c); \
					d arg4 = UDI_VA_ARG(arglist,d,va_d); \
					e arg5 = UDI_VA_ARG(arglist,e,va_e);
#define _UDI_VA_ARGS_6(a,b,c,d,e,f,va_a,va_b,va_c,va_d,va_e,va_f) \
					a arg1 = UDI_VA_ARG(arglist,a,va_a); \
					b arg2 = UDI_VA_ARG(arglist,b,va_b); \
					c arg3 = UDI_VA_ARG(arglist,c,va_c); \
					d arg4 = UDI_VA_ARG(arglist,d,va_d); \
					e arg5 = UDI_VA_ARG(arglist,e,va_e); \
					f arg6 = UDI_VA_ARG(arglist,f,va_f);
#define _UDI_VA_ARGS_7(a,b,c,d,e,f,g,va_a,va_b,va_c,va_d,va_e,va_f,va_g) \
					a arg1 = UDI_VA_ARG(arglist,a,va_a); \
					b arg2 = UDI_VA_ARG(arglist,b,va_b); \
					c arg3 = UDI_VA_ARG(arglist,c,va_c); \
					d arg4 = UDI_VA_ARG(arglist,d,va_d); \
					e arg5 = UDI_VA_ARG(arglist,e,va_e); \
					f arg6 = UDI_VA_ARG(arglist,f,va_f); \
					g arg7 = UDI_VA_ARG(arglist,g,va_g);

#define _UDI_ARG_VARS_0
#define _UDI_ARG_VARS_1 	,arg1
#define _UDI_ARG_VARS_2 	,arg1,arg2
#define _UDI_ARG_VARS_3 	,arg1,arg2,arg3
#define _UDI_ARG_VARS_4 	,arg1,arg2,arg3,arg4
#define _UDI_ARG_VARS_5 	,arg1,arg2,arg3,arg4,arg5
#define _UDI_ARG_VARS_6 	,arg1,arg2,arg3,arg4,arg5, arg6
#define _UDI_ARG_VARS_7 	,arg1,arg2,arg3,arg4,arg5, arg6,arg7

#define _UDI_ARG_MEMBERS_0() \
					char dummy;
#define _UDI_ARG_MEMBERS_1(a) \
					a arg1;
#define _UDI_ARG_MEMBERS_2(a,b) \
					a arg1; \
					b arg2;
#define _UDI_ARG_MEMBERS_3(a,b,c) \
					a arg1; \
					b arg2; \
					c arg3;
#define _UDI_ARG_MEMBERS_4(a,b,c,d) \
					a arg1; \
					b arg2; \
					c arg3; \
					d arg4;
#define _UDI_ARG_MEMBERS_5(a,b,c,d,e) \
					a arg1; \
					b arg2; \
					c arg3; \
					d arg4; \
					e arg5;
#define _UDI_ARG_MEMBERS_6(a,b,c,d,e,f) \
					a arg1; \
					b arg2; \
					c arg3; \
					d arg4; \
					e arg5; \
					f arg6;
#define _UDI_ARG_MEMBERS_7(a,b,c,d,e,f,g) \
					a arg1; \
					b arg2; \
					c arg3; \
					d arg4; \
					e arg5; \
					f arg6; \
					g arg7;

#define _UDI_ARG_LIST_0()
#define _UDI_ARG_LIST_1(a) 		,a arg1
#define _UDI_ARG_LIST_2(a,b) 		,a arg1, b arg2
#define _UDI_ARG_LIST_3(a,b,c) 		,a arg1, b arg2, c arg3 
#define _UDI_ARG_LIST_4(a,b,c,d) 	,a arg1, b arg2, c arg3, d arg4
#define _UDI_ARG_LIST_5(a,b,c,d,e) 	,a arg1, b arg2, c arg3, d arg4, \
						e arg5
#define _UDI_ARG_LIST_6(a,b,c,d,e,f) 	,a arg1, b arg2, c arg3, d arg4, \
						e arg5, f arg6
#define _UDI_ARG_LIST_7(a,b,c,d,e,f,g) 	,a arg1, b arg2, c arg3, d arg4, \
						e arg5, f arg6, g arg7

#define _UDI_MP_ARGS_0
#define _UDI_MP_ARGS_1 ,mp->arg1
#define _UDI_MP_ARGS_2 ,mp->arg1,mp->arg2
#define _UDI_MP_ARGS_3 ,mp->arg1,mp->arg2,mp->arg3
#define _UDI_MP_ARGS_4 ,mp->arg1,mp->arg2,mp->arg3,mp->arg4
#define _UDI_MP_ARGS_5 ,mp->arg1,mp->arg2,mp->arg3,mp->arg4,mp->arg5
#define _UDI_MP_ARGS_6 ,mp->arg1,mp->arg2,mp->arg3,mp->arg4,mp->arg5,mp->arg6
#define _UDI_MP_ARGS_7 ,mp->arg1,mp->arg2,mp->arg3,mp->arg4,mp->arg5,mp->arg6,\
			mp->arg7

/*
 * The following macros are used to concatenate two argument lists.
 */
#define _UDI_L_0()			(
#define _UDI_L_1(a)			(a,
#define _UDI_L_2(a,b)			(a,b,
#define _UDI_L_3(a,b,c)			(a,b,c,
#define _UDI_L_4(a,b,c,d)		(a,b,c,d,
#define _UDI_L_5(a,b,c,d,e)		(a,b,c,d,e,
#define _UDI_L_6(a,b,c,d,e,f)		(a,b,c,d,e,f,
#define _UDI_L_7(a,b,c,d,e,f,g)		(a,b,c,d,e,f,g,

#define _UDI_R_0()			)
#define _UDI_R_1(a)			a)
#define _UDI_R_2(a,b)			a,b)
#define _UDI_R_3(a,b,c)			a,b,c)
#define _UDI_R_4(a,b,c,d)		a,b,c,d)
#define _UDI_R_5(a,b,c,d,e)		a,b,c,d,e)
#define _UDI_R_6(a,b,c,d,e,f)		a,b,c,d,e,f)
#define _UDI_R_7(a,b,c,d,e,f,g)		a,b,c,d,e,f,g)

#define __UDI_VA_ARGLIST(argc,list) \
		_UDI_VA_ARGS_##argc list

#define _UDI_CAT_LIST(argc,list1,list2) \
		_UDI_L_##argc list1 _UDI_R_##argc list2

#define _UDI_VA_ARGLIST(argc,list1,list2) \
		__UDI_VA_ARGLIST(argc, \
			_UDI_CAT_LIST(argc, list1, list2))

/*
 * Now here's UDI_MEI_STUBS itself, based on all of the above macros.
 */
#define UDI_MEI_STUBS(op_name, cb_type, argc, args, arg_types, arg_va_list, \
			meta_ops_num, vec_idx) \
		_UDI_MEI_STUBS(op_name, cb_type, argc, args, arg_types, \
				arg_va_list, meta_ops_num, vec_idx, \
				&udi_meta_info)

#define _UDI_MEI_STUBS(op_name, cb_type, argc, args, arg_types, arg_va_list, \
			ops_num, op_idx, meta_idx) \
  void op_name ( cb_type *cb \
		_UDI_ARG_LIST_##argc arg_types) \
  { \
	udi_mei_call ( UDI_GCB(cb), meta_idx, ops_num, op_idx \
			_UDI_ARG_VARS_##argc );  \
  } \
  static void  \
    op_name##_direct ( udi_op_t *op, udi_cb_t *gcb, va_list arglist )  \
  { \
	_UDI_VA_ARGLIST(argc, arg_types, arg_va_list) \
	(*(op_name##_op_t *)op) ( \
		UDI_MCB(gcb, cb_type) _UDI_ARG_VARS_##argc ); \
  } \
  static void \
    op_name##_backend ( udi_op_t *op, udi_cb_t *gcb, void *marshal_space )\
  { \
	struct op_name##_marshal { \
		_UDI_ARG_MEMBERS_##argc arg_types \
	} *mp = marshal_space; \
	mp = mp; \
	(*(op_name##_op_t *)op) ( UDI_MCB(gcb, cb_type) _UDI_MP_ARGS_##argc ); \
  }

/*
 *  The extraneous assignment of 'mp = mp' in the backend function above
 *  (which is not in the spec and results in zero code in any optimizing
 *  compiler) is there to suppress "unused" warnings portably in the
 *  ops that have zero arguments.
 */

#endif /* _UDI_CORE_MEI_H */

