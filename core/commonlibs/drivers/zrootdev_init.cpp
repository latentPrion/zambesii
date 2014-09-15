
#include "zrootdev.h"


/**	EXPLANATION:
 * This is the only driver which is allowed to violate the UDI specification
 * by including and linking to non-UDI code and symbols.
 *
 * That is because this driver is essentially a part of the kernel; it is the
 * embodiment of the kernel encapsulated within a UDI driver for the purpose
 * of making communication between drivers and the kernel easier.
 *
 * It acts as the parent of all devices, and the convergence point and eventual
 * collection point of all udi_intr* related requests to the kernel, etc.
 *
 * It also enumerates the ramdisk and vchipset child devices, and eventually,
 * the actual chipset devices.
 **/
static udi_ubit8_t			blankFlags=0;

static const udi_mgmt_ops_t		zrootdev_mgmt_ops =
{
	&zrootdev_usage_ind,
	&zrootdev_enumerate_req,
	&zrootdev_devmgmt_req,
	&zrootdev_final_cleanup_req
};

static const udi_gio_provider_ops_t	zrootdev_gio_provider_ops =
{
	&zrootdev_gio_channel_event_ind,
	&zrootdev_gio_bind_req,
	&zrootdev_gio_unbind_req,
	&zrootdev_gio_xfer_req,
	&zrootdev_gio_event_res
};

static const udi_bus_bridge_ops_t	zrootdev_bus_bridge_ops =
{
	&zrootdev_bus_channel_event_ind,
	&zrootdev_bus_bind_req,
	&zrootdev_bus_unbind_req,
	&zrootdev_intr_attach_req,
	&zrootdev_intr_detach_req
};

static const udi_primary_init_t		zrootdev_primary_init_info =
{
	&zrootdev_mgmt_ops, &blankFlags,
	0, 0,
	sizeof(udi_init_context_t) + 0,
	0, 0
};

static const udi_ops_init_t		zrootdev_ops_init_info[] =
{
	// Begin child bind ops.
	{
		1, 1, 2,
		0,
		(udi_ops_vector_t *)&zrootdev_bus_bridge_ops,
		&blankFlags
	},
	{
		2, 2, 1,
		0,
		(udi_ops_vector_t *)&zrootdev_gio_provider_ops,
		&blankFlags
	}
};

extern "C" const udi_init_t			zrootdev_init_info =
{
	&zrootdev_primary_init_info, NULL,
	zrootdev_ops_init_info,
	NULL, NULL, NULL
};

