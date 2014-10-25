/*
 * Zambesii-specific UDI MGMT definitions. Our augmentations to match our
 * approach to the management metalanguage's publicly exposed call interface.
 */

#ifndef ZBZ__UDI_CORE_META_MGMT_H
#define ZBZ__UDI_CORE_META_MGMT_H

#ifndef _UDI_H_INSIDE
#error "zbz_udi_mgmt.h must not be #included directly; use udi.h instead."
#endif

/**	EXPLANATION:
 * We have no problem with most of the stuff in udi_mgmt.h. The things we don't
 * like, we've suppressed with "#if 0" .. "#endif" preprocessing.
 *
 * In here, we will seek to create certain illusions of standardization for
 * the Zambesii mgmt meta implementation. These are mostly just illusions which
 * will allow udi_mgmt to look just like every other metalanguage.
 *
 * Specifically:
 *	* The core spec does not define UDI_MGMT_OPS_NUM.
 *	* The core spec does not define UDI_MGMT_MA_OPS_NUM.
 *	* The core spec does not define UDI_MGMT_CB_NUM.
 *	* The core spec does not define UDI_MGMT_ENUMERATE_CB_NUM.
 *	* The core spec does not define UDI_MGMT_USAGE_CB_NUM.
 *
 * We will define these in a centralized location in our headers.
 **/

/* In keeping with the other metas, the client (child) ops vector is given the
 * lower ops_num.
 **/
#define UDI_MGMT_MA_OPS_NUM		1
#define UDI_MGMT_OPS_NUM		2

/* We just go by the order the functions are found laid out in the ops vector.
 * usage_ind is first in the ops vector, so its CB_NUM will come first as well.
 * enumerate_req is second, so its CB_NUM will come second, and so on.
 **/
#define UDI_MGMT_USAGE_CB_NUM		1
#define UDI_MGMT_ENUMERATE_CB_NUM	2
#define UDI_MGMT_CB_NUM			3

// udi_*_op_t typedefs; the ones not provided by udi_mgmt.h.
typedef void (udi_usage_res_op_t)(udi_usage_cb_t *cb);
typedef void (udi_enumerate_ack_op_t)(
	udi_enumerate_cb_t *cb,
	udi_ubit8_t enumeration_result,
	udi_index_t ops_idx);

typedef void (udi_devmgmt_ack_op_t)(
	udi_mgmt_cb_t *cb,
	udi_ubit8_t flags,
	udi_status_t status);

typedef void (udi_final_cleanup_ack_op_t)(udi_mgmt_cb_t *cb);

#endif /* ZBZ__UDI_CORE_META_MGMT_H */
