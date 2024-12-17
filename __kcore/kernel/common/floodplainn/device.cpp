
#include <debug.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kclib/stdlib.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/debugPipe.h>
#include <kernel/common/floodplainn/device.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>


/**	EXPLANATION:
 * Global array of device classes recognized by the kernel. This array is used
 * to initialize the device classes in the by-class tree of the floodplainn VFS.
 *
 * The second array below maps UDI metalanguage names to kernel device class
 * names. We use it to detect what classes of functionality a device exports.
 **/
utf8Char		*driverClasses[] =
{
	CC"unknown",		// 0
	CC"unrecognized",	// 1
	CC"bus",		// 2
	CC"io-controller",	// 3
	CC"generic-io",		// 4
	CC"network",		// 5
	CC"audio-output",	// 6
	CC"audio-input",	// 7
	CC"visual-output",	// 8
	CC"visual-input",	// 9
	CC"storage",		// 10
	CC"character-input",	// 11
	CC"coordinate-input",	// 12
	CC"battery",		// 13
	CC"gpio",		// 14
	CC"biometric-input",	// 15
	CC"printer",		// 16
	CC"scanner",		// 17
	CC"irq-controller",	// 18
	CC"timer",		// 19
	CC"clock",		// 20
	CC"card-reader",	// 21
	CC"kernel-abstraction",	// 22
	CC"entropy-source",	// 23
	CC"tactile-input",	// 24
	CC"tactile-output",	// 25
	CC"cerebral-input",	// 26
	CC"cerebral-output",	// 27
	NULL
};

sDriverClassMapEntry	driverClassMap[] =
{
	{ CC"udi_bridge", 2 },
	{ CC"udi_nic", 5 },
	{ CC"udi_gio", 4 },
	{ CC"udi_scsi", 3 },
	{ CC"zbz_root", 22 },
	{ NULL, 0 }
};

