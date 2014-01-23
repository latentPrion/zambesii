
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/memory>
#include <kernel/common/process.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <commonlibs/libzbzcore/libzbzcore.h>


static void driverPath0(threadC *self);
static error_t instantiateDevice(floodplainnC::instantiateDeviceMsgS *msg);

void __klibzbzcoreDriverPath(threadC *self)
{
//	fplainn::driverC				*driver;
	streamMemPtrC<messageStreamC::iteratorS>	msgIt;
	syscallbackC					*callback;

	/**	EXPLANATION:
	 * This is essentially like a kind of udi_core lib.
	 *
	 * We will check all requirements and ensure they exist; then we'll
	 * proceed to spawn regions, etc. We need to do a message loop because
	 * we will have to do async calls to the ZUDI Index server, and possibly
	 * other subsystems.
	 **/

	msgIt = new (self->parent->memoryStream.memAlloc(
		PAGING_BYTES_TO_PAGES(sizeof(*msgIt))))
		messageStreamC::iteratorS;

	if (msgIt.get() == NULL)
	{
		printf(FATAL LZBZCORE"driverPath: proc 0x%x: Failed to alloc "
			"mem for msg iterator.\n",
			self->getFullId());

		self->parent->sendResponse(ERROR_MEMORY_NOMEM);
		taskTrib.dormant(self->getFullId());
	};

	driverPath0(self);

	for (;;)
	{
		self->getTaskContext()->messageStream.pull(msgIt.get());
		callback = (syscallbackC *)msgIt->header.privateData;

		switch (msgIt->header.subsystem)
		{
		case MSGSTREAM_SUBSYSTEM_FLOODPLAINN:
			switch (msgIt->header.function)
			{
			case MSGSTREAM_FPLAINN_INSTANTIATE_DEVICE_REQ:
				floodplainnC::instantiateDeviceMsgS	*msg;
				error_t					reterr;

				msg = (floodplainnC::instantiateDeviceMsgS *)
					msgIt.get();

				printf(NOTICE LZBZCORE"instantiateDevice() "
					"call (function %d).\n",
					msg->header.function);

				reterr = instantiateDevice(msg);

				floodplainn.instantiateDeviceAck(
					msg->header.sourceId, msg->path,
					reterr,
					msg->header.privateData);

				continue;

			default:
				break; // NOP and fallthrough.
			};

		default:
			if (callback == NULL)
			{
				printf(WARNING LZBZCORE"driverPath: message "
					"with no callback continuation.\n");

				break;
			};

			(*callback)(msgIt.get());
			asyncContextCache->free(callback);
			break;
		};
	};
}

static syscallbackDataF driverPath1;
static void driverPath0(threadC * /*self*/)
{
	zudiIndexServer::loadDriverRequirementsReq(
		newSyscallback(&driverPath1));
}

static void driverPath1(messageStreamC::iteratorS *msgIt, void *)
{
	threadC					*self;
	driverProcessC				*selfProcess;
	const driverInitEntryS			*driverInit;
	fplainn::driverC			*drv;
	fplainn::driverInstanceC		*drvInstance;

	self = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	selfProcess = (driverProcessC *)self->parent;

	// Get the device.
	assert_fatal(floodplainn.findDriver(self->parent->fullName, &drv)
		== ERROR_SUCCESS);

	assert_fatal(
		(drvInstance = drv->getInstance(
			selfProcess->addrSpaceBinding)) != NULL);

	printf(NOTICE LZBZCORE"driverPath1: Ret from loadRequirementsReq: %s "
		"(%d).\n",
		strerror(msgIt->header.error), msgIt->header.error);

	/**	EXPLANATION:
	 * In the userspace libzbzcore, we would have to make calls to the VFS
	 * to load and link all of the metalanguage libraries, as well as the
	 * driver module itself.
	 *
	 * However, since this is the kernel syslib, we can skip all of that,
	 * and move directly into getting the udi_init_t structures for all of
	 * the components involved.
	 **/
	driverInit = floodplainn.findDriverInitInfo(drv->shortName);
	if (driverInit == NULL)
	{
		printf(NOTICE LZBZCORE"driverPath1: Failed to find "
			"udi_init_info for %s.\n",
			drv->shortName);

		return;
	};

	// Set parent bind ops vectors.
	for (uarch_t i=0; i<drv->nParentBops; i++)
	{
		udi_ops_init_t		*tmp;
		udi_ops_vector_t	*opsVector=NULL;

		for (tmp =driverInit->udi_init_info->ops_init_list;
			tmp->ops_idx != 0;
			tmp++)
		{
			if (tmp->ops_idx == drv->parentBops[i].opsIndex)
				{ opsVector = tmp->ops_vector; };
		};

		if (opsVector == NULL)
		{
			printf(ERROR LZBZCORE"driverPath1: Failed to obtain "
				"ops vector addr for parent bop with meta idx "
				"%d.\n",
				drv->parentBops[i].metaIndex);

			return;
		};

		drvInstance->setParentBopVector(
			drv->parentBops[i].metaIndex, opsVector);
	};

	for (uarch_t i=0; i<drv->nParentBops; i++)
	{
		printf(NOTICE LZBZCORE"driverPath1: pid 0x%x: pbop meta %d, ops vec 0x%p.\n",
			self->getFullId(),
			drvInstance->parentBopVectors[i].metaIndex,
			drvInstance->parentBopVectors[i].opsVector);
	};

	self->parent->sendResponse(msgIt->header.error);
}

