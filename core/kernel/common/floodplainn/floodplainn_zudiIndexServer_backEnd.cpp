
#include <debug.h>
#include <__ksymbols.h>
#include <zui.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/ptrList.h>
#include <__kclasses/memReservoir.h>
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


zuiServer::newDeviceActionE	newDeviceAction;

zudiIndexParserC		kernelIndex(zudiIndexParserC::SOURCE_KERNEL),
				ramdiskIndex(zudiIndexParserC::SOURCE_RAMDISK),
				externalIndex(zudiIndexParserC::SOURCE_EXTERNAL);

zudiIndexParserC		*zudiIndexes[3] =
{
	&kernelIndex,
	&ramdiskIndex,
	&externalIndex
};

 void *getEsp(void)
{
	void		*esp;

	asm volatile (
		"movl	%%esp, %0\n\t"
		: "=r" (esp));

	return esp;
}

status_t fplainnIndexServer_detectDriver_compareEnumerationAttributes(
	fplainn::deviceC *device, zui::device::headerS *devline,
	zui::driver::headerS *metaHdr
	)
{
	// Return value is the rank of the driver.
	status_t			ret=ERROR_NO_MATCH;
	zui::rank::headerS		currRank;
	heapArrC<utf8Char>		attrValueTmp;

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
			zui::device::attrDataS		devlineData;
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
			for (uarch_t k=0; k<device->nEnumerationAttrs; k++)
			{
				if (strcmp8(
					CC device->enumerationAttrs[k]->attr_name,
					currRankAttrName) != 0)
				{
					continue;
				};

				/* Even if they have the same name, if the types
				 * differ, we can't consider it a match.
				 **/
				if (device->enumerationAttrs[k]->attr_type
					!= devlineData.attr_type)
				{
					break;
				};

				switch (device->enumerationAttrs[k]->attr_type)
				{
				case UDI_ATTR_STRING:
					// Get the string from the index.
					if (zudiIndexes[0]->getString(
						devlineData.attr_valueOff,
						attrValueTmp.get())
						!= ERROR_SUCCESS)
					{ break; };

					if (!strcmp8(
						CC device->enumerationAttrs[k]
							->attr_value,
						attrValueTmp.get()))
					{
						attrMatched = 1;
					};

					break;
				case UDI_ATTR_UBIT32:
					if (UDI_ATTR32_GET(device->enumerationAttrs[k]->attr_value)
						== UDI_ATTR32_GET((ubit8 *)&devlineData.attr_valueOff))
					{
						attrMatched = 1;
					};

					break;
				case UDI_ATTR_BOOLEAN:
					if (device->enumerationAttrs[k]->attr_value[0]
						== *(ubit8 *)&devlineData
							.attr_valueOff)
					{
						attrMatched = 1;
					};

					break;
				case UDI_ATTR_ARRAY8:
					if (device->enumerationAttrs[k]->attr_length
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
						device->enumerationAttrs[k]
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

void fplainnIndexServer_detectDriverReq(
	zasyncStreamC::zasyncMsgS *request,
	zuiServer::indexMsgS *requestData
	)
{
	error_t					err;
	floodplainnC::zudiIndexMsgS		*response;
	fplainn::deviceC			*dev;
	zui::device::headerS			devlineHdr,
						*currDevlineHdr;
	heapArrC<utf8Char>			devlineName;
	heapObjC<zui::headerS>			indexHdr;
	heapObjC<zui::driver::headerS>		driverHdr, metaHdr;
	// FIXME: big memory leak on this list within this function.
	ptrListC<zui::device::headerS>		matchingDevices;
	status_t				bestRank=-1;
	asyncResponseC				myResponse;
	void					*handle;

	/** FIXME: Memory leaks all over this function.
	 **/
	response = new floodplainnC::zudiIndexMsgS(
		request->header.sourceId,
		MSGSTREAM_SUBSYSTEM_FLOODPLAINN, requestData->command,
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
	devlineName = new utf8Char[DRIVER_LONGNAME_MAXLEN];
	indexHdr = new zui::headerS;
	driverHdr = new zui::driver::headerS;
	metaHdr = new zui::driver::headerS;

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

	err = matchingDevices.initialize();
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINNIDX"detectDriver: Failed to initialize "
			"matchingDevices list.\n");

		myResponse(err);
		return;
	};

	// Set the requested index
	dev->requestedIndex = requestData->index;

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
		i < indexHdr->nSupportedDevices
		&& zudiIndexes[0]->indexedGetDevice(i, &devlineHdr)
			== ERROR_SUCCESS;
		i++)
	{
		zui::driver::metalanguageS		devlineMetalanguage;
		status_t				rank;
		utf8Char				devlineMetaName[
			DRIVER_METALANGUAGE_MAXLEN];

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
					ptrListC<zui::device::headerS>;

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

			currDevlineHdr = new zui::device::headerS;
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
		dev->driverIndex = zuiServer::INDEX_KERNEL;
	}
	else
	{
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
		handle = NULL;
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

		// If this next readString fails, it's fine, kind of.
		zudiIndexes[0]->getMessageString(
			driverHdr.get(), driverHdr->nameIndex,
			dev->longName);

		dev->driverDetected = 1;
		dev->driverIndex = zuiServer::INDEX_KERNEL;
	};

	// In both cases, driverInstance should be NULL.
	dev->driverInstance = NULL;

	// Free the list.
	handle = NULL;
	while ((currDevlineHdr = matchingDevices.getNextItem(&handle)))
		{ delete currDevlineHdr; };
}

void fplainnIndexServer_newDeviceActionReq
(
	zasyncStreamC::zasyncMsgS *request,
	zuiServer::indexMsgS *requestData
	)
{
	floodplainnC::zudiIndexMsgS		*response;
	asyncResponseC				myResponse;

	response = new floodplainnC::zudiIndexMsgS(
		request->header.sourceId,
		MSGSTREAM_SUBSYSTEM_FLOODPLAINN, requestData->command,
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
	response->info.action = ::newDeviceAction;

	if (requestData->command == ZUISERVER_SET_NEWDEVICE_ACTION_REQ)
		{ ::newDeviceAction = requestData->action; };

	myResponse(ERROR_SUCCESS);
}

 udi_mei_init_t *__kindex_getMetaInfoFor(
	utf8Char *metaName, zui::headerS *indexHdr
	)
{
	heapObjC<zui::driver::headerS>	metaHdr;
	zui::driver::provisionS		provTmp;
	ubit8				indexNo=(int)zuiServer::INDEX_KERNEL;

	metaHdr = new zui::driver::headerS;
	if (metaHdr == NULL) { return NULL; };

	for (uarch_t i=0;
		i<indexHdr->nSupportedMetas
		&& zudiIndexes[indexNo]->indexedGetProvision(i, &provTmp)
			== ERROR_SUCCESS;
		i++)
	{
		utf8Char		provName[DRIVER_METALANGUAGE_MAXLEN];
		const metaInitEntryS	*metaInitInfo;

		zudiIndexes[indexNo]->getProvisionString(&provTmp, provName);
		if (strcmp8(metaName, provName) != 0) { continue; };

		/* If we find a provision that matches the requested meta name,
		 * we get the driver header for the metalanguage that provides
		 * that meta name.
		 *
		 * Then we look up that meta's shortname in the meta init info
		 * list embedded in the kernel.
		 **/
		if (zudiIndexes[indexNo]->getDriverHeader(
			indexHdr, provTmp.driverId, metaHdr.get()))
		{
			printf(ERROR FPLAINNIDX"loadDriver: provision %s "
				"refers to inexistent driver-ID %d.\n",
				provName, provTmp.driverId);

			return NULL;
		};

		// Now look up the meta's shortname in the init info list.
		metaInitInfo = floodplainn.findMetaInitInfo(
			CC metaHdr->shortName);

		if (metaInitInfo != NULL)
			{ return metaInitInfo->udi_meta_info; };
	};

	return NULL;
}

void fplainnIndexer_loadDriverReq
(
	zasyncStreamC::zasyncMsgS *request,
	zuiServer::indexMsgS *requestData
	)
{
	floodplainnC::zudiIndexMsgS		*response;
	asyncResponseC				myResponse;
	heapObjC<fplainn::driverC>		driver;
	heapObjC<zui::headerS> 			indexHdr;
	heapObjC<zui::driver::headerS>		driverHdr;
	heapArrC<utf8Char>			tmpString;
	fplainn::deviceC			*device;
	error_t					err;
	const driverInitEntryS			*driverInitEntry;

	response = new floodplainnC::zudiIndexMsgS(
		request->header.sourceId,
		MSGSTREAM_SUBSYSTEM_FLOODPLAINN, requestData->command,
		sizeof(*response), 0, request->header.privateData);

	if (response == NULL)
	{
		printf(ERROR FPLAINNIDX"loadDriverReq: "
			"unable to allocate response message.\n"
			"\tSender thread 0x%x may be frozen indefinitely.\n",
			request->header.sourceId);

		return;
	};

	myResponse(response);

	response->set(
		requestData->command, requestData->path,
		requestData->index, requestData->action);

	/**	EXPLANATION:
	 * First check to see if the driver has already been loaded; if not,
	 * load it. We use a locked iteration through the list, but frankly that
	 * may not be necessary. That said, it's only a spinlock acquire and
	 * release, so not really an issue.
	 **/
	err = floodplainn.getDevice(requestData->path, &device);
	if (err != ERROR_SUCCESS) { myResponse(err); return; };

	if (floodplainn.findDriver(device->driverFullName, driver.addressOf())
		== ERROR_SUCCESS)
	{
			/* "Driver" currently points to an object that was in
			 * the loaded driver list, and should not yet be freed.
			 * We release() the memory it points to before leaving.
			 **/
			driver.release();
			myResponse(ERROR_SUCCESS);
			return;
	};

	/* Else, the driver wasn't loaded already. Proceed to fill out the
	 * driver information object.
	 **/
	driver = new fplainn::driverC(device->driverIndex);
	indexHdr = new zui::headerS;
	driverHdr = new zui::driver::headerS;
	tmpString = new utf8Char[DRIVER_FULLNAME_MAXLEN];

	if (driver == NULL || indexHdr == NULL || driverHdr == NULL
		|| tmpString.get() == NULL)
		{ myResponse(ERROR_MEMORY_NOMEM); return; };

	err = zudiIndexes[0]->getHeader(indexHdr.get());
	if (err != ERROR_SUCCESS) { myResponse(err); return; };

	// Find the driver in the index using its basepath+shortname.

	err = zudiIndexes[0]->findDriver(
		indexHdr.get(), device->driverFullName, driverHdr.get());

	if (err != ERROR_SUCCESS) { myResponse(err); return; };

	// Fill out the strings. Not going to error check these.
	err = driver->initialize(
		CC driverHdr->basePath, CC driverHdr->shortName);

	if (err != ERROR_SUCCESS) { myResponse(err); return; };

	err = zudiIndexes[0]->getMessageString(
		driverHdr.get(), driverHdr->supplierIndex, driver->supplier);

	err = zudiIndexes[0]->getMessageString(
		driverHdr.get(), driverHdr->contactIndex,
		driver->supplierContact);

	err = zudiIndexes[0]->getMessageString(
		driverHdr.get(), driverHdr->nameIndex, driver->longName);

	if (driver->index == zuiServer::INDEX_KERNEL)
	{
		/* Only needed for kernel-index drivers. Others will have their
		 * udi_init_info provided by their driver binaries in userspace.
		 * Kernel-index drivers are embedded in the kernel image, and
		 * have their init_info structs embedded as well, so we must
		 * fill this out for them.
		 **/
		driverInitEntry = floodplainn.findDriverInitInfo(
			driver->shortName);

		if (driverInitEntry == NULL)
		{
			printf(ERROR FPLAINNIDX"loadDriver: failed to find "
				"init_info for driver %s.\n",
				driver->shortName);

			return;
		};

		driver->driverInitInfo = driverInitEntry->udi_init_info;
	};

	// Copy all the module information:
	err = driver->preallocateModules(driverHdr->nModules);
	if (driverHdr->nModules > 0 && err != ERROR_SUCCESS)
		{ myResponse(ERROR_MEMORY_NOMEM); return; };

	for (uarch_t i=0; i<driverHdr->nModules; i++)
	{
		zui::driver::moduleS		currModule;

		if (zudiIndexes[0]->indexedGetModule(
			driverHdr.get(), i, &currModule) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		if (zudiIndexes[0]->getModuleString(
			&currModule, tmpString.get()) != ERROR_SUCCESS)
			{ myResponse(ERROR_UNKNOWN); return; };

		new (&driver->modules[i]) fplainn::driverC::moduleS(
			currModule.index, tmpString.get());
	};

	// Now copy all the region information:
	err = driver->preallocateRegions(driverHdr->nRegions);
	if (driverHdr->nRegions > 0 && err != ERROR_SUCCESS)
		{ myResponse(ERROR_MEMORY_NOMEM); return; };

	for (uarch_t i=0; i<driverHdr->nRegions; i++)
	{
		zui::driver::regionS		currRegion;

		if (zudiIndexes[0]->indexedGetRegion(
			driverHdr.get(), i, &currRegion) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		new (&driver->regions[i]) fplainn::driverC::regionS(
			currRegion.index, currRegion.moduleIndex,
			currRegion.flags);
	};

	// Now copy all the requirements information:
	err = driver->preallocateRequirements(driverHdr->nRequirements);
	if (driverHdr->nRequirements > 0 && err != ERROR_SUCCESS)
		{ myResponse(ERROR_MEMORY_NOMEM); return; };

	for (uarch_t i=0; i<driverHdr->nRequirements; i++)
	{
		zui::driver::requirementS	currRequirement;

		if (zudiIndexes[0]->indexedGetRequirement(
			driverHdr.get(), i, &currRequirement) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		if (zudiIndexes[0]->getRequirementString(
			&currRequirement, tmpString.get()) != ERROR_SUCCESS)
			{ myResponse(ERROR_UNKNOWN); return; };

		new (&driver->requirements[i]) fplainn::driverC::requirementS(
			tmpString.get(), currRequirement.version);
	};

	// Next, metalanguage index information.
	err = driver->preallocateMetalanguages(driverHdr->nMetalanguages);
	if (driverHdr->nMetalanguages > 0 && err != ERROR_SUCCESS)
		{ myResponse(ERROR_MEMORY_NOMEM); return; };

	for (uarch_t i=0; i<driverHdr->nMetalanguages; i++)
	{
		zui::driver::metalanguageS	currMetalanguage;
		udi_mei_init_t			*metaInfo=NULL;

		if (zudiIndexes[0]->indexedGetMetalanguage(
			driverHdr.get(), i, &currMetalanguage) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		if (zudiIndexes[0]->getMetalanguageString(
			&currMetalanguage, tmpString.get()) != ERROR_SUCCESS)
			{ myResponse(ERROR_UNKNOWN); return; };

		if (driver->index == zuiServer::INDEX_KERNEL)
		{
			metaInfo = __kindex_getMetaInfoFor(
				tmpString.get(), indexHdr.get());

			if (metaInfo == NULL)
			{
				printf(ERROR FPLAINNIDX"loadDriver: "
					"(kernel driver %s):\n\tFailed to find "
					"a udi_meta_info struct in kernel for "
					"metalanguage %s.\n",
					driver->shortName, tmpString.get());

				return;
			};
		};

		new (&driver->metalanguages[i]) fplainn::driverC::metalanguageS(
			currMetalanguage.index, tmpString.get(), metaInfo);
	};

	err = driver->preallocateChildBops(driverHdr->nChildBops);
	if (driverHdr->nChildBops > 0 && err != ERROR_SUCCESS)
		{ myResponse(ERROR_MEMORY_NOMEM); return; };

	for (uarch_t i=0; i<driverHdr->nChildBops; i++)
	{
		zui::driver::childBopS		currBop;

		if (zudiIndexes[0]->indexedGetChildBop(
			driverHdr.get(), i, &currBop) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		new (&driver->childBops[i]) fplainn::driverC::childBopS(
			currBop.metaIndex, currBop.regionIndex,
			currBop.opsIndex);
	};

	err = driver->preallocateParentBops(driverHdr->nParentBops);
	if (driver->nParentBops > 0 && err != ERROR_SUCCESS)
		{ myResponse(ERROR_MEMORY_NOMEM); return; };

	for (uarch_t i=0; i<driverHdr->nParentBops; i++)
	{
		zui::driver::parentBopS	currBop;

		if (zudiIndexes[0]->indexedGetParentBop(
			driverHdr.get(), i, &currBop) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		new (&driver->parentBops[i]) fplainn::driverC::parentBopS(
			currBop.metaIndex, currBop.regionIndex,
			currBop.opsIndex, currBop.bindCbIndex);
	};

	err = driver->preallocateInternalBops(driverHdr->nInternalBops);
	if (driverHdr->nInternalBops > 0 && err != ERROR_SUCCESS)
		{ myResponse(ERROR_MEMORY_NOMEM); return; };

	for (uarch_t i=0; i<driverHdr->nInternalBops; i++)
	{
		zui::driver::internalBopS	currBop;

		if (zudiIndexes[0]->indexedGetInternalBop(
			driverHdr.get(), i, &currBop) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		new (&driver->internalBops[i]) fplainn::driverC::internalBopS(
			currBop.metaIndex, currBop.regionIndex,
			currBop.opsIndex0, currBop.opsIndex1,
			currBop.bindCbIndex);
	};

	err = driver->detectClasses();
	if (err != ERROR_SUCCESS) { myResponse(err); return; };

	driver->dump();
	// Release the managed memory as well when inserting.
	err = floodplainn.driverList.insert(driver.release());
	if (err != ERROR_SUCCESS) { myResponse(err); return; };
	myResponse(ERROR_SUCCESS);
}

void fplainnIndexServer_loadRequirementsReq
(
	zasyncStreamC::zasyncMsgS *request,
	zuiServer::indexMsgS *requestData
	)
{
	floodplainnC::zudiIndexMsgS	*response;
	asyncResponseC			myResponse;
	fplainn::driverC		*drv;
	error_t				err;
	heapObjC<zui::headerS>		indexHdr;
	heapObjC<zui::driver::headerS>	metaHdr;
	heapArrC<utf8Char>		tmpName;

	response = new floodplainnC::zudiIndexMsgS(
		request->header.sourceId,
		MSGSTREAM_SUBSYSTEM_FLOODPLAINN, requestData->command,
		sizeof(*response), 0, request->header.privateData);

	if (response == NULL)
	{
		printf(ERROR FPLAINNIDX"loadRequirementReq: "
			"unable to allocate response.\n"
			"\tSender thread 0x%x may be frozen indefinitely.\n",
			request->header.sourceId);

		return;
	};

	myResponse(response);
	response->set(
		requestData->command, requestData->path, requestData->index);

	indexHdr = new zui::headerS;
	metaHdr = new zui::driver::headerS;
	tmpName = new utf8Char[DRIVER_FULLNAME_MAXLEN];

	if (indexHdr.get() == NULL || metaHdr.get() == NULL
		|| tmpName.get() == NULL)
		{ myResponse(ERROR_MEMORY_NOMEM); return; };

	err = zudiIndexes[0]->getHeader(indexHdr.get());
	if (err != ERROR_SUCCESS)
	{
		printf(NOTICE FPLAINNIDX"loadReqmReq: Failed to get ZUDI index "
			"header.\n");

		myResponse(err); return;
	};

	err = floodplainn.findDriver(requestData->path, &drv);
	if (err != ERROR_SUCCESS) { myResponse(ERROR_UNINITIALIZED); return; };

	if (drv->allRequirementsSatisfied)
		{ myResponse(ERROR_SUCCESS); return; };

	for (uarch_t i=0; i<drv->nRequirements; i++)
	{
		zui::driver::provisionS	currProv;
		sarch_t				isSatisfied=0;

		for (uarch_t j=0;
			j<indexHdr->nSupportedMetas
			&& zudiIndexes[0]->indexedGetProvision(j, &currProv)
				== ERROR_SUCCESS;
			j++)
		{
			uarch_t		bPathLen;
			utf8Char	provName[DRIVER_METALANGUAGE_MAXLEN];

			zudiIndexes[0]->getProvisionString(&currProv, provName);

			/* Compare string against driver requirement.
			 * If match, get the meta lib header that corresponds,
			 * and copy its fullname into the device's
			 * requirements array.
			 **/
			if (strcmp8(drv->requirements[i].name, provName) != 0)
				{ continue; };

			/* Found a provision for the required meta. Get the
			 * driver header for the meta, and fill in the meta's
			 * fullname.
			 **/
			err = zudiIndexes[0]->getDriverHeader(
				indexHdr.get(), currProv.driverId,
				metaHdr.get());

			if (err != ERROR_SUCCESS)
			{
				printf(ERROR FPLAINNIDX"loadReqmReq: "
					"provision's driverId %d references "
					"inexistent driver header.\n",
					currProv.driverId);

				break;
			};

			bPathLen = strlen8(CC metaHdr->basePath);
			drv->requirements[i].fullName = new utf8Char[
				bPathLen + strlen8(CC metaHdr->shortName) + 2];

			/* We choose to continue to the next requirement
			 * instead of just returning immediately.
			 *
			 * This is because it may be useful to the caller to
			 * see which requirements /can/ be satisfied, even if
			 * some inexplicable error occurs before we can process
			 * all of the requirements.
			 **/
			if (drv->requirements[i].fullName == NULL)
				{ continue; };

			strcpy8(
				drv->requirements[i].fullName,
				CC metaHdr->basePath);

			if (bPathLen > 0
				&& drv->requirements[i].fullName[bPathLen -1]
					!= '/')
			{
				strcat8(drv->requirements[i].fullName, CC "/");
			};

			strcat8(
				drv->requirements[i].fullName,
				CC metaHdr->shortName);

			isSatisfied = 1;
			break;
		};

		if (!isSatisfied)
		{
			printf(ERROR FPLAINNIDX"loadReqmReq: meta requirement "
				"%s for driver %s could not "
				"be satisfied.\n",
				drv->requirements[i].name,
				drv->shortName);

			myResponse(ERROR_NOT_FOUND);
			return;
		};
	};

	drv->allRequirementsSatisfied = 1;
	myResponse(ERROR_SUCCESS);
}

void fplainnIndexServer_newDeviceInd
(
	zasyncStreamC::zasyncMsgS *request,
	zuiServer::indexMsgS *requestData
	)
{
	// The originContext /is/ the response that will eventually be sent.
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
	originContext = new floodplainnC::zudiIndexMsgS(
		request->header.sourceId,
		MSGSTREAM_SUBSYSTEM_FLOODPLAINN, requestData->command,
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
		requestData->index, zuiServer::NDACTION_NOTHING);

	// If the currently set action is "do nothing", just exit immediately.
	if (::newDeviceAction == zuiServer::NDACTION_NOTHING) {
		myResponse(ERROR_SUCCESS); return;
	};

	err = zuiServer::detectDriverReq(
		originContext->info.path, originContext->info.index,
		originContext, 0);

	if (err != ERROR_SUCCESS) { myResponse(err); return; };
	myResponse(DONT_SEND_RESPONSE);
}

void fplainnIndexServer_newDeviceInd1
(
	floodplainnC::zudiIndexMsgS *response
	)
{
	floodplainnC::zudiIndexMsgS		*originContext;
	asyncResponseC				myResponse;
	error_t					err;

	/**	EXPLANATION:
	 * The "request" we're dealing with is actually a response message from
	 * a syscall we ourselves performed; we name it accordingly ("response")
	 * for aided readability.
	 **/
	originContext = (floodplainnC::zudiIndexMsgS *)response->header
		.privateData;

	myResponse(originContext);

	if (response->header.error != ERROR_SUCCESS)
	{
		myResponse(response->header.error);
		return;
	};

	originContext->info.action = zuiServer::NDACTION_DETECT_DRIVER;
	// If newDeviceAction is detect-and-stop, stop and send response now.
	if (::newDeviceAction == zuiServer::NDACTION_DETECT_DRIVER)
		{ myResponse(ERROR_SUCCESS); return; };

	// Else we continue. Next we do a loadDriver() call.
	err = zuiServer::loadDriverReq(
		originContext->info.path, originContext);

	if (err != ERROR_SUCCESS)
		{ myResponse(err); return; };

	myResponse(DONT_SEND_RESPONSE);
}

void fplainnIndexServer_newDeviceInd2
(
	floodplainnC::zudiIndexMsgS *response
	)
{
	floodplainnC::zudiIndexMsgS	*originContext;
	asyncResponseC			myResponse;
	error_t				err;
	driverProcessC			*newProcess;

	originContext = (floodplainnC::zudiIndexMsgS *)response->header
		.privateData;

	myResponse(originContext);

	if (response->header.error != ERROR_SUCCESS)
	{
		myResponse(response->header.error);
		return;
	};

	originContext->info.action = zuiServer::NDACTION_LOAD_DRIVER;
	if (::newDeviceAction == zuiServer::NDACTION_LOAD_DRIVER)
		{ myResponse(ERROR_SUCCESS); return; };

	// Else now we spawn the driver process.
	err = processTrib.spawnDriver(
		originContext->info.path, NULL,
		PRIOCLASS_DEFAULT,
		0, originContext,
		&newProcess);

	if (err != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINNIDX"newDeviceInd2: spawnDriver failed "
			"because %s (%d).\n",
			strerror(err), err);

		myResponse(err);
		return;
	};

	originContext->info.processId = newProcess->id;
	myResponse(DONT_SEND_RESPONSE);
}

void fplainnIndexServer_newDeviceInd3
(messageStreamC::headerS *response)
{
	floodplainnC::zudiIndexMsgS	*originContext;
	asyncResponseC			myResponse;
	error_t				err;

	originContext = (floodplainnC::zudiIndexMsgS *)response->privateData;
	myResponse(originContext);

	if (response->error != ERROR_SUCCESS)
	{
		myResponse(response->error);
		return;
	};

	originContext->info.action = zuiServer::NDACTION_SPAWN_DRIVER;
	if (::newDeviceAction == zuiServer::NDACTION_SPAWN_DRIVER)
		{ myResponse(ERROR_SUCCESS); return; };

	// Finally, we instantiate the device.
	err = floodplainn.instantiateDeviceReq(
		originContext->info.path, originContext);

	if (err != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINNIDX"newDeviceInd3: instantiateDevReq "
			"failed because %s (%d).\n",
			strerror(err), err);

		myResponse(err); return;
	};

	myResponse(DONT_SEND_RESPONSE);
}

void fplainnIndexServer_newDeviceInd4
(
	floodplainnC::zudiKernelCallMsgS *response
	)
{
	floodplainnC::zudiIndexMsgS	*originContext;
	asyncResponseC			myResponse;
	originContext =
		(floodplainnC::zudiIndexMsgS *)response->header.privateData;

	myResponse(originContext);

	if (response->header.error != ERROR_SUCCESS)
	{
		myResponse(response->header.error);
		return;
	};

	originContext->info.action = zuiServer::NDACTION_INSTANTIATE;
	myResponse(ERROR_SUCCESS);
}

 void handleUnknownRequest
(zasyncStreamC::zasyncMsgS *request)
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

void fplainnIndexServer_handleRequest
(
	zasyncStreamC::zasyncMsgS *msg,
	zuiServer::indexMsgS *requestData, threadC *self
	)
{
	error_t		err;

	// Ensure that no process is sending us bogus request data.
	if (msg->dataNBytes != sizeof(zuiServer::indexMsgS))
	{
		printf(WARNING FPLAINNIDX"Thread 0x%x is sending spoofed "
			"request data.\n",
			msg->header.sourceId);

		return;
	};

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
	case ZUISERVER_DETECTDRIVER_REQ:
		fplainnIndexServer_detectDriverReq(
			(zasyncStreamC::zasyncMsgS *)msg, requestData);

		break;

	case ZUISERVER_LOAD_REQUIREMENTS_REQ:
		fplainnIndexServer_loadRequirementsReq(
			(zasyncStreamC::zasyncMsgS *)msg, requestData);

		break;

	case ZUISERVER_LOADDRIVER_REQ:
		fplainnIndexer_loadDriverReq(
			(zasyncStreamC::zasyncMsgS *)msg, requestData);

		break;

	case ZUISERVER_GET_NEWDEVICE_ACTION_REQ:
	case ZUISERVER_SET_NEWDEVICE_ACTION_REQ:
		fplainnIndexServer_newDeviceActionReq(
			(zasyncStreamC::zasyncMsgS *)msg, requestData);

		break;

	case ZUISERVER_NEWDEVICE_IND:
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
	heapObjC<messageStreamC::iteratorS>		gcb;
	heapObjC<zuiServer::indexMsgS>			requestData;
	zui::headerS					__kindexHeader;

	self = static_cast<threadC *>(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask() );

	gcb = new messageStreamC::iteratorS;
	requestData = new zuiServer::indexMsgS(
		0, NULL, zuiServer::INDEX_KERNEL);

	if (gcb.get() == NULL || requestData.get() == NULL)
	{
		printf(FATAL"Failed to allocate heap mem for message loop.\n"
			"\tDormanting forever.\n");

		taskTrib.dormant(self->getFullId());
	};

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
		"drivers, %d provisions, %d devices.\n",
		__kindexHeader.majorVersion, __kindexHeader.minorVersion,
		__kindexHeader.endianness, __kindexHeader.nRecords,
		__kindexHeader.nSupportedMetas,
		__kindexHeader.nSupportedDevices);

	for (;;)
	{
		self->getTaskContext()->messageStream.pull(gcb.get());

		// If some thread is forwarding syscall responses to us:
		if ((gcb->header.subsystem != MSGSTREAM_SUBSYSTEM_FLOODPLAINN
			&& gcb->header.subsystem != MSGSTREAM_SUBSYSTEM_ZASYNC
			&& gcb->header.subsystem != MSGSTREAM_SUBSYSTEM_PROCESS
			&& gcb->header.subsystem != MSGSTREAM_SUBSYSTEM_ZUDI)
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
			case ZUISERVER_DETECTDRIVER_REQ:
				fplainnIndexServer_newDeviceInd1(
					(floodplainnC::zudiIndexMsgS *)
						gcb.get());


				break;

			case ZUISERVER_LOADDRIVER_REQ:
				fplainnIndexServer_newDeviceInd2(
					(floodplainnC::zudiIndexMsgS *)
						gcb.get());

				break;
			};
			break;

		case MSGSTREAM_SUBSYSTEM_ZUDI:
			switch (gcb->header.function)
			{
			case MSGSTREAM_FPLAINN_ZUDI___KCALL:
				fplainnIndexServer_newDeviceInd4(
					(floodplainnC::zudiKernelCallMsgS *)
						gcb.get());

				break;
			};
			break;

		case MSGSTREAM_SUBSYSTEM_PROCESS:
			if (gcb->header.function
				!= MSGSTREAM_PROCESS_SPAWN_DRIVER)
			{
				break;
			};

			fplainnIndexServer_newDeviceInd3(
					(messageStreamC::headerS *)gcb.get());

			break;

		default:
			printf(NOTICE FPLAINNIDX"Unknown message.\n");
		};
	};
}

