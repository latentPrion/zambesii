
#define UDI_VERSION		0x101
#include <udi.h>
#define UDI_PHYSIO_VERSION	0x101
#include <udi_physio.h>


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

// udi_bus_device_ops_t
#define UDI_BUS_BIND_ACK_NUM		1
#define UDI_BUS_UNBIND_ACK_NUM		2
#define UDI_INTR_ATTACH_ACK_NUM		3
#define UDI_INTR_DETACH_ACK_NUM		4
// udi_bus_bridge_ops_t
#define UDI_BUS_BIND_REQ_NUM		1
#define UDI_BUS_UNBIND_REQ_NUM		2
#define UDI_INTR_ATTACH_REQ_NUM		3
#define UDI_INTR_DETACH_REQ_NUM		4
// udi_intr_handler_ops_t
#define UDI_INTR_EVENT_IND_NUM		1
// udi_intr_dispatcher_ops_t
#define UDI_INTR_EVENT_RDY_NUM		1

#define udi_meta_info		udi_bridge_meta_info
extern udi_mei_init_t		udi_bridge_meta_info;

UDI_MEI_STUBS(
	udi_bus_bind_req, udi_bus_bind_cb_t,
	0, (), (), (),
	UDI_BUS_DEVICE_OPS_NUM, UDI_BUS_BIND_REQ_NUM)

UDI_MEI_STUBS(
	udi_bus_unbind_req, udi_bus_bind_cb_t,
	0, (), (), (),
	UDI_BUS_DEVICE_OPS_NUM, UDI_BUS_UNBIND_REQ_NUM)

UDI_MEI_STUBS(
	udi_intr_attach_req, udi_intr_attach_cb_t,
	0, (), (), (),
	UDI_BUS_DEVICE_OPS_NUM, UDI_INTR_ATTACH_REQ_NUM)

UDI_MEI_STUBS(
	udi_intr_detach_req, udi_intr_detach_cb_t,
	0, (), (), (),
	UDI_BUS_DEVICE_OPS_NUM, UDI_INTR_DETACH_REQ_NUM)

UDI_MEI_STUBS(
	udi_bus_bind_ack, udi_bus_bind_cb_t,
	3,
	(dma_constraints, preferred_endianness, status),
	(udi_dma_constraints_t, udi_ubit8_t, udi_status_t),
	(UDI_VA_DMA_CONSTRAINTS_T, UDI_VA_UBIT8_T, UDI_VA_STATUS_T),
	UDI_BUS_BRIDGE_OPS_NUM, UDI_BUS_BIND_ACK_NUM)

UDI_MEI_STUBS(
	udi_bus_unbind_ack, udi_bus_bind_cb_t,
	0, (), (), (),
	UDI_BUS_BRIDGE_OPS_NUM, UDI_BUS_UNBIND_ACK_NUM)

UDI_MEI_STUBS(
	udi_intr_attach_ack, udi_intr_attach_cb_t,
	1,
	(status), (udi_status_t), (UDI_VA_STATUS_T),
	UDI_BUS_BRIDGE_OPS_NUM, UDI_INTR_ATTACH_ACK_NUM)

UDI_MEI_STUBS(
	udi_intr_detach_ack, udi_intr_detach_cb_t,
	0, (), (), (),
	UDI_BUS_BRIDGE_OPS_NUM, UDI_INTR_DETACH_ACK_NUM)

UDI_MEI_STUBS(
	udi_intr_event_ind, udi_intr_event_cb_t,
	1, (flags), (udi_ubit8_t), (UDI_VA_UBIT8_T),
	UDI_BUS_INTR_HANDLER_OPS_NUM, UDI_INTR_EVENT_IND_NUM)

UDI_MEI_STUBS(
	udi_intr_event_rdy, udi_intr_event_cb_t,
	0, (), (), (),
	UDI_BUS_INTR_DISPATCH_OPS_NUM, UDI_INTR_EVENT_RDY_NUM)


/* CB layouts.
 **/
const static udi_ubit8_t	blankLayout[] = { UDI_DL_END };
const static udi_ubit8_t	*udi_bus_bind_cb_layout = blankLayout;
const static udi_ubit8_t	udi_intr_attach_cb_layout[] =
	{ UDI_DL_INDEX_T, UDI_DL_UBIT8_T, UDI_DL_PIO_HANDLE_T, UDI_DL_END };

const static udi_ubit8_t	udi_intr_detach_cb_layout[] =
	{ UDI_DL_INDEX_T, UDI_DL_END };

const static udi_ubit8_t	udi_intr_event_cb_layout_preserve[] =
	{ UDI_DL_BUF, 0, 0, 0, UDI_DL_UBIT16_T, UDI_DL_END };

const static udi_ubit8_t	udi_intr_event_cb_layout_nopreserve[] =
	{ UDI_DL_BUF, 0, 0, 1, UDI_DL_UBIT16_T, UDI_DL_END };


const static udi_ubit8_t	udi_bus_bind_ack_layout[] =
{
	UDI_DL_DMA_CONSTRAINTS_T, UDI_DL_UBIT8_T, UDI_DL_STATUS_T, UDI_DL_END
};

