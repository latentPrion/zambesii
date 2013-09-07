
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>


/**	EXPLANATION:
 * This process is responsible for keeping an index of all drivers available
 * and answers requests from the kernel:
 *
 * ZMESSAGE_FLOODPLAINN_LOAD_DRIVER
 *	This message causes the driver indexer to seek out a driver for the
 *	device argument passed in the request message.
 **/

static void *getEsp(void)
{
	void		*esp;

	asm volatile (
		"movl	%%esp, %0\n\t"
		: "=r" (esp));

	return esp;
}

void floodplainnC::driverIndexerEntry(void)
{
	threadC			*self;
	zmessage::iteratorS	gcb;

	self = static_cast<threadC *>(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask() );

	floodplainn.indexerThreadId = self->getFullId();
	floodplainn.indexerQueueId = ZMESSAGE_SUBSYSTEM_USER0;

	__kprintf(NOTICE"Floodplainn-indexer executing;\n"
		"\tprocess ID: 0x%x. ESP: 0x%p. RequestQ ID: %d. Dormanting.\n",
		self->getFullId(), getEsp(),
		floodplainn.indexerQueueId);

	for (;;)
	{
		self->getTaskContext()->messageStream.pull(&gcb);
		switch (gcb.header.subsystem)
		{
		case ZMESSAGE_SUBSYSTEM_USER0:
			floodplainnC::driverIndexRequestS	*request;

			request = (floodplainnC::driverIndexRequestS *)&gcb;
			__kprintf(NOTICE"Request: device %s, indexlevel %d.\n",
				request->deviceName, request->indexLevel);

			break;

		default:
			__kprintf(NOTICE"Unknown message.\n");
		};
	};
}

