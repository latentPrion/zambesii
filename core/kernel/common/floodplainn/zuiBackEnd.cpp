
#include <debug.h>
#include <arch/debug.h>
#include <__ksymbols.h>
#include <zui.h>
#include <__kstdlib/callback.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/heapList.h>
#include <__kclasses/memReservoir.h>
#include <commonlibs/libzudiIndexParser.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/floodplainn/zui.h>


/**	EXPLANATION:
 * The ZUDI Index reader service reads and manages the ZUDI indexes in the
 * kernel image, the kernel ramdisk and off the external disk into RAM and
 * provides driver detection and support services.
 *
 * Is an implementation of the ZUDI Reader API. The current revision of the API
 * can be found in /core/kernel/common/zudiReader.h.
 **/


fplainn::Zui::newDeviceActionE	newDeviceAction =
	fplainn::Zui::NDACTION_INSTANTIATE;

ZudiIndexParser		kernelIndex(ZudiIndexParser::SOURCE_KERNEL),
			ramdiskIndex(ZudiIndexParser::SOURCE_RAMDISK),
			externalIndex(ZudiIndexParser::SOURCE_EXTERNAL);

ZudiIndexParser		*zudiIndexes[3] =
{
	&kernelIndex,
	&ramdiskIndex,
	&externalIndex
};

namespace zuiBackend
{
	status_t detectDriver_compareEnumerationAttributes(
		fplainn::Device *device, zui::device::sHeader *devline,
		zui::driver::sHeader *metaHdr);

	udi_mei_init_t *__kindex_getMetaInfoFor(
		utf8Char *metaName, zui::sHeader *indexHdr);

	void handleRequest(
		ZAsyncStream::sZAsyncMsg *msg,
		fplainn::Zui::sIndexZAsyncMsg *requestData, Thread *self);

	void handleUnknownRequest(ZAsyncStream::sZAsyncMsg *request);

	void detectDriverReq(
		ZAsyncStream::sZAsyncMsg *request,
		fplainn::Zui::sIndexZAsyncMsg *requestData);

	void loadDriverReq(
		ZAsyncStream::sZAsyncMsg *request,
		fplainn::Zui::sIndexZAsyncMsg *requestData);

	void loadRequirementsReq(
		ZAsyncStream::sZAsyncMsg *request,
		fplainn::Zui::sIndexZAsyncMsg *requestData);

	void newDeviceActionReq(
		ZAsyncStream::sZAsyncMsg *request,
		fplainn::Zui::sIndexZAsyncMsg *requestData);

	void newDeviceInd(
		ZAsyncStream::sZAsyncMsg *request,
		fplainn::Zui::sIndexZAsyncMsg *requestData);

	static void newDeviceInd1(
		MessageStream::sHeader *_response,
		fplainn::Zui::sIndexMsg *originCtxt);

	static void newDeviceInd2(
		MessageStream::sHeader *_response,
		fplainn::Zui::sIndexMsg *originCtxt);

	static void newDeviceInd3(
		MessageStream::sHeader *response,
		fplainn::Zui::sIndexMsg *originCtxt);

	static void newDeviceInd4(
		MessageStream::sHeader *_response,
		fplainn::Zui::sIndexMsg *originCtxt);
}

