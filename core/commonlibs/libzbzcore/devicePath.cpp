
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/memory>
#include <commonlibs/libzbzcore/libzbzcore.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/process.h>


namespace region
{
	void main_wrapper(void *);
	error_t main(
		threadC *self,
		__klzbzcore::driver::__kcall::callerContextS *ctxt);

	error_t main_handleMgmtCall(
		floodplainnC::zudiMgmtCallMsgS *, fplainn::deviceC *, ubit16);

	error_t main_handleServiceCallAck();
	error_t main_handleMeiCallMsg();
}

error_t __klzbzcore::driver::__kcall::instantiateDevice(callerContextS *ctxt)
{
	fplainn::deviceC			*dev;
	fplainn::driverC			*drv;
	threadC					*self;
	error_t					err;

	self = (threadC *)cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentTask();

	drv = self->parent->getDriverInstance()->driver;
	assert_fatal(floodplainn.getDevice(ctxt->devicePath, &dev)
		== ERROR_SUCCESS);

	printf(NOTICE LZBZCORE"instantiateDevice(%s).\n", ctxt->devicePath);

	/**	EXPLANATION:
	 * 1. Spawn region threads.
	 * 2. Spawn channels.
	 * 3. Should be fine from there for now.
	 **/
	// Now we create threads for all the regions.
	for (uarch_t i=0; i<drv->nRegions; i++)
	{
		threadC				*newThread;
		heapObjC<udi_init_context_t>	rdata;

		// The +1 is to ensure the allocation size isn't 0.
		rdata = (udi_init_context_t *)new ubit8[
			drv->driverInitInfo->primary_init_info->rdata_size + 1];

		// We pass the context to each thread.
		err = self->parent->spawnThread(
			&region::main_wrapper, ctxt,
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

	/**	EXPLANATION:
	 * Execution moves on with the region threads. This thread will now
	 * block, waiting until all of the region threads have finished
	 * initializing, and have reported back to it.
	 **/
	return ERROR_SUCCESS;
}

error_t __klzbzcore::driver::__kcall::instantiateDevice1(callerContextS *ctxt)
{
	error_t		ret;

	/* This can only be reached after all the region threads have begun
	 * executing. This function itself is a mini sync-point between the main
	 * thread of logic and the region threads that it has spawned.
	 **/

	// Spawn the parent bind channel.
	ret = floodplainn.spawnChildBindChannel(
		CC"by-id/0", ctxt->devicePath, CC"zbz_root",
		(udi_ops_vector_t *)0xF00115);

	if (ret != ERROR_SUCCESS)
	{
		printf(NOTICE LZBZCORE"dev %s: Failed to spawn cbind chan\n",
			ctxt->devicePath);

		return ret;
	};

	/* At this point, all regions are spawned, and all channels too.
	 * We now need to wait for the region threads to finish initializing.
	 * They will each send a completion notification to this main thread
	 * when they are done. We can exit and wait.
	 **/
	floodplainn.udi_usage_ind(
		ctxt->devicePath, UDI_RESOURCES_NORMAL, ctxt);

printf(NOTICE"Just called floodplainn.usage_ind on dev %s.\n",
	ctxt->devicePath);
	return ERROR_SUCCESS;
}

error_t __klzbzcore::driver::__kcall::instantiateDevice2(callerContextS *ctxt)
{
printf(NOTICE LZBZCORE"HERE! 0x%p\n", ctxt);
	return ERROR_SUCCESS;
}

void region::main_wrapper(void *)
{
	error_t						err;
	threadC						*self;
	__klzbzcore::driver::__kcall::callerContextS	*ctxt;

	self = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	ctxt = (__klzbzcore::driver::__kcall::callerContextS *)
		self->getPrivateData();

	err = region::main(self, ctxt);
	if (err != ERROR_SUCCESS)
	{
		// Send negative ack, exit.
		floodplainn.instantiateDeviceAck(
			ctxt->sourceTid, ctxt->devicePath,
			err, ctxt->privateData);

		taskTrib.dormant(self->getFullId());
	};
}

error_t region::main(
	threadC *self, __klzbzcore::driver::__kcall::callerContextS *ctxt
	)
{
	heapObjC<messageStreamC::iteratorS>		msgIt;
	fplainn::deviceC				*dev;
	ubit16						regionIndex;
	udi_init_context_t				*rdata;
	error_t						err;
	udi_init_t					*init_info;

	init_info = self->parent->getDriverInstance()->driver->driverInitInfo;
	msgIt = new messageStreamC::iteratorS;
	if (msgIt == NULL)
	{
		printf(ERROR LZBZCORE"regionEntry: Failed to allocate msg "
			"iterator.\n");

		return ERROR_MEMORY_NOMEM;
	};

	err = floodplainn.getDevice(ctxt->devicePath, &dev);
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR LZBZCORE"regionEntry: Device path %s returned "
			"from main thread failed to resolve.\n",
			ctxt->devicePath);

		return ERROR_INVALID_RESOURCE_NAME;
	};

	/* Force a sync operation with the main thread. We can't continue until
	 * we can guarantee that the main thread has filled out our region
	 * information in our device-instance object.
	 *
	 * Otherwise it will be filled with garbage and we will read garbage
	 * data.
	 **/
	self->getTaskContext()->messageStream.postMessage(
		self->parent->id, 0, __klzbzcore::driver::MAINTHREAD_U0_SYNC,
		ctxt->devicePath);

	// Wait for the sync response before continuing.
	self->getTaskContext()->messageStream.pull(msgIt.get());
	assert_fatal(
		dev->instance->getRegionInfo(
			self->getFullId(), &regionIndex, &rdata)
			== ERROR_SUCCESS);

	printf(NOTICE LZBZCORE"Device %s, region %d running: rdata 0x%x.\n",
		ctxt->devicePath, regionIndex, rdata);

	/* Next, we spawn all internal bind channels.
	 * internal_bind_ops meta_idx region_idx ops0_idx ops1_idx1 bind_cb_idx.
	 * Small sanity check here before moving on.
	 **/

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

			return ERROR_INVALID_ARG_VAL;
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

			return ERROR_INVALID_ARG_VAL;
		};

		err = floodplainn.spawnInternalBindChannel(
			ctxt->devicePath, regionIndex, opsVector0, opsVector1);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZBZCORE"regionEntry %d: Failed to spawn "
				"internal bind channel because %s(%d).\n",
				regionIndex, strerror(err), err);

			return err;
		};
	};

	/* Send a message to the main thread, letting it know that a region
	 * thead's initialization is now done.
	 **/
	self->getTaskContext()->messageStream.postMessage(
		self->parent->id, 0,
		__klzbzcore::driver::MAINTHREAD_U0_REGION_INIT_COMPLETE_IND,
		ctxt);

	printf(NOTICE LZBZCORE"Region %d, dev %s: Entering event loop.\n",
		regionIndex, ctxt->devicePath);

	for (;FOREVER;)
	{
		self->getTaskContext()->messageStream.pull(msgIt.get());
		printf(NOTICE"%s dev, region %d, got a message.\n"
			"\tsubsys %d, func %d, sourcetid 0x%x targettid 0x%x\n",
			dev->driverInstance->driver->longName, regionIndex,
			msgIt->header.subsystem, msgIt->header.function,
			msgIt->header.sourceId, msgIt->header.targetId);

		switch (msgIt->header.subsystem)
		{
		case MSGSTREAM_SUBSYSTEM_ZUDI:
			switch (msgIt->header.function)
			{
			case MSGSTREAM_FPLAINN_ZUDI_MGMT_CALL:
				region::main_handleMgmtCall(
					(floodplainnC::zudiMgmtCallMsgS *)
						msgIt.get(),
					dev, regionIndex);

				break;
			};

			break;

		default:
			printf(NOTICE"dev %s, rgn %d: unknown subsystem %d\n",
				msgIt->header.subsystem);
		};
	};

	taskTrib.dormant(self->getFullId());
}

