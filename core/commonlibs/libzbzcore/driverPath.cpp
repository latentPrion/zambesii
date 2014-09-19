
#include <debug.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/memReservoir.h>
#include <kernel/common/process.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <commonlibs/libzbzcore/libzbzcore.h>


class __klzbzcore::driver::MainCb
: public _Callback<__kmainCbFn>
{
	Thread		*self;
	mainCbFn	*callerCb;

public:
	MainCb(__kmainCbFn *kcb, Thread *self, mainCbFn *callerCb)
	: _Callback<__kmainCbFn>(kcb),
	self(self), callerCb(callerCb)
	{}

	virtual void operator()(MessageStream::sHeader *msg)
		{ function(msg, self, callerCb); }
};

void __klzbzcore::driver::main(Thread *self, mainCbFn *callerCb)
{
	fplainn::DriverInstance		*drvInst;

	drvInst = self->parent->getDriverInstance();

	/**	EXPLANATION:
	 * This is essentially like a kind of udi_core lib.
	 *
	 * We will check all requirements and ensure they exist; then we'll
	 * proceed to spawn regions, etc. We need to do a message loop because
	 * we will have to do async calls to the ZUDI Index server, and possibly
	 * other subsystems.
	 **/

	floodplainn.zui.loadDriverRequirementsReq(
		new MainCb(&main1, self, callerCb));

	for (;FOREVER;)
	{
		MessageStream::sHeader		*iMsg;
		Callback			*callback;

		self->messageStream.pull(&iMsg);
		callback = (Callback *)iMsg->privateData;

		switch (iMsg->subsystem)
		{
		case MSGSTREAM_SUBSYSTEM_USER0:
			localService::handler(iMsg, drvInst, self);
			// Posted messages aren't garbage collected in kdomain.
			delete iMsg;
			break;

		case MSGSTREAM_SUBSYSTEM_ZUDI:
			switch (iMsg->function)
			{
			case MSGSTREAM_ZUDI___KCALL:
				__kcontrol::handler(
					(fplainn::Zudi::sKernelCallMsg *)iMsg);

				break;

			case MSGSTREAM_ZUDI_MGMT_CALL:
				/* Would be a call to main_handleMgmtCall()
				 * here, had I not cleaned up.
				 **/
				printf(FATAL"This would have called into "
					"udi_mgmt_ops_vector_t had it not been "
					"deprecated.\n");

				panic(ERROR_UNKNOWN);
				break;

			default:
				break; // NOP and fallthrough.
			};
			break;

		default:
			if (callback == NULL)
			{
				printf(WARNING LZBZCORE"driver:main: message "
					"with no callback continuation.\n");

				break;
			};

			(*callback)(iMsg);
			delete callback;
			break;
		};
	};
}

void __klzbzcore::driver::main1(
	MessageStream::sHeader *iMsg, Thread *self, mainCbFn *callerCb
	)
{
	DriverProcess			*selfProcess;
	fplainn::DriverInstance		*drvInst;
	const udi_init_t		*driverInitInfo;

	selfProcess = (DriverProcess *)self->parent;
	drvInst = self->parent->getDriverInstance();

	driverInitInfo = drvInst->driver->driverInitInfo;
	printf(NOTICE LZBZCORE"driverPath1: Ret from loadRequirementsReq: %s "
		"(%d).\n",
		strerror(iMsg->error), iMsg->error);

	if (iMsg->error != ERROR_SUCCESS)
		{ callerCb(self, iMsg->error); };

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

		callerCb(self, ERROR_INVALID_STATE);
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

			callerCb(self, ERROR_INVALID_STATE);
		};

		drvInst->setChildBopVector(
			drvInst->driver->childBops[i].metaIndex, opsVector);
	};

	// Done with the spawnDriver syscall at this point.
	printf(NOTICE LZBZCORE"spawnDriver(%s, NUMA%d): done. PID 0x%x.\n",
		drvInst->driver->shortName,
		drvInst->bankId, selfProcess->id);

	callerCb(self, ERROR_SUCCESS);
}

