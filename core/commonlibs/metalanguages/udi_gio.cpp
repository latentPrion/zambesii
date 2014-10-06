
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

// udi_gio_provider_ops_t
#define UDI_GIO_BIND_REQ_NUM		1
#define UDI_GIO_UNBIND_REQ_NUM		2
#define UDI_GIO_XFER_REQ_NUM		3
#define UDI_GIO_EVENT_RES_NUM		4

// udi_gio_client_ops_t
#define UDI_GIO_BIND_ACK_NUM		1
#define UDI_GIO_UNBIND_ACK_NUM		2
#define UDI_GIO_XFER_ACK_NUM		3
#define UDI_GIO_XFER_NAK_NUM		4
#define UDI_GIO_EVENT_IND_NUM		5

#define udi_meta_info			udi_gio_meta_info
extern udi_mei_init_t			udi_gio_meta_info;

UDI_MEI_STUBS(
	udi_gio_bind_req, udi_gio_bind_cb_t,
	0, (), (), (),
	UDI_GIO_PROVIDER_OPS_NUM, UDI_GIO_BIND_REQ_NUM)

UDI_MEI_STUBS(
	udi_gio_bind_ack, udi_gio_bind_cb_t,
	3,
	(device_size_lo, device_size_hi, status),
	(udi_ubit32_t, udi_ubit32_t, udi_status_t),
	(UDI_VA_UBIT32_T, UDI_VA_UBIT32_T, UDI_VA_STATUS_T),
	UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_BIND_ACK_NUM)

UDI_MEI_STUBS(
	udi_gio_unbind_req, udi_gio_bind_cb_t,
	0, (), (), (),
	UDI_GIO_PROVIDER_OPS_NUM, UDI_GIO_UNBIND_REQ_NUM)

UDI_MEI_STUBS(
	udi_gio_unbind_ack, udi_gio_bind_cb_t,
	0, (), (), (),
	UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_UNBIND_ACK_NUM)

UDI_MEI_STUBS(
	udi_gio_xfer_req, udi_gio_xfer_cb_t,
	0, (), (), (),
	UDI_GIO_PROVIDER_OPS_NUM, UDI_GIO_XFER_REQ_NUM)

UDI_MEI_STUBS(
	udi_gio_xfer_ack, udi_gio_xfer_cb_t,
	0, (), (), (),
	UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_XFER_ACK_NUM)

UDI_MEI_STUBS(
	udi_gio_xfer_nak, udi_gio_xfer_cb_t,
	1,
	(status), (udi_status_t), (UDI_VA_STATUS_T),
	UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_XFER_NAK_NUM)

UDI_MEI_STUBS(
	udi_gio_event_ind, udi_gio_event_cb_t,
	0, (), (), (),
	UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_EVENT_IND_NUM)

UDI_MEI_STUBS(
	udi_gio_event_res, udi_gio_event_cb_t,
	0, (), (), (),
	UDI_GIO_PROVIDER_OPS_NUM, UDI_GIO_EVENT_RES_NUM)


const static udi_ubit8_t	blankLayout[] = { UDI_DL_END };

#define ZUDI_DL_XFER_CONSTRAINTS_T			\
	UDI_DL_UBIT32_T, UDI_DL_UBIT32_T, UDI_DL_UBIT32_T, \
	UDI_DL_BOOLEAN_T, UDI_DL_BOOLEAN_T, UDI_DL_BOOLEAN_T

/* CB layouts.
 **/
const static udi_ubit8_t	udi_gio_bind_cb_layout[] =
	{ ZUDI_DL_XFER_CONSTRAINTS_T, UDI_DL_END };

const static udi_ubit8_t	udi_gio_xfer_cb_layout[] =
{
	UDI_DL_UBIT8_T, UDI_DL_INLINE_DRIVER_TYPED,
	UDI_DL_BUF,
		offsetof(udi_gio_xfer_cb_t, op),
		UDI_GIO_DIR_WRITE,
		UDI_GIO_DIR_WRITE,
	UDI_DL_END
};