// Must hold device->s.lock.readAcquire() before calling this.
status_t zuiBackend::detectDriver_compareEnumerationAttributes(
	fplainn::Device *device, zui::device::sHeader *devline,
	zui::driver::sHeader *metaHdr
	)
{
	// Return value is the rank of the driver.
	status_t			ret=ERROR_NO_MATCH;
	zui::rank::sHeader		currRank;
	HeapArr<utf8Char>		attrValueTmp;

	/**	CAVEAT:
	 *	LOCKING:
	 * The lock for the the device pointed to by "device" must be
	 * readAcquire()'d before calling this function.
	 */

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
			zui::device::sAttrData		devlineData;
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
			for (uarch_t k=0; k<device->s.rsrc.nEnumerationAttrs;
				k++)
			{
				if (strcmp8(
					CC device->s.rsrc.enumerationAttrs[k]
						->attr_name,
					currRankAttrName) != 0)
				{
					continue;
				};

				/* Even if they have the same name, if the types
				 * differ, we can't consider it a match.
				 **/
				if (device->s.rsrc.enumerationAttrs[k]
					->attr_type != devlineData.attr_type)
				{
					break;
				};

				switch (device->s.rsrc.enumerationAttrs[k]
					->attr_type)
				{
				case UDI_ATTR_STRING:
					// Get the string from the index.
					if (zudiIndexes[0]->getString(
						devlineData.attr_valueOff,
						attrValueTmp.get())
						!= ERROR_SUCCESS)
					{ break; };

					if (!strcmp8(
						CC device->s.rsrc
							.enumerationAttrs[k]
							->attr_value,
						attrValueTmp.get()))
					{
						attrMatched = 1;
					};

					break;
				case UDI_ATTR_UBIT32:
					if (UDI_ATTR32_GET(
						device->s.rsrc
							.enumerationAttrs[k]
							->attr_value)
						== UDI_ATTR32_GET(
							(ubit8 *)&devlineData
								.attr_valueOff))
					{
						attrMatched = 1;
					};

					break;
				case UDI_ATTR_BOOLEAN:
					if (device->s.rsrc.enumerationAttrs[k]
						->attr_value[0]
						== *(ubit8 *)&devlineData
							.attr_valueOff)
					{
						attrMatched = 1;
					};

					break;
				case UDI_ATTR_ARRAY8:
					if (device->s.rsrc.enumerationAttrs[k]
						->attr_length
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
						device->s.rsrc
							.enumerationAttrs[k]
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

void zuiBackend::detectDriverReq(
	ZAsyncStream::sZAsyncMsg *request,
	fplainn::Zui::sIndexZAsyncMsg *requestData
	)
{
	error_t					err;
	fplainn::Zui::sIndexMsg			*response;
	fplainn::Device				*dev;
	zui::device::sHeader			devlineHdr,
						*currDevlineHdr;
	HeapArr<utf8Char>			devlineName;
	HeapObj<zui::sHeader>			indexHdr;
	HeapObj<zui::driver::sHeader>		driverHdr, metaHdr;
	// FIXME: big memory leak on this list within this function.
	HeapList<zui::device::sHeader>		matchingDevices;
	status_t				bestRank=-1;
	AsyncResponse				myResponse;
	uarch_t					rwflags;

	/** FIXME: Memory leaks all over this function.
	 **/
	response = new fplainn::Zui::sIndexMsg(
		request->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUI, requestData->command,
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
	indexHdr = new zui::sHeader;
	driverHdr = new zui::driver::sHeader;
	metaHdr = new zui::driver::sHeader;

	if (devlineName == NULL || indexHdr == NULL || driverHdr == NULL
		|| metaHdr == NULL)
	{
		printf(WARNING FPLAINNIDX"detectDriver: no heap memory.\n");
		myResponse(ERROR_MEMORY_NOMEM);
		return;
	};

	printf(NOTICE FPLAINNIDX"detectDriver: devname %s. Src %x, target %x."
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

	// Get the index header for the current selected index.
	err = zudiIndexes[0]->getHeader(indexHdr.get());
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINNIDX"detectDriver: Failed to get index "
			"header.\n");

		myResponse(err); return;
	};

	dev->s.lock.readAcquire(&rwflags);

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
		zui::driver::sMetalanguage		devlineMetalanguage;
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
		rank = detectDriver_compareEnumerationAttributes(
			dev, &devlineHdr, metaHdr.get());

		// If the driver was a match:
		if (rank >= 0)
		{
			// If the match outranks the current best match:
			if (rank > bestRank)
			{
				// Clear the list and restart.
				// FIXME: memory leak here.
				HeapList<zui::device::sHeader>::Iterator	it
					= matchingDevices.begin(0);

				for(; it != matchingDevices.end(); ++it)
				{
					currDevlineHdr = *it;
					delete currDevlineHdr;
				};

				matchingDevices.~HeapList();
				new (&matchingDevices)
					HeapList<zui::device::sHeader>;

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

			currDevlineHdr = new zui::device::sHeader;
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

		dev->s.lock.readReleaseWriteAcquire(rwflags);

		dev->driverFullName[0] = '\0';
		dev->s.rsrc.driverDetected = 0;
	}
	else
	{
		uarch_t				basePathLen;
		zui::driver::sMetalanguage	detectedDevlineMeta;
		utf8Char			detectedDevlineMetaName[
			DRIVER_METALANGUAGE_MAXLEN];


		/* If more than one driver matches, warn the user that we are
		 * about to make an arbitrary decision.
		 **/
		if (matchingDevices.getNItems() > 1)
		{
			printf(WARNING FPLAINNIDX"detectDriver: %s: Multiple "
				"viable matches. Arbitratily choosing first.\n",
				requestData->path);
		};

		HeapList<zui::device::sHeader>::Iterator		it =
			matchingDevices.begin(0);

		/* Shouldn't need to error check for NULL, because we are
		 * guaranteed to have at least 1 item in the list if we have
		 * reached here.
		 **/
		currDevlineHdr = *it;

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

		if (zudiIndexes[0]->getMetalanguage(
			driverHdr.get(), currDevlineHdr->metaIndex,
			&detectedDevlineMeta) != ERROR_SUCCESS)
		{
			printf(FATAL FPLAINNIDX"detectDriver: Chosen driver "
				"somehow doesn't contain meta_idx %d for "
				"chosen device line.\n",
				currDevlineHdr->metaIndex);

			myResponse(ERROR_NOT_FOUND); return;
		};

		if (zudiIndexes[0]->getString(
			detectedDevlineMeta.nameOff, detectedDevlineMetaName)
			!= ERROR_SUCCESS)
		{
			printf(FATAL FPLAINNIDX"detectDriver: meta line's name "
				"offset failed to retrieve a string.\n");

			myResponse(ERROR_NO_MATCH); return;
		};

		myResponse(ERROR_SUCCESS);

		dev->s.lock.readReleaseWriteAcquire(rwflags);

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

		dev->s.rsrc.driverDetected = 1;
	};

	dev->s.rsrc.driverIndex = fplainn::Zui::INDEX_KERNEL;
	// Set the requested index
	dev->s.rsrc.requestedIndex = requestData->index;

	// In both cases, driverInstance should be NULL.
	if (dev->driverInstance != NULL)
	{
		dev->s.lock.writeRelease();
		panic(FATAL FPLAINNIDX"detectDriver: device has a "
			"driverInstance set already.");
	}

	dev->s.lock.writeRelease();

	// Free the list.
	HeapList<zui::device::sHeader>::Iterator		it =
		matchingDevices.begin(0);

	for (; it != matchingDevices.end(); ++it) { delete *it; };
}

void zuiBackend::newDeviceActionReq(
	ZAsyncStream::sZAsyncMsg *request,
	fplainn::Zui::sIndexZAsyncMsg *requestData
	)
{
	fplainn::Zui::sIndexMsg			*response;
	AsyncResponse				myResponse;

	response = new fplainn::Zui::sIndexMsg(
		request->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUI, requestData->command,
		sizeof(*response),
		request->header.flags, request->header.privateData);

	if (response == NULL)
	{
		printf(ERROR FPLAINNIDX"newDeviceActionReq: "
			"unable to allocate response message.\n"
			"\tSender thread %x may be frozen indefinitely.\n",
			request->header.sourceId);

		return;
	};

	myResponse(response);
	response->info.action = ::newDeviceAction;

	if (requestData->command == MSGSTREAM_ZUI_SET_NEWDEVICE_ACTION_REQ)
		{ ::newDeviceAction = requestData->action; };

	myResponse(ERROR_SUCCESS);
}

udi_mei_init_t *zuiBackend::__kindex_getMetaInfoFor(
	utf8Char *metaName, zui::sHeader *indexHdr
	)
{
	HeapObj<zui::driver::sHeader>	metaHdr;
	zui::driver::sProvision		provTmp;
	ubit8				indexNo=(int)fplainn::Zui::INDEX_KERNEL;

	metaHdr = new zui::driver::sHeader;
	if (metaHdr == NULL) { return NULL; };

	for (uarch_t i=0;
		i<indexHdr->nSupportedMetas
		&& zudiIndexes[indexNo]->indexedGetProvision(i, &provTmp)
			== ERROR_SUCCESS;
		i++)
	{
		utf8Char		provName[DRIVER_METALANGUAGE_MAXLEN];
		const sMetaInitEntry	*metaInitInfo;

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
		metaInitInfo = floodplainn.zudi.findMetaInitInfo(
			CC metaHdr->shortName);

		if (metaInitInfo != NULL)
			{ return metaInitInfo->udi_meta_info; };
	};

	return NULL;
}

void zuiBackend::loadDriverReq(
	ZAsyncStream::sZAsyncMsg *request,
	fplainn::Zui::sIndexZAsyncMsg *requestData
	)
{
	fplainn::Zui::sIndexMsg			*response;
	AsyncResponse				myResponse;
	HeapObj<fplainn::Driver>		driver;
	HeapObj<zui::sHeader> 			indexHdr;
	HeapObj<zui::driver::sHeader>		driverHdr;
	HeapArr<utf8Char>			tmpString;
	fplainn::Device				*device;
	fplainn::Zui::indexE			devDriverIndex;
	error_t					err;
	uarch_t					rwflags;

	response = new fplainn::Zui::sIndexMsg(
		request->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUI, requestData->command,
		sizeof(*response), 0, request->header.privateData);

	if (response == NULL)
	{
		printf(ERROR FPLAINNIDX"loadDriverReq: "
			"unable to allocate response message.\n"
			"\tSender thread %x may be frozen indefinitely.\n",
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

	if (floodplainn.zudi.findDriver(
		device->driverFullName, driver.addressOf()) == ERROR_SUCCESS)
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
	device->s.lock.readAcquire(&rwflags);
	devDriverIndex = device->s.rsrc.driverIndex;
	device->s.lock.readRelease(rwflags);

	driver = new fplainn::Driver(devDriverIndex);

	indexHdr = new zui::sHeader;
	driverHdr = new zui::driver::sHeader;
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

#if 0
	if (driver->index == fplainn::Zui::INDEX_KERNEL)
	{
		/* Only needed for kernel-index drivers. Others will have their
		 * udi_init_info provided by their driver binaries in userspace.
		 * Kernel-index drivers are embedded in the kernel image, and
		 * have their init_info structs embedded as well, so we must
		 * fill this out for them.
		 **/

		driver->driverInitInfo = driverInitEntry->udi_init_info;
	};
#endif

	// Copy all the module information:
	err = driver->preallocateModules(driverHdr->nModules);
	if (driverHdr->nModules > 0 && err != ERROR_SUCCESS)
		{ myResponse(ERROR_MEMORY_NOMEM); return; };

	for (uarch_t i=0; i<driverHdr->nModules; i++)
	{
		zui::driver::sModule		currModule;

		if (zudiIndexes[0]->indexedGetModule(
			driverHdr.get(), i, &currModule) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		if (zudiIndexes[0]->getModuleString(
			&currModule, tmpString.get()) != ERROR_SUCCESS)
			{ myResponse(ERROR_UNKNOWN); return; };

		new (&driver->modules[i]) fplainn::Driver::sModule(
			currModule.index, tmpString.get());
	};

	// Now copy all the region information:
	err = driver->preallocateRegions(driverHdr->nRegions);
	if (driverHdr->nRegions > 0 && err != ERROR_SUCCESS)
		{ myResponse(ERROR_MEMORY_NOMEM); return; };

	for (uarch_t i=0; i<driverHdr->nRegions; i++)
	{
		zui::driver::sRegion		currRegion;

		if (zudiIndexes[0]->indexedGetRegion(
			driverHdr.get(), i, &currRegion) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		new (&driver->regions[i]) fplainn::Driver::sRegion(
			currRegion.index, currRegion.moduleIndex,
			currRegion.flags);
	};

	/* Now copy all the requirements information:
	 *
	 * Just like the metalanguage metadata below, we make extra room for
	 * one more requirement -- the mgmt meta.
	 */
	err = driver->preallocateRequirements(driverHdr->nRequirements + 1);
	if (err != ERROR_SUCCESS) { myResponse(ERROR_MEMORY_NOMEM); return; };

	const sMetaInitEntry			*udiMgmtMetaInitEntry;

	udiMgmtMetaInitEntry = floodplainn.zudi.findMetaInitInfo(CC"udi_mgmt");
	if (udiMgmtMetaInitEntry == NULL)
	{
		printf(ERROR FPLAINNIDX"loadDriver %s: failed to load "
			"udi_meta_info for MGMT meta!\n",
			driver->shortName);

		myResponse(ERROR_UNKNOWN);
		return;
	}

	new (&driver->requirements[0]) fplainn::Driver::sRequirement(
		CC"udi_mgmt", 0x101, udiMgmtMetaInitEntry->udi_meta_info);

	for (uarch_t i=0; i<driverHdr->nRequirements; i++)
	{
		zui::driver::sRequirement	currRequirement;
		udi_mei_init_t			*metaInfo=NULL;

		if (zudiIndexes[0]->indexedGetRequirement(
			driverHdr.get(), i, &currRequirement) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		if (zudiIndexes[0]->getRequirementString(
			&currRequirement, tmpString.get()) != ERROR_SUCCESS)
			{ myResponse(ERROR_UNKNOWN); return; };

		if (driver->index == fplainn::Zui::INDEX_KERNEL)
		{
			metaInfo = __kindex_getMetaInfoFor(
				tmpString.get(), indexHdr.get());

			if (metaInfo == NULL)
			{
				printf(ERROR FPLAINNIDX"loadDriver: "
					"(kernel driver %s):\n\tFailed to find "
					"a udi_meta_info struct in kernel for "
					"'requires' line %s.\n",
					driver->shortName, tmpString.get());

				myResponse(ERROR_NO_MATCH);
				return;
			};
		};

		new (&driver->requirements[i] + 1)
			fplainn::Driver::sRequirement(
				tmpString.get(), currRequirement.version,
				metaInfo);
	};

	/* Next, metalanguage index information.
	 * We always allocate room for one more, because we insinuate the MGMT
	 * metalanguage as an implicit child_bind_op meta for every driver.
	 **/
	err = driver->preallocateMetalanguages(driverHdr->nMetalanguages + 1);
	if (err != ERROR_SUCCESS) { myResponse(ERROR_MEMORY_NOMEM); return; };
	new (&driver->metalanguages[0]) fplainn::Driver::sMetalanguage(
		0, CC"udi_mgmt");

	for (uarch_t i=0; i<driverHdr->nMetalanguages; i++)
	{
		zui::driver::sMetalanguage	currMetalanguage;

		if (zudiIndexes[0]->indexedGetMetalanguage(
			driverHdr.get(), i, &currMetalanguage) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		if (zudiIndexes[0]->getMetalanguageString(
			&currMetalanguage, tmpString.get()) != ERROR_SUCCESS)
			{ myResponse(ERROR_UNKNOWN); return; };

		// Place it at the position i + 1; index 0 is always MGMT.
		new (&driver->metalanguages[i + 1]) fplainn::Driver::sMetalanguage(
			currMetalanguage.index, tmpString.get());
	};

	// We also preallocate an extra childBop for the MGMT meta.
	err = driver->preallocateChildBops(driverHdr->nChildBops + 1);
	if (err != ERROR_SUCCESS) { myResponse(ERROR_MEMORY_NOMEM); return; };
	new (&driver->childBops[0]) fplainn::Driver::sChildBop(0, 0, 0);

	for (uarch_t i=0; i<driverHdr->nChildBops; i++)
	{
		zui::driver::sChildBop		currBop;

		if (zudiIndexes[0]->indexedGetChildBop(
			driverHdr.get(), i, &currBop) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		new (&driver->childBops[i + 1]) fplainn::Driver::sChildBop(
			currBop.metaIndex, currBop.regionIndex,
			currBop.opsIndex);
	};

	err = driver->preallocateParentBops(driverHdr->nParentBops);
	if (driver->nParentBops > 0 && err != ERROR_SUCCESS)
		{ myResponse(ERROR_MEMORY_NOMEM); return; };

	for (uarch_t i=0; i<driverHdr->nParentBops; i++)
	{
		zui::driver::sParentBop	currBop;

		if (zudiIndexes[0]->indexedGetParentBop(
			driverHdr.get(), i, &currBop) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		new (&driver->parentBops[i]) fplainn::Driver::sParentBop(
			currBop.metaIndex, currBop.regionIndex,
			currBop.opsIndex, currBop.bindCbIndex);
	};

	err = driver->preallocateInternalBops(driverHdr->nInternalBops);
	if (driverHdr->nInternalBops > 0 && err != ERROR_SUCCESS)
		{ myResponse(ERROR_MEMORY_NOMEM); return; };

	for (uarch_t i=0; i<driverHdr->nInternalBops; i++)
	{
		zui::driver::sInternalBop	currBop;

		if (zudiIndexes[0]->indexedGetInternalBop(
			driverHdr.get(), i, &currBop) != ERROR_SUCCESS)
			{ myResponse(ERROR_NOT_FOUND); return; };

		new (&driver->internalBops[i]) fplainn::Driver::sInternalBop(
			currBop.metaIndex, currBop.regionIndex,
			currBop.opsIndex0, currBop.opsIndex1,
			currBop.bindCbIndex);
	};

	err = driver->detectClasses();
	if (err != ERROR_SUCCESS) { myResponse(err); return; };

	// Release the managed memory as well when inserting.
	err = floodplainn.zudi.driverList.insert(driver.release());
	if (err != ERROR_SUCCESS) { myResponse(err); return; };
	myResponse(ERROR_SUCCESS);
}

void zuiBackend::loadRequirementsReq(
	ZAsyncStream::sZAsyncMsg *request,
	fplainn::Zui::sIndexZAsyncMsg *requestData
	)
{
	fplainn::Zui::sIndexMsg		*response;
	AsyncResponse			myResponse;
	fplainn::Driver			*drv;
	error_t				err;
	HeapObj<zui::sHeader>		indexHdr;
	HeapObj<zui::driver::sHeader>	metaHdr;
	HeapArr<utf8Char>		tmpName;

	response = new fplainn::Zui::sIndexMsg(
		request->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUI, requestData->command,
		sizeof(*response), 0, request->header.privateData);

	if (response == NULL)
	{
		printf(ERROR FPLAINNIDX"loadRequirementReq: "
			"unable to allocate response.\n"
			"\tSender thread %x may be frozen indefinitely.\n",
			request->header.sourceId);

		return;
	};

	myResponse(response);
	response->set(
		requestData->command, requestData->path, requestData->index);

	indexHdr = new zui::sHeader;
	metaHdr = new zui::driver::sHeader;
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

	err = floodplainn.zudi.findDriver(requestData->path, &drv);
	if (err != ERROR_SUCCESS) { myResponse(ERROR_UNINITIALIZED); return; };

	if (drv->allRequirementsSatisfied)
		{ myResponse(ERROR_SUCCESS); return; };

	for (uarch_t i=0; i<drv->nRequirements; i++)
	{
		zui::driver::sProvision		currProv;
		sarch_t				isSatisfied=0;

		if (strcmp8(drv->requirements[i].name, CC"udi_mgmt") == 0)
		{
			/* MGMT meta will either be satisfied by libzbzcore
			 * and the service calls (for kernelspace drivers),
			 * or by libzbzcore in userspace for userspace
			 * drivers.
			 *
			 * Either way, we don't need to try to satisfy it here.
			 */
			isSatisfied = 1;
			continue;
		}

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

typedef void (__knewDeviceIndCbFn)(
	MessageStream::sHeader *msg, fplainn::Zui::sIndexMsg *originCtxt);

class NewDeviceIndCb
: public _Callback<__knewDeviceIndCbFn>
{
	fplainn::Zui::sIndexMsg	*originCtxt;

public:
	NewDeviceIndCb(
		__knewDeviceIndCbFn *kcb, fplainn::Zui::sIndexMsg *originCtxt)
	: _Callback<__knewDeviceIndCbFn>(kcb),
	originCtxt(originCtxt)
	{}

	virtual void operator()(MessageStream::sHeader *msg)
		{ (*function)(msg, originCtxt); }
};

static __knewDeviceIndCbFn	zuiBackend::newDeviceInd1;
static __knewDeviceIndCbFn	zuiBackend::newDeviceInd2;
static __knewDeviceIndCbFn	zuiBackend::newDeviceInd3;
static __knewDeviceIndCbFn	zuiBackend::newDeviceInd4;

void zuiBackend::newDeviceInd(
	ZAsyncStream::sZAsyncMsg *request,
	fplainn::Zui::sIndexZAsyncMsg *requestData
	)
{
	// The originCtxt /is/ the response that will eventually be sent.
	fplainn::Zui::sIndexMsg			*originCtxt;
	error_t					err;
	AsyncResponse				myResponse;

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
	originCtxt = new fplainn::Zui::sIndexMsg(
		request->header.sourceId,
		MSGSTREAM_SUBSYSTEM_ZUI, requestData->command,
		sizeof(*originCtxt),
		request->header.flags, request->header.privateData);

	if (originCtxt == NULL)
	{
		printf(ERROR FPLAINNIDX"newDeviceInd: "
			"unable to allocate response, or origin context.\n"
			"\tSender thread %x may be frozen indefinitely.\n",
			request->header.sourceId);

		return;
	};

	myResponse(originCtxt);
	originCtxt->set(
		requestData->command, requestData->path,
		requestData->index, fplainn::Zui::NDACTION_NOTHING);

	// If the currently set action is "do nothing", just exit immediately.
	if (::newDeviceAction == fplainn::Zui::NDACTION_NOTHING) {
		myResponse(ERROR_SUCCESS); return;
	};

	err = floodplainn.zui.detectDriverReq(
		originCtxt->info.path, originCtxt->info.index,
		new NewDeviceIndCb(
			&zuiBackend::newDeviceInd1, originCtxt),
		0);

	if (err != ERROR_SUCCESS) { myResponse(err); return; };
	myResponse(DONT_SEND_RESPONSE);
}

static void zuiBackend::newDeviceInd1(
	MessageStream::sHeader *_response, fplainn::Zui::sIndexMsg *originCtxt
	)
{
	fplainn::Zui::sIndexMsg			*response;
	AsyncResponse				myResponse;
	error_t					err;

	/**	EXPLANATION:
	 * The "request" we're dealing with is actually a response message from
	 * a syscall we ourselves performed; we name it accordingly ("response")
	 * for aided readability.
	 **/
	response = (fplainn::Zui::sIndexMsg *)_response;
	myResponse(originCtxt);

	if (response->header.error != ERROR_SUCCESS)
	{
		myResponse(response->header.error);
		return;
	};

	originCtxt->info.action = fplainn::Zui::NDACTION_DETECT_DRIVER;
	// If newDeviceAction is detect-and-stop, stop and send response now.
	if (::newDeviceAction == fplainn::Zui::NDACTION_DETECT_DRIVER)
		{ myResponse(ERROR_SUCCESS); return; };

	// Else we continue. Next we do a loadDriver() call.
	err = floodplainn.zui.loadDriverReq(
		originCtxt->info.path,
		new NewDeviceIndCb(
			zuiBackend::newDeviceInd2, originCtxt));

	if (err != ERROR_SUCCESS)
		{ myResponse(err); return; };

	myResponse(DONT_SEND_RESPONSE);
}

static void zuiBackend::newDeviceInd2(
	MessageStream::sHeader *_response, fplainn::Zui::sIndexMsg *originCtxt
	)
{
	fplainn::Zui::sIndexMsg		*response;
	AsyncResponse			myResponse;
	error_t				err;
	DriverProcess			*newProcess;

	myResponse(originCtxt);

	response = (fplainn::Zui::sIndexMsg *)_response;
	if (response->header.error != ERROR_SUCCESS)
	{
		myResponse(response->header.error);
		return;
	};

	originCtxt->info.action = fplainn::Zui::NDACTION_LOAD_DRIVER;
	if (::newDeviceAction == fplainn::Zui::NDACTION_LOAD_DRIVER)
		{ myResponse(ERROR_SUCCESS); return; };

	// Else now we spawn the driver process.
	err = processTrib.spawnDriver(
		originCtxt->info.path, NULL,
		PRIOCLASS_DEFAULT,
		0,
		new NewDeviceIndCb(zuiBackend::newDeviceInd3, originCtxt),
		&newProcess);

	if (err != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINNIDX"newDeviceInd2: spawnDriver failed "
			"because %s (%d).\n",
			strerror(err), err);

		myResponse(err);
		return;
	};

	originCtxt->info.processId = newProcess->id;
	myResponse(DONT_SEND_RESPONSE);
}

static void zuiBackend::newDeviceInd3(
	MessageStream::sHeader *response, fplainn::Zui::sIndexMsg *originCtxt
	)
{
	AsyncResponse			myResponse;
	error_t				err;

	myResponse(originCtxt);

	if (response->error != ERROR_SUCCESS)
	{
		myResponse(response->error);
		return;
	};

	originCtxt->info.action = fplainn::Zui::NDACTION_SPAWN_DRIVER;
	if (::newDeviceAction == fplainn::Zui::NDACTION_SPAWN_DRIVER)
		{ myResponse(ERROR_SUCCESS); return; };

	// Finally, we instantiate the device.
	err = floodplainn.zudi.instantiateDeviceReq(
		originCtxt->info.path,
		new NewDeviceIndCb(
			zuiBackend::newDeviceInd4, originCtxt));

	if (err != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINNIDX"newDeviceInd3: instantiateDevReq "
			"failed because %s (%d).\n",
			strerror(err), err);

		myResponse(err); return;
	};

	myResponse(DONT_SEND_RESPONSE);
}

static void zuiBackend::newDeviceInd4(
	MessageStream::sHeader *_response, fplainn::Zui::sIndexMsg *originCtxt
	)
{
	fplainn::Zudi::sKernelCallMsg		*response;
	AsyncResponse				myResponse;

	myResponse(originCtxt);


	response = (fplainn::Zudi::sKernelCallMsg *)_response;
	if (response->header.error != ERROR_SUCCESS)
	{
		myResponse(response->header.error);
		return;
	};

	originCtxt->info.action = fplainn::Zui::NDACTION_INSTANTIATE;
	myResponse(ERROR_SUCCESS);
}

void zuiBackend::handleUnknownRequest(ZAsyncStream::sZAsyncMsg *request)
{
	AsyncResponse		myResponse;

	// Just send a response with an error value.
	MessageStream::sHeader	*response;

	response = new MessageStream::sHeader(request->header);
	if (response == NULL)
	{
		printf(ERROR FPLAINNIDX"HandleUnknownRequest: "
			"unable to allocate response message.\n"
			"\tSender thread %x may be frozen indefinitely.\n",
			request->header.sourceId);

		return;
	};

	response->subsystem = MSGSTREAM_SUBSYSTEM_ZUI;
	myResponse(response);
	myResponse(ERROR_UNKNOWN);
}

void zuiBackend::handleRequest(
	ZAsyncStream::sZAsyncMsg *msg,
	fplainn::Zui::sIndexZAsyncMsg *requestData, Thread *self
	)
{
	error_t		err;

	// Ensure that no process is sending us bogus request data.
	if (msg->dataNBytes != sizeof(fplainn::Zui::sIndexZAsyncMsg))
	{
		printf(WARNING FPLAINNIDX"Thread %x is sending spoofed "
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
	case MSGSTREAM_ZUI_DETECTDRIVER_REQ:
		detectDriverReq(
			(ZAsyncStream::sZAsyncMsg *)msg, requestData);

		break;

	case MSGSTREAM_ZUI_LOAD_REQUIREMENTS_REQ:
		loadRequirementsReq(
			(ZAsyncStream::sZAsyncMsg *)msg, requestData);

		break;

	case MSGSTREAM_ZUI_LOADDRIVER_REQ:
		loadDriverReq(
			(ZAsyncStream::sZAsyncMsg *)msg, requestData);

		break;

	case MSGSTREAM_ZUI_GET_NEWDEVICE_ACTION_REQ:
	case MSGSTREAM_ZUI_SET_NEWDEVICE_ACTION_REQ:
		newDeviceActionReq(
			(ZAsyncStream::sZAsyncMsg *)msg, requestData);

		break;

	case MSGSTREAM_ZUI_NEWDEVICE_IND:
		newDeviceInd(
			(ZAsyncStream::sZAsyncMsg *)msg, requestData);

		break;

	default:
		printf(ERROR FPLAINNIDX"Unknown command request %d.\n",
			requestData->command);

		handleUnknownRequest(msg);
		break;
	};
}

void fplainn::Zui::main(void *)
{
	Thread						*self;
	MessageStream::sHeader				*gcb;
	HeapObj<fplainn::Zui::sIndexZAsyncMsg>		requestData;
	zui::sHeader					__kindexHeader;

	self = static_cast<Thread *>(
		cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread() );

	requestData = new sIndexZAsyncMsg(0, NULL, INDEX_KERNEL);

	if (requestData.get() == NULL)
	{
		printf(FATAL"Failed to allocate heap mem for request data "
			"buffer.\n\tAborting.\n");

		taskTrib.kill(self); return;
	};

	if (zudiIndexes[0]->initialize(CC"[__kindex]") != ERROR_SUCCESS)
		{ panic(ERROR_UNINITIALIZED); };

	if (zudiIndexes[1]->initialize(CC"@h:__kramdisk/drivers32")
		!= ERROR_SUCCESS)
		{ panic(ERROR_UNINITIALIZED); };

	if (zudiIndexes[2]->initialize(CC"@h:__kroot/zambesii/drivers32")
		!= ERROR_SUCCESS)
		{ panic(ERROR_UNINITIALIZED); };

	floodplainn.zui.setServerTid(self->getFullId());
	self->parent->zasyncStream.listen(self->getFullId(), 1);

	printf(NOTICE FPLAINNIDX"Floodplainn-indexer executing;\n"
		"\tprocess ID: %x. ESP: %p. Listening; dormanting.\n",
		self->getFullId(), debug::getStackPointer());

	zudiIndexes[0]->getHeader(&__kindexHeader);
	printf(NOTICE FPLAINNIDX"KERNEL_INDEX: Version %d.%d. (%s), holds %d "
		"drivers, %d provisions, %d devices.\n"
		"\tEntering event loop.\n",
		__kindexHeader.majorVersion, __kindexHeader.minorVersion,
		__kindexHeader.endianness, __kindexHeader.nRecords,
		__kindexHeader.nSupportedMetas,
		__kindexHeader.nSupportedDevices);

	for (;FOREVER;)
	{
		self->messageStream.pull(&gcb);

		/**	FIXME:
		 * It is possible to cause the kernel to execute arbitrary code
		 * by sending syscall responses to it with the address of some
		 * payload in the privateData member of the message.
		 **/
		switch (gcb->subsystem)
		{
		case MSGSTREAM_SUBSYSTEM_ZASYNC:
			switch (gcb->function)
			{
			case MSGSTREAM_ZASYNC_SEND:
				zuiBackend::handleRequest(
					(ZAsyncStream::sZAsyncMsg *)gcb,
					requestData.get(), self);

				break;
			};

			break;

		default:
			Callback		*callback;

			callback = (Callback *)gcb->privateData;
			if (callback == NULL)
			{
				printf(NOTICE FPLAINNIDX"Unknown message with "
					"no callback.\n");

				break;
			};

			(*callback)(gcb);
			delete callback;
			break;
		};

		delete gcb;
	};

	taskTrib.kill(self);
}
