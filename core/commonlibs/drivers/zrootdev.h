#ifndef _ZAMBESII_ROOT_DEVICE_DRIVER_HEADER_H
	#define _ZAMBESII_ROOT_DEVICE_DRIVER_HEADER_H

	#define UDI_VERSION		0x101
	#include <udi.h>
	#define UDI_PHYSIO_VERSION	0x101
	#include <udi_physio.h>

udi_usage_ind_op_t		zrootdev_usage_ind;
udi_devmgmt_req_op_t		zrootdev_devmgmt_req;
udi_enumerate_req_op_t		zrootdev_enumerate_req;
udi_final_cleanup_req_op_t	zrootdev_final_cleanup_req;

udi_channel_event_ind_op_t	zrootdev_bus_channel_event_ind;
udi_bus_bind_req_op_t		zrootdev_bus_bind_req;
udi_bus_unbind_req_op_t		zrootdev_bus_unbind_req;
udi_intr_attach_req_op_t	zrootdev_intr_attach_req;
udi_intr_detach_req_op_t	zrootdev_intr_detach_req;

udi_channel_event_ind_op_t	zrootdev_intr_channel_event_ind;
udi_intr_event_rdy_op_t		zrootdev_intr_event_rdy;

#endif
