
#include <tests.h>
#include <debug.h>
#define UDI_VERSION		0x101
#include <udi.h>
#undef UDI_VERSION
#include <__kstdlib/callback.h>
#include <__kstdlib/__kcxxlib/memory>
#include <kernel/common/thread.h>
#include <kernel/common/messageStream.h>
#include <kernel/common/zasyncStream.h>
#include <kernel/common/floodplainn/zum.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/floodplainn/movableMemory.h>
#include <kernel/common/floodplainn/initInfo.h>
#include <kernel/common/floodplainn/floodplainnStream.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <commonlibs/libzbzcore/libzbzcore.h>

#include <commonlibs/libzbzcore/libzudi.h>
#include <commonlibs/libzbzcore/libzbzcore.h>
#include <kernel/common/floodplainn/initInfo.h>
#include <kernel/common/floodplainn/movableMemory.h>
template <class T, int N>
struct sMovableMemObject
{
	sMovableMemObject(void) : hdr(sizeof(T) * N) {}

	fplainn::sMovableMemory		hdr;
	T				a[N];
};

namespace tests
{
namespace kernel
{
namespace floodplainn
{
namespace zum
{
TESTS_FN_MAKE_PROTOTYPE_DEFVARS(sendAMessage)
{
	/* This test code here is a snippet from main.c which was used to test
	 * the ability to send IPC messages across a Stream2Driver channel using
	 * FloodplainnStream::send().
	 *
	 * We send a GIO message. I'm not sure what sGxcb::sbHdr is used for,
	 * so you might have to investigate to find out.
	 *	RE: Answer: the sControlBlock header is only relevant when
	 * 	doing IPC via udi_mei_call().
	 * 	If you're doing the IPC via FloodplainnStream::send() or
	 * 	fplainn::Zudi::send() directly, then there's no need for it.
	 *	The sControlBlock header is created when the CB is cloned for
	 * 	a driver's use in calleeCloneCB(), and it's accessed when a cb
	 * 	is passed to udi_mei_call().
	 */
/*	struct sGxcb
	{
		sGxcb(void)
		{
			memset(&cb, 0, sizeof(cb));
			cb.tr_params = &tr_params;
		}

		lzudi::sControlBlock	cbHdr;
		udi_gio_xfer_cb_t	cb;
		struct _{
			_(void) { a[0] = a[1] = 0xDEaDBEEF; }
			ubit32		a[2];
		} tr_params;
	} cb;

	extern udi_mei_init_t			udi_gio_meta_info;
	fplainn::MetaInit			mParser(&udi_gio_meta_info);
	udi_layout_t				tmpLay[] =
		{ UDI_DL_UBIT32_T, UDI_DL_UBIT32_T, UDI_DL_END };
//		{ UDI_DL_END };

	udi_layout_t		*l[3] = {
		mParser.getOpTemplate(1, 3)->visible_layout,
		mParser.getOpTemplate(1, 3)->marshal_layout,
		//__klzbzcore::region::channel::mgmt::layouts::channel_event_ind,
		tmpLay
	};

	error_t send(
		fplainn::Endpoint *endp,
		udi_cb_t *gcb, udi_layout_t *layouts[3],
		utf8Char *metaName, udi_index_t meta_ops_num, udi_index_t op_idx,
		void *privateData,
		...);

	self->parent->floodplainnStream.send(
		dev->instance->mgmtEndpoint, &cb.cb.gcb,
		l, CC"udi_gio",
		UDI_GIO_PROVIDER_OPS_NUM, 3, NULL,
		0xFF, 0xCaFEBaBE);*/
}

TESTS_FN_MAKE_PROTOTYPE_DEFVARS(makeARequestToZumWithMovableMemory)
{
	/* Test code here is some code from main.cpp which was used to test
	 * our ability to call ZUM.enumerateReq(), passing in a MovableMemory
	 * object to be marshalled to the drive on the other end.
	 *
	 * The things being tested were:
	 * * Marshaling of the filter list elements which must have a
	 *   MovableMemory header prepended to them.
	 * * IPC to the ZUM server thread.
	 * * Receipt of the returned message from the driver, and the subsequent
	 *   unmarshalling of said message.
	 */
	struct sGecb
	{
		sGecb(void)
		{
			memset(&cb, 0, sizeof(cb));
		}

		lzudi::sControlBlock			hdr;
		udi_enumerate_cb_t		cb;
	} ecb;

	sMovableMemObject<udi_instance_attr_list_t, 2>	ta;
	sMovableMemObject<udi_filter_element_t, 2>	tf;

	{
		strcpy8(CC ta.a[0].attr_name, CC"attr0");
		strcpy8(CC ta.a[0].attr_value, CC"attr0-value");
		ta.a[0].attr_type = 1;
		strcpy8(CC ta.a[1].attr_name, CC"attr1");
		strcpy8(CC ta.a[1].attr_value, CC"attr1-value");
		ta.a[1].attr_type = 1;
	};
	{
		strcpy8(CC tf.a[0].attr_name, CC"filter0");
		strcpy8(CC tf.a[1].attr_name, CC"filter1");
	};
	{
		ecb.cb.attr_valid_length = 1;
		ecb.cb.filter_list_length = 2;
		ecb.cb.attr_list = ta.a;
		ecb.cb.filter_list = tf.a;
	};

	__kcbFn		__kotp;
	floodplainn.zum.enumerateReq(
		CC"by-id", UDI_ENUMERATE_START, &ecb.cb,
		new __kCallback(&__kotp));

	{
		/* This code is used after the enumerateReq call returns to
		 * process the outputs.
		 */
		printf(NOTICE"here, finished enum req.\n");
		printf(NOTICE ORIENT"msg %p: %d attr @%p, %d filt @%p.\n",
			msg,
			ecb->attr_valid_length,
			ecb->attr_list,
			ecb->filter_list_length,
			ecb->filter_list);

		floodplainn.zum.getEnumerateReqAttrsAndFilters(ecb, a, tf.a);

		{
			ecb->filter_list_length = 2;
			ecb->filter_list = tf.a;
		};
	}
}

testFn	*tests[] = {
};

}
}
}
}
