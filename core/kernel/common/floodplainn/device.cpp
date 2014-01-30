
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kclib/stdlib.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/debugPipe.h>
#include <kernel/common/floodplainn/device.h>


error_t fplainn::driverC::preallocateModules(uarch_t nModules)
{
	modules = new moduleS[nModules];
	if (modules == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nModules = nModules;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateRegions(uarch_t nRegions)
{
	regions = new regionS[nRegions];
	if (regions == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nRegions = nRegions;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateRequirements(uarch_t nRequirements)
{
	requirements = new requirementS[nRequirements];
	if (requirements == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nRequirements = nRequirements;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateMetalanguages(uarch_t nMetalanguages)
{
	metalanguages = new metalanguageS[nMetalanguages];
	if (metalanguages == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nMetalanguages = nMetalanguages;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateChildBops(uarch_t nChildBops)
{
	childBops = new childBopS[nChildBops];
	if (childBops == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nChildBops = nChildBops;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateParentBops(uarch_t nParentBops)
{
	parentBops = new parentBopS[nParentBops];
	if (parentBops == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nParentBops = nParentBops;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateInternalBops(uarch_t nInternalBops)
{
	internalBops = new internalBopS[nInternalBops];
	if (internalBops == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nInternalBops = nInternalBops;
	return ERROR_SUCCESS;
}

error_t fplainn::driverInstanceC::initialize(void)
{
	if (driver->nChildBops > 0)
	{
		childBopVectors = new childBopS[driver->nChildBops];
		if (childBopVectors == NULL) { return ERROR_MEMORY_NOMEM; };

		memset(
			childBopVectors, 0,
			sizeof(*childBopVectors) * driver->nChildBops);

		for (uarch_t i=0; i<driver->nChildBops; i++)
		{
			childBopVectors[i].metaIndex =
				driver->childBops[i].metaIndex;
		};
	};

	return ERROR_SUCCESS;
}

error_t fplainn::driverInstanceC::addHostedDevice(utf8Char *path)
{
	heapPtrC<utf8Char*>	tmp;
	utf8Char		**old;
	heapPtrC<utf8Char>	str;
	uarch_t			len;

	for (uarch_t i=0; i<nHostedDevices; i++) {
		if (!strcmp8(hostedDevices[i], path)) { return ERROR_SUCCESS; };
	};

	len = strlen8(path);

	tmp = new utf8Char*[nHostedDevices + 1];
	str = new utf8Char[len + 1];
	tmp.useArrayDelete = str.useArrayDelete = 1;

	if (tmp == NULL || str == NULL) { return ERROR_MEMORY_NOMEM; };
	strcpy8(str.get(), path);

	if (nHostedDevices > 0)
	{
		memcpy(tmp.get(), hostedDevices,
			sizeof(*hostedDevices) * nHostedDevices);
	};

	tmp[nHostedDevices] = str.release();
	old = hostedDevices;
	hostedDevices = tmp.release();
	nHostedDevices++;

	delete[] old;
	return ERROR_SUCCESS;
}

void fplainn::driverInstanceC::removeHostedDevice(utf8Char *path)
{
	utf8Char	*tmp;

	for (uarch_t i=0; i<nHostedDevices; i++)
	{
		if (strcmp8(hostedDevices[i], path) != 0) { continue; };

		tmp = hostedDevices[i];
		memmove(
			&hostedDevices[i], &hostedDevices[i+1],
			sizeof(*hostedDevices) * (nHostedDevices - i - 1));

		nHostedDevices--;
		delete[] tmp;
		return;
	};
}

error_t fplainn::deviceInstanceC::initialize(void)
{
	regions = new regionS[device->driverInstance->driver->nRegions];
	if (regions == NULL) { return ERROR_MEMORY_NOMEM; };

	for (uarch_t i=0; i<device->driverInstance->driver->nRegions; i++)
	{
		regions[i].index = device->driverInstance->driver->regions[i]
			.index;
	};

	return ERROR_SUCCESS;
}

error_t fplainn::deviceInstanceC::addChannel(channelS *newChan)
{
	channelS		**tmp, **old;

	tmp = new channelS*[nChannels + 1];
	if (tmp == NULL) { return ERROR_MEMORY_NOMEM; };

	if (nChannels > 0)
		{ memcpy(tmp, channels, sizeof(*channels) * nChannels); };

	tmp[nChannels] = newChan;
	old = channels;
	channels = tmp;
	nChannels++;
	return ERROR_SUCCESS;
}

void fplainn::deviceInstanceC::removeChannel(channelS *chan)
{
	for (uarch_t i=0; i<nChannels; i++)
	{
		if (channels[i] != chan) { continue; };

		memmove(
			&channels[i], &channels[i+1],
			sizeof(*channels) * (nChannels - i - 1));

		nChannels--;
		return;
	};
}

void fplainn::deviceC::dumpEnumerationAttributes(void)
{
	utf8Char		*fmtChar;

	printf(NOTICE"Device: @0x%p, id %d, %d enum attrs.\n\tlongname %s.\n",
		this, id, nEnumerationAttrs, longName);

	for (uarch_t i=0; i<nEnumerationAttrs; i++)
	{
		switch (enumerationAttrs[i]->attr_type)
		{
		case UDI_ATTR_STRING: fmtChar = CC"%s"; break;
		case UDI_ATTR_UBIT32: fmtChar = CC"0x%x"; break;
		case UDI_ATTR_ARRAY8: fmtChar = CC"%x"; break;
		case UDI_ATTR_BOOLEAN: fmtChar = CC"%x"; break;
		default:
			printf(ERROR"Unknown device attr type %d.\n",
				enumerationAttrs[i]->attr_type);
			return;
		};

		// This recursive %s feature is actually pretty nice.
		printf(NOTICE"Attr: name %s, type %d, val %r.\n",
			enumerationAttrs[i]->attr_name, enumerationAttrs[i]->attr_type,
			fmtChar,
			(!strcmp8(fmtChar, CC"%s"))
				? (void *)enumerationAttrs[i]->attr_value
				: (void *)(uintptr_t)UDI_ATTR32_GET(
					enumerationAttrs[i]->attr_value));
	};
}

error_t fplainn::deviceC::getEnumerationAttribute(
	utf8Char *name, udi_instance_attr_list_t *attrib
	)
{
	// Simple search for an attribute with the name supplied.
	for (uarch_t i=0; i<nEnumerationAttrs; i++)
	{
		if (strcmp8(CC enumerationAttrs[i]->attr_name, CC name) == 0)
		{
			*attrib = *enumerationAttrs[i];
			return ERROR_SUCCESS;
		};
	};

	return ERROR_NOT_FOUND;
}

error_t fplainn::deviceC::setEnumerationAttribute(
	udi_instance_attr_list_t *attrib
	)
{
	error_t					ret;
	udi_instance_attr_list_t		**attrArrayMem, **tmp;
	heapPtrC<udi_instance_attr_list_t>	destAttrMem;
	sarch_t					releaseAttrMem=0;

	if (attrib == NULL) { return ERROR_INVALID_ARG; };

	destAttrMem = new udi_instance_attr_list_t;
	if (destAttrMem == NULL) { return ERROR_MEMORY_NOMEM; };

	// Can do other checks, such as checks on supplied attr's "type" etc.

	// Check to see if the attrib already exists.
	ret = getEnumerationAttribute(CC attrib->attr_name, destAttrMem.get());
	if (ret != ERROR_SUCCESS)
	{
		// Allocate mem for the new array of pointers to attribs.
		attrArrayMem = new udi_instance_attr_list_t *[
			nEnumerationAttrs + 1];

		if (attrArrayMem == NULL) { return ERROR_MEMORY_NOMEM; };

		if (nEnumerationAttrs > 0)
		{
			memcpy(
				attrArrayMem, enumerationAttrs,
				sizeof(*enumerationAttrs) * nEnumerationAttrs);
		};

		attrArrayMem[nEnumerationAttrs] = destAttrMem.get();
		/* Since we have to use the allocated memory to store the new
		 * attribute permanently, we have to call release() on the
		 * pointer management object later on. Set this bool to indicate
		 * that.
		 **/
		releaseAttrMem = 1;
		tmp = enumerationAttrs;
		enumerationAttrs = attrArrayMem;
		nEnumerationAttrs++;

		delete tmp;
	};

	memcpy(destAttrMem.get(), attrib, sizeof(*attrib));
	if (releaseAttrMem) { destAttrMem.release(); };
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::moduleS::addAttachedRegion(ubit16 regionIndex)
{
	ubit16		*old;

	old = regionIndexes;
	regionIndexes = new ubit16[nAttachedRegions + 1];
	if (regionIndexes == NULL) { return ERROR_MEMORY_NOMEM; };
	if (nAttachedRegions > 0)
	{
		memcpy(
			regionIndexes, old,
			nAttachedRegions * sizeof(*regionIndexes));
	};

	delete[] old;

	regionIndexes[nAttachedRegions++] = regionIndex;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::addInstance(numaBankId_t bid, processId_t pid)
{
	driverInstanceC		*old, *newInstance;
	error_t			ret;

	newInstance = getInstance(bid);
	if (newInstance == NULL)
	{
		old = instances;
		instances = new driverInstanceC[nInstances + 1];
		if (instances == NULL) { return ERROR_MEMORY_NOMEM; };
		if (nInstances > 0) {
			memcpy(instances, old, nInstances * sizeof(*instances));
		};

		delete[] old;

		newInstance = &instances[nInstances];
	};

	new (newInstance) driverInstanceC(this, bid, pid);
	ret = newInstance->initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	nInstances++;
	return ERROR_SUCCESS;
}

void fplainn::driverC::dump(void)
{
	printf(NOTICE"Driver: index %d: %s/%s\n\t(%s).\n\tSupplier %s; "
		"Contact %s.\n"
		"%d mods, %d rgns, %d req's, %d metas, %d cbops, %d pbops, "
		"%d ibops.\n",
		basePath, shortName, longName, supplier, supplierContact,
		nModules, nRegions, nRequirements, nMetalanguages,
		nChildBops, nParentBops, nInternalBops);
}

