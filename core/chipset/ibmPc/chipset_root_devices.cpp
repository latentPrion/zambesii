
#define UDI_VERSION 0x101
#define UDI_PHYSIO_VERSION 0x101
#include <udi.h>
#include <udi_physio.h>
#include "../../commonlibs/drivers/zrootdev.h"

struct child_dev chipset_devs[] = {
	{
		.childId = 3,
		.identifier = "chipset",
		.address_locator = "root-dev3",
		.bus_type = "zbz_root",
		.zbz_root_device_type = "chipset"
	}
};

udi_ubit8_t chipset_getNRootDevices(void)
{
	return sizeof(chipset_devs) / sizeof(*chipset_devs);
}

udi_boolean_t chipset_root_dev_to_enum_attrs(
	udi_ubit32_t index, udi_instance_attr_list_t *outattrs,
	udi_ubit32_t *outChildId)
{
	const unsigned N_BASE_DEVS = zrootdev_get_n_base_devs();
	const unsigned N_CHIPSET_DEVS = chipset_getNRootDevices();

	if (index < N_BASE_DEVS) { return false; }

	index -= N_BASE_DEVS;

	if (index >= N_CHIPSET_DEVS) { return false; }

	zrootdev_child_dev_to_enum_attrs(
		&chipset_devs[index], outattrs, outChildId);

	return true;
}

void chipset_root_dev_mark_released(udi_ubit32_t childId)
{
	const int N_CHIPSET_DEVS = chipset_getNRootDevices();

	for (int i = 0; i < N_CHIPSET_DEVS; i++)
	{
		if (chipset_devs[i].childId == childId) {
			chipset_devs[i].enum_released = 1;
		}
	}
}
