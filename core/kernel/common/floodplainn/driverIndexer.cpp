
#include <__ksymbols.h>
#include <zudiIndex.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/ptrList.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>


/**	EXPLANATION:
 * This process is responsible for keeping an index of all drivers available
 * and answers requests from the kernel:
 *
 * ZMESSAGE_FLOODPLAINN_LOAD_DRIVER
 *	This message causes the driver indexer to seek out a driver for the
 *	device argument passed in the request message.
 **/

static zudiIndex_headerS				*__kindexHeader=
	(zudiIndex_headerS *)&chipset_udi_index_driver_headers;

static zudiIndex_driverHeaderS				*__kdriverHeaders=
	(zudiIndex_driverHeaderS *)&__kindexHeader[1];

static zudiIndex_driverDataS				*__kdriverData=
	(zudiIndex_driverDataS *)&chipset_udi_index_driver_data;

static zudiIndex_regionS				*__kregionIndex=
	(zudiIndex_regionS *)&chipset_udi_index_regions;

static zudiIndex_deviceHeaderS				*__kdeviceIndex=
	(zudiIndex_deviceHeaderS *)&chipset_udi_index_devices;

static zudiIndex_rankHeaderS				*__krankIndex=
	(zudiIndex_rankHeaderS *)&chipset_udi_index_ranks;

static zudiIndex_messageS				*__kmessageIndex=
	(zudiIndex_messageS *)&chipset_udi_index_messages;

static zudiIndex_disasterMessageS			*__kdisasterIndex=
	(zudiIndex_disasterMessageS *)&chipset_udi_index_disaster_messages;

static zudiIndex_readableFileS				*__kreadableFileIndex=
	(zudiIndex_readableFileS *)&chipset_udi_index_readable_files;

static zudiIndex_messageFileS				*__kmessageFileIndex=
	(zudiIndex_messageFileS *)&chipset_udi_index_message_files;

static zudiIndex_provisionS				*__kprovisionIndex=
	(zudiIndex_provisionS *)&chipset_udi_index_provisions;

static void *getEsp(void)
{
	void		*esp;

	asm volatile (
		"movl	%%esp, %0\n\t"
		: "=r" (esp));

	return esp;
}

namespace zudiIndex
{
	zudiIndex_driverHeaderS *findMetalanguage(const utf8Char *metaName);

	zudiIndex_driverHeaderS *getDriverHeader(ubit16 id);
	zudiIndex_driverDataS *getDriverData(ubit16 id);
	zudiIndex_rankHeaderS *getRanks(zudiIndex_driverHeaderS *metaHeader);
	zudiIndex_deviceDataS *getDeviceAttribute(
		zudiIndex_deviceHeaderS *dHeader, const utf8Char *name);

	zudiIndex_messageS *getMessage(
		zudiIndex_driverHeaderS *driverHeader, ubit16 index);

	namespace driver
	{
		zudiIndex_driverDataS::metalanguageS *getMetalanguage(
			const zudiIndex_driverHeaderS *dHeader, ubit16 id);
	}
}

#define GET_NEXT_RANK(__r)			\
	((zudiIndex_rankHeaderS *)((uintptr_t)(__r) \
		+ sizeof(*(__r)) \
		+ (sizeof(zudiIndex_rankDataS) * (__r)->nAttributes)))

