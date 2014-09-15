
// This is the only driver where this is allowed.
#include <__kclasses/debugPipe.h>

#include "zrootdev.h"


void zrootdev_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	(void)cb; (void)resource_level;
printf(NOTICE"Zambesii root device driver executing.\n");
}

void zrootdev_enumerate_req(
	udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level
	)
{
	(void)cb; (void)enumeration_level;
}

void zrootdev_devmgmt_req(
	udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID
	)
{
	(void)cb; (void)mgmt_op; (void)parent_ID;
}

void zrootdev_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	(void)cb;
}

void zrootdev_bus_channel_event_ind(udi_channel_event_cb_t *cb)
{
	(void)cb;
}

void zrootdev_bus_bind_req(udi_bus_bind_cb_t *cb)
{
	(void)cb;
}

void zrootdev_bus_unbind_req(udi_bus_bind_cb_t *cb)
{
	(void)cb;
}

void zrootdev_intr_attach_req(udi_intr_attach_cb_t *cb)
{
	(void)cb;
}

void zrootdev_intr_detach_req(udi_intr_detach_cb_t *cb)
{
	(void)cb;
}

void zrootdev_gio_channel_event_ind(udi_channel_event_cb_t *cb)
{
	(void)cb;
}

void zrootdev_gio_bind_req(udi_gio_bind_cb_t *cb)
{
	(void)cb;
}

void zrootdev_gio_unbind_req(udi_gio_bind_cb_t *cb)
{
	(void)cb;
}

void zrootdev_gio_xfer_req(udi_gio_xfer_cb_t *cb)
{
	(void)cb;
}

void zrootdev_gio_event_res(udi_gio_event_cb_t *cb)
{
	(void)cb;
}