const static udi_ubit8_t	udi_intr_attach_ack_layout[] =
	{ UDI_DL_STATUS_T, UDI_DL_END };

// udi_bus_device_ops_t
const static udi_mei_op_template_t	udi_bus_device_op_template_list[] =
{
	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_bus_bind_ack, ACK,
		(UDI_MEI_OP_STATE_CHANGE),
		UDI_BUS_BIND_CB_NUM,
		0, 0,
		udi_bus_bind_cb_layout,
		udi_bus_bind_ack_layout),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_bus_unbind_ack, ACK,
		(UDI_MEI_OP_STATE_CHANGE),
		UDI_BUS_BIND_CB_NUM,
		0, 0,
		udi_bus_bind_cb_layout, blankLayout),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_intr_attach_ack, ACK,
		(0),
		UDI_BUS_INTR_ATTACH_CB_NUM,
		0, 0,
		udi_intr_attach_cb_layout,
		udi_intr_attach_ack_layout),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_intr_detach_ack, ACK,
		(0),
		UDI_BUS_INTR_DETACH_CB_NUM,
		0, 0,
		udi_intr_detach_cb_layout, blankLayout),

	ZUDI_MEI_BLANK_OP_TEMPLATE_CREATE
};

// udi_bus_bridge_ops_t
const static udi_mei_op_template_t	udi_bus_bridge_op_template_list[] =
{
	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_bus_bind_req, REQ,
		(UDI_MEI_OP_STATE_CHANGE),
		UDI_BUS_BIND_CB_NUM,
		UDI_BUS_DEVICE_OPS_NUM, UDI_BUS_BIND_ACK_NUM,
		udi_bus_bind_cb_layout, blankLayout),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_bus_unbind_req, REQ,
		(UDI_MEI_OP_STATE_CHANGE),
		UDI_BUS_BIND_CB_NUM,
		UDI_BUS_DEVICE_OPS_NUM, UDI_BUS_UNBIND_ACK_NUM,
		udi_bus_bind_cb_layout, blankLayout),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_intr_attach_req, REQ,
		(0),
		UDI_BUS_INTR_ATTACH_CB_NUM,
		UDI_BUS_DEVICE_OPS_NUM, UDI_INTR_ATTACH_ACK_NUM,
		udi_intr_attach_cb_layout, blankLayout),

	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_intr_detach_req, REQ,
		(0),
		UDI_BUS_INTR_DETACH_CB_NUM,
		UDI_BUS_DEVICE_OPS_NUM, UDI_INTR_DETACH_ACK_NUM,
		udi_intr_detach_cb_layout, blankLayout),

	ZUDI_MEI_BLANK_OP_TEMPLATE_CREATE
};

const static udi_ubit8_t	udi_intr_event_ind_layout[] =
	{ UDI_DL_UBIT8_T, UDI_DL_END };

// udi_intr_handler_ops_t
const static udi_mei_op_template_t	udi_intr_handler_op_template_list[] =
{
	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_intr_event_ind, IND,
		(0),
		UDI_BUS_INTR_EVENT_CB_NUM,
		UDI_BUS_INTR_DISPATCH_OPS_NUM, UDI_INTR_EVENT_RDY_NUM,
		udi_intr_event_cb_layout_preserve,
		udi_intr_event_ind_layout),

	ZUDI_MEI_BLANK_OP_TEMPLATE_CREATE
};

// udi_intr_dispatcher_ops_t
const static udi_mei_op_template_t	udi_intr_dispatcher_op_template_list[] =
{
	ZUDI_MEI_OP_TEMPLATE_CREATE(
		udi_intr_event_rdy, RDY,
		(0),
		UDI_BUS_INTR_EVENT_CB_NUM,
		0, 0,
		udi_intr_event_cb_layout_nopreserve, blankLayout),

	ZUDI_MEI_BLANK_OP_TEMPLATE_CREATE
};

const static udi_mei_ops_vec_template_t	udi_bridge_ops_vec_template_list[] =
{
	// udi_bus_device_ops_t
	{
		UDI_BUS_DEVICE_OPS_NUM,
		UDI_MEI_REL_BIND | UDI_MEI_REL_EXTERNAL | UDI_MEI_REL_INITIATOR,
		udi_bus_device_op_template_list
	},
	// udi_bus_bridge_ops_t
	{
		UDI_BUS_BRIDGE_OPS_NUM,
		UDI_MEI_REL_BIND | UDI_MEI_REL_EXTERNAL,
		udi_bus_bridge_op_template_list
	},
	// udi_intr_handler_ops_t
	{
		UDI_BUS_INTR_HANDLER_OPS_NUM,
		UDI_MEI_REL_EXTERNAL | UDI_MEI_REL_INITIATOR,
		udi_intr_handler_op_template_list
	},
	// udi_intr_dispatcher_ops_t
	{
		UDI_BUS_INTR_DISPATCH_OPS_NUM,
		UDI_MEI_REL_EXTERNAL,
		udi_intr_dispatcher_op_template_list
	},

	ZUDI_MEI_BLANK_OPS_VEC_TEMPLATE_CREATE
};

const udi_mei_init_t		udi_bridge_meta_info =
{
	udi_bridge_ops_vec_template_list
};