struct udi_mgmt_contextBlockS
{
	udi_mgmt_contextBlockS(floodplainnC::zudiMgmtCallMsgS *msg)
	:
	privateData(msg->header.privateData), sourceTid(msg->header.sourceId)
	{
		strncpy8(this->devicePath, msg->path, FVFS_PATH_MAXLEN);
	}

	void		*privateData;
	processId_t	sourceTid;
	utf8Char	devicePath[FVFS_PATH_MAXLEN];
};

error_t region::main_handleMgmtCall(
	floodplainnC::zudiMgmtCallMsgS *msg,
	fplainn::deviceC *dev, ubit16 regionIndex
	)
{
	udi_init_t		*initInfo;
	udi_mgmt_ops_t		*mgmtOps;
	udi_mgmt_contextBlockS	*contextBlock;

	initInfo = dev->driverInstance->driver->driverInitInfo;
	mgmtOps = initInfo->primary_init_info->mgmt_ops;

	contextBlock = new udi_mgmt_contextBlockS(msg);
	if (contextBlock == NULL)
	{
		return ERROR_MEMORY_NOMEM;
	};

	if (initInfo->primary_init_info->mgmt_scratch_requirement > 0)
	{
		msg->cb.ucb.gcb.scratch = new ubit8[
			initInfo->primary_init_info->mgmt_scratch_requirement];

		if (msg->cb.ucb.gcb.scratch == NULL)
		{
			delete contextBlock;
			return ERROR_MEMORY_NOMEM;
		};
	};

	// Very critical continuation.
	msg->cb.ucb.gcb.initiator_context = contextBlock;

	switch (msg->mgmtOp)
	{
	case floodplainnC::zudiMgmtCallMsgS::MGMTOP_USAGE:
		printf(NOTICE"%s, rgn %d: udi_usage_ind call received!\n",
			msg->path, regionIndex);

		mgmtOps->usage_ind_op(&msg->cb.ucb, msg->usageLevel);
		break;

	default:
		printf(NOTICE"dev %s, rgn %d: unknown command %d!\n",
			msg->path, regionIndex);

		break;
	};

	delete[] (ubit8 *)msg->cb.ucb.gcb.scratch;
	return ERROR_SUCCESS;
}

void udi_usage_res(udi_usage_cb_t *cb)
{
	udi_mgmt_contextBlockS		*contextBlock;

	/**	EXPLANATION:
	 * Basically, the driver will eventually call udi_usage_res(). This is
	 * the entry point it will call into. In here, we execute a syscall,
	 * essentially.
	 *
	 *	FIXME:
	 * In the same vein as the initiator_context comments in udi_usage_ind,
	 * we are dependent on the pointer stored in initiator_context in this
	 * function too.
	 *
	 * We just extract the required information from the control block, and
	 * call the kernel with the _res operation.
	 **/
printf(NOTICE"in res\n");
	contextBlock = (udi_mgmt_contextBlockS *)cb->gcb.initiator_context;

	floodplainn.udi_usage_res(
		contextBlock->devicePath,
		contextBlock->sourceTid, contextBlock->privateData);

	delete contextBlock;
}
