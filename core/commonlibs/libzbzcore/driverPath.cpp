
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/memory>
#include <kernel/common/process.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <commonlibs/libzbzcore/libzbzcore.h>


#define DRIVERPATH_U0_GET_THREAD_DEVICE_PATH		(0)

static void driverPath0(threadC *self);
static error_t instantiateDevice(floodplainnC::instantiateDeviceMsgS *msg);
static error_t getThreadDevicePath(
	fplainn::driverInstanceC *drvInst, processId_t tid, utf8Char *path
	)
{
	error_t		ret;

	/**	EXPLANATION:
	 * Newly started threads will not know which device they belong to.
	 * They will send a request to the main thread, asking it which device
	 * they should be servicing.
	 *
	 * The main thread will use the source thread's ID to search through all
	 * the devices hosted by this driver instance, and find out which
	 * device has a region with the source thread's TID.
	 **/
	for (uarch_t i=0; i<drvInst->nHostedDevices; i++)
	{
		utf8Char		*currDevPath=drvInst->hostedDevices[i];
		fplainn::deviceC	*currDev;

		ret = floodplainn.getDevice(currDevPath, &currDev);
		// Odd, but keep searching anyway.
		if (ret != ERROR_SUCCESS) { continue; };

		for (uarch_t j=0; j<drvInst->driver->nRegions; j++)
		{
			ubit16			idx;
			udi_init_context_t	*ic;

			if (currDev->instance->getRegionInfo(tid, &idx, &ic)
				!= ERROR_SUCCESS)
				{ continue; };

			strcpy8(path, currDevPath);
			return ERROR_SUCCESS;
		};
	};

	return ERROR_NOT_FOUND;
}

void __klibzbzcoreDriverPath(threadC *self)
{
//	fplainn::driverC				*driver;
	streamMemPtrC<messageStreamC::iteratorS>	msgIt;
	syscallbackC					*callback;
	fplainn::driverInstanceC			*drvInst;

	drvInst = self->parent->getDriverInstance();

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
		case MSGSTREAM_SUBSYSTEM_USER0:
			switch (msgIt->header.function)
			{
			case DRIVERPATH_U0_GET_THREAD_DEVICE_PATH:
				assert_fatal(
					getThreadDevicePath(
						drvInst,
						msgIt->header.sourceId,
						(utf8Char *)msgIt->header
							.privateData)
						== ERROR_SUCCESS);

				self->getTaskContext()->messageStream
					.postMessage(
					msgIt->header.sourceId,
					MSGSTREAM_SUBSYSTEM_USER0,
					DRIVERPATH_U0_GET_THREAD_DEVICE_PATH,
					NULL);

				break;

			default:
				break;
			};
			break;

		case MSGSTREAM_SUBSYSTEM_FLOODPLAINN:
			switch (msgIt->header.function)
			{
			case MSGSTREAM_FPLAINN_INSTANTIATE_DEVICE_REQ:
				floodplainnC::instantiateDeviceMsgS	*msg;
				error_t					reterr;

				msg = (floodplainnC::instantiateDeviceMsgS *)
					msgIt.get();

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
	fplainn::driverInstanceC		*drvInst;
	const udi_init_t			*driverInitInfo;

	self = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	selfProcess = (driverProcessC *)self->parent;
	drvInst = self->parent->getDriverInstance();

	driverInitInfo = drvInst->driver->driverInitInfo;
	printf(NOTICE LZBZCORE"driverPath1: Ret from loadRequirementsReq: %s "
		"(%d).\n",
		strerror(msgIt->header.error), msgIt->header.error);

	/**	EXPLANATION:
	 * In the userspace libzbzcore, we would have to make calls to the VFS
	 * to load and link all of the metalanguage libraries, as well as the
	 * driver module itself.
	 *
	 * However, since this is the kernel syslib, we can skip all of that.
	 **/
	// Set management op vector and scratch, etc.
	drvInst->setMgmtChannelInfo(
		driverInitInfo->primary_init_info->mgmt_ops,
		driverInitInfo->primary_init_info->mgmt_scratch_requirement,
		*driverInitInfo->primary_init_info->mgmt_op_flags);

	// Set child bind ops vectors.
	for (uarch_t i=0; i<drvInst->driver->nChildBops; i++)
	{
		udi_ops_init_t		*tmp;
		udi_ops_vector_t	*opsVector=NULL;

		for (tmp=driverInitInfo->ops_init_list;
			tmp->ops_idx != 0;
			tmp++)
		{
			if (tmp->ops_idx == drvInst->driver
				->childBops[i].opsIndex)
			{
				opsVector = tmp->ops_vector;
			};
		};

		if (opsVector == NULL)
		{
			printf(ERROR LZBZCORE"driverPath1: Failed to obtain "
				"ops vector addr for child bop with meta idx "
				"%d.\n",
				drvInst->driver->childBops[i].metaIndex);

			return;
		};

		drvInst->setChildBopVector(
			drvInst->driver->childBops[i].metaIndex, opsVector);
	};

	printf(NOTICE LZBZCORE"spawnDriver(%s, NUMA%d): done. PID 0x%x.\n",
		drvInst->driver->shortName,
		drvInst->bankId, selfProcess->id);

	self->parent->sendResponse(msgIt->header.error);
}

static void regionThreadEntry(void *);
static error_t instantiateDevice(floodplainnC::instantiateDeviceMsgS *msg)
{
	fplainn::deviceC		*dev;
	fplainn::driverC		*drv;
	threadC				*self;
	error_t				err;

	self = (threadC *)cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentTask();

	drv = self->parent->getDriverInstance()->driver;
	assert_fatal(floodplainn.getDevice(msg->path, &dev)
		== ERROR_SUCCESS);

	printf(NOTICE LZBZCORE"instantiateDevice(%s).\n", msg->path);

	/**	EXPLANATION:
	 * 1. Spawn region threads.
	 * 2. Spawn channels.
	 * 3. Should be fine from there for now.
	 **/
	// Now we create threads for all the regions.
	for (uarch_t i=0; i<drv->nRegions; i++)
	{
		threadC				*newThread;
		heapPtrC<udi_init_context_t>	rdata;

		// The +1 is to ensure the allocation size isn't 0.
		rdata = (udi_init_context_t *)new ubit8[
			drv->driverInitInfo->primary_init_info->rdata_size + 1];

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

			return err;
		};

		// Store the TID in the device's region metadata.
		dev->instance->setRegionInfo(
			drv->regions[i].index, newThread->getFullId(),
			rdata.release());
	};

	// Spawn the parent bind channel meanwhile.
	floodplainn.spawnChildBindChannel(
		CC"by-id/0", msg->path, CC"zbz_root",
		(udi_ops_vector_t *)0xF00115);

	return ERROR_SUCCESS;
}

