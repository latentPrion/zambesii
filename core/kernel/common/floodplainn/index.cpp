
#include <__ksymbols.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <kernel/common/floodplainn/index.h>


error_t zudiIndexC::randomAccessBufferC::initialize(
	utf8Char *indexPath, utf8Char *fileName
	)
{
	uarch_t		strLen, indexPathLen;

	if (bufferSize == 0) { return ERROR_INVALID_ARG_VAL; };

	indexPathLen = strlen8(indexPath);
	strLen = indexPathLen + strlen8(fileName) + 1;
	fullName = new utf8Char[strLen];
	if (fullName == NULL) { return ERROR_MEMORY_NOMEM; };

	strcpy8(fullName, indexPath);
	// Ensure there is a '/' between path and filename.
	if (indexPath[indexPathLen - 1] != '/')
	{
		strcpy8(&fullName[indexPathLen], CC"/");
		indexPathLen++;
	};

	strcpy8(&fullName[indexPathLen], fileName);

	buffer.rsrc.buffer = new ubit8[bufferSize];
	if (buffer.rsrc.buffer == NULL) { return ERROR_MEMORY_NOMEM; };
	buffer.rsrc.bufferEnd = &buffer.rsrc.buffer[bufferSize];

	return ERROR_SUCCESS;
}

error_t zudiIndexC::randomAccessBufferC::readString(
	utf8Char *buff, uarch_t offset
	)
{
	if (bufferSize == 0)
	{
		buffer.lock.acquire();
		strcpy8(buff, CC &buffer.rsrc.buffer[offset]);
		buffer.lock.release();
		return ERROR_SUCCESS;
	};

	UNIMPLEMENTED("zudiIndexC::randomAccessBufferC::read")
	return ERROR_UNIMPLEMENTED;
}

error_t zudiIndexC::randomAccessBufferC::read(
	void *buff, uarch_t offset, uarch_t nBytes)
{
	if (bufferSize == 0)
	{
		buffer.lock.acquire();

		// Check for read overflows.
		if (&buffer.rsrc.buffer[offset + nBytes]
			> buffer.rsrc.bufferEnd)
		{
			buffer.lock.release();

			printf(ERROR ZUDIIDX"RAB::read: Overflow: bufferEnd "
				"0x%p, read would have accessed up to 0x%p.\n",
				buffer.rsrc.bufferEnd,
				&buffer.rsrc.buffer[offset + nBytes - 1]);

			/* Beware when changing this return value. A caller
			 * depends on the return value being LIMIT_OVERFLOWED
			 * on EOF of the underlying file/buffer.
			 **/
			return ERROR_LIMIT_OVERFLOWED;
		};

		memcpy(buff, &buffer.rsrc.buffer[offset], nBytes);

		buffer.lock.release();
		return ERROR_SUCCESS;
	};

	UNIMPLEMENTED("zudiIndexC::randomAccessBufferC::read")
	return ERROR_UNIMPLEMENTED;
}

void zudiIndexC::randomAccessBufferC::discardBuffer(void)
{
	if (bufferSize == 0) { return; }

	buffer.rsrc.buffer = buffer.rsrc.bufferEnd = NULL;
	UNIMPLEMENTED("zudiIndexC::randomAccessBufferC::discardBuffer")
}

error_t zudiIndexC::initialize(utf8Char *indexPath)
{
	error_t		ret;

	if (source != SOURCE_KERNEL && indexPath == NULL)
	{
		printf(ERROR ZUDIIDX"initialize: indexPath=NULL for non "
			"KERNEL index.\n");

		return ERROR_INVALID_ARG;
	};

	if (source == SOURCE_KERNEL && indexPath != NULL)
	{
		printf(WARNING ZUDIIDX"initialize: indexPath should be NULL "
			"for KERNEL index. Ignoring.\n");
	};

	strcpy8(this->indexPath, indexPath);

	if (source == SOURCE_KERNEL)
	{
		driverIndex.initialize(
			&__kudi_index_drivers,
			&__kudi_index_drivers_end);

		dataIndex.initialize(
			&__kudi_index_data,
			&__kudi_index_data_end);

		deviceIndex.initialize(
			&__kudi_index_devices,
			&__kudi_index_devices_end);

		provisionIndex.initialize(
			&__kudi_index_provisions,
			&__kudi_index_provisions_end);

		rankIndex.initialize(
			&__kudi_index_ranks,
			&__kudi_index_ranks_end);

		stringIndex.initialize(
			&__kudi_index_strings,
			&__kudi_index_strings_end);
	}
	else
	{
		ret = driverIndex.initialize(indexPath, CC"drivers.zudi-index");
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = dataIndex.initialize(indexPath, CC"data.zudi-index");
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = deviceIndex.initialize(indexPath, CC"devices.zudi-index");
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = provisionIndex.initialize(
			indexPath, CC"provisions.zudi-index");

		if (ret != ERROR_SUCCESS) { return ret; };
		ret = rankIndex.initialize(indexPath, CC"ranks.zudi-index");
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = stringIndex.initialize(indexPath, CC"strings.zudi-index");
		if (ret != ERROR_SUCCESS) { return ret; };
	};

	return ERROR_SUCCESS;
}

error_t zudiIndexC::getDriverHeader(ubit16 id, zudi::driver::headerS *retobj)
{
	for (uarch_t i=0;
		driverIndex.read(
			retobj,
			sizeof(zudi::driver::headerS) + (sizeof(*retobj) * i),
			sizeof(*retobj))
			== ERROR_SUCCESS;
		i++)
	{
		if (retobj->id == id) { return ERROR_SUCCESS; };
	};

	return ERROR_NO_MATCH;
}

