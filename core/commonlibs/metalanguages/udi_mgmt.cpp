
#define UDI_VERSION		0x101
#include <udi.h>


#define ZUDI_MEI_BLANK_OP_TEMPLATE_CREATE			\
	{NULL,0,0,0,0,0,0,0,NULL,NULL,NULL,NULL}

#define ZUDI_MEI_BLANK_OPS_VEC_TEMPLATE_CREATE			\
	{0,0,0}

#define ZUDI_MEI_OP_TEMPLATE_CREATE( \
	name,opcat,flags,cbnum,compvec,compop,vislay,marshlay \
	) \
	ZUDI_MEI_EXCEPTION_OP_TEMPLATE_CREATE( \
		name, opcat, flags, cbnum, \
		compvec, compop, 0, 0, \
		vislay, marshlay)

#define ZUDI_MEI_EXCEPTION_OP_TEMPLATE_CREATE( \
	name,opcat,flags,cbnum,compvec,compop,exceptvec,exceptop, \
	vislay,marshlay \
	) \
	{ \
		#name, UDI_MEI_OPCAT_ ## opcat, \
		flags, \
		cbnum, \
		compvec, compop, \
		exceptvec, exceptop, \
		name ## _direct, name ## _backend, \
		vislay, marshlay \
	}

// For the sake of neatness.
#define UDI_MGMT_OPS_NUM		1
#define UDI_MGMT_MA_OPS_NUM		2
// udi_mgmt_ops_t
#define UDI_USAGE_IND_NUM		1
#define UDI_ENUMERATE_REQ_NUM		2
#define UDI_DEVMGMT_REQ_NUM		3
#define UDI_FINAL_CLEANUP_REQ_NUM	4
// udi_mgmt_ma_ops_t
#define UDI_USAGE_RES_NUM		1
#define UDI_ENUMERATE_ACK_NUM		2
#define UDI_DEVMGMT_ACK_NUM		3
#define UDI_FINAL_CLEANUP_ACK_NUM	4

#define udi_meta_info		udi_mgmt_meta_info
extern udi_mei_init_t		udi_mgmt_meta_info;

// udi_mgmt_ops_t
UDI_MEI_STUBS(
	udi_usage_ind, udi_usage_cb_t,
	1,
	(resource_level), (udi_ubit8_t), (UDI_VA_UBIT8_T),
	UDI_MGMT_OPS_NUM, UDI_USAGE_IND_NUM)

UDI_MEI_STUBS(
	udi_enumerate_req, udi_enumerate_cb_t,
	1,
	(enumeration_level), (udi_ubit8_t), (UDI_VA_UBIT8_T),
	UDI_MGMT_OPS_NUM, UDI_ENUMERATE_REQ_NUM)

UDI_MEI_STUBS(
	udi_devmgmt_req, udi_mgmt_cb_t,
	2,
	(mgmt_op, parent_ID),
	(udi_ubit8_t, udi_ubit8_t),
	(UDI_VA_UBIT8_T, UDI_VA_UBIT8_T),
	UDI_MGMT_OPS_NUM, UDI_DEVMGMT_REQ_NUM)

UDI_MEI_STUBS(
	udi_final_cleanup_req, udi_mgmt_cb_t,
	0, (), (), (),
	UDI_MGMT_OPS_NUM, UDI_FINAL_CLEANUP_REQ_NUM)

// udi_mgmt_ma_ops_t
UDI_MEI_STUBS(
	udi_usage_res, udi_usage_cb_t,
	0, (), (), (),
	UDI_MGMT_MA_OPS_NUM, UDI_USAGE_RES_NUM)

UDI_MEI_STUBS(
	udi_enumerate_ack, udi_enumerate_cb_t,
	2,
	(enumeration_level, ops_idx),
	(udi_ubit8_t, udi_index_t),
	(UDI_VA_UBIT8_T, UDI_VA_INDEX_T),
	UDI_MGMT_MA_OPS_NUM, UDI_ENUMERATE_ACK_NUM)

UDI_MEI_STUBS(
	udi_devmgmt_ack, udi_mgmt_cb_t,
	2,
	(flags, status),
	(udi_ubit8_t, udi_status_t),
	(UDI_VA_UBIT8_T, UDI_VA_STATUS_T),
	UDI_MGMT_MA_OPS_NUM, UDI_DEVMGMT_ACK_NUM)

UDI_MEI_STUBS(
	udi_final_cleanup_ack, udi_mgmt_cb_t,
	0, (), (), (),
	UDI_MGMT_MA_OPS_NUM, UDI_FINAL_CLEANUP_ACK_NUM)

const static udi_mei_op_template_t	udi_mgmt_op_template_list[] =
{
	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_usage_ind, IND,
		(0),
		0,
		0, UDI_USAGE_RES_NUM,
		NULL, NULL),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_enumerate_req, REQ,
		(0),
		0,
		0, UDI_ENUMERATE_ACK_NUM,
		NULL, NULL),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_devmgmt_req, REQ,
		(0),
		0,
		0, UDI_DEVMGMT_ACK_NUM,
		NULL, NULL),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_final_cleanup_req, REQ,
		(0),
		0,
		0, UDI_FINAL_CLEANUP_ACK_NUM,
		NULL, NULL),

	ZUDI_MEI_BLANK_OP_TEMPLATE_CREATE
};

const static udi_mei_op_template_t	udi_mgmt_ma_op_template_list[] =
{
	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_usage_res, RES,
		(0),
		0,
		0, 0,
		NULL, NULL),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_enumerate_ack, ACK,
		(0),
		0,
		0, 0,
		NULL, NULL),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_devmgmt_ack, ACK,
		(UDI_MEI_OP_STATE_CHANGE),
		0,
		0, 0,
		NULL, NULL),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_final_cleanup_ack, ACK,
		(UDI_MEI_OP_STATE_CHANGE),
		0,
		0, 0,
		NULL, NULL),

	ZUDI_MEI_BLANK_OP_TEMPLATE_CREATE
};

const static udi_mei_ops_vec_template_t	udi_mgmt_ma_ops_vec_template_list[] =
{
	{
		UDI_MGMT_MA_OPS_NUM,
		UDI_MEI_REL_EXTERNAL | UDI_MEI_REL_INITIATOR,
		udi_mgmt_ma_op_template_list
	},

	{
		UDI_MGMT_OPS_NUM,
		UDI_MEI_REL_EXTERNAL,
		udi_mgmt_op_template_list
	},

	ZUDI_MEI_BLANK_OPS_VEC_TEMPLATE_CREATE
};

udi_mei_init_t			udi_mgmt_meta_info =
{
	udi_mgmt_ops_vec_template_list
};
