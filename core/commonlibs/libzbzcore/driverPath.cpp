
#include <debug.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/memReservoir.h>
#include <kernel/common/process.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <commonlibs/libzbzcore/libzbzcore.h>


error_t __klzbzcore::driver::u0::getThreadDevicePath(
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
		fplainn::Device	*currDev;
		utf8Char		*currDevPath=
			drvInst->hostedDevices[i].get();

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

error_t __klzbzcore::driver::u0::regionInitInd(
	__klzbzcore::driver::__kcall::callerContextS *ctxt, ubit32 function
	)
{
	fplainn::Device		*dev;
	uarch_t				totalNotifications,
					totalSuccesses;

	/* This message is sent by the regions of a newly
	 * instantiated device when they have finished
	 * initializing. We could the number of regions that
	 * have finished, and when all are done, if they all
	 * finish successfully, we send a response message to
	 * the device to proceed with calling udi_usage_ind.
	 **/
	assert_fatal(
		floodplainn.getDevice(ctxt->devicePath, &dev) == ERROR_SUCCESS);

	if (function == __klzbzcore::driver::MAINTHREAD_U0_REGION_INIT_COMPLETE_IND)
		{ dev->instance->nRegionsInitialized++; }
	else
		{ dev->instance->nRegionsFailed++; };

	totalSuccesses = dev->instance->nRegionsInitialized;
	totalNotifications = dev->instance->nRegionsInitialized
		+ dev->instance->nRegionsFailed;

	// If all regions haven't sent notifications yet, wait for them.
	if (totalNotifications != dev->driverInstance->driver->nRegions)
		{ return ERROR_SUCCESS; };

	if (totalSuccesses != totalNotifications) {
		return ERROR_INITIALIZATION_FAILURE;
	};

	/* If all regions succeeded, continue executing instantiateDevice at
	 * instantiateDevice1.
	 **/
	return __klzbzcore::driver::__kcall::instantiateDevice1(ctxt);
}

void __klzbzcore::driver::main_handleU0Request(
	messageStreamC::sIterator *msgIt, fplainn::driverInstanceC *drvInst,
	Thread *self
	)
{
	error_t			err;

	switch (msgIt->header.function)
	{
	case MAINTHREAD_U0_SYNC:
		// Just a force-sync operation.
		self->getTaskContext()->messageStream.postMessage(
			msgIt->header.sourceId,
			0, MAINTHREAD_U0_SYNC, NULL);

		return;

	case MAINTHREAD_U0_GET_THREAD_DEVICE_PATH_REQ:
		/**	DEPRECATED.
		 * This message is sent by the region threads of a newly
		 * instantiated device while they are initializing. The threads
		 * send a message asking the main thread which device they
		 * pertain to. We respond with the name of their device.
		 **/
		assert_fatal(
			u0::getThreadDevicePath(
				drvInst,
				msgIt->header.sourceId,
				(utf8Char *)msgIt->header.privateData)
				== ERROR_SUCCESS);

		self->getTaskContext()->messageStream.postMessage(
			msgIt->header.sourceId,
			0, MAINTHREAD_U0_GET_THREAD_DEVICE_PATH_REQ,
			NULL);

		return;

	case MAINTHREAD_U0_REGION_INIT_COMPLETE_IND:
	case MAINTHREAD_U0_REGION_INIT_FAILURE_IND:
		__kcall::callerContextS		*ctxt;

		/* u0::regionInitInd() calls instantiateDevice1()
		 * if all the regions successfully initialize.
		 **/
		ctxt = (__kcall::callerContextS *)msgIt->header.privateData;
		err = u0::regionInitInd(
			ctxt, msgIt->header.function);

		if (err != ERROR_SUCCESS)
		{
			printf(NOTICE LZBZCORE"dev %s: not all regions "
				"initialized successfully! Killing\n",
				msgIt->header.privateData);

			floodplainn.instantiateDeviceAck(
				ctxt->sourceTid, ctxt->devicePath, err,
				ctxt->privateData);
		};

		return;

	default:
		printf(WARNING LZBZCORE"drv: user0 Q: unknown message %d.\n",
			msgIt->header.function);
	};
}

void __klzbzcore::driver::main_handleKernelCall(
	Floodplainn::zudiKernelCallMsgS *msg
	)
{
	error_t			err;

	switch (msg->command)
	{
	case Floodplainn::zudiKernelCallMsgS::CMD_INSTANTIATE_DEVICE:
		__kcall::callerContextS		*callerContext;

		/* Sent by the kernel when it wishes to create a new instance
		 * of a device that is to be serviced by this driver process.
		 *
		 * We must either clone the request message, or else save the
		 * sourceTid and privateData members, so that when we are ready
		 * to call instantiateDeviceAck(), we can send the ack to the
		 * correct source thread, with its privateData info preserved.
		 **/
		// TODO: remove, artifact.
		callerContext = new __kcall::callerContextS(msg);
		if (callerContext == NULL)
		{
			printf(ERROR LZBZCORE"drvPath: process__kcall: "
				"__KOP_INST_DEV: failed to alloc caller "
				"context block.\n\tCaller 0x%x, device %s.\n",
				msg->header.sourceId, msg->path);

			return;
		};

		err = __kcall::instantiateDevice(callerContext);
		if (err != ERROR_SUCCESS)
		{
			// TODO: Kill all the region threads that were spawned.
			floodplainn.instantiateDeviceAck(
				callerContext->sourceTid,
				callerContext->devicePath,
				err,
				callerContext->privateData);

			delete callerContext;
		};

		break;

	default:
		printf(WARNING LZBZCORE"drvPath: process__kcall: Unknown "
			"command %d.\n",
			msg->command);

		break;
	};
}

void __klzbzcore::driver::main_handleMgmtCall(
	Floodplainn::zudiMgmtCallMsgS *msg
	)
{
	error_t		err;

	(void)err;
	switch (msg->mgmtOp)
	{
	case Floodplainn::zudiMgmtCallMsgS::MGMTOP_USAGE:
		err = __kcall::instantiateDevice2(
			(__klzbzcore::driver::__kcall::callerContextS *)
				msg->header.privateData);

		break;
	case Floodplainn::zudiMgmtCallMsgS::MGMTOP_ENUMERATE:
	case Floodplainn::zudiMgmtCallMsgS::MGMTOP_DEVMGMT:
	case Floodplainn::zudiMgmtCallMsgS::MGMTOP_FINAL_CLEANUP:
	default:
		printf(WARNING LZBZCORE"drvPath: handleMgmtCall: unknown "
			"management op %d\n",
			msg->mgmtOp);

		break;
	};
}

static error_t driverPath0(Thread *self);
error_t __klzbzcore::driver::main(Thread *self)
{
	streamMemC<messageStreamC::sIterator>		msgIt;
	Syscallback					*callback;
	fplainn::driverInstanceC			*drvInst;
	error_t						ret;

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
		messageStreamC::sIterator;

	if (msgIt.get() == NULL)
	{
		printf(FATAL LZBZCORE"driverPath: proc 0x%x: Failed to alloc "
			"mem for msg iterator.\n",
			self->getFullId());

		return ERROR_MEMORY_NOMEM;
	};

	ret = driverPath0(self);
	if (ret != ERROR_SUCCESS)
	{
		printf(ERROR LZBZCORE"driverPath0 failed!\n");
		return ret;
	};

	for (;;)
	{
		self->getTaskContext()->messageStream.pull(msgIt.get());
		callback = (Syscallback *)msgIt->header.privateData;

		switch (msgIt->header.subsystem)
		{
		case MSGSTREAM_SUBSYSTEM_USER0:
			main_handleU0Request(msgIt.get(), drvInst, self);
			break;

		case MSGSTREAM_SUBSYSTEM_ZUDI:
			switch (msgIt->header.function)
			{
			case MSGSTREAM_FPLAINN_ZUDI___KCALL:
				main_handleKernelCall(
					(Floodplainn::zudiKernelCallMsgS *)
						msgIt.get());

				break;

			case MSGSTREAM_FPLAINN_ZUDI_MGMT_CALL:
				main_handleMgmtCall(
					(Floodplainn::zudiMgmtCallMsgS *)
						msgIt.get());

				break;

			default:
				break; // NOP and fallthrough.
			};
			break;

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

static syscallbackDataF driverPath1_wrapper;
static error_t driverPath0(Thread * /*self*/)
{
	zuiServer::loadDriverRequirementsReq(
		newSyscallback(&driverPath1_wrapper));

	return ERROR_SUCCESS;
}

static error_t driverPath1(messageStreamC::sIterator *msgIt, Thread *self);
static void driverPath1_wrapper(messageStreamC::sIterator *msgIt, void *)
{
	error_t			err;
	Thread			*self;

	self = (Thread *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentTask();

	err = driverPath1(msgIt, self);
	self->parent->sendResponse(err);
}

static error_t driverPath1(messageStreamC::sIterator *msgIt, Thread *self)
{
	driverProcessC				*selfProcess;
	fplainn::driverInstanceC		*drvInst;
	const udi_init_t			*driverInitInfo;

	selfProcess = (driverProcessC *)self->parent;
	drvInst = self->parent->getDriverInstance();

	driverInitInfo = drvInst->driver->driverInitInfo;
	printf(NOTICE LZBZCORE"driverPath1: Ret from loadRequirementsReq: %s "
		"(%d).\n",
		strerror(msgIt->header.error), msgIt->header.error);

	if (msgIt->header.error != ERROR_SUCCESS) {
		return msgIt->header.error;
	};

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

	if (drvInst->driver->nInternalBops != drvInst->driver->nRegions - 1)
	{
		printf(NOTICE LZBZCORE"driverPath1: %s: driver has %d ibops, "
			"but %d regions.\n",
			drvInst->driver->shortName,
			drvInst->driver->nInternalBops,
			drvInst->driver->nRegions);

		return ERROR_INVALID_STATE;
	};

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

		// Skip the MGMT meta's child bop for now.
		if (drvInst->driver->childBops[i].metaIndex == 0)
		{
			// drvInst->setChildBopVector(
			//	0, driverInitInfo->primary_init_info->mgmt_ops);

			continue;
		};

		if (opsVector == NULL)
		{
			printf(ERROR LZBZCORE"driverPath1: Failed to obtain "
				"ops vector addr for child bop with meta idx "
				"%d.\n",
				drvInst->driver->childBops[i].metaIndex);

			return ERROR_INVALID_STATE;
		};

		drvInst->setChildBopVector(
			drvInst->driver->childBops[i].metaIndex, opsVector);
	};

	// Done with the spawnDriver syscall at this point.
	printf(NOTICE LZBZCORE"spawnDriver(%s, NUMA%d): done. PID 0x%x.\n",
		drvInst->driver->shortName,
		drvInst->bankId, selfProcess->id);

	return ERROR_SUCCESS;
}