error_t zudiIndexC::findMetalanguage(
	utf8Char *metaName, zudi::driver::headerS *retobj
	)
{
	zudi::driver::provisionS	currProvision;

	for (uarch_t i=0;
		provisionIndex.indexedRead(&currProvision, i) == ERROR_SUCCESS;
		i++)
	{
		utf8Char	currProvName[ZUDI_DRIVER_METALANGUAGE_MAXLEN];

		if (getString(currProvision.nameOff, currProvName)
			!= ERROR_SUCCESS)
		{
			continue;
		};

		if (strcmp8(currProvName, metaName) != 0) { continue; };

		/* Found a provider for the required metalanguage. Return the
		 * driver-header whose ID it holds.
		 **/
		if (getDriverHeader(currProvision.driverId, retobj)
			!= ERROR_SUCCESS)
		{
			continue;
		};

		return ERROR_SUCCESS;
	};

	return ERROR_NOT_FOUND;
}

error_t zudiIndexC::findDriver(
	utf8Char *fullName, zudi::driver::headerS *retobj
	)
{
	utf8Char		*nameTmp;
	uarch_t			base;

	base = sizeof(zudi::headerS);

	nameTmp = new utf8Char[
		ZUDI_DRIVER_BASEPATH_MAXLEN + ZUDI_DRIVER_SHORTNAME_MAXLEN];

	if (nameTmp == NULL) { return ERROR_MEMORY_NOMEM; };

	for (uarch_t i=0;
		driverIndex.read(
			retobj, base + sizeof(*retobj) * i,
			sizeof(*retobj)) == ERROR_SUCCESS;
		i++)
	{
		uarch_t		basePathLen;

		/* Copy the basepath+shortname, ensuring they are separated by
		 * a '/' character.
		 **/
		basePathLen = strlen8(CC retobj->basePath);
		strcpy8(nameTmp, CC retobj->basePath);
		if (basePathLen > 0 && retobj->basePath[basePathLen - 1] != '/')
		{
			strcpy8(&nameTmp[basePathLen], CC "/");
			basePathLen++;
		};

		strcpy8(&nameTmp[basePathLen], CC retobj->shortName);

		// Now we can compare.
		if (strcmp8(nameTmp, fullName) == 0)
		{
			delete[] nameTmp;
			return ERROR_SUCCESS;
		};
	};

	delete[] nameTmp;
	return ERROR_NOT_FOUND;
}

error_t zudiIndexC::findDeviceData(
	zudi::device::headerS *devHeader, utf8Char *attrName,
	zudi::device::attrDataS *retobj
	)
{
	error_t			ret;
	utf8Char		nameTmp[UDI_MAX_ATTR_NAMELEN];

	for (uarch_t i=0;
		i < devHeader->nAttributes
		&& indexedGetDeviceData(devHeader, i, retobj) == ERROR_SUCCESS;
		i++)
	{
		ret = getString(retobj->attr_nameOff, nameTmp);
		if (ret != ERROR_SUCCESS) { return ret; };

		if (strcmp8(nameTmp, attrName) == 0) { return ERROR_SUCCESS; };
	};

	return ERROR_NOT_FOUND;
}

error_t zudiIndexC::indexedGetDeviceData(
	zudi::device::headerS *devHeader, ubit16 idx,
	zudi::device::attrDataS *retobj
	)
{
	return dataIndex.read(
		retobj, devHeader->dataOff + sizeof(*retobj) * idx,
		sizeof(*retobj));
}

error_t zudiIndexC::getMessage(
	zudi::driver::headerS *drvHeader,
	ubit16 index, zudi::driver::messageS *retobj
	)
{
	uarch_t		messageBase;

	messageBase = drvHeader->messagesOffset;
	for (uarch_t i=0;
		dataIndex.read(
			retobj,
			messageBase + sizeof(*retobj) * i,
			sizeof(*retobj)) == ERROR_SUCCESS
		&& i < drvHeader->nMessages;
		i++)
	{
		if (retobj->index == index) { return ERROR_SUCCESS; };
	};

	return ERROR_NO_MATCH;
}

error_t zudiIndexC::getMessageString(
	zudi::driver::headerS *drvHeader,
	ubit16 index, utf8Char *string
	)
{
	error_t			ret;
	zudi::driver::messageS	msg;

	ret = getMessage(drvHeader, index, &msg);
	if (ret != ERROR_SUCCESS) { return ret; };

	return stringIndex.readString(string, msg.messageOff);
}

error_t zudiIndexC::getMetalanguage(
	zudi::driver::headerS *drvHeader, ubit16 index,
	zudi::driver::metalanguageS *retobj
	)
{
	uarch_t		metaBase;

	metaBase = drvHeader->metalanguagesOffset;
	for (uarch_t i=0;
		dataIndex.read(
			retobj, metaBase + sizeof(*retobj) * index,
			sizeof(*retobj)) == ERROR_SUCCESS
		&& i<drvHeader->nMetalanguages;
		i++)
	{
		if (retobj->index == index) { return ERROR_SUCCESS; };
	};

	return ERROR_NO_MATCH;
}

error_t zudiIndexC::indexedGetRank(
	zudi::driver::headerS *metaHeader, uarch_t index,
	zudi::rank::headerS *retobj
	)
{
	return rankIndex.read(
		retobj, metaHeader->rankFileOffset + sizeof(*retobj) * index,
		sizeof(*retobj));
}

error_t zudiIndexC::indexedGetRankAttrString(
	zudi::rank::headerS *rankHeader, uarch_t idx, utf8Char *retstr
	)
{
	error_t			ret;
	zudi::rank::rankAttrS	attr;

	ret = dataIndex.read(
		&attr, rankHeader->dataOff + sizeof(attr) * idx, sizeof(attr));

	if (ret != ERROR_SUCCESS) { return ret; };

	return getString(attr.nameOff, retstr);
}

