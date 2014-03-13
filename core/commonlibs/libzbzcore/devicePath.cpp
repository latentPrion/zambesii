
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/memory>
#include <commonlibs/libzbzcore/libzbzcore.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/process.h>


namespace region
{
	void main(void *);

	error_t main_handleMgmtCall(
		floodplainnC::zudiMgmtCallMsgS *, fplainn::deviceC *, ubit16);

	error_t main_handleServiceCallAck();
	error_t main_handleMeiCallMsg();
}

namespace instantiateDevice
{
	error_t instantiateDevice1();
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
		heapPtrC<udi_init_context_t>	rdata;

		// The +1 is to ensure the allocation size isn't 0.
		rdata = (udi_init_context_t *)new ubit8[
			drv->driverInitInfo->primary_init_info->rdata_size + 1];

		// We pass the context to each thread.
		err = self->parent->spawnThread(
			&region::main, ctxt,
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

}


	// Spawn the parent bind channel meanwhile.
	return floodplainn.spawnChildBindChannel(
		CC"by-id/0", ctxt->devicePath, CC"zbz_root",
		(udi_ops_vector_t *)0xF00115);

	/* At this point, all regions are spawned, and all channels too.
	 * We now need to wait for the region threads to finish initializing.
	 * They will each send a completion notification to this main thread
	 * when they are done. We can exit and wait.
	 **/
}

void region::main(void *)
{
	threadC						*self;
	heapPtrC<messageStreamC::iteratorS>		msgIt;
	utf8Char					devPath[96];
	fplainn::deviceC				*dev;
	ubit16						regionIndex;
	udi_init_context_t				*rdata;
	error_t						err;
	udi_init_t					*init_info;
	__klzbzcore::driver::__kcall::callerContextS	*ctxt;

	self = (threadC *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	ctxt = (__klzbzcore::driver::__kcall::callerContextS *)
		self->getPrivateData();

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
		self->parent->id, 0,
		__klzbzcore::driver::MAINTHREAD_U0_GET_THREAD_DEVICE_PATH_REQ,
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

	/* Send a message to the main thread, letting it know that a region
	 * thead's initialization is now done.
	 **/
	self->getTaskContext()->messageStream.postMessage(
		self->parent->id, 0,
		__klzbzcore::driver::MAINTHREAD_U0_REGION_INIT_COMPLETE_IND,
		ctxt);

	printf(NOTICE LZBZCORE"Region %d, dev %s: Entering event loop.\n",
		regionIndex, devPath);

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

error_t region::main_handleMgmtCall(
	floodplainnC::zudiMgmtCallMsgS *msg,
	fplainn::deviceC *dev, ubit16 regionIndex
	)
{
	udi_init_t		*initInfo;
	udi_mgmt_ops_t		*mgmtOps;

	initInfo = dev->driverInstance->driver->driverInitInfo;
	mgmtOps = initInfo->primary_init_info->mgmt_ops;

	if (initInfo->primary_init_info->mgmt_scratch_requirement > 0)
	{
		msg->cb.ucb.gcb.scratch = new ubit8[
			initInfo->primary_init_info->mgmt_scratch_requirement];
	};

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
