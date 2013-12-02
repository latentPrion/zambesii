
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
 * MSGSTREAM_FLOODPLAINN_LOAD_DRIVER
 *	This message causes the driver indexer to seek out a driver for the
 *	device argument passed in the request message.
 **/


static floodplainnC::newDeviceActionE		newDeviceAction;

static void *getEsp(void)
{
	void		*esp;

	asm volatile (
		"movl	%%esp, %0\n\t"
		: "=r" (esp));

	return esp;
}

#define GET_NEXT_RANK(__r)			\
	((zudi::rank::headerS *)((uintptr_t)(__r) \
		+ sizeof(*(__r)) \
		+ (sizeof(zudi::rank::rankAttrS) * (__r)->nAttributes)))

static status_t fplainnIndexer_detectDriver_compareEnumerationAttributes(
	fplainn::deviceC *device, zudi::device::headerS *devline,
	zudi::driver::headerS *metaHeader
	)
{
	// Return value is the rank of the driver.
	status_t			ret=ERROR_NO_MATCH;
	zudi::rank::headerS		currRank;
	utf8Char			*attrValueTmp;

	attrValueTmp = new ubit8[UDI_MAX_ATTR_SIZE];
	if (attrValueTmp == NULL) { return ERROR_MEMORY_NOMEM; };

	// Run through all the rank lines for the relevant metalanguage.
	for (uarch_t i=0;
		i<metaHeader->nRanks
		&& floodplainn.zudiIndexes[0].indexedGetRank(
			metaHeader, i, &currRank) == ERROR_SUCCESS;
		i++)
	{
		ubit16		nAttributesMatched;

		if (currRank.driverId != metaHeader->id)
		{
			printf(ERROR FPLAINNIDX"compareEnumAttr: Rank props "
				"line's driverId doesn't match driverId of "
				"parent meta.\n");

			break;
		};

		// Run through all the attributes for the current rank line.
		nAttributesMatched = 0;
		for (uarch_t j=0; j<currRank.nAttributes; j++)
		{
			zudi::device::attrDataS		devlineData;
			sarch_t				attrMatched=0;
			utf8Char			currRankAttrName[
				UDI_MAX_ATTR_NAMELEN];

			if (floodplainn.zudiIndexes[0].indexedGetRankAttrString(
				&currRank, j, currRankAttrName)
				!= ERROR_SUCCESS)
			{
				printf(ERROR FPLAINNIDX"compareEnumAttr: rank "
					"attr string extraction failed.\n");

				// TBH, we probably shouldn't even try to go on.
				break;
			};

			/* If the device props line doesn't have the attrib,
			 * skip this rank props line.
			 **/
			if (floodplainn.zudiIndexes[0].findDeviceData(
				devline, currRankAttrName, &devlineData)
				!= ERROR_SUCCESS)
			{ break; };

			// Compare each attrib against the device's enum attribs
			for (uarch_t k=0; k<device->nEnumerationAttribs; k++)
			{
				if (strcmp8(
					CC device->enumeration[k]->attr_name,
					currRankAttrName) != 0)
				{
					continue;
				};

				/* Even if they have the same name, if the types
				 * differ, we can't consider it a match.
				 **/
				if (device->enumeration[k]->attr_type
					!= devlineData.attr_type)
				{
					break;
				};

				switch (device->enumeration[k]->attr_type)
				{
				case UDI_ATTR_STRING:
					// Get the string from the index.
					if (floodplainn.zudiIndexes[0]
						.getString(
							devlineData
								.attr_valueOff,
							attrValueTmp)
						!= ERROR_SUCCESS)
					{ break; };

					if (!strcmp8(
						CC device->enumeration[k]
							->attr_value,
						attrValueTmp))
					{
						attrMatched = 1;
					};

					break;
				case UDI_ATTR_UBIT32:
					if (UDI_ATTR32_GET(device->enumeration[k]->attr_value)
						== UDI_ATTR32_GET((ubit8 *)&devlineData.attr_valueOff))
					{
						attrMatched = 1;
					};

					break;
				case UDI_ATTR_BOOLEAN:
					if (device->enumeration[k]->attr_value[0]
						== *(ubit8 *)&devlineData
							.attr_valueOff)
					{
						attrMatched = 1;
					};

					break;
				case UDI_ATTR_ARRAY8:
					if (device->enumeration[k]->attr_length
						!= devlineData.attr_length)
					{
						break;
					};

					if (floodplainn.zudiIndexes[0]
						.getArray(devlineData
							.attr_valueOff,
						devlineData.attr_length,
						(ubit8 *)attrValueTmp)
						!= ERROR_SUCCESS)
					{
						break;
					};

					if (!memcmp(
						device->enumeration[k]
							->attr_value,
						attrValueTmp,
						devlineData.attr_length))
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

		/* If all the attributes in this rankline matched, and this
		 * rankline's rank value is higher than the previous ranking,
		 * set this to be the new rank.
		 **/
		if (nAttributesMatched == currRank.nAttributes) {
			if (ret < currRank.rank) { ret = currRank.rank; };
		};
	};

	return ret;
}


#define GET_NEXT_DEVICE_HEADER_OFFSET(__dh)			\
	(zudi::device::headerS *)((uintptr_t)(&__dh[1]) \
		+ ((__dh)->nAttributes * sizeof(zudiIndex_deviceDataS)))

static void fplainnIndexer_detectDriverReq(messageStreamC::iteratorS *gmsg)
{
	threadC					*self;
	error_t					err;
	floodplainnC::driverIndexMsgS		*request, *response;
	fplainn::deviceC			*dev;
	zudi::device::headerS			devlineHeader,
						*currDevlineHeader;
	utf8Char				*devlineName;
	zudi::driver::headerS			*driverHeader;
	// FIXME: big memory leak on this list within this function.
	ptrListC<zudi::device::headerS>		matchingDevices;
	status_t				bestRank=-1;

	/** FIXME: Memory leaks all over this function.
	 **/

	self = static_cast<threadC *>(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask() );

	request = (floodplainnC::driverIndexMsgS *)gmsg;
	response = new floodplainnC::driverIndexMsgS(*request);

	if (response == NULL)
	{
		printf(FATAL FPLAINNIDX"detectDriver: No heap mem for response "
			"message.\n");

		// This is actually panic() worthy.
		return;
	};

	response->header.subsystem = MSGSTREAM_SUBSYSTEM_FLOODPLAINN;

	devlineName = new utf8Char[ZUDI_MESSAGE_MAXLEN];
	driverHeader = new zudi::driver::headerS;
	if (devlineName == NULL || driverHeader == NULL)
	{
		printf(WARNING FPLAINNIDX"detectDriver: no heap memory.\n");
		response->header.error = ERROR_MEMORY_NOMEM;
		messageStreamC::enqueueOnThread(
			response->header.targetId, &response->header);

		return;
	};

	printf(NOTICE FPLAINNIDX"detectDriver: devname %s. Src 0x%x, target 0x%x."
		"\n", request->deviceName,
		request->header.sourceId, request->header.targetId);

	err = floodplainn.getDevice(request->deviceName, &dev);
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINNIDX"detectDriver: invalid device %s.\n",
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
	
	for (uarch_t i=0;
		floodplainn.zudiIndexes[0].indexedGetDevice(i, &devlineHeader)
			== ERROR_SUCCESS;
		i++)
	{
		zudi::driver::headerS			metaHeader;
		zudi::driver::metalanguageS		devlineMetalanguage;
		status_t				rank;
		utf8Char				devlineMetaName[
			ZUDI_DRIVER_METALANGUAGE_MAXLEN];


		if (floodplainn.zudiIndexes[0].getDriverHeader(
			devlineHeader.driverId, driverHeader) != ERROR_SUCCESS)
		{
			printf(FATAL FPLAINNIDX"detectDriver: index DEVICE has "
				"invalid driverId %d. Skipping.\n",
				devlineHeader.driverId);

			continue;
		};

		if (floodplainn.zudiIndexes[0].getMessageString(
			driverHeader, devlineHeader.messageIndex, devlineName)
			!= ERROR_SUCCESS)
		{
			printf(FATAL FPLAINNIDX"detectDriver: index DEVICE has "
				"invalid device name msg_idx %d.\n",
				devlineHeader.messageIndex);

			continue;
		};

		// Now get the metalanguage for this device props line.
		if (floodplainn.zudiIndexes[0].getMetalanguage(
			driverHeader, devlineHeader.metaIndex,
			&devlineMetalanguage) != ERROR_SUCCESS)
		{
			printf(FATAL FPLAINNIDX"detectDriver: index DEVICE has "
				"invalid meta_idx %d. Skipping.\n",
				devlineHeader.metaIndex);

			continue;
		};

		if (floodplainn.zudiIndexes[0].getString(
			devlineMetalanguage.nameOff, devlineMetaName)
			!= ERROR_SUCCESS)
		{
			printf(FATAL FPLAINNIDX"detectDriver: meta line's name "
				"offset failed to retrieve a string.\n");

			continue;
		};

		if (floodplainn.zudiIndexes[0].findMetalanguage(
			devlineMetaName, &metaHeader) != ERROR_SUCCESS)
		{
			printf(FATAL FPLAINNIDX"detectDriver: DEVICE line refers "
				"to inexistent metalanguage %s.\n",
				devlineMetaName);

			continue;
		};

		/* Now compare the device line attributes to the actual device's
		 * attributes.
		 **/
		rank = fplainnIndexer_detectDriver_compareEnumerationAttributes(
			dev, &devlineHeader, &metaHeader);

		// If the driver was a match:
		if (rank >= 0)
		{
			// If the match outranks the current best match:
			if (rank > bestRank)
			{
				void		*handle;

				// Clear the list and restart.
				// FIXME: memory leak here.
				handle = NULL;
				while ((currDevlineHeader = matchingDevices
					.getNextItem(&handle)))
				{
					delete currDevlineHeader;
				};

				matchingDevices.~ptrListC();
				new (&matchingDevices)
					ptrListC<zudi::device::headerS>;

				err = matchingDevices.initialize();
				if (err != ERROR_SUCCESS)
				{
					printf(ERROR FPLAINNIDX"detectDriver: "
						"Failed to initialize "
						"matchingDevices list.\n");

					continue;
				};

				// New highest rank found.
				bestRank = rank;
			};

			currDevlineHeader = new zudi::device::headerS;
			if (currDevlineHeader == NULL)
			{
				printf(ERROR FPLAINNIDX"detectDriver: failed to "
					"alloc mem for matching device. Not "
					"inserted into list.\n");

				continue;
			};

			*currDevlineHeader = devlineHeader;
			err = matchingDevices.insert(currDevlineHeader);
			if (err != ERROR_SUCCESS)
			{
				printf(ERROR FPLAINNIDX"detectDriver: Failed to "
					"insert device line into "
					"matchingDevices.\n");

				continue;
			};

			printf(NOTICE FPLAINNIDX"detectDriver: Devnode \"%s\" "
				"ranks %d, matching device\n\t\"%s\"\n",
				request->deviceName, rank,
				devlineName);
		};
	};

	if (matchingDevices.getNItems() == 0)
	{
		/* If a device previously had a driver detected for it, and
		 * detectDriver() is called on it a second time, but this time
		 * no driver is found, the previously detected driver is not
		 * left behind.
		 *
		 * This enables detectDriver() to be used as a sort of "flush"
		 * operation when the driver index is updated to remove an
		 * undesired driver, should the user so choose to do so.
		 **/
		response->header.error = ERROR_NO_MATCH;
		dev->driverFullName[0] = '\0';
		dev->driverDetected = 0;
		dev->isKernelDriver = 0;
	}
	else
	{
		void		*handle=NULL;
		uarch_t		basePathLen;

		/* If more than one driver matches, warn the user that we are
		 * about to make an arbitrary decision.
		 **/
		if (matchingDevices.getNItems() > 1)
		{
			printf(WARNING FPLAINNIDX"detectDriver: %s: Multiple "
				"viable matches. Arbitratily choosing first.\n",
				request->deviceName);
		};

		// Shouldn't need to error check this call.
		currDevlineHeader = matchingDevices.getNextItem(&handle);

		response->header.error = floodplainn.zudiIndexes[0]
			.getDriverHeader(
				currDevlineHeader->driverId, driverHeader);

		if (response->header.error != ERROR_SUCCESS)
		{
			printf(ERROR FPLAINNIDX"detectDriver: getDriverHeader "
				"failed for the chosen driver.\n");

			messageStreamC::enqueueOnThread(
				response->header.targetId, &response->header);

			return;
		};

		response->header.error = ERROR_SUCCESS;

		strcpy8(dev->driverFullName, CC driverHeader->basePath);
		basePathLen = strlen8(CC driverHeader->basePath);
		// Ensure there's a '/' between the basepath and the shortname.
		if (basePathLen > 0
			&& dev->driverFullName[basePathLen - 1] != '/')
		{
			strcpy8(&dev->driverFullName[basePathLen], CC"/");
			basePathLen++;
		};

		strcpy8(
			&dev->driverFullName[basePathLen],
			CC driverHeader->shortName);

		dev->driverDetected = 1;
		dev->isKernelDriver = 1;
	};

	messageStreamC::enqueueOnThread(
		response->header.targetId, &response->header);
}

static void fplainnIndexer_newDeviceActionReq(messageStreamC::iteratorS *gmsg)
{
	floodplainnC::newDeviceActionMsgS	*request;
	floodplainnC::newDeviceActionMsgS	*response;

	request = (floodplainnC::newDeviceActionMsgS *)gmsg;

	response = new floodplainnC::newDeviceActionMsgS(*request);
	if (response == NULL)
	{
		printf(ERROR FPLAINNIDX"newDeviceActionReq: "
			"unable to allocate response message.\n"
			"\tSender thread 0x%x may be frozen indefinitely.\n",
			request->header.sourceId);

		return;
	};

	response->header.subsystem = MSGSTREAM_SUBSYSTEM_FLOODPLAINN;
	response->action = ::newDeviceAction;

	if (request->header.function
		== MSGSTREAM_FPLAINN_SET_NEWDEVICE_ACTION_REQ)
	{
		::newDeviceAction = request->action;
	};

	messageStreamC::enqueueOnThread(
		response->header.targetId, &response->header);
}

#if 0
static void flplainnIndexer_loadDriverReq(messageStreamC::iteratorS *gmsg)
{
	floodplainnC::driverIndexMsgS		*request, *response;
	void					*handle;
	fplainn::driverC			*driverTmp, *driver;
	fplainn::deviceC			*device;
	error_t					err;

printf(NOTICE"Just entered loadDriverReq handler.\n");
	request = (floodplainnC::driverIndexMsgS *)gmsg;

	response = new floodplainnC::driverIndexMsgS(
		request->deviceName, floodplainnC::INDEX_KERNEL);

	if (response == NULL)
	{
		printf(ERROR FPLAINNIDX"loadDriverReq: "
			"unable to allocate response message.\n"
			"\tSender thread 0x%x may be frozen indefinitely.\n",
			request->header.sourceId);

		return;
	};

	response->header = request->header;
	response->header.subsystem = MSGSTREAM_SUBSYSTEM_FLOODPLAINN;

	/**	EXPLANATION:
	 * First check to see if the driver has already been loaded; if not,
	 * load it. We use a locked iteration through the list, but frankly that
	 * may not be necessary. That said, it's only a spinlock acquire and
	 * release, so not really an issue.
	 **/
	err = floodplainn.getDevice(request->deviceName, &device);
	if (err != ERROR_SUCCESS) {
		response->header.error = ERROR_SUCCESS; goto sendResponse;
	};

printf(NOTICE"Got device on %s.\n", request->deviceName);
	handle = NULL;
	driver = NULL;
	driverTmp = floodplainn.driverList.getNextItem(&handle);
	for (; driverTmp != NULL;
		driverTmp = floodplainn.driverList.getNextItem(&handle))
	{
		if (strcmp8(driverTmp->fullName, device->driverFullName) == 0)
		{
printf(NOTICE"Found a match.\n");
			driver = driverTmp;
			break;
		};
	};

	/* If the driver was found in the list of already-loaded drivers, just
	 * exit successfully early.
	 **/
	if (driver != NULL)
		{ response->header.error = ERROR_SUCCESS; goto sendResponse; };

printf(NOTICE"Driver not found in loaded list.\n");
	// Else, driver wasn't loaded already. Proceed.
	driver = new fplainn::driverC();
	if (driver == NULL) {
		response->header.error = ERROR_MEMORY_NOMEM; goto sendResponse;
	};

	// Fill out the driver information object.
	zudi::driver::headerS		*driverHeader;
	void		*driverData;

	// Find the driver in the index using its basepath+shortname.
	driverHeader = zudiIndex::findDriver(device->driverFullName);
	if (driverHeader == NULL)
	{
		response->header.error = ERROR_INVALID_RESOURCE_NAME;
		goto sendResponse;
	};

	driverData = zudiIndex::getDriverDataFor(driverHeader->id);
	if (driverData == NULL) {
		response->header.error = ERROR_UNKNOWN; goto sendResponse;
	};

printf(NOTICE"Found driver header @ 0x%p.\n", driverHeader);

	// Copy the modules information:
	zudi::driver::moduleS		*currModule;

	currModule = zudiIndex::driver::getFirstModule(driverHeader);
	for (uarch_t i=0; i<driverHeader->nModules; i++)
	{
		err = driver->addModule(
			currModule->index, currModule->filename);

		if (err != ERROR_SUCCESS)
		{
			response->header.error = err;
			goto sendResponse;
		};
	};

	// Now copy all the region information:

	response->header.error = ERROR_SUCCESS;

sendResponse:
	messageStreamC::enqueueOnThread(
		response->header.targetId, &response->header);
}
#endif

static void fplainnIndexer_newDeviceInd(messageStreamC::iteratorS *gmsg)
{
	// The originContext /is/ the response that will eventually be sent.
	floodplainnC::newDeviceMsgS		*request, *originContext;
	error_t					err;

	/**	EXPLANATION:
	 * Based on the current new-device-action setting, we will do a series
	 * of things:
	 *
	 * If newdevice action isn't "do nothing", we will try to detect a
	 * driver for the device. Then, if newdevice-action isn't "stop after
	 * driver detection", we will try to load the driver. Then, if
	 * newdevice action isn't "stop after loading the driver", we will
	 * proceed to instantiate the device.
	 *
	 * This request requires the indexer thread to make syscalls on behalf
	 * of the caller, so we have to copy the caller's request message before
	 * making any extra calls.
	 **/
	request = (floodplainnC::newDeviceMsgS *)gmsg;

	originContext = new floodplainnC::newDeviceMsgS(*request);
	if (originContext == NULL)
	{
		printf(ERROR FPLAINNIDX"newDeviceInd: "
			"unable to allocate response, or origin context.\n"
			"\tSender thread 0x%x may be frozen indefinitely.\n",
			request->header.sourceId);

		return;
	};

	// Set the last completed action to default of "nothing".
	originContext->lastCompletedAction = floodplainnC::NDACTION_NOTHING;
	originContext->header.subsystem = MSGSTREAM_SUBSYSTEM_FLOODPLAINN;

	// If the currently set action is "do nothing", just exit immediately.
	if (::newDeviceAction == floodplainnC::NDACTION_NOTHING) {
		originContext->header.error = ERROR_SUCCESS; goto sendResponse;
	};
	
	err = floodplainn.detectDriverReq(
		originContext->deviceName, floodplainnC::INDEX_KERNEL,
		0, originContext, 0);

	if (err != ERROR_SUCCESS) {
		originContext->header.error = err; goto sendResponse;
	};

	return;

sendResponse:
	messageStreamC::enqueueOnThread(
		originContext->header.targetId, &originContext->header);
}

static void fplainnIndexer_newDeviceInd1(messageStreamC::iteratorS *gmsg)
{
	floodplainnC::driverIndexMsgS		*response;
	floodplainnC::newDeviceMsgS		*originContext;
//	error_t					err;

	response = (floodplainnC::driverIndexMsgS *)gmsg;
	originContext = (floodplainnC::newDeviceMsgS *)gmsg->header.privateData;

	if (response->header.error != ERROR_SUCCESS)
	{
		originContext->header.error = response->header.error;
		goto sendResponse;
	};

	originContext->lastCompletedAction =
		floodplainnC::NDACTION_DETECT_DRIVER;
	// If newDeviceAction is detect-and-stop, stop and send response now.
	if (::newDeviceAction == floodplainnC::NDACTION_DETECT_DRIVER) {
		response->header.error = ERROR_SUCCESS; goto sendResponse;
	};

#if 0
	// Else we continue. Next we do a loadDriver() call.
	err = floodplainn.loadDriverReq(originContext->deviceName, originContext);
	if (err != ERROR_SUCCESS) {
		originContext->header.error = err; goto sendResponse;
	};
printf(NOTICE"here.\n");

	return;
#endif

sendResponse:
	messageStreamC::enqueueOnThread(
		originContext->header.targetId, &originContext->header);
}

static void fplainnIndexer_newDeviceInd2(messageStreamC::iteratorS *gmsg)
{
	floodplainnC::driverIndexMsgS	*response;
	floodplainnC::newDeviceMsgS	*originContext;

	response = (floodplainnC::driverIndexMsgS *)gmsg;
	originContext = (floodplainnC::newDeviceMsgS *)gmsg->header.privateData;

	originContext->lastCompletedAction = floodplainnC::NDACTION_LOAD_DRIVER;
	messageStreamC::enqueueOnThread(
		originContext->header.targetId, &originContext->header);
}

static void handleUnknownRequest(messageStreamC::headerS *request)
{
	// Just send a response with an error value.
	messageStreamC::headerS	*response;

	response = new messageStreamC::headerS(*request);
	if (response == NULL)
	{
		printf(ERROR FPLAINNIDX"HandleUnknownRequest: "
			"unable to allocate response message.\n"
			"\tSender thread 0x%x may be frozen indefinitely.\n",
			request->sourceId);

		return;
	};

	response->subsystem = MSGSTREAM_SUBSYSTEM_FLOODPLAINN;
	response->error = ERROR_UNKNOWN;
	messageStreamC::enqueueOnThread(response->targetId, response);
}

void floodplainnC::driverIndexerEntry(void)
{
	threadC				*self;
	messageStreamC::iteratorS	gcb;
	zudi::headerS			__kindexHeader;

	self = static_cast<threadC *>(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask() );

	floodplainn.indexerThreadId = self->getFullId();
	floodplainn.indexerQueueId = MSGSTREAM_SUBSYSTEM_REQ0;

	printf(NOTICE FPLAINNIDX"Floodplainn-indexer executing;\n"
		"\tprocess ID: 0x%x. ESP: 0x%p. RequestQ ID: %d. Dormanting.\n",
		self->getFullId(), getEsp(),
		floodplainn.indexerQueueId);

	floodplainn.zudiIndexes[0].getHeader(&__kindexHeader);
	printf(NOTICE FPLAINNIDX"Index: Version %d.%d. (%s), holds %d drivers.\n",
		__kindexHeader.majorVersion, __kindexHeader.minorVersion,
		__kindexHeader.endianness, __kindexHeader.nRecords);

	for (;;)
	{
		self->getTaskContext()->messageStream.pull(&gcb);

		// If some thread is forwarding syscall responses to us:
		if (gcb.header.subsystem != MSGSTREAM_SUBSYSTEM_REQ0
			&& gcb.header.sourceId != self->getFullId())
		{
			printf(ERROR FPLAINNIDX"Syscall response message "
				"forwarded to this thread by thread 0x%x.\n"
				"\tPossible exploit attempt. Rejecting.\n",
				gcb.header.sourceId);

			// Reject the message.
			continue;
		};

		switch (gcb.header.subsystem)
		{
		case MSGSTREAM_SUBSYSTEM_REQ0:
			switch (gcb.header.function)
			{
			case MSGSTREAM_FPLAINN_DETECTDRIVER_REQ:
				fplainnIndexer_detectDriverReq(&gcb);
				break;

			case MSGSTREAM_FPLAINN_LOADDRIVER_REQ:
//				flplainnIndexer_loadDriverReq(&gcb);
				printf(WARNING FPLAINNIDX"Main loop: loadDriver request found.\n");
				break;

			case MSGSTREAM_FPLAINN_GET_NEWDEVICE_ACTION_REQ:
			case MSGSTREAM_FPLAINN_SET_NEWDEVICE_ACTION_REQ:
				fplainnIndexer_newDeviceActionReq(&gcb);
				break;

			case MSGSTREAM_FPLAINN_NEWDEVICE_IND:
				fplainnIndexer_newDeviceInd(&gcb);
				break;

			default:
				printf(ERROR FPLAINNIDX"Unknown request %d.\n",
					gcb.header.function);

				handleUnknownRequest(&gcb.header);
				break;
			};

			break;

		case MSGSTREAM_SUBSYSTEM_FLOODPLAINN:
			switch (gcb.header.function)
			{
			case MSGSTREAM_FPLAINN_DETECTDRIVER_REQ:
				fplainnIndexer_newDeviceInd1(&gcb);
				break;

			case MSGSTREAM_FPLAINN_LOADDRIVER_REQ:
				fplainnIndexer_newDeviceInd2(&gcb);
				break;
			};
			break;
		default:
			printf(NOTICE FPLAINNIDX"Unknown message.\n");
		};
	};
}