static void regionThreadEntry(void *);
static void regionThreadEntry(void *)
{
	threadC					*self;
	heapPtrC<messageStreamC::iteratorS>	msgIt;
	fplainn::driverC			*drv;
	fplainn::deviceC			*dev;
	ubit16					regionId=0;

	self = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	// Not error checking this.
	assert_fatal(
		floodplainn.getDevice(self->parent->fullName, &dev)
		== ERROR_SUCCESS);

	drv = dev->driverInstance->driver;
	// Find out which region this thread is servicing.
	for (uarch_t i=0; i<drv->nRegions; i++)
	{
		if (dev->instance->regions[i].tid == self->getFullId())
		{
			regionId = dev->instance->regions[i].index;
			break;
		};
	};

	msgIt = new messageStreamC::iteratorS;
	if (msgIt == NULL)
	{
		printf(ERROR LZBZCORE"region %d: Failed to allocate msg "
			"iterator.\n",
			regionId);
	};

	printf(NOTICE LZBZCORE"region %d: running, TID 0x%x. Dormanting.\n",
		regionId, self->getFullId());

	for (;;)
	{
		self->getTaskContext()->messageStream.pull(msgIt.get());
	};

	taskTrib.dormant(self->getFullId());
}

static error_t instantiateDevice(floodplainnC::instantiateDeviceMsgS *msg)
{
	(void)msg;
	return ERROR_SUCCESS;
}

#if 0
	/* Initialize regions first; this is the simplest implementation path
	 * to follow.
	 **/
	dev->instance->regions = new fplainn::deviceInstanceC::regionS[drv->nRegions];
	if (dev->regions == NULL)
	{
		printf(ERROR LZBZCORE"driverPath1: Failed to allocate device "
			"region metadata.\n");

		return;
	};

	// Now we create threads for all the other regions.
	for (uarch_t i=0; i<drv->nRegions; i++)
	{
		threadC		*newThread;

		/* No need to spawn a new thread for the primary region. This
		 * thread will become its thread soon.
		 **/
		if (drv->regions[i].index != 0)
		{
			err = self->parent->spawnThread(
				&regionThreadEntry, NULL,
				NULL, (taskC::schedPolicyE)0, 0,
				0, &newThread);

			if (err != ERROR_SUCCESS)
			{
				printf(ERROR LZBZCORE"driverPath1: failed to "
					"spawn thread for region %d because "
					"%s(%d).\n",
					drv->regions[i].index,
					strerror(err), err);

				return;
			};
		}
		else
		{
			// Set myRegion.
			myRegion = &dev->regions[i];
			// Allocate primary region's region data.
			myRegion->rdata = (udi_init_context_t *)new ubit8[
				driverInit->udi_init_info
					->primary_init_info->rdata_size + 1];

			assert_fatal(myRegion->rdata != NULL);
		};

		// Store the TID in the device's region metadata.
		dev->regions[i].index = drv->regions[i].index;
		dev->regions[i].tid = (drv->regions[i].index == 0)
			? self->getFullId()
			: newThread->getFullId();
	};

	assert_fatal(myRegion != NULL);

	for (uarch_t i=0; i<drv->nRegions; i++)
	{
		printf(NOTICE LZBZCORE"driverPath1: Region %d, TID 0x%x.\n",
			dev->regions[i].index, dev->regions[i].tid);
	};

	/* Next, we spawn all internal bind channels.
	 * internal_bind_ops meta_idx region_idx ops0_idx ops1_idx bind_cb_idx.
	 * Small sanity check here before moving on.
	 **/
	if (drv->nInternalBops != drv->nRegions - 1)
	{
		printf(NOTICE LZBZCORE"driverPath1: driver has %d ibops, "
			"but %d regions.\n",
			drv->nInternalBops, drv->nRegions);

		self->parent->sendResponse(ERROR_INVALID_ARG_VAL);
		return;
	};

	for (uarch_t i=0; i<drv->nInternalBops; i++)
	{
		fplainn::driverC::internalBopS	*currBop;
		fplainn::deviceC::regionS	*targetRegion=NULL;
		udi_ops_vector_t		*opsVector0=NULL,
						*opsVector1=NULL;

		currBop = &drv->internalBops[i];
		// Get the region metadata for the target region.
		for (uarch_t i=0; i<drv->nRegions; i++)
		{
			if (dev->regions[i].index == currBop->regionIndex) {
				targetRegion = &dev->regions[i];
			};
		};

		assert_fatal(targetRegion != NULL);

		for (udi_ops_init_t *tmp=driverInit->udi_init_info->ops_init_list;
			tmp->ops_idx != 0; tmp++)
		{
			if (tmp->ops_idx == currBop->opsIndex0)
				{ opsVector0 = tmp->ops_vector; };

			if (tmp->ops_idx == currBop->opsIndex1)
				{ opsVector1 = tmp->ops_vector; };
		};

		assert_fatal(opsVector0 != NULL && opsVector1 != NULL);

		printf(NOTICE LZBZCORE"About to sys spawn internal bind chan:\n"
			"\tto: [r%d, ops%d ctx0x%d], using ops%d.\n",
			currBop->regionIndex,
			currBop->opsIndex0, targetRegion->rdata,
			currBop->opsIndex1);

		floodplainn.zudi_sys_channel_spawn(
			myRegion->tid, opsVector0, NULL,
			targetRegion->tid, opsVector1, NULL);
	};

#endif