void __klzbzcore::driver::localService::handler(
	MessageStream::sHeader *iMsg, fplainn::DriverInstance *drvInst,
	Thread *self
	)
{
	switch (iMsg->function)
	{
	case REGION_INIT_SYNC_REQ:
		// Just a force-sync operation.
		self->messageStream.postMessage(
			iMsg->sourceId, 0, REGION_INIT_SYNC_REQ,
			iMsg->privateData);

		return;

	case GET_THREAD_DEVICE_PATH_REQ:
		/**	DEPRECATED.
		 * This message is sent by the region threads of a newly
		 * instantiated device while they are initializing. The threads
		 * send a message asking the main thread which device they
		 * pertain to. We respond with the name of their device.
		 **/
		assert_fatal(getThreadDevicePathReq(
			drvInst,
			iMsg->sourceId,
			(utf8Char *)iMsg->privateData)
			== ERROR_SUCCESS);

		self->messageStream.postMessage(
			iMsg->sourceId,
			0, GET_THREAD_DEVICE_PATH_REQ,
			NULL);

		return;

	case REGION_INIT_IND:
		fplainn::Zudi::sKernelCallMsg		*ctxt;

		ctxt = (fplainn::Zudi::sKernelCallMsg *)iMsg->privateData;
		regionInitInd(ctxt, iMsg->error);
		return;

	default:
		printf(WARNING LZBZCORE"drv: user0 Q: unknown message %d.\n",
			iMsg->function);
	};
}

