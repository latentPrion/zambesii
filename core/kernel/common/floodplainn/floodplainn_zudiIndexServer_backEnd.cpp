
#include <__ksymbols.h>
#include <zudiIndex.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/ptrList.h>
#include <commonlibs/libzudiIndexParser.h>
#include <kernel/common/zudiIndexServer.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>


/**	EXPLANATION:
 * The ZUDI Index reader service reads and manages the ZUDI indexes in the
 * kernel image, the kernel ramdisk and off the external disk into RAM and
 * provides driver detection and support services.
 *
 * Is an implementation of the ZUDI Reader API. The current revision of the API
 * can be found in /core/kernel/common/zudiReader.h.
 **/


static zudiIndexServer::newDeviceActionE		newDeviceAction;

zudiIndexParserC		kernelIndex(zudiIndexParserC::SOURCE_KERNEL),
				ramdiskIndex(zudiIndexParserC::SOURCE_RAMDISK),
				externalIndex(zudiIndexParserC::SOURCE_EXTERNAL);

zudiIndexParserC		*zudiIndexes[3] =
{
	&kernelIndex,
	&ramdiskIndex,
	&externalIndex
};

static void *getEsp(void)
{
	void		*esp;

	asm volatile (
		"movl	%%esp, %0\n\t"
		: "=r" (esp));

	return esp;
}