static status_t fplainnIndexer_loadDriver_compareEnumerationAttributes(
	fplainn::deviceC *device, zudiIndex_deviceHeaderS *devline,
	zudiIndex_rankHeaderS *const ranks,
	zudiIndex_driverHeaderS *metaHeader
	)
{
	zudiIndex_rankDataS		*rankAttribs;
	// Return value is the rank of the driver.
	status_t			ret=ERROR_NO_MATCH;
	zudiIndex_rankHeaderS		*currRank;

	currRank = ranks;
	// Run through all the rank lines for the relevant metalanguage.
	for (uarch_t i=0;
		i<metaHeader->nRanks
		&& currRank < (zudiIndex_rankHeaderS *)
			&chipset_udi_index_ranks_end;
		currRank = GET_NEXT_RANK(currRank), i++)
	{
		ubit16		nAttributesMatched;

		if (currRank->driverId != metaHeader->id)
		{
			printf(ERROR FPLAINNIDX"compareEnumAttr: Rank props "
				"line's driverId doesn't match driverId of "
				"parent meta.\n");

			break;
		};

		// Run through all the attributes for the current rank line.
		nAttributesMatched = 0;
		rankAttribs = (zudiIndex_rankDataS *)&currRank[1];
		for (uarch_t j=0; j<currRank->nAttributes; j++)
		{
			zudiIndex_deviceDataS	*devlineData;
			sarch_t			attrMatched=0;

			devlineData = zudiIndex::getDeviceAttribute(
				devline, CC rankAttribs[j].name);

			/* If the device props line doesn't have the attrib,
			 * skip this rank props line.
			 **/
			if (devlineData == NULL) { break; };

			// Compare each attrib against the device's enum attribs
			for (uarch_t k=0; k<device->nEnumerationAttribs; k++)
			{
				if (strcmp8(
					CC device->enumeration[k]->name,
					CC rankAttribs[j].name) != 0)
				{
					continue;
				};

				if (device->enumeration[k]->type
					!= devlineData->type)
				{
					break;
				};

				switch (device->enumeration[k]->type)
				{
				case ZUDI_DEVICE_ATTR_STRING:
					if (!strcmp8(
						CC device->enumeration[k]->value
							.string,
						CC devlineData->value.string))
					{
						attrMatched = 1;
					};

					break;
				case ZUDI_DEVICE_ATTR_UBIT32:
					if (device->enumeration[k]->value
						.unsigned32
						== devlineData->value.unsigned32)
					{
						attrMatched = 1;
					};

					break;
				case ZUDI_DEVICE_ATTR_BOOL:
					if (device->enumeration[k]->value.boolval
						== devlineData->value.boolval)
					{
						attrMatched = 1;
					};

					break;
				case ZUDI_DEVICE_ATTR_ARRAY8:
					if (device->enumeration[k]->size
						!= devlineData->size)
					{
						break;
					};

					if (memcmp(
						device->enumeration[k]->value
							.array8,
						devlineData->value.array8,
						devlineData->size))
					{
						attrMatched = 1;
					};

					break;
				default:
					printf(ERROR FPLAINNIDX"compareEnum"
						"Attr: Invalid attrib type.\n");

					break;
				};

				if (attrMatched) { break; };
			};

			if (attrMatched) { nAttributesMatched++; };
		};

		if (nAttributesMatched == currRank->nAttributes) {
			if (ret < currRank->rank) { ret = currRank->rank; };
		};
	};

	return ret;
}

#define GET_NEXT_DEVICE_HEADER_OFFSET(__dh)			\
	(zudiIndex_deviceHeaderS *)((uintptr_t)(&__dh[1]) \
		+ ((__dh)->nAttributes * sizeof(zudiIndex_deviceDataS)))

