
// This is the only driver where this is allowed.
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>

#include "zrootdev.h"

static inline primary_rdata_t *
get_primary_rdata(udi_cb_t *cb)
{
	assert_fatal(cb->context != NULL);
	return static_cast<primary_rdata_t *>(cb->context);
}

void zrootdev_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	primary_rdata_t		*rd = get_primary_rdata(&cb->gcb);

	(void)cb; (void)resource_level;

	printf(NOTICE"We are saved by grace, through faith,\n\tand that not of "
		"ourselves: but it is a gift of El.\n"
		"\trdata %p, scratch %p, channel %p, resource_lvl %d.\n",
		cb->gcb.context, cb->gcb.scratch, cb->gcb.channel,
		resource_level);

	memset(&rd->e, 0, sizeof(rd->e));
	udi_usage_res(cb);
}

udi_buf_write_call_t zrootdev_enumerate_req1;

void zrootdev_enumerate_req(
	udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level
	)
{
	primary_rdata_t		*rd=get_primary_rdata(&cb->gcb);
	const int		N_BASE_DEVS = zrootdev_get_n_base_devs();
	const int		N_CHIPSET_DEVS = chipset_getNRootDevices();

	udi_ubit8_t		enum_result;

	(void)cb; (void)enumeration_level;

printf(NOTICE"enum!\tcb %p, %d attr @%p, %d filt @%p, rdata @%p.\n",
	cb,
	cb->attr_valid_length, cb->attr_list,
	cb->filter_list_length, cb->filter_list,
	rd);

	if (enumeration_level == UDI_ENUMERATE_START
		|| enumeration_level == UDI_ENUMERATE_START_RESCAN)
	{
		rd->e.curr_dev_idx = 0;
	}

	switch (enumeration_level)
	{
	case UDI_ENUMERATE_DIRECTED:
		enum_result = UDI_ENUMERATE_FAILED;
		break;

	case UDI_ENUMERATE_RELEASE:
		if (cb->child_ID > N_BASE_DEVS + N_CHIPSET_DEVS)
		{
			enum_result = UDI_ENUMERATE_FAILED;
			break;
		}

		zrootdev_root_dev_mark_released(cb->child_ID);
		chipset_root_dev_mark_released(cb->child_ID);
		enum_result = UDI_ENUMERATE_RELEASED;
		break;

	case UDI_ENUMERATE_START:
	case UDI_ENUMERATE_START_RESCAN:
	case UDI_ENUMERATE_NEXT:
	case UDI_ENUMERATE_NEW:
		enum_result = UDI_ENUMERATE_OK;
		if (rd->e.curr_dev_idx < N_BASE_DEVS)
		{
			zrootdev_root_dev_to_enum_attrs(
				rd->e.curr_dev_idx,
				cb->attr_list,
				&cb->child_ID);

			cb->attr_valid_length = 4;
			rd->e.curr_dev_idx++;
		}
		else if (rd->e.curr_dev_idx < N_BASE_DEVS + N_CHIPSET_DEVS)
		{
			chipset_root_dev_to_enum_attrs(
				rd->e.curr_dev_idx,
				cb->attr_list,
				&cb->child_ID);

			cb->attr_valid_length = 4;
			rd->e.curr_dev_idx++;
		}
		else if (enumeration_level == UDI_ENUMERATE_NEW)
		{
			/* If it's ENUMERATE_NEW and we've exhausted the known
			 * devices (i.e, curr_enum_dev_idx > N_BASE + N_CHIPSET)
			 * then store the posted CB and return.
			 */
			rd->e.posted_cb = cb;
			return;
		}
		else {
			enum_result = UDI_ENUMERATE_DONE;
		}
		break;

	default:
		enum_result = UDI_ENUMERATE_FAILED;
		break;
	}

	udi_enumerate_ack(cb, enum_result, 1);
	return;
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
printf(NOTICE"zrootdev: final_cleanup.\n");
	udi_final_cleanup_ack(cb);
}

static utf8Char *events[] =
{
	CC"CHANNEL_CLOSED", CC"CHANNEL_BOUND", CC"CHANNEL_OP_ABORTED"
};

void zrootdev_child_bus_channel_event_ind(udi_channel_event_cb_t *cb)
{
	(void)cb;
	printf(NOTICE"ayo~! %s. Chan %p.\n"
		"\tcb @%p, bind_cb @%p, bind_cb->scratch %p.\n",
		events[cb->event], cb->gcb.channel,
		cb, cb->params.internal_bound.bind_cb,
		(cb->params.internal_bound.bind_cb)
			? cb->params.internal_bound.bind_cb->scratch
			: NULL);

	udi_channel_event_complete(cb, UDI_OK);
}

void zrootdev_child_bus_bind_req(udi_bus_bind_cb_t *cb)
{
	(void)cb;
}

void zrootdev_child_bus_unbind_req(udi_bus_bind_cb_t *cb)
{
	(void)cb;
}

void zrootdev_child_intr_attach_req(udi_intr_attach_cb_t *cb)
{
	(void)cb;
}

void zrootdev_child_intr_detach_req(udi_intr_detach_cb_t *cb)
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
