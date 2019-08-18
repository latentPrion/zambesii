
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
 *
 **	DRIVER STRUCTURE:
 * Externally exposed interfaces:
 *	* We will have to expose a udi_bridge child_bind_ops.
 *
 **/
static udi_ubit8_t			blankFlags=0;

static const udi_mgmt_ops_t		zrootdev_mgmt_ops =
{
	&zrootdev_usage_ind,
	&zrootdev_enumerate_req,
	&zrootdev_devmgmt_req,
	&zrootdev_final_cleanup_req
};

/**	EXPLANATION:
 * We will never handle IRQs. We are the kernel. Whatever logic is needed to
 * handle IRQs is actually kernel logic, and will not be done by this driver.
 *
 * We only need an intr_dispatcher_ops_t ops vector since we will only ever be
 * dispatching IRQs and not handling them.
 **/
static const udi_intr_dispatcher_ops_t	zrootdev_intr_dispatcher_ops =
{
	&zrootdev_intr_channel_event_ind,
	&zrootdev_intr_event_rdy
};

static const udi_bus_bridge_ops_t	zrootdev_bus_bridge_ops =
{
	&zrootdev_bus_channel_event_ind,
	&zrootdev_bus_bind_req,
	&zrootdev_bus_unbind_req,
	&zrootdev_intr_attach_req,
	&zrootdev_intr_detach_req
};

static const udi_ops_init_t		zrootdev_ops_init_info[] =
{
	// ChildBOp: 1 0 1: zbz_root (is udi_bridge).
	{
		1, 1, 2,
		0,
		(udi_ops_vector_t *)&zrootdev_bus_bridge_ops,
		&blankFlags
	},
	// Dynamic channel: udi_bridge; dispatcher ops vector.
	{
		2, 2, 3,
		0,
		(udi_ops_vector_t *)&zrootdev_intr_dispatcher_ops,
		&blankFlags
	},

	// EOList.
	{ 0, 0, 0, 0, NULL, NULL }
};

static const udi_primary_init_t		zrootdev_primary_init_info =
{
	&zrootdev_mgmt_ops, &blankFlags,
	0,
	/* zbz_root enumerated devices require 6 attrs.
	 * (4 generic enumeration attributes + meta specific attrs.)
	 **/
	6,
	sizeof(udi_init_context_t) + 0,
	0, 0
};

static const udi_secondary_init_t	sectmp[] =
{
	{ 1, 5 },
	{ 2, 32 },
	{ 3, 8 },
	{ 0, 0 }
};

udi_layout_t				l20b[] =
	{ UDI_DL_UBIT8_T, UDI_DL_UBIT32_T, UDI_DL_INLINE_UNTYPED,
	UDI_DL_UBIT16_T, UDI_DL_UBIT16_T, UDI_DL_UBIT16_T,
UDI_DL_END };

udi_layout_t				l12b[] =
	{ UDI_DL_ARRAY,
		3, UDI_DL_UBIT32_T,
	UDI_DL_END,
UDI_DL_END };

udi_layout_t				l32b[] =
	{ UDI_DL_ARRAY,
		2, UDI_DL_UBIT8_T, UDI_DL_UBIT16_T,
		UDI_DL_MOVABLE_UNTYPED, UDI_DL_MOVABLE_UNTYPED,
		UDI_DL_INLINE_UNTYPED,
		UDI_DL_MOVABLE_TYPED,
			UDI_DL_UBIT8_T, UDI_DL_UBIT16_T,
			UDI_DL_UBIT32_T,
		UDI_DL_END,
		UDI_DL_UBIT16_T, UDI_DL_UBIT8_T, UDI_DL_UBIT8_T,
	UDI_DL_END,
UDI_DL_END };

static const udi_cb_init_t		cbtmp[] =
{
	{ 1, 1, 1, 0, 0, NULL },
	{ 2, 1, 1, 0, 0, NULL },
	{ 3, 1, 1, 0, 0, NULL },
	{ 4, 1, 2, 2, 0, NULL },
	{ 5, 1, 2, 2, 0, NULL },
	{ 6, 1, 2, 3, 0, NULL },
	{ 7, 3, 2, 3, 0, NULL },
	{ 0, 0, 0, 0, 0, NULL }
};

extern "C" const udi_init_t			zrootdev_init_info =
{
	&zrootdev_primary_init_info,
	// No secondary regions needed.
	sectmp,
	zrootdev_ops_init_info,
	cbtmp, NULL, NULL
};

