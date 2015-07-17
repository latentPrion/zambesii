
// This is the only driver where this is allowed.
#include <__kclasses/debugPipe.h>

#include "zrootdev.h"


void zrootdev_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	(void)cb; (void)resource_level;

	printf(NOTICE"We are saved by grace, through faith,\n\tand that not of "
		"ourselves: but it is a gift of El.\n"
		"\trdata 0x%p, scratch 0x%p, channel 0x%p, resource_lvl %d.\n",
		cb->gcb.context, cb->gcb.scratch, cb->gcb.channel,
		resource_level);
	udi_usage_res(cb);
}

ubit32 ___=0;
void zrootdev_enumerate_req(
	udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level
	)
{
	(void)cb; (void)enumeration_level;

printf(NOTICE"enum!\tcb 0x%p, %d attr @0x%p, %d filt @0x%p.\n",
	cb,
	cb->attr_valid_length, cb->attr_list,
	cb->filter_list_length, cb->filter_list);

/*	for (uarch_t i=0; i<cb->attr_valid_length; i++) {
		printf(CC"\tattr %s.\n", cb->attr_list[i].attr_name);
	};

	for (uarch_t i=0; i<cb->filter_list_length; i++) {
		printf(CC"\tfilt %s.\n", cb->filter_list[i].attr_name);
	};*/

	if (___ >3) {
		udi_enumerate_ack(cb, UDI_ENUMERATE_DONE, 0);
	} else {
		cb->child_ID = ___++;
		udi_enumerate_ack(cb, UDI_ENUMERATE_OK, 0);
	};
}

void zrootdev_devmgmt_req(
	udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID
	)
{
	(void)cb; (void)mgmt_op; (void)parent_ID;

printf(NOTICE"zrootdev: devmgmt_req: op %d, parent %d.\n",
	mgmt_op, parent_ID);
	udi_devmgmt_ack(cb, 0, 0);
}

void zrootdev_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	(void)cb;
}

static utf8Char *events[] =
{
	CC"CHANNEL_CLOSED", CC"CHANNEL_BOUND", CC"CHANNEL_OP_ABORTED"
};

void zrootdev_bus_channel_event_ind(udi_channel_event_cb_t *cb)
{
	(void)cb;
	printf(NOTICE"ayo~! %s. Chan 0x%p.\n"
		"\tcb @0x%p, bind_cb @0x%p, bind_cb->scratch 0x%p.\n",
		events[cb->event], cb->gcb.channel,
		cb, cb->params.internal_bound.bind_cb,
		(cb->params.internal_bound.bind_cb)
			? cb->params.internal_bound.bind_cb->scratch
			: NULL);

	udi_channel_event_complete(cb, UDI_OK);
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

void zrootdev_intr_channel_event_ind(udi_channel_event_cb_t *cb)
{
	(void)cb;
}

void zrootdev_intr_event_rdy(udi_intr_event_cb_t *intr_event_cb)
{
	(void)intr_event_cb;
}