static void fplainnIndexer_loadDriver(zmessage::iteratorS *gmsg)
{
	threadC					*self;
	error_t					err;
	floodplainnC::driverIndexRequestS	*request;
	floodplainnC::driverIndexResponseS	*response;
	fplainn::deviceC			*dev;
	zudiIndex_deviceHeaderS			*devlineHeader;
	zudiIndex_messageS			*devlineName;
	zudiIndex_driverHeaderS			*driverHeader;
	ptrListC<zudiIndex_deviceHeaderS>	matchingDevices;
	status_t				bestRank=-1;

	self = static_cast<threadC *>(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask() );

	request = (floodplainnC::driverIndexRequestS *)gmsg;
	response = new floodplainnC::driverIndexResponseS;
	if (response == NULL)
	{
		printf(FATAL FPLAINNIDX"loadDriver: No heap mem for response "
			"message.\n");

		// This is actually panic() worthy.
		return;
	};

	response->header.sourceId = request->header.sourceId;
	response->header.targetId = request->header.targetId;
	response->header.size = sizeof(*response);
	response->header.subsystem = ZMESSAGE_SUBSYSTEM_FLOODPLAINN;
	response->header.function = ZMESSAGE_FPLAINN_LOADDRIVER;
	strcpy8(response->deviceName, request->deviceName);

	printf(NOTICE FPLAINNIDX"loadDriver: devname %s. Src 0x%x, target 0x%x."
		"\n", request->deviceName,
		request->header.sourceId, request->header.targetId);

	err = floodplainn.getDevice(request->deviceName, &dev);
	if (err != ERROR_SUCCESS)
	{
		printf(NOTICE FPLAINNIDX"loadDriver: invalid device %s.\n",
			request->deviceName);

		response->header.error = ERROR_INVALID_RESOURCE_NAME;
		messageStreamC::enqueueOnThread(
			response->header.targetId, &response->header);

		return;
	};

	/**	EXPLANATION:
	 * Search through the device index. For each device record, use its
	 * metalanguage-index to find the corresponding metalanguage and its
	 * "rank" declarations. Then use the rank declarations to determine
	 * whether or not the device record matches the device we have been
	 * asked to locate a driver for.
	 **/
	devlineHeader = __kdeviceIndex;
	for (; devlineHeader
		< (zudiIndex_deviceHeaderS *)&chipset_udi_index_devices_end;
		devlineHeader = GET_NEXT_DEVICE_HEADER_OFFSET(devlineHeader))
	{
		zudiIndex_driverHeaderS			*metaHeader;
		zudiIndex_driverDataS::metalanguageS	*devlineMetalanguage;
		zudiIndex_rankHeaderS			*devlineRanks;
		status_t				rank;

		driverHeader = zudiIndex::getDriverHeader(
			devlineHeader->driverId);

		if (driverHeader == NULL)
		{
			printf(FATAL FPLAINNIDX"loadDriver: index DEVICE has "
				"invalid driverId %d. Skipping.\n",
				devlineHeader->driverId);

			continue;
		};

		devlineName = zudiIndex::getMessage(
			driverHeader, devlineHeader->messageIndex);

		if (devlineName == NULL)
		{
			printf(FATAL FPLAINNIDX"loadDriver: index DEVICE has "
				"invalid device name msg_idx %d.\n",
				devlineHeader->messageIndex);

			continue;
		};

		// Now get the metalanguage for this device props line.
		devlineMetalanguage = zudiIndex::driver::getMetalanguage(
			driverHeader, devlineHeader->metaIndex);

		if (devlineMetalanguage == NULL)
		{
			printf(FATAL FPLAINNIDX"loadDriver: index DEVICE has "
				"invalid meta_idx %d. Skipping.\n",
				devlineHeader->metaIndex);

			continue;
		};

		metaHeader = zudiIndex::findMetalanguage(
			CC devlineMetalanguage->name);

		if (metaHeader == NULL)
		{
			printf(FATAL FPLAINNIDX"loadDriver: DEVICE line refers "
				"to inexistent metalanguage %s.\n",
				devlineMetalanguage->name);

			continue;
		};

		devlineRanks = zudiIndex::getRanks(metaHeader);
		if (devlineRanks == NULL)
		{
			printf(FATAL FPLAINNIDX"loadDriver: DEVICE line refers "
				"to a meta with no ranks.\n");

			continue;
		};

		/* Now compare the device line attributes to the actual device's
		 * attributes.
		 **/
		rank = fplainnIndexer_loadDriver_compareEnumerationAttributes(
			dev, devlineHeader, devlineRanks, metaHeader);

		if (rank >= 0 && rank > bestRank)
		{
			matchingDevices.~ptrListC();
			new (&matchingDevices)
				ptrListC<zudiIndex_deviceHeaderS *>;

			err = matchingDevices.initialize();
			if (err != ERROR_SUCCESS)
			{
				printf(ERROR FPLAINNIDX"loadDriver: Failed to "
					"initialize matchingDevices list.\n");

				continue;
			};

			bestRank = rank;
		};

		if (rank >= 0)
		{
			err = matchingDevices.insert(devlineHeader);
			if (err != ERROR_SUCCESS)
			{
				printf(ERROR FPLAINNIDX"loadDriver: Failed to "
					"insert device line into "
					"matchingDevices.\n");

				continue;
			};

			printf(NOTICE FPLAINNIDX"loadDriver: Devnode \"%s\" "
				"ranks %d, matching device\n\t\"%s\"\n",
				request->deviceName, rank,
				devlineName->message);
		};
	};

	if (matchingDevices.getNItems() == 0) {
		response->header.error = ERROR_NO_MATCH;
	}
	else
	{
		void		*handle=NULL;

		if (matchingDevices.getNItems() > 1)
		{
			printf(WARNING FPLAINNIDX"loadDriver: %s: Multiple "
				"viable matches. Choosing first.\n",
				request->deviceName);
		};

		response->header.error = ERROR_SUCCESS;

		// Shouldn't need to error check these calls.
		devlineHeader = matchingDevices.getNextItem(&handle);
		driverHeader = zudiIndex::getDriverHeader(
			devlineHeader->driverId);

		strcpy8(dev->driverFullName, CC driverHeader->basePath);
		strcpy8(
			&dev->driverFullName[
				strlen8(CC driverHeader->basePath)],
			CC driverHeader->shortName);
	};

	messageStreamC::enqueueOnThread(
		response->header.targetId, &response->header);
}