error_t fplainn::Driver::preallocateModules(uarch_t nModules)
{
	if (nModules == 0) { return ERROR_SUCCESS; };
	modules = new sModule[nModules];
	if (modules == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nModules = nModules;
	return ERROR_SUCCESS;
}

error_t fplainn::Driver::preallocateRegions(uarch_t nRegions)
{
	if (nRegions == 0) { return ERROR_SUCCESS; };
	regions = new sRegion[nRegions];
	if (regions == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nRegions = nRegions;
	return ERROR_SUCCESS;
}

error_t fplainn::Driver::preallocateRequirements(uarch_t nRequirements)
{
	if (nRequirements == 0) { return ERROR_SUCCESS; };
	requirements = new sRequirement[nRequirements];
	if (requirements == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nRequirements = nRequirements;
	return ERROR_SUCCESS;
}

error_t fplainn::Driver::preallocateMetalanguages(uarch_t nMetalanguages)
{
	if (nMetalanguages == 0) { return ERROR_SUCCESS; };
	metalanguages = new sMetalanguage[nMetalanguages];
	if (metalanguages == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nMetalanguages = nMetalanguages;
	return ERROR_SUCCESS;
}

error_t fplainn::Driver::preallocateChildBops(uarch_t nChildBops)
{
	if (nChildBops == 0) { return ERROR_SUCCESS; };
	childBops = new sChildBop[nChildBops];
	if (childBops == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nChildBops = nChildBops;
	return ERROR_SUCCESS;
}

error_t fplainn::Driver::preallocateParentBops(uarch_t nParentBops)
{
	if (nParentBops == 0) { return ERROR_SUCCESS; };
	parentBops = new sParentBop[nParentBops];
	if (parentBops == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nParentBops = nParentBops;
	return ERROR_SUCCESS;
}

error_t fplainn::Driver::preallocateInternalBops(uarch_t nInternalBops)
{
	if (nInternalBops == 0) { return ERROR_SUCCESS; };
	internalBops = new sInternalBop[nInternalBops];
	if (internalBops == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nInternalBops = nInternalBops;
	return ERROR_SUCCESS;
}

static inline void noop(void) { }

static sbit8 fillInClass(
	utf8Char (*classes)[DRIVER_CLASS_MAXLEN], ubit16 insertionIndex,
	utf8Char *className
	)
{
	for (uarch_t i=0; i<insertionIndex; i++)
	{
		// Check for duplicate classes.
		if (strcmp8(classes[i], className) != 0) { continue; };
		// If duplicate, don't allow nClasses to be incremented.
		return 0;
	};

	strcpy8(classes[insertionIndex], className);
	return 1;
}

error_t fplainn::Driver::detectClasses(void)
{
	uarch_t		nRecognizedClasses=0;
	sMetalanguage	*metalanguage;

	/* Making 2 passes. First pass establishes the number of metalanguages
	 * the kernel recognizes; second pass allocates the array of classes
	 * for the driver, and fills it in.
	 **/

	state.lock.writeAcquire();

	for (ubit8 pass=1; pass <= 2; pass++)
	{
		for (uarch_t i=0; i<nChildBops; i++)
		{
			ubit16		currMetaIndex;

			currMetaIndex = childBops[i].metaIndex;
			metalanguage = getMetalanguage(currMetaIndex);
			if (metalanguage == NULL)
			{
				// Don't repeat the warning on the 2nd pass.
				(pass == 1)
				? printf(WARNING"Driver::detectClasses: drv "
					"%s/%s:\n",
					"\tMeta index %d in child bops doesn't "
					"map to any meta declaration.\n",
					basePath, shortName, currMetaIndex)
				: noop();

				continue;
			};

			/* Search through all the metalanguages that the driver
			 * exports as child_bind_ops, and see if the kernel can
			 * recognize any of them.
			 **/
			for (sDriverClassMapEntry *tmp=driverClassMap;
				tmp->metaName != NULL;
				tmp++)
			{
				if (strcmp8(metalanguage->name, tmp->metaName))
					{ continue; };

				if (pass == 1) { nRecognizedClasses++; }
				else
				{
					state.rsrc.nClasses += fillInClass(
						state.rsrc.classes,
						state.rsrc.nClasses,
						driverClasses[tmp->classIndex]);
				};

				break;
			};
		};

		// Don't execute anything beyond here on 2nd pass.
		if (pass == 2) { break; };

		/* If we couldn't recognize any of the metalanguage interfaces
		 * it exports, just class it as "unknown".
		 **/
		if (nRecognizedClasses == 0)
		{
			state.rsrc.classes = new utf8Char[1][
				DRIVER_CLASS_MAXLEN];

			if (state.rsrc.classes == NULL)
			{
				state.lock.writeRelease();
				return ERROR_MEMORY_NOMEM;
			};

			// driver class 1 is set in stone as "unrecognized".
			strcpy8(state.rsrc.classes[0], driverClasses[1]);
			state.rsrc.nClasses = 1;

			state.lock.writeRelease();
			return ERROR_SUCCESS;
		};

		state.rsrc.classes = new utf8Char[
			nRecognizedClasses][DRIVER_CLASS_MAXLEN];

		if (state.rsrc.classes == NULL)
		{
			state.lock.writeRelease();
			return ERROR_MEMORY_NOMEM;
		};
	};

	state.lock.writeRelease();
	return ERROR_SUCCESS;
}

error_t fplainn::DriverInstance::initialize(void)
{
	sChildBop		*newmem;

	if (driver->nChildBops > 0)
	{
		newmem = new sChildBop[driver->nChildBops];
		if (newmem == NULL) { return ERROR_MEMORY_NOMEM; };

		memset(
			newmem, 0,
			sizeof(*newmem) * driver->nChildBops);

		for (uarch_t i=0; i<driver->nChildBops; i++)
		{
			newmem[i].metaIndex =
				driver->childBops[i].metaIndex;
		};

		s.lock.writeAcquire();
		s.rsrc.childBopVectors = newmem;
		s.lock.writeRelease();
	};

	return ERROR_SUCCESS;
}

error_t fplainn::DriverInstance::addHostedDevice(utf8Char *path)
{
	HeapArr<HeapArr<utf8Char> >	tmp, old;
	uarch_t				len, rwflags;

	s.lock.readAcquire(&rwflags);

	for (uarch_t i=0; i<s.rsrc.nHostedDevices; i++)
	{
		if (!strcmp8(s.rsrc.hostedDevices[i].get(), path))
		{
			s.lock.readRelease(rwflags);
			return ERROR_SUCCESS;
		};
	};

	len = strlen8(path);

	tmp = new HeapArr<utf8Char>[s.rsrc.nHostedDevices + 1];
	if (tmp == NULL) { return ERROR_MEMORY_NOMEM; };
	tmp[s.rsrc.nHostedDevices] = new utf8Char[len + 1];
	if (tmp[s.rsrc.nHostedDevices] == NULL) { return ERROR_MEMORY_NOMEM; };

	strcpy8(tmp[s.rsrc.nHostedDevices].get(), path);

	if (s.rsrc.nHostedDevices > 0)
	{
		memcpy(
			// Cast to void* silences Clang++ warning.
			static_cast<void *>(tmp.get()),
			static_cast<void *>(s.rsrc.hostedDevices.get()),
			sizeof(*s.rsrc.hostedDevices) * s.rsrc.nHostedDevices);
	};

	s.lock.readReleaseWriteAcquire(rwflags);

	// old will auto-free on exit.
	old = s.rsrc.hostedDevices;
	for (uarch_t i=0; i<s.rsrc.nHostedDevices; i++)
		{ old[i].release(); };

	s.rsrc.hostedDevices = tmp;
	s.rsrc.nHostedDevices++;

	s.lock.writeRelease();

	return ERROR_SUCCESS;
}

void fplainn::DriverInstance::removeHostedDevice(utf8Char *path)
{
	HeapArr<utf8Char>	tmp;
	uarch_t			rwflags;

	s.lock.readAcquire(&rwflags);

	for (uarch_t i=0; i<s.rsrc.nHostedDevices; i++)
	{
		if (strcmp8(s.rsrc.hostedDevices[i].get(), path) != 0)
			{ continue; };

		s.lock.readReleaseWriteAcquire(rwflags);

		// tmp will auto delete the mem on exit.
		tmp = s.rsrc.hostedDevices[i];
		memmove(
			// Cast to void* silences Clang++ warning.
			static_cast<void *>(&s.rsrc.hostedDevices[i]),
			static_cast<void *>(&s.rsrc.hostedDevices[i+1]),
			sizeof(*s.rsrc.hostedDevices)
				* (s.rsrc.nHostedDevices - i - 1));

		s.rsrc.nHostedDevices--;

		s.lock.writeRelease();
		return;
	};

	s.lock.readRelease(rwflags);
}

error_t fplainn::DriverInstance::getHostedDevicePathByTid(
	processId_t tid, utf8Char *path
	)
{
	error_t		ret;
	uarch_t		rwflags;

	/**	EXPLANATION:
	 * Newly started threads will not know which device they belong to.
	 * They will send a request to the main thread, asking it which device
	 * they should be servicing.
	 *
	 * The main thread will use the source thread's ID to search through all
	 * the devices hosted by this driver instance, and find out which
	 * device has a region with the source thread's TID.
	 **/
	s.lock.readAcquire(&rwflags);

	for (uarch_t i=0; i<s.rsrc.nHostedDevices; i++)
	{
		fplainn::Device		*currDev;
		utf8Char		*currDevPath=
			s.rsrc.hostedDevices[i].get();

		ret = floodplainn.getDevice(currDevPath, &currDev);
		// Odd, but keep searching anyway.
		if (ret != ERROR_SUCCESS) { continue; };

		for (uarch_t j=0; j<driver->nRegions; j++)
		{
			ubit16			idx;

			if (currDev->instance->getRegionInfo(tid, &idx)
				!= ERROR_SUCCESS)
				{ continue; };

			strcpy8(path, currDevPath);

			s.lock.readRelease(rwflags);
			return ERROR_SUCCESS;
		};
	};

	s.lock.readRelease(rwflags);
	return ERROR_NOT_FOUND;
}

error_t fplainn::DeviceInstance::initialize(void)
{
	error_t			ret;

	regions = new Region[device->driverInstance->driver->nRegions];
	if (regions == NULL) { return ERROR_MEMORY_NOMEM; };

	for (uarch_t i=0; i<device->driverInstance->driver->nRegions; i++)
	{
		ret = regions[i].initialize();
		if (ret != ERROR_SUCCESS)
		{
			printf(ERROR"DevInst: init: Failed for region %d.\n",
				device->driverInstance->driver->regions[i]
					.index);

			return ret;
		};

		regions[i].parent = this;
		regions[i].index = device->driverInstance->driver->regions[i]
			.index;
	};

	ret = channels.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	ret = regionLocalMetadata.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	return ERROR_SUCCESS;
}

fplainn::Channel *
fplainn::DeviceInstance::getChannelByEndpoint(Endpoint *endpoint)
{
	HeapList<Channel>::Iterator		it;

	for (it = channels.begin(0); it != channels.end(); ++it)
	{
		Channel	*currChannel = *it;

		if (!currChannel->hasEndpoint(endpoint)) { continue; };
		return currChannel;
	};

	return NULL;
}

error_t fplainn::DeviceInstance::getRegionInfo(processId_t tid, ubit16 *index)
{
	for (uarch_t i=0; i<device->driverInstance->driver->nRegions; i++)
	{
		if (regions[i].thread->getFullId() != tid) { continue; };
		*index = regions[i].index;
		return ERROR_SUCCESS;
	};

	return ERROR_NOT_FOUND;
}

void fplainn::DeviceInstance::setRegionInfo(ubit16 index, processId_t tid)
{
	Thread		*tmp;

	tmp = processTrib.getThread(tid);
	if (tmp == NULL) { return; };

	for (uarch_t i=0; i<device->driverInstance->driver->nRegions; i++)
	{
		if (regions[i].index != index) { continue; };

		regions[i].thread = tmp;
		return;
	};
}

error_t fplainn::DeviceInstance::getRegionInfo(ubit16 index, processId_t *tid)
{
	for (uarch_t i=0; i<device->driverInstance->driver->nRegions; i++)
	{
		if (regions[i].index != index) { continue; };
		*tid = regions[i].thread->getFullId();
		return ERROR_SUCCESS;
	};

	return ERROR_NOT_FOUND;
}

void fplainn::DeviceInstance::setThreadRegionPointer(processId_t tid)
{
	Region			*region=NULL;
	Thread			*regionThread;

	for (uarch_t i=0; i<device->driverInstance->driver->nRegions; i++)
	{
		if (regions[i].thread->getFullId() != tid) { continue; };

		region = &regions[i];
		break;
	};

	// Not found.
	if (region == NULL) { return; };

	// Link the region to its thread.
	regionThread = processTrib.getThread(tid);
	if (regionThread == NULL) { /* Weird. */ return; };

	regionThread->setRegion(region);
}

void fplainn::DeviceInstance::dumpChannels(void)
{
	printf(NOTICE"dumpChannels: devInst %s, %d channels.\n",
		device->longName, channels.getNItems());

	HeapList<fplainn::Channel>::Iterator		iChan;

	for (iChan = channels.begin(0); iChan != channels.end(); ++iChan)
	{
		fplainn::Channel		*chan = *iChan;

		chan->dump();
	};
}

void fplainn::Device::dumpEnumerationAttributes(void)
{
	utf8Char		*fmtChar;
	uarch_t			rwflags;

	s.lock.readAcquire(&rwflags);

	printf(NOTICE"Device: @%p, bid %d, %d enum attrs.\n\tlongname %s.\n",
		this, bankId,
		s.rsrc.nEnumerationAttrs, longName);

	for (uarch_t i=0; i<s.rsrc.nEnumerationAttrs; i++)
	{
		switch (s.rsrc.enumerationAttrs[i]->attr_type)
		{
		case UDI_ATTR_STRING: fmtChar = CC"%s"; break;
		case UDI_ATTR_UBIT32: fmtChar = CC"%x"; break;
		case UDI_ATTR_ARRAY8: fmtChar = CC"%x"; break;
		case UDI_ATTR_BOOLEAN: fmtChar = CC"%x"; break;
		default:
			printf(ERROR"Unknown device attr type %d.\n",
				s.rsrc.enumerationAttrs[i]->attr_type);
			continue;
		};

		// This recursive %s feature is actually pretty nice.
		printf(NOTICE"Attr: name %s, type %d, val %r.\n",
			s.rsrc.enumerationAttrs[i]->attr_name,
			s.rsrc.enumerationAttrs[i]->attr_type,
			fmtChar,
			(!strcmp8(fmtChar, CC"%s"))
				? (void *)s.rsrc.enumerationAttrs[i]->attr_value
				: (void *)(uintptr_t)UDI_ATTR32_GET(
					s.rsrc.enumerationAttrs[i]->attr_value));
	};

	s.lock.readRelease(rwflags);
}

error_t fplainn::Device::findEnumerationAttribute(
	utf8Char *name, udi_instance_attr_list_t **attr
	)
{
	uarch_t		rwflags;

	s.lock.readAcquire(&rwflags);

	for (uarch_t i=0; i<s.rsrc.nEnumerationAttrs; i++)
	{
		if (strcmp8(CC s.rsrc.enumerationAttrs[i]->attr_name, name)
			!= 0)
			{ continue; };

		*attr = s.rsrc.enumerationAttrs[i].get();

		s.lock.readRelease(rwflags);
		return ERROR_SUCCESS;
	};

	s.lock.readRelease(rwflags);
	return ERROR_NOT_FOUND;
}

error_t fplainn::Device::getEnumerationAttribute(
	utf8Char *name, udi_instance_attr_list_t *attrib
	)
{
	error_t				ret;
	udi_instance_attr_list_t	*tmp;

	// Simple search for an attribute with the name supplied.
	ret = findEnumerationAttribute(name, &tmp);
	if (ret != ERROR_SUCCESS) { return ERROR_NOT_FOUND; };

	memcpy(attrib, tmp, sizeof(*tmp));
	return ERROR_SUCCESS;
}

error_t fplainn::Device::addEnumerationAttribute(
	udi_instance_attr_list_t *attrib
	)
{
	error_t					ret;
	udi_instance_attr_list_t		*attrTmp;
	HeapArr<HeapObj<udi_instance_attr_list_t> >
						newArray, oldArray;
	uarch_t					prevNAttrs;

	if (attrib == NULL) { return ERROR_MEMORY_NOMEM; };

	ret = findEnumerationAttribute(CC attrib->attr_name, &attrTmp);
	if (ret == ERROR_SUCCESS)
	{
		// If the attribute already exists, just overwrite its value.
		memcpy(attrTmp, attrib, sizeof(*attrib));
		return ERROR_SUCCESS;
	};

	s.lock.writeAcquire();

	// Reuse attrTmp.
	attrTmp = new udi_instance_attr_list_t;
	if (attrTmp == NULL)
	{
		s.lock.writeRelease();
		return ERROR_MEMORY_NOMEM;
	};

	newArray = new HeapObj<udi_instance_attr_list_t>[
		s.rsrc.nEnumerationAttrs + 1];

	if (newArray == NULL)
	{
		s.lock.writeRelease();

		delete attrTmp;
		return ERROR_MEMORY_NOMEM;
	};

	memcpy(attrTmp, attrib, sizeof(*attrib));
	newArray[s.rsrc.nEnumerationAttrs] = attrTmp;

	if (s.rsrc.nEnumerationAttrs > 0)
	{
		memcpy(
			// Cast to void* silences Clang++ warning.
			static_cast<void *>(newArray.get()),
			static_cast<void *>(s.rsrc.enumerationAttrs.get()),
			sizeof(*s.rsrc.enumerationAttrs)
				* s.rsrc.nEnumerationAttrs);
	};


	prevNAttrs = s.rsrc.nEnumerationAttrs;
	oldArray = s.rsrc.enumerationAttrs;
	s.rsrc.enumerationAttrs = newArray;
	s.rsrc.nEnumerationAttrs++;

	s.lock.writeRelease();

	for (uarch_t i=0; i<prevNAttrs; i++) { oldArray[i].release(); };
	return ERROR_SUCCESS;
}

error_t fplainn::Device::createParentTag(
	fvfs::Tag *tag, utf8Char *enumeratingMetaName, ubit16 *newIdRetval)
{
	ParentTag	*old, *tmp;

	if (newIdRetval == NULL) { return ERROR_INVALID_ARG; }
	if (tag == NULL) { return ERROR_INVALID_ARG_VAL; }

	s.lock.writeAcquire();

	tmp = new ParentTag[s.rsrc.nParentTags + 1];
	if (tmp == NULL)
	{
		s.lock.writeRelease();
		return ERROR_MEMORY_NOMEM;
	}

	if (s.rsrc.nParentTags > 0)
	{
		memcpy(
			tmp, s.rsrc.parentTags,
			s.rsrc.nParentTags * sizeof(*s.rsrc.parentTags));
	}

	new (&tmp[s.rsrc.nParentTags]) ParentTag(
		s.rsrc.parentTagCounter, tag, enumeratingMetaName);

	old = s.rsrc.parentTags;
	*newIdRetval = s.rsrc.parentTagCounter;

	s.rsrc.parentTags = tmp;
	s.rsrc.nParentTags++;
	s.rsrc.parentTagCounter++;
	// Return and assign the new parentTag.

	s.lock.writeRelease();

	delete[] old;
	return ERROR_SUCCESS;
}

error_t fplainn::Driver::sModule::addAttachedRegion(ubit16 regionIndex)
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

error_t fplainn::Driver::addOrModifyOpsInit(ubit16 opsIndex, ubit16 metaIndex)
{
	sOpsInit	*old;

	if (opsIndex == 0 || metaIndex == 0) { return ERROR_INVALID_ARG_VAL; }

	// Does the opsIndex already exist?
	old = getOpsInit(opsIndex);
	if (old != NULL)
	{
		old->metaIndex = metaIndex;
		return ERROR_SUCCESS;
	}

	old = opsInits;
	opsInits = new sOpsInit[nOpsInits + 1];
	if (opsInits == NULL)
	{
		opsInits = old;
		return ERROR_MEMORY_NOMEM;
	}

	if (nOpsInits > 0)
	{
		memcpy(
			opsInits, old,
			nOpsInits * sizeof(*opsInits));
	}

	delete[] old;
	opsInits[nOpsInits++] = sOpsInit(opsIndex, metaIndex);
	return ERROR_SUCCESS;
}

error_t fplainn::Driver::addInstance(numaBankId_t bid, processId_t pid)
{
	DriverInstance		*old, *newInstance;
	error_t			ret;

	newInstance = getInstance(bid);
	// If one already exists for this NUMA bank:
	if (newInstance != NULL) {
		return ERROR_SUCCESS;
	}

	state.lock.writeAcquire();

	old = state.rsrc.instances;
	state.rsrc.instances = new DriverInstance[state.rsrc.nInstances + 1];
	if (state.rsrc.instances == NULL)
	{
		// Rollback.
		state.rsrc.instances = old;

		state.lock.writeRelease();
		return ERROR_MEMORY_NOMEM;
	};

	if (state.rsrc.nInstances > 0)
	{
		memcpy(
			// Cast to void* silences Clang++ warning.
			static_cast<void *>(state.rsrc.instances),
			static_cast<void *>(old),
			state.rsrc.nInstances * sizeof(*state.rsrc.instances));
	};

	delete[] old;

	newInstance = &state.rsrc.instances[state.rsrc.nInstances];

	new (newInstance) DriverInstance(this, bid, pid);
	ret = newInstance->initialize();
	if (ret != ERROR_SUCCESS)
	{
		state.lock.writeRelease();
		return ret;
	};

	state.rsrc.nInstances++;

	state.lock.writeRelease();
	return ERROR_SUCCESS;
}

void fplainn::Driver::dump(void)
{
	uarch_t		rwflags;

	state.lock.readAcquire(&rwflags);

	printf(NOTICE"Driver: index %d: %s/%s\n\t(%s).\n\tSupplier %s; "
		"Contact %s.\n"
		"\t%d mods, %d rgns, %d req's, %d metas, %d cbops, %d pbops, "
		"%d ibops, %d ops_vecs, %d classes.\n",
		index, basePath, shortName, longName, supplier, supplierContact,
		nModules, nRegions, nRequirements, nMetalanguages,
		nChildBops, nParentBops, nInternalBops, nOpsInits,
		state.rsrc.nClasses);

	for (uarch_t i=0; i<state.rsrc.nClasses; i++) {
		printf(NOTICE"\tdriver: class %s\n", state.rsrc.classes[i]);
	};

	state.lock.readRelease(rwflags);
}