error_t __klzbzcore::driver::localService::getThreadDevicePathReq(
	fplainn::DriverInstance *drvInst, processId_t tid, utf8Char *path
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
		fplainn::Device		*currDev;
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

void __klzbzcore::driver::localService::regionInitInd(
	fplainn::Zudi::sKernelCallMsg *ctxt, error_t error
	)
{
	fplainn::Device		*dev;
	uarch_t			totalNotifications,
				totalSuccesses;
	error_t			err;

	/**	EXPLANATION:
	 * Each time a region thread finishes intializing, it sends
	 * a message here. We track these and increment a counter
	 * until we know we have responses from all of the threads.
	 *
	 * Whether or not all threads have successfully initialized,
	 * we then call instantiateDeviceReq1().
	 **/
	err = floodplainn.getDevice(ctxt->path, &dev);
	if (err != ERROR_SUCCESS)
	{
		printf(WARNING LZBZCORE"regionInitInd(%s): device suddenly "
			"doesn't exist!\n",
			ctxt->path);

		/*	XXX:
		 * Should we send the ack response now? Certainly, the ACK will
		 * never be sent after here, because the counter for
		 * totalNotifications will never reach the required total number
		 * of regions.
		 *
		 * If we send it now, worst case scenario is that the kernel
		 * is unable to lookup the device path either. But best case
		 * is that we also get to use pass the "ctxt" object to the
		 * GC code in driver::instantiateDeviceAck.
		 *
		 * BUT, that is also bad because we will be delete()ing the ctxt
		 * block while it is potentially still being used by the other
		 * threads.
		 *
		 * If we never send it, we'll have the same scenario, except
		 * that we avoid the potential trampling case above. Probably
		 * best never to send it; when the driver process is closed,
		 * all memory will be reclaimed.
		 *
		 * Can also impose restrictions on removing devices such that
		 * they cannot be removed while the kernel is trying to
		 * instantiate them.
		 **/
		return;
	};

	if (error == ERROR_SUCCESS)
		{ dev->instance->nRegionsInitialized++; }
	else
		{ dev->instance->nRegionsFailed++; };

	totalSuccesses = dev->instance->nRegionsInitialized;
	totalNotifications = dev->instance->nRegionsInitialized
		+ dev->instance->nRegionsFailed;

	// If all regions haven't sent notifications yet, wait for them.
	if (totalNotifications != dev->driverInstance->driver->nRegions)
		{ return; };

	if (totalSuccesses != totalNotifications)
	{
		printf(NOTICE LZBZCORE"dev %s: not all regions "
			"initialized successfully!\n",
			ctxt->path);

		__klzbzcore::driver::__kcontrol::instantiateDeviceAck(
			ctxt, ERROR_INITIALIZATION_FAILURE);
	};

	/* If all regions succeeded, continue executing instantiateDevice at
	 * instantiateDevice1.
	 **/
	return __klzbzcore::driver::__kcontrol::instantiateDeviceReq1(ctxt);
}

void __klzbzcore::driver::__kcontrol::handler(fplainn::Zudi::sKernelCallMsg *msg)
{
	fplainn::Zudi::sKernelCallMsg		*copy;

	copy = new fplainn::Zudi::sKernelCallMsg(*msg);
	if (copy != NULL) {
		copy->set(msg->path, msg->command);
	}
	else
	{
		printf(ERROR LZBZCORE"drvPath: process__kcall: "
			"__KOP_INST_DEV: failed to alloc caller "
			"context block.\n\tCaller 0x%x, device %s.\n",
			msg->header.sourceId, msg->path);
	};


	switch (msg->command)
	{
	case fplainn::Zudi::sKernelCallMsg::CMD_INSTANTIATE_DEVICE:
		/* Sent by the kernel when it wishes to create a new instance
		 * of a device that is to be serviced by this driver process.
		 *
		 * We must either clone the request message, or else save the
		 * sourceTid and privateData members, so that when we are ready
		 * to call instantiateDeviceAck(), we can send the ack to the
		 * correct source thread, with its privateData info preserved.
		 **/
		if (copy == NULL)
		{
			/* Have to call the Floodplainn method directly.
			 * Local ack also deletes the message (double free).
			 **/
			floodplainn.zudi.instantiateDeviceAck(
				msg->header.sourceId, msg->path,
				ERROR_MEMORY_NOMEM,
				msg->header.privateData);

			return;
		};

		instantiateDeviceReq(copy);
		break;

	default:
		printf(WARNING LZBZCORE"drvPath: process__kcall: Unknown "
			"command %d.\n",
			msg->command);

		break;
	};
}

void __klzbzcore::driver::__kcontrol::instantiateDeviceAck(
	fplainn::Zudi::sKernelCallMsg *callerContext, error_t err
	)
{
	// TODO: Kill all the region threads that were spawned.
	floodplainn.zudi.instantiateDeviceAck(
		callerContext->header.sourceId,
		callerContext->path,
		err,
		callerContext->header.privateData);

	delete callerContext;
}

void __klzbzcore::driver::__kcontrol::instantiateDeviceReq(
	fplainn::Zudi::sKernelCallMsg *ctxt
	)
{
	fplainn::Device			*dev;
	fplainn::Driver			*drv;
	Thread				*self;
	error_t				err;

	self = (Thread *)cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentThread();

	drv = self->parent->getDriverInstance()->driver;
	err = floodplainn.getDevice(ctxt->path, &dev);
	if (err != ERROR_SUCCESS)
	{
		printf(FATAL LZBZCORE"instDevReq(%s): device path invalid.\n",
			ctxt->path);

		instantiateDeviceAck(ctxt, err); return;
	};

	printf(NOTICE LZBZCORE"instDevReq(%s).\n", ctxt->path);

	/**	EXPLANATION:
	 * 1. Spawn region threads.
	 * 2. Spawn channels.
	 * 3. Should be fine from there for now.
	 **/
	// Now we create threads for all the regions.
	for (uarch_t i=0; i<drv->nRegions; i++)
	{
		Thread				*newThread;
		HeapObj<udi_init_context_t>	rdata;

		// The +1 is to ensure the allocation size isn't 0.
		rdata = (udi_init_context_t *)new ubit8[
			drv->driverInitInfo->primary_init_info->rdata_size + 1];

		// We pass the context to each thread.
		err = self->parent->spawnThread(
			&::__klzbzcore::region::main, ctxt,
			NULL, (Thread::schedPolicyE)0, 0,
			0, &newThread);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZBZCORE"instDevReq: failed to "
				"spawn thread for region %d because "
				"%s(%d).\n",
				drv->regions[i].index,
				strerror(err), err);

			instantiateDeviceAck(ctxt, err); return;
		};

		// Store the TID in the device's region metadata.
		dev->instance->setRegionInfo(
			drv->regions[i].index, newThread->getFullId(),
			rdata.release());

		dev->instance->setThreadRegionPointer(
			newThread->getFullId());
	};

	/**	EXPLANATION:
	 * Execution moves on with the region threads. This thread will now
	 * block, waiting until all of the region threads have finished
	 * initializing, and have reported back to it.
	 **/

	printf(NOTICE LZBZCORE"instDevReq(%s): Done spawning all threads.\n",
		ctxt->path);
}

