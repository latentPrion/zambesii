
#include <__kstdlib/__kclib/string8.h>
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

/* base_devs holds the attrs for those devices which are not chipset-specific
 * such as the __kramdisk and vchipset
 */
static struct child_dev base_devs[] = {
	{
		.childId = 1,
		.identifier = "ramdisk",
		.address_locator = "root-dev1",

		.bus_type = "zbz_root",
		.zbz_root_device_type = "ramdisk"
	},
	{
		.childId = 2,
		.identifier = "vchipset",
		.address_locator = "root-dev2",

		.bus_type = "zbz_root",
		.zbz_root_device_type = "virtual-chipset"
	}
};

udi_ubit8_t zrootdev_get_n_base_devs(void)
{
	return sizeof(base_devs) / sizeof(*base_devs);
}

void zrootdev_root_dev_mark_released(udi_ubit32_t childId)
{
	const int N_BASE_DEVS = zrootdev_get_n_base_devs();

	for (int i = 0; i < N_BASE_DEVS; i++)
	{
		if (base_devs[i].childId == childId) {
			base_devs[i].enum_released = 1;
		}
	}
}

#define COPY_DEV_MEMBER_STRING_TO_ATTR(devvar,attrvar,membername) \
	(attrvar)->attr_type = UDI_ATTR_STRING; \
	(attrvar)->attr_length = strnlen8( \
		CC (devvar)->membername, UDI_MAX_ATTR_SIZE); \
	strncpy8(CC (attrvar)->attr_name, CC #membername, UDI_MAX_ATTR_NAMELEN); \
	strncpy8( \
		CC (attrvar)->attr_value, CC (devvar)->membername, \
		UDI_MAX_ATTR_SIZE);

void zrootdev_child_dev_to_enum_attrs(
	struct child_dev *dev, udi_instance_attr_list_t *outattrs,
	udi_ubit32_t *outChildId
	)
{
	*outChildId = dev->childId;

	COPY_DEV_MEMBER_STRING_TO_ATTR(dev, &outattrs[0], identifier);
	COPY_DEV_MEMBER_STRING_TO_ATTR(dev, &outattrs[1], address_locator);
	COPY_DEV_MEMBER_STRING_TO_ATTR(dev, &outattrs[2], bus_type);
	COPY_DEV_MEMBER_STRING_TO_ATTR(dev, &outattrs[3], zbz_root_device_type);
}

udi_boolean_t zrootdev_root_dev_to_enum_attrs(
	udi_ubit32_t index, udi_instance_attr_list_t *outattrs,
	udi_ubit32_t *outChildId
	)
{
	const unsigned N_BASE_DEVS = zrootdev_get_n_base_devs();

	if (index >= N_BASE_DEVS) { return false; }

	zrootdev_child_dev_to_enum_attrs(
		&base_devs[index], outattrs, outChildId);

	return true;
}

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
	&zrootdev_child_bus_channel_event_ind,
	&zrootdev_child_bus_bind_req,
	&zrootdev_child_bus_unbind_req,
	&zrootdev_child_intr_attach_req,
	&zrootdev_child_intr_detach_req
};

static const udi_ops_init_t		zrootdev_ops_init_info[] =
{
	// ChildBOp: 1 0 1: zbz_root (is udi_bridge).
	{
		1, 2, 2,
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
	/* zbz_root enumerated devices require 4 attrs.
	 * (2 generic enumeration attributes + 2 meta specific attrs.)
	 **/
	4,
	sizeof(udi_init_context_t) + sizeof(primary_rdata_t),
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
	{ 1, 3, 1, 0, 0, NULL },
	{ 2, 3, 1, 0, 0, NULL },
	{ 3, 4, 1, 0, 0, NULL },
	{ 4, 4, 2, 2, 0, NULL },
	{ 5, 3, 2, 2, 0, NULL },
	{ 6, 4, 2, 3, 0, NULL },
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