void floodplainnC::driverIndexerEntry(void)
{
	threadC			*self;
	zmessage::iteratorS	gcb;

	self = static_cast<threadC *>(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask() );

	floodplainn.indexerThreadId = self->getFullId();
	floodplainn.indexerQueueId = ZMESSAGE_SUBSYSTEM_REQ0;

	printf(NOTICE FPLAINNIDX"Floodplainn-indexer executing;\n"
		"\tprocess ID: 0x%x. ESP: 0x%p. RequestQ ID: %d. Dormanting.\n",
		self->getFullId(), getEsp(),
		floodplainn.indexerQueueId);

	printf(NOTICE FPLAINNIDX"Index: Version %d.%d. (%s), holds %d drivers.\n",
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
				printf(ERROR FPLAINNIDX"Unknown request "
					"%d.\n",
					gcb.header.function);
			};

			break;

		default:
			printf(NOTICE FPLAINNIDX"Unknown message.\n");
		};
	};
}

zudiIndex_driverHeaderS *zudiIndex::findMetalanguage(const utf8Char *name)
{
	zudiIndex_provisionS		*provisions;
	zudiIndex_driverHeaderS		*ret;

	provisions = __kprovisionIndex;
	for (; provisions < (zudiIndex_provisionS *)
		&chipset_udi_index_provisions_end;
		provisions++)
	{
		if (strcmp8(CC provisions->name, name) != 0)
			{ continue; };

		ret = zudiIndex::getDriverHeader(provisions->driverId);
		if (ret == NULL)
		{
			printf(FATAL FPLAINNIDX"findMetalanguage: a PROVISION "
				"props line (%s) has an invalid driverId.\n",
				CC provisions->name);

			continue;
		};

		return ret;
	};

	return NULL;
}

zudiIndex_driverHeaderS *zudiIndex::getDriverHeader(ubit16 id)
{
	zudiIndex_driverHeaderS		*ret;

	ret = __kdriverHeaders;
	for (; ret < (zudiIndex_driverHeaderS *)
		&chipset_udi_index_driver_headers_end;
		ret++)
	{
		if (ret->id == id) { return ret; };
	};

	return NULL;
}

zudiIndex_driverDataS *zudiIndex::getDriverData(ubit16 id)
{
	zudiIndex_driverHeaderS		*dh;
	zudiIndex_driverDataS		*ret;

	dh = getDriverHeader(id);
	if (dh == NULL) { return NULL; };

	ret = (zudiIndex_driverDataS *)
		((uintptr_t)__kdriverData + dh->dataFileOffset);

	return ret;
}

zudiIndex_driverDataS::metalanguageS *zudiIndex::driver::getMetalanguage(
	const zudiIndex_driverHeaderS *dHeader, ubit16 id
	)
{
	zudiIndex_driverDataS::metalanguageS	*metas;

	metas = (zudiIndex_driverDataS::metalanguageS *)
		((uintptr_t)__kdriverData + dHeader->metalanguagesOffset);

	for (uarch_t i=0; i<dHeader->nMetalanguages; i++) {
		if (metas[i].index == id) { return &metas[i]; };
	};

	return NULL;
}

zudiIndex_rankHeaderS *zudiIndex::getRanks(zudiIndex_driverHeaderS *metaHeader)
{
	zudiIndex_rankHeaderS		*ret;

	ret = (zudiIndex_rankHeaderS *)
		((uintptr_t)__krankIndex + metaHeader->rankFileOffset);

	return ret;
}

zudiIndex_deviceDataS *zudiIndex::getDeviceAttribute(
	zudiIndex_deviceHeaderS *dHeader, const utf8Char *name)
{
	zudiIndex_deviceDataS		*dData;

	dData = (zudiIndex_deviceDataS *)&dHeader[1];

	for (uarch_t i=0; i<dHeader->nAttributes; i++) {
		if (strcmp8(CC dData[i].name, name) == 0) { return &dData[i]; };
	};

	return NULL;
}

zudiIndex_messageS *zudiIndex::getMessage(
	zudiIndex_driverHeaderS *driverHeader, ubit16 index
	)
{
	zudiIndex_messageS		*ret;

	ret = __kmessageIndex;
	for (; ret < (zudiIndex_messageS *)&chipset_udi_index_messages_end;
		ret++)
	{
		if (ret->index == index) { return ret; };
	};

	return NULL;
}