void __klzbzcore::driver::__kcontrol::instantiateDeviceReq1(
	fplainn::Zudi::sKernelCallMsg *ctxt
	)
{
	error_t				ret;
	fplainn::Device			*dev;
//	Thread				*self;

	/* This can only be reached after all the region threads have begun
	 * executing. This function itself is a mini sync-point between the main
	 * thread of logic and the region threads that it has spawned.
	 **/
//	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();
	ret = floodplainn.getDevice(ctxt->path, &dev);
	if (ret != ERROR_SUCCESS)
	{
		printf(FATAL LZBZCORE"instDevReq1 %s: Device has disappeared "
			"from FVFS while being instantiated.\n",
			ctxt->path);

		panic(ERROR_INVALID_RESOURCE_NAME);
	}

	// If the device has parents, spawn bind channels to its parents.
	for (uarch_t i=0; i<dev->getNParentTags(); i++)
	{
		fplainn::Device::sParentTag		*pTag;
		utf8Char				parentFullName[
			FVFS_TAG_NAME_MAXLEN + 1];

		pTag = dev->indexedGetParentTag(i);
		// Should never happen, though.
		if (pTag == NULL)
		{
			instantiateDeviceAck(ctxt, ERROR_NOT_FOUND);
			return;
		};

		ret = pTag->tag->getFullName(
			parentFullName, sizeof(parentFullName) - 1);

		if (ret != ERROR_SUCCESS)
		{
			printf(ERROR LZBZCORE"instDevReq1: %s: Unable to get "
				"fullname of\n\tparent %d (name %s).\n",
				ctxt->path, pTag->id, pTag->tag->getName());

			instantiateDeviceAck(ctxt, ret);
			return;
		};

		printf(NOTICE"instDevReq1 %s: Binding to parent %s, meta %s.\n",
			ctxt->path, parentFullName, "zbz_root");

		ret = floodplainn.zudi.spawnChildBindChannel(
			parentFullName, ctxt->path, CC"zbz_root",
			(udi_ops_vector_t *)0xF00115);

		if (ret != ERROR_SUCCESS)
		{
			printf(NOTICE LZBZCORE"instDevReq1 %s: Failed to spawn cbind "
				"chan\n",
				ctxt->path);

			instantiateDeviceAck(ctxt, ret); return;
		};
	};

	/**	EXPLANATION:
	 * At this point the parent bind channel has been spawned. All of the
	 * region threads have also spawned their child bind channel and are
	 * dormanting on their queues.
	 *
	 * All we need to do now is execute a metalanguage connect() to the
	 * "udi_mgmt" metalanguage of the device. With our connection handle,
	 * we can then send udi_usage_ind() followed by udi_channel_event_ind(),
	 * etc to all of the new device's internal bind channels, and finally
	 * launch the device.
	 *
	 * Since we connect to the device on behalf of the kernel, we must also
	 * be ready to intercept all kernel mgmt meta calls in our __kcontrol
	 * handler. So the sequence looks something like (paraphrased):
	 *
	 *	self->parent->floodplainnStream.connect(
	 *		ctxt->path, "udi_mgmt", &mgmtHandle);
	 *
	 *	mgmtCb = udi_cb_alloc(CB_IDX);
	 *	udi_usage_ind(mgmtCb, UDI_USAGE_NORMAL);
	 *	for (each; region; other; than; primary; region) {
	 *		udi_channel_event_ind()
	 *	};
	 *
	 *	udi_channel_event_ind(parent_bind_channel);
	 **/

	printf(NOTICE LZBZCORE"instDevReq1 %s: Done.\n", ctxt->path);
	instantiateDeviceAck(ctxt, ERROR_SUCCESS);
}

void __klzbzcore::driver::__kcontrol::instantiateDeviceReq2(
	fplainn::Zudi::sKernelCallMsg *ctxt
	)
{
printf(NOTICE LZBZCORE"instDevReq2: HERE! 0x%p\n", ctxt);
	return;
}

#if 0
void __klzbzcore::driver::main_handleMgmtCall(
	Floodplainn::sZudiMgmtCallMsg *msg
	)
{
	error_t		err;

	(void)err;
	switch (msg->mgmtOp)
	{
	case Floodplainn::sZudiMgmtCallMsg::MGMTOP_USAGE:
		err = __kcall::instantiateDevice2(
			(__klzbzcore::driver::__kcall::fplainn::Zudi::sKernelCallMsg *)
				msg->header.privateData);

		break;
	case Floodplainn::sZudiMgmtCallMsg::MGMTOP_ENUMERATE:
	case Floodplainn::sZudiMgmtCallMsg::MGMTOP_DEVMGMT:
	case Floodplainn::sZudiMgmtCallMsg::MGMTOP_FINAL_CLEANUP:
	default:
		printf(WARNING LZBZCORE"drvPath: handleMgmtCall: unknown "
			"management op %d\n",
			msg->mgmtOp);

		break;
	};
}
#endif