const static udi_ubit8_t	udi_gio_event_cb_layout[] =
	{ UDI_DL_UBIT8_T, UDI_DL_INLINE_DRIVER_TYPED, UDI_DL_END };

static const udi_mei_op_template_t	udi_gio_provider_op_template_list[] =
{
	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_gio_bind_req, REQ,
		(UDI_MEI_OP_STATE_CHANGE),
		UDI_GIO_BIND_CB_NUM,
		UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_BIND_ACK_NUM,
		udi_gio_bind_cb_layout, blankLayout),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_gio_unbind_req, REQ,
		(UDI_MEI_OP_STATE_CHANGE),
		UDI_GIO_BIND_CB_NUM,
		UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_UNBIND_ACK_NUM,
		udi_gio_bind_cb_layout, blankLayout),

	ZUDI_MEI_EXCEPTION_OP_TEMPLATE_CREATE(
		udi_gio_xfer_req, REQ,
		(0),
		UDI_GIO_XFER_CB_NUM,
		UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_XFER_ACK_NUM,
		UDI_GIO_CLIENT_OPS_NUM, UDI_GIO_XFER_NAK_NUM,
		udi_gio_xfer_cb_layout, blankLayout),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_gio_event_res, RES,
		(0),
		UDI_GIO_EVENT_CB_NUM,
		0, 0,
		udi_gio_event_cb_layout, blankLayout),

	ZUDI_MEI_BLANK_OP_TEMPLATE_CREATE
};

const static udi_ubit8_t	udi_gio_bind_ack_layout[] =
	{ UDI_DL_UBIT32_T, UDI_DL_UBIT32_T, UDI_DL_STATUS_T, UDI_DL_END };

const static udi_ubit8_t	udi_gio_xfer_nak_layout[] =
	{ UDI_DL_STATUS_T, UDI_DL_END };

static const udi_mei_op_template_t	udi_gio_client_op_template_list[] =
{
	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_gio_bind_ack, ACK,
		(UDI_MEI_OP_STATE_CHANGE),
		UDI_GIO_BIND_CB_NUM,
		0, 0,
		udi_gio_bind_cb_layout, udi_gio_bind_ack_layout),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_gio_unbind_ack, ACK,
		(UDI_MEI_OP_STATE_CHANGE),
		UDI_GIO_BIND_CB_NUM,
		0, 0,
		udi_gio_bind_cb_layout, blankLayout),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_gio_xfer_ack, ACK,
		(0),
		UDI_GIO_XFER_CB_NUM,
		0, 0,
		udi_gio_xfer_cb_layout, blankLayout),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_gio_xfer_nak, NAK,
		(0),
		UDI_GIO_XFER_CB_NUM,
		0, 0,
		udi_gio_xfer_cb_layout, udi_gio_xfer_nak_layout),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_gio_event_ind, IND,
		(0),
		UDI_GIO_EVENT_CB_NUM,
		UDI_GIO_PROVIDER_OPS_NUM, UDI_GIO_EVENT_RES_NUM,
		udi_gio_event_cb_layout, blankLayout),

	ZUDI_MEI_BLANK_OP_TEMPLATE_CREATE
};

static const udi_mei_ops_vec_template_t	udi_gio_ops_vec_template_list[] =
{
	{
		UDI_GIO_PROVIDER_OPS_NUM,
		UDI_MEI_REL_BIND | UDI_MEI_REL_EXTERNAL | UDI_MEI_REL_INTERNAL,
		udi_gio_provider_op_template_list
	},
	{
		UDI_GIO_CLIENT_OPS_NUM,
		UDI_MEI_REL_BIND | UDI_MEI_REL_INITIATOR
			| UDI_MEI_REL_EXTERNAL | UDI_MEI_REL_INTERNAL,
		udi_gio_client_op_template_list
	},

	ZUDI_MEI_BLANK_OPS_VEC_TEMPLATE_CREATE
};

const udi_mei_init_t			udi_gio_meta_info =
{
	udi_gio_ops_vec_template_list
};
