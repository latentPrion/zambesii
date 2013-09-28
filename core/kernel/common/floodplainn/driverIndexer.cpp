
#include <__ksymbols.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/floodplainn/zudiIndex.h>


/**	EXPLANATION:
 * This process is responsible for keeping an index of all drivers available
 * and answers requests from the kernel:
 *
 * ZMESSAGE_FLOODPLAINN_LOAD_DRIVER
 *	This message causes the driver indexer to seek out a driver for the
 *	device argument passed in the request message.
 **/

static zudiIndexHeaderS					*__kindexHeader=
	(zudiIndexHeaderS *)&chipset_udi_index_driver_headers;

static zudiIndexDriverS::zudiIndexDriverHeaderS		*__kdriverHeaders=
	(zudiIndexDriverS::zudiIndexDriverHeaderS *)&__kindexHeader[1];

static zudiIndexDriverS::zudiIndexDriverDataS		*__kdriverData=
	(zudiIndexDriverS::zudiIndexDriverDataS *)
	&chipset_udi_index_driver_data;

static zudiIndexRegionS					*__kregionIndex=
	(zudiIndexRegionS *)&chipset_udi_index_regions;

static zudiIndexDeviceS::zudiIndexDeviceHeaderS		*__kdeviceIndex=
	(zudiIndexDeviceS::zudiIndexDeviceHeaderS *)
	&chipset_udi_index_devices;

static zudiIndexRankS::zudiIndexRankHeaderS		*__krankIndex=
	(zudiIndexRankS::zudiIndexRankHeaderS *)
	&chipset_udi_index_ranks;

static zudiIndexMessageS				*__kmessageIndex=
	(zudiIndexMessageS *)&chipset_udi_index_messages;

static zudiIndexDisasterMessageS			*__kdisasterIndex=
	(zudiIndexDisasterMessageS *)&chipset_udi_index_disaster_messages;

static zudiIndexReadableFileS				*__kreadableFileIndex=
	(zudiIndexReadableFileS *)&chipset_udi_index_readable_files;

static zudiIndexMessageFileS				*__kmessageFileIndex=
	(zudiIndexMessageFileS *)&chipset_udi_index_message_files;

static void *getEsp(void)
{
	void		*esp;

	asm volatile (
		"movl	%%esp, %0\n\t"
		: "=r" (esp));

	return esp;
}

void fplainnIndexer_loadDriver(zmessage::iteratorS *gmsg)
{
	floodplainnC::driverIndexRequestS	*req=
		(floodplainnC::driverIndexRequestS *)gmsg;

	__kprintf(NOTICE FPLAINNIDX"loadDriver: devname %s.\n",
		req->deviceName);
}

void floodplainnC::driverIndexerEntry(void)
{
	threadC			*self;
	zmessage::iteratorS	gcb;

	self = static_cast<threadC *>(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask() );

	floodplainn.indexerThreadId = self->getFullId();
	floodplainn.indexerQueueId = ZMESSAGE_SUBSYSTEM_REQ0;

	__kprintf(NOTICE FPLAINNIDX"Floodplainn-indexer executing;\n"
		"\tprocess ID: 0x%x. ESP: 0x%p. RequestQ ID: %d. Dormanting.\n",
		self->getFullId(), getEsp(),
		floodplainn.indexerQueueId);

	__kprintf(NOTICE FPLAINNIDX"Index: Version %d.%d. (%s), holds %d drivers.\n",
		__kindexHeader->majorVersion, __kindexHeader->minorVersion,
		__kindexHeader->endianness, __kindexHeader->nRecords);

	for (;;)
	{
		self->getTaskContext()->messageStream.pull(&gcb);
		switch (gcb.header.subsystem)
		{
		case ZMESSAGE_SUBSYSTEM_REQ0:
			switch (gcb.header.function)
			{
			case ZMESSAGE_FPLAINN_LOADDRIVER:
				fplainnIndexer_loadDriver(&gcb);
				break;

			default:
				__kprintf(ERROR FPLAINNIDX"Unknown request "
					"%d.\n",
					gcb.header.function);
			};

			break;

		default:
			__kprintf(NOTICE FPLAINNIDX"Unknown message.\n");
		};
	};
}

