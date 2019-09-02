#ifndef _ZAMBESII_ROOT_DEVICE_DRIVER_HEADER_H
	#define _ZAMBESII_ROOT_DEVICE_DRIVER_HEADER_H

	#define UDI_VERSION		0x101
	#include <udi.h>
	#define UDI_PHYSIO_VERSION	0x101
	#include <udi_physio.h>

#define ZROOTDEV_N_CHILDREN		(1)

struct child_dev
{
	udi_ubit16_t		childId;
	udi_boolean_t		enum_released;
	// Generic enum attrs.
	const char		*identifier,
				*address_locator,

	// zbz_root enum attrs.
				*bus_type,
				*zbz_root_device_type;
};

/* chipset_devs holds the attrs for those devices which are chipset specific
 * such as the chipset itself, or the multiple chipset top level devices if
 * the machine is multi-mobo, etc.
 */
extern struct child_dev chipset_devs[];

udi_ubit8_t chipset_getNRootDevices(void);
udi_ubit8_t zrootdev_get_n_base_devs(void);

void zrootdev_child_dev_to_enum_attrs(
	struct child_dev *dev, udi_instance_attr_list_t *outattrs,
	udi_ubit32_t *outChildId);

udi_boolean_t zrootdev_root_dev_to_enum_attrs(
	udi_ubit32_t index, udi_instance_attr_list_t *outattrs,
	udi_ubit32_t *outChildId);

void zrootdev_root_dev_mark_released(udi_ubit32_t childId);

/* To be defined by the per-chipset code. */
udi_boolean_t chipset_root_dev_to_enum_attrs(
	udi_ubit32_t index, udi_instance_attr_list_t *outattrs,
	udi_ubit32_t *outChildId);

void chipset_root_dev_mark_released(udi_ubit32_t childId);

typedef struct primary_rdata
{
	udi_init_context_t	init_context;

	struct enum_state
	{
		udi_index_t		curr_dev_idx;
		udi_enumerate_cb_t	*posted_cb;
	} e;
} primary_rdata_t;

udi_usage_ind_op_t		zrootdev_usage_ind;
udi_devmgmt_req_op_t		zrootdev_devmgmt_req;
udi_enumerate_req_op_t		zrootdev_enumerate_req;
udi_final_cleanup_req_op_t	zrootdev_final_cleanup_req;

/* Operations with our children */
udi_channel_event_ind_op_t	zrootdev_child_bus_channel_event_ind;
udi_bus_bind_req_op_t		zrootdev_child_bus_bind_req;
udi_bus_unbind_req_op_t		zrootdev_child_bus_unbind_req;
udi_intr_attach_req_op_t	zrootdev_child_intr_attach_req;
udi_intr_detach_req_op_t	zrootdev_child_intr_detach_req;

udi_channel_event_ind_op_t	zrootdev_intr_channel_event_ind;
udi_intr_event_rdy_op_t		zrootdev_intr_event_rdy;

#endif