static void regionThreadEntry(void *)
{
	threadC					*self;
	heapPtrC<messageStreamC::iteratorS>	msgIt;
	utf8Char				devPath[96];
	fplainn::deviceC			*dev;
	ubit16					regionIndex;
	udi_init_context_t			*rdata;
	error_t					err;
	udi_init_t				*init_info;

	self = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	init_info = self->parent->getDriverInstance()->driver->driverInitInfo;
	msgIt = new messageStreamC::iteratorS;
	if (msgIt == NULL)
	{
		printf(ERROR LZBZCORE"regionEntry: Failed to allocate msg "
			"iterator.\n");

		taskTrib.dormant(self->getFullId());
	};

	// Ask the main thread to tell us which device we are.
	self->getTaskContext()->messageStream.postMessage(
		self->parent->id, 0, DRIVERPATH_U0_GET_THREAD_DEVICE_PATH,
		devPath);

	// Get response.
	self->getTaskContext()->messageStream.pull(msgIt.get());
	err = floodplainn.getDevice(devPath, &dev);
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR LZBZCORE"regionEntry: Device path %s returned "
			"from main thread failed to resolve.\n",
			devPath);

		taskTrib.dormant(self->getFullId());
	};

	assert_fatal(
		dev->instance->getRegionInfo(
			self->getFullId(), &regionIndex, &rdata)
			== ERROR_SUCCESS);

	printf(NOTICE LZBZCORE"Device %s, region %d running: rdata 0x%x.\n",
		devPath, regionIndex, rdata);

	/* Next, we spawn all internal bind channels.
	 * internal_bind_ops meta_idx region_idx ops0_idx ops1_idx1 bind_cb_idx.
	 * Small sanity check here before moving on.
	 **/
	if (dev->driverInstance->driver->nInternalBops
		!= dev->driverInstance->driver->nRegions - 1)
	{
		printf(NOTICE LZBZCORE"regionEntry %d: driver has %d ibops, "
			"but %d regions.\n",
			regionIndex,
			dev->driverInstance->driver->nInternalBops,
			dev->driverInstance->driver->nRegions);

		taskTrib.dormant(self->getFullId());
	};

	// Now spawn this region's internal bind channel.
	if (regionIndex != 0)
	{
		ubit16			opsIndex0, opsIndex1;
		udi_ops_vector_t	*opsVector0=NULL, *opsVector1=NULL;
		udi_ops_init_t		*ops_info_tmp;

		err = floodplainn.getInternalBopVectorIndexes(
			regionIndex, &opsIndex0, &opsIndex1);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZBZCORE"regionEntry %d: Failed to get "
				"internal bops vector indexes.\n",
				regionIndex);

			taskTrib.dormant(self->getFullId());
		};

		ops_info_tmp = init_info->ops_init_list;
		for (; ops_info_tmp->ops_idx != 0; ops_info_tmp++)
		{
			if (ops_info_tmp->ops_idx == opsIndex0)
				{ opsVector0 = ops_info_tmp->ops_vector; };

			if (ops_info_tmp->ops_idx == opsIndex1)
				{ opsVector1 = ops_info_tmp->ops_vector; };
		};

		if (opsVector0 == NULL || opsVector1 == NULL)
		{
			printf(ERROR LZBZCORE"regionEntry %d: Failed to get "
				"ops vector addresses for intern. bind chan.\n",
				regionIndex);

			taskTrib.dormant(self->getFullId());
		};

		err = floodplainn.spawnInternalBindChannel(
			devPath, regionIndex, opsVector0, opsVector1);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZBZCORE"regionEntry %d: Failed to spawn "
				"internal bind channel because %s(%d).\n",
				regionIndex, strerror(err), err);

			taskTrib.dormant(self->getFullId());
		};
	};

	printf(NOTICE LZBZCORE"Region %d, dev %s: Entering event loop.\n",
		regionIndex, devPath);

	for (;;)
	{
		self->getTaskContext()->messageStream.pull(msgIt.get());
		printf(NOTICE"%s dev, region %d, got a message.\n",
			dev->driverInstance->driver->longName, regionIndex);
	};

	taskTrib.dormant(self->getFullId());
}