static status_t fplainnIndexServer_detectDriver_compareEnumerationAttributes(
	fplainn::deviceC *device, zudi::device::headerS *devline,
	zudi::driver::headerS *metaHdr
	)
{
	// Return value is the rank of the driver.
	status_t			ret=ERROR_NO_MATCH;
	zudi::rank::headerS		currRank;
	heapPtrC<utf8Char>		attrValueTmp;

	attrValueTmp.useArrayDelete = 1;
	attrValueTmp = new ubit8[UDI_MAX_ATTR_SIZE];
	if (attrValueTmp == NULL) { return ERROR_MEMORY_NOMEM; };

	// Run through all the rank lines for the relevant metalanguage.
	for (uarch_t i=0;
		i<metaHdr->nRanks
		&& zudiIndexes[0]->indexedGetRank(
			metaHdr, i, &currRank) == ERROR_SUCCESS;
		i++)
	{
		ubit16		nAttributesMatched;

		if (currRank.driverId != metaHdr->id)
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

			if (zudiIndexes[0]->indexedGetRankAttrString(
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
			if (zudiIndexes[0]->findDeviceData(
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
					if (zudiIndexes[0]->getString(
						devlineData.attr_valueOff,
						attrValueTmp.get())
						!= ERROR_SUCCESS)
					{ break; };

					if (!strcmp8(
						CC device->enumeration[k]
							->attr_value,
						attrValueTmp.get()))
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

					if (zudiIndexes[0]->getArray(
						devlineData.attr_valueOff,
						devlineData.attr_length,
						(ubit8 *)attrValueTmp.get())
						!= ERROR_SUCCESS)
					{
						break;
					};

					if (!memcmp(
						device->enumeration[k]
							->attr_value,
						attrValueTmp.get(),
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

static void fplainnIndexServer_detectDriverReq(
	zasyncStreamC::zasyncMsgS *gmsg,
	zudiIndexServer::zudiIndexMsgS *requestData
	)
{
	threadC					*self;
	error_t					err;
	zasyncStreamC::zasyncMsgS		*request;
	floodplainnC::zudiIndexMsgS		*response;
	fplainn::deviceC			*dev;
	zudi::device::headerS			devlineHdr,
						*currDevlineHdr;
	heapPtrC<utf8Char>			devlineName;
	heapPtrC<zudi::headerS>			indexHdr;
	heapPtrC<zudi::driver::headerS>		driverHdr, metaHdr;
	// FIXME: big memory leak on this list within this function.
	ptrListC<zudi::device::headerS>		matchingDevices;
	status_t				bestRank=-1;
	asyncResponseC				myResponse;

	/** FIXME: Memory leaks all over this function.
	 **/

	self = static_cast<threadC *>(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask() );

	request = (zasyncStreamC::zasyncMsgS *)gmsg;
	response = new floodplainnC::zudiIndexMsgS(
		request->header.sourceId,
		MSGSTREAM_SUBSYSTEM_FLOODPLAINN, ZUDIIDX_SERVER_DETECTDRIVER_REQ,
		sizeof(*response),
		request->header.flags, request->header.privateData);

	if (response == NULL)
	{
		printf(FATAL FPLAINNIDX"detectDriver: No heap mem for response "
			"message.\n");

		// This is actually panic() worthy.
		return;
	};

	myResponse(response);
	// Allocate all necessary memory at once.
	devlineName = new utf8Char[ZUDI_MESSAGE_MAXLEN];
	devlineName.useArrayDelete = 1;
	indexHdr = new zudi::headerS;
	driverHdr = new zudi::driver::headerS;
	metaHdr = new zudi::driver::headerS;

	if (devlineName == NULL || indexHdr == NULL || driverHdr == NULL
		|| metaHdr == NULL)
	{
		printf(WARNING FPLAINNIDX"detectDriver: no heap memory.\n");
		myResponse(ERROR_MEMORY_NOMEM);
		return;
	};

	printf(NOTICE FPLAINNIDX"detectDriver: devname %s. Src 0x%x, target 0x%x."
		"\n", requestData->path,
		request->header.sourceId, request->header.targetId);

	err = floodplainn.getDevice(requestData->path, &dev);
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINNIDX"detectDriver: invalid device %s.\n",
			requestData->path);

		myResponse(ERROR_INVALID_RESOURCE_NAME);
		return;
	};

	// Get the index header for the current selected index.
	err = zudiIndexes[0]->getHeader(indexHdr.get());
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINNIDX"detectDriver: Failed to get index "
			"header.\n");

		myResponse(err); return;
	};

	/**	EXPLANATION:
	 * Search through the device index. For each device record, use its
	 * metalanguage-index to find the corresponding metalanguage and its
	 * "rank" declarations. Then use the rank declarations to determine
	 * whether or not the device record matches the device we have been
	 * asked to locate a driver for.
	 **/
	for (uarch_t i=0;
		zudiIndexes[0]->indexedGetDevice(i, &devlineHdr)
			== ERROR_SUCCESS;
		i++)
	{
		zudi::driver::metalanguageS		devlineMetalanguage;
		status_t				rank;
		utf8Char				devlineMetaName[
			ZUDI_DRIVER_METALANGUAGE_MAXLEN];

		if (zudiIndexes[0]->getDriverHeader(
			indexHdr.get(), devlineHdr.driverId, driverHdr.get())
			!= ERROR_SUCCESS)
		{
			printf(FATAL FPLAINNIDX"detectDriver: index DEVICE has "
				"invalid driverId %d. Skipping.\n",
				devlineHdr.driverId);

			continue;
		};

		if (zudiIndexes[0]->getMessageString(
			driverHdr.get(), devlineHdr.messageIndex,
			devlineName.get()) != ERROR_SUCCESS)
		{
			printf(FATAL FPLAINNIDX"detectDriver: index DEVICE has "
				"invalid device name msg_idx %d.\n",
				devlineHdr.messageIndex);

			continue;
		};
printf(NOTICE"DEVICE: (driver %d '%s'), Device name: %s.\n\t%d %d %d attrs.\n",
	devlineHdr.driverId, driverHdr->shortName, devlineName.get(),
	devlineHdr.messageIndex, devlineHdr.metaIndex, devlineHdr.nAttributes);

		// Now get the metalanguage for this device props line.
		if (zudiIndexes[0]->getMetalanguage(
			driverHdr.get(), devlineHdr.metaIndex,
			&devlineMetalanguage) != ERROR_SUCCESS)
		{
			printf(FATAL FPLAINNIDX"detectDriver: index DEVICE has "
				"invalid meta_idx %d. Skipping.\n",
				devlineHdr.metaIndex);

			continue;
		};

		if (zudiIndexes[0]->getString(
			devlineMetalanguage.nameOff, devlineMetaName)
			!= ERROR_SUCCESS)
		{
			printf(FATAL FPLAINNIDX"detectDriver: meta line's name "
				"offset failed to retrieve a string.\n");

			continue;
		};

		if (zudiIndexes[0]->findMetalanguage(
			indexHdr.get(), devlineMetaName, metaHdr.get())
			!= ERROR_SUCCESS)
		{
			printf(FATAL FPLAINNIDX"detectDriver: DEVICE line refers "
				"to inexistent metalanguage %s.\n",
				devlineMetaName);

			continue;
		};

		/* Now compare the device line attributes to the actual device's
		 * attributes.
		 **/
		rank = fplainnIndexServer_detectDriver_compareEnumerationAttributes(
			dev, &devlineHdr, metaHdr.get());

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
				while ((currDevlineHdr = matchingDevices
					.getNextItem(&handle)))
				{
					delete currDevlineHdr;
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

			currDevlineHdr = new zudi::device::headerS;
			if (currDevlineHdr == NULL)
			{
				printf(ERROR FPLAINNIDX"detectDriver: failed to "
					"alloc mem for matching device. Not "
					"inserted into list.\n");

				continue;
			};

			*currDevlineHdr = devlineHdr;
			err = matchingDevices.insert(currDevlineHdr);
			if (err != ERROR_SUCCESS)
			{
				printf(ERROR FPLAINNIDX"detectDriver: Failed to "
					"insert device line into "
					"matchingDevices.\n");

				continue;
			};

			printf(NOTICE FPLAINNIDX"detectDriver: Devnode \"%s\" "
				"ranks %d, matching device\n\t\"%s\"\n",
				requestData->path, rank,
				devlineName.get());
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
		myResponse(ERROR_NO_MATCH);
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
				requestData->path);
		};

		// Shouldn't need to error check this call.
		currDevlineHdr = matchingDevices.getNextItem(&handle);

		response->header.error = zudiIndexes[0]->getDriverHeader(
			indexHdr.get(), currDevlineHdr->driverId,
			driverHdr.get());

		if (response->header.error != ERROR_SUCCESS)
		{
			printf(ERROR FPLAINNIDX"detectDriver: getDriverHeader "
				"failed for the chosen driver.\n");

			myResponse(response->header.error);
			return;
		};

		myResponse(ERROR_SUCCESS);

		strcpy8(dev->driverFullName, CC driverHdr->basePath);
		basePathLen = strlen8(CC driverHdr->basePath);
		// Ensure there's a '/' between the basepath and the shortname.
		if (basePathLen > 0
			&& dev->driverFullName[basePathLen - 1] != '/')
		{
			strcpy8(&dev->driverFullName[basePathLen], CC"/");
			basePathLen++;
		};

		strcpy8(
			&dev->driverFullName[basePathLen],
			CC driverHdr->shortName);

		dev->driverDetected = 1;
		dev->isKernelDriver = 1;
	};
}

static void fplainnIndexServer_newDeviceActionReq(
	zasyncStreamC::zasyncMsgS *gmsg,
	zudiIndexServer::zudiIndexMsgS *requestData
	)
{
	zasyncStreamC::zasyncMsgS		*request;
	floodplainnC::zudiIndexMsgS		*response;
	asyncResponseC				myResponse;

	request = (zasyncStreamC::zasyncMsgS *)gmsg;

	response = new floodplainnC::zudiIndexMsgS(
		request->header.sourceId,
		MSGSTREAM_SUBSYSTEM_FLOODPLAINN, 0,
		sizeof(*response),
		request->header.flags, request->header.privateData);

	if (response == NULL)
	{
		printf(ERROR FPLAINNIDX"newDeviceActionReq: "
			"unable to allocate response message.\n"
			"\tSender thread 0x%x may be frozen indefinitely.\n",
			request->header.sourceId);

		return;
	};

	myResponse(response);
	response->header.function = requestData->command;
	response->info.action = ::newDeviceAction;

	if (requestData->command == ZUDIIDX_SERVER_SET_NEWDEVICE_ACTION_REQ)
		{ ::newDeviceAction = requestData->action; };

	myResponse(ERROR_SUCCESS);
}

#if 0
static void flplainnIndexer_loadDriverReq(zasyncStreamC::zasyncMsgS *gmsg)
{
	floodplainnC::zudiIndexMsgS		*request, *response;
	void					*handle;
	fplainn::driverC			*driverTmp, *driver;
	fplainn::deviceC			*device;
	error_t					err;

printf(NOTICE"Just entered loadDriverReq handler.\n");
	request = (floodplainnC::zudiIndexMsgS *)gmsg;

	response = new floodplainnC::zudiIndexMsgS(
		requestData->path, floodplainnC::INDEX_KERNEL);

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
	err = floodplainn.getDevice(requestData->path, &device);
	if (err != ERROR_SUCCESS) {
		response->header.error = ERROR_SUCCESS; goto sendResponse;
	};

printf(NOTICE"Got device on %s.\n", requestData->path);
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
	zudi::driver::headerS		*driverHdr;
	void		*driverData;

	// Find the driver in the index using its basepath+shortname.
	driverHdr = zudiIndex::findDriver(device->driverFullName);
	if (driverHdr == NULL)
	{
		response->header.error = ERROR_INVALID_RESOURCE_NAME;
		goto sendResponse;
	};

	driverData = zudiIndex::getDriverDataFor(driverHdr->id);
	if (driverData == NULL) {
		response->header.error = ERROR_UNKNOWN; goto sendResponse;
	};

printf(NOTICE"Found driver header @ 0x%p.\n", driverHdr);

	// Copy the modules information:
	zudi::driver::moduleS		*currModule;

	currModule = zudiIndex::driver::getFirstModule(driverHdr);
	for (uarch_t i=0; i<driverHdr->nModules; i++)
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

static void fplainnIndexServer_newDeviceInd(
	zasyncStreamC::zasyncMsgS *gmsg,
	zudiIndexServer::zudiIndexMsgS *requestData
	)
{
	// The originContext /is/ the response that will eventually be sent.
	zasyncStreamC::zasyncMsgS		*request;
	floodplainnC::zudiIndexMsgS		*originContext;
	error_t					err;
	asyncResponseC				myResponse;

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
	request = (zasyncStreamC::zasyncMsgS *)gmsg;

	originContext = new floodplainnC::zudiIndexMsgS(
		request->header.sourceId,
		MSGSTREAM_SUBSYSTEM_FLOODPLAINN,
		ZUDIIDX_SERVER_NEWDEVICE_IND,
		sizeof(*originContext),
		request->header.flags, request->header.privateData);

	if (originContext == NULL)
	{
		printf(ERROR FPLAINNIDX"newDeviceInd: "
			"unable to allocate response, or origin context.\n"
			"\tSender thread 0x%x may be frozen indefinitely.\n",
			request->header.sourceId);

		return;
	};

	myResponse(originContext);
	originContext->set(
		requestData->command, requestData->path,
		requestData->index, zudiIndexServer::NDACTION_NOTHING);

	// If the currently set action is "do nothing", just exit immediately.
	if (::newDeviceAction == zudiIndexServer::NDACTION_NOTHING) {
		myResponse(ERROR_SUCCESS); return;
	};

	err = zudiIndexServer::detectDriverReq(
		originContext->info.path, originContext->info.index,
		originContext, 0);

	if (err != ERROR_SUCCESS) { myResponse(err); return; };
	myResponse(DONT_SEND_RESPONSE);
}

static void fplainnIndexServer_newDeviceInd1(messageStreamC::iteratorS *gmsg)
{
	floodplainnC::zudiIndexMsgS		*response, *originContext;
	asyncResponseC				myResponse;
//	error_t					err;

	/**	EXPLANATION:
	 * The "request" we're dealing with is actually a response message from
	 * a syscall we ourselves performed; we name it accordingly ("response")
	 * for aided readability.
	 **/
	response = (floodplainnC::zudiIndexMsgS *)gmsg;
	originContext = (floodplainnC::zudiIndexMsgS *)gmsg->header.privateData;
	myResponse(originContext);

	if (response->header.error != ERROR_SUCCESS)
	{
		myResponse(response->header.error);
		return;
	};

	originContext->info.action = zudiIndexServer::NDACTION_DETECT_DRIVER;
	// If newDeviceAction is detect-and-stop, stop and send response now.
	if (::newDeviceAction == zudiIndexServer::NDACTION_DETECT_DRIVER)
		{ myResponse(ERROR_SUCCESS); return; };

#if 0
	// Else we continue. Next we do a loadDriver() call.
	err = floodplainn.loadDriverReq(originContext->deviceName, originContext);
	if (err != ERROR_SUCCESS) {
		originContext->header.error = err; goto sendResponse;
	};
printf(NOTICE"here.\n");

	return;
#endif

	myResponse(ERROR_SUCCESS);
}

static void fplainnIndexServer_newDeviceInd2(messageStreamC::iteratorS *gmsg)
{
	floodplainnC::zudiIndexMsgS	*response, *originContext;
	asyncResponseC			myResponse;

	response = (floodplainnC::zudiIndexMsgS *)gmsg;
	originContext = (floodplainnC::zudiIndexMsgS *)gmsg->header.privateData;
	myResponse(originContext);

	originContext->info.action = zudiIndexServer::NDACTION_LOAD_DRIVER;
	myResponse(ERROR_SUCCESS);
}

static void handleUnknownRequest(zasyncStreamC::zasyncMsgS *request)
{
	asyncResponseC		myResponse;

	// Just send a response with an error value.
	messageStreamC::headerS	*response;

	response = new messageStreamC::headerS(request->header);
	if (response == NULL)
	{
		printf(ERROR FPLAINNIDX"HandleUnknownRequest: "
			"unable to allocate response message.\n"
			"\tSender thread 0x%x may be frozen indefinitely.\n",
			request->header.sourceId);

		return;
	};

	response->subsystem = MSGSTREAM_SUBSYSTEM_FLOODPLAINN;
	myResponse(response);
	myResponse(ERROR_UNKNOWN);
}

void fplainnIndexServer_handleRequest(
	zasyncStreamC::zasyncMsgS *msg,
	zudiIndexServer::zudiIndexMsgS *requestData, threadC *self
	)
{
	error_t		err;

	err = self->parent->zasyncStream.receive(
		msg->dataHandle, requestData, 0);

	if (err != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINNIDX"handleRequest: receive() failed "
			"because %s.\n",
			strerror(err));

		return;
	};

	switch (requestData->command)
	{
	case ZUDIIDX_SERVER_DETECTDRIVER_REQ:
		fplainnIndexServer_detectDriverReq(
			(zasyncStreamC::zasyncMsgS *)msg, requestData);
		break;

	case ZUDIIDX_SERVER_LOADDRIVER_REQ:
//		flplainnIndexer_loadDriverReq(
//			(zasyncStreamC::zasyncMsgS *)msg, requestData);

		break;

	case ZUDIIDX_SERVER_GET_NEWDEVICE_ACTION_REQ:
	case ZUDIIDX_SERVER_SET_NEWDEVICE_ACTION_REQ:
		fplainnIndexServer_newDeviceActionReq(
			(zasyncStreamC::zasyncMsgS *)msg, requestData);

		break;

	case ZUDIIDX_SERVER_NEWDEVICE_IND:
		fplainnIndexServer_newDeviceInd(
			(zasyncStreamC::zasyncMsgS *)msg, requestData);

		break;

	default:
		printf(ERROR FPLAINNIDX"Unknown command request %d.\n",
			requestData->command);

		handleUnknownRequest(msg);
		break;
	};
}

void floodplainnC::indexReaderEntry(void)
{
	threadC						*self;
	heapPtrC<messageStreamC::iteratorS>		gcb;
	heapPtrC<zudiIndexServer::zudiIndexMsgS>	requestData;
	zudi::headerS					__kindexHeader;

	self = static_cast<threadC *>(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask() );

	gcb = new messageStreamC::iteratorS;
	requestData = new zudiIndexServer::zudiIndexMsgS(
		0, NULL, zudiIndexServer::INDEX_KERNEL);

	if (zudiIndexes[0]->initialize(CC"[__kindex]") != ERROR_SUCCESS)
		{ panic(ERROR_UNINITIALIZED); };

	if (zudiIndexes[1]->initialize(CC"@h:__kramdisk/drivers32")
		!= ERROR_SUCCESS)
	{ panic(ERROR_UNINITIALIZED); };

	if (zudiIndexes[2]->initialize(CC"@h:__kroot/zambesii/drivers32")
		!= ERROR_SUCCESS)
	{ panic(ERROR_UNINITIALIZED); };

	floodplainn.zudiIndexServerTid = self->getFullId();
	self->parent->zasyncStream.listen(self->getFullId(), 1);

	printf(NOTICE FPLAINNIDX"Floodplainn-indexer executing;\n"
		"\tprocess ID: 0x%x. ESP: 0x%p. Listening; dormanting.\n",
		self->getFullId(), getEsp());

	zudiIndexes[0]->getHeader(&__kindexHeader);
	printf(NOTICE FPLAINNIDX"KERNEL_INDEX: Version %d.%d. (%s), holds %d "
		"drivers.\n",
		__kindexHeader.majorVersion, __kindexHeader.minorVersion,
		__kindexHeader.endianness, __kindexHeader.nRecords);

	for (;;)
	{
		self->getTaskContext()->messageStream.pull(gcb.get());

		// If some thread is forwarding syscall responses to us:
		if ((gcb->header.subsystem != MSGSTREAM_SUBSYSTEM_FLOODPLAINN
			&& gcb->header.subsystem != MSGSTREAM_SUBSYSTEM_ZASYNC)
			&& gcb->header.sourceId != self->getFullId())
		{
			printf(ERROR FPLAINNIDX"Syscall response message "
				"forwarded to this thread.\n\tFrom TID 0x%x. "
				"Possible exploit attempt. Rejecting.\n",
				gcb->header.sourceId);

			// Reject the message.
			continue;
		};

		switch (gcb->header.subsystem)
		{
		case MSGSTREAM_SUBSYSTEM_ZASYNC:
			switch (gcb->header.function)
			{
			case MSGSTREAM_ZASYNC_SEND:
				fplainnIndexServer_handleRequest(
					(zasyncStreamC::zasyncMsgS *)gcb.get(),
					requestData.get(), self);

				break;
			};

			break;

		case MSGSTREAM_SUBSYSTEM_FLOODPLAINN:
			switch (gcb->header.function)
			{
			case ZUDIIDX_SERVER_DETECTDRIVER_REQ:
				fplainnIndexServer_newDeviceInd1(gcb.get());
				break;

			case ZUDIIDX_SERVER_LOADDRIVER_REQ:
				fplainnIndexServer_newDeviceInd2(gcb.get());
				break;
			};
			break;
		default:
			printf(NOTICE FPLAINNIDX"Unknown message.\n");
		};
	};
}

