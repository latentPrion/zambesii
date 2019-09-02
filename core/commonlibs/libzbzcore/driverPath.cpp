
#include <debug.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/memReservoir.h>
#include <kernel/common/process.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/floodplainn/initInfo.h>
#include <commonlibs/libzbzcore/libzbzcore.h>


class __klzbzcore::driver::MainCb
: public _Callback<__kmainCbFn>
{
	Thread		*self;
	mainCbFn	*callerCb;
	CachedInfo	*driverCachedInfo;

public:
	MainCb(
		__kmainCbFn *kcb, Thread *self, CachedInfo *driverCachedInfo,
		mainCbFn *callerCb)
	: _Callback<__kmainCbFn>(kcb),
	self(self), callerCb(callerCb), driverCachedInfo(driverCachedInfo)
	{}

	virtual void operator()(MessageStream::sHeader *msg)
		{ function(msg, driverCachedInfo, self, callerCb); }
};

error_t __klzbzcore::driver::CachedInfo::generateMetaInfoCache(void)
{
	fplainn::DriverInstance			*drvInst;

	drvInst = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
		->parent->getDriverInstance();

	for (uarch_t i=0; i<drvInst->driver->nRequirements; i++)
	{
		sMetaDescriptor			*tmpMetaDesc;
		const sMetaInitEntry		*tmpMetaInit;
		fplainn::Driver::sMetalanguage	*drvMeta;

		// NOTE: Could also skip metaIndex 0, to skip udi_mgmt entirely.

		tmpMetaInit = floodplainn.zudi.findMetaInitInfo(
			drvInst->driver->requirements[i].name);

		if (tmpMetaInit == NULL)
		{
			printf(ERROR LZBZCORE"genMetaInfoCache: Failed to get "
				"udi_meta_info for 'requires' line (%s).\n",
				drvInst->driver->requirements[i].name);

			return ERROR_NOT_FOUND;
		};

		drvMeta = drvInst->driver->getMetalanguage(
			drvInst->driver->requirements[i].name);

		if (drvMeta == NULL)
		{
			printf(ERROR LZBZCORE"genMetaInfoCache: Failed to get "
				"meta %s for driver %s.\n",
				drvInst->driver->requirements[i].name,
				drvInst->driver->shortName);

			return ERROR_NOT_FOUND;
		}

		tmpMetaDesc = new sMetaDescriptor(
			drvMeta->name,
			drvMeta->index,
			tmpMetaInit->udi_meta_info);

		if (tmpMetaDesc == NULL)
		{
			printf(ERROR LZBZCORE"genMetaInfoCache: Failed to "
				"alloc sMetaDescriptor for meta %s.\n",
				drvInst->driver->metalanguages[i].name);

			return ERROR_MEMORY_NOMEM;
		};

		if (metaInfos.insert(tmpMetaDesc) != ERROR_SUCCESS)
		{
			printf(ERROR LZBZCORE"genMetaInfoCache: Unknown error "
				"occured while attempting to insert entry into "
				"cache.\n");

			delete tmpMetaDesc;
			return ERROR_GENERAL;
		};
	};

	return ERROR_SUCCESS;
}

error_t __klzbzcore::driver::CachedInfo::generateMetaCbCache(void)
{
	udi_cb_init_t			*currCbInfo;
	List<sMetaCbInfo>::Iterator	it;
	sMetaCbInfo			*curr;

	assert_fatal(this->initInfo != NULL);

	/**	EXPLANATION:
	 * We run through the list multiple times, and for each
	 * meta_cb_idx we find, we search all of the entries for matches
	 * and get the highest scratch requirement for that meta_cb_idx.
	 *
	 * We'll use a 2-pass approach. First we run through all the cb_init
	 * entries, looking for unique meta_cb_num entries, and add each unique
	 * meta_cb_num entry to a list.
	 *
	 * In the second run, we take each meta_cb_num in the list and search
	 * through all the cb_init entries looking for matches, and seek the
	 * match with the highest scratch_requirement.
	 **/

	// Create list entries for each meta_cb_num.
	currCbInfo = initInfo->cb_init_list;
	for (; currCbInfo != NULL && currCbInfo->cb_idx != 0; currCbInfo++)
	{
		sbit8					foundMetaCb=0;
		sMetaCbInfo			*tmp;

		it = metaCbInfo.begin();
		for (; it != metaCbInfo.end(); ++it)
		{
			sMetaCbInfo			*curr = *it;

			if (curr->metaIndex == currCbInfo->meta_idx
				&& curr->metaCbNum == currCbInfo->meta_cb_num)
			{
				foundMetaCb = 1;
				break;
			};
		};

		if (foundMetaCb) { continue; };

		/* If we reached here, it means we have yet to allocate
		 * a list entry for this meta_cb_num.
		 **/
		tmp = new sMetaCbInfo(
			currCbInfo->meta_idx,
			currCbInfo->meta_cb_num,
			currCbInfo->scratch_requirement);

		if (tmp == NULL)
		{
			printf(ERROR LZBZCORE"genMetaCbScratchCache: Failed to "
				"alloc meta scratch cache metadata for "
				"meta_idx %d, meta_cb_num %d.\n",
				currCbInfo->meta_idx, currCbInfo->meta_cb_num);

			return ERROR_MEMORY_NOMEM;
		};

		metaCbInfo.insert(tmp);
	};

	/* Now for the 2nd pass: find the highest scratch_requirement for each
	 * meta_cb_num.
	 **/
	it = metaCbInfo.begin();
	for (; it != metaCbInfo.end(); ++it)
	{
		curr = *it;

		currCbInfo = initInfo->cb_init_list;
		for (; currCbInfo != NULL && currCbInfo->cb_idx != 0;
			currCbInfo++)
		{
			// Only compare scratch sizes if the meta_cb_nums match.
			if (currCbInfo->meta_idx != curr->metaIndex
				|| currCbInfo->meta_cb_num != curr->metaCbNum)
				{ continue; };

			/* If the current cb_info has a higher
			 * scratch_requirement then the scratchRequirement
			 * currently recorded for this meta_cb_num, we record
			 * the new scratch_requirement as the highest.
			 **/
			if (currCbInfo->scratch_requirement
				> curr->scratchRequirement)
			{
				curr->scratchRequirement =
					currCbInfo->scratch_requirement;
			};
		};
	};

	/* Third pass:
	 * Link each meta_cb_num to its visible_layout. This will be used to
	 * determine the total size of each meta_cb_num, as well as where in
	 * the control block the UDI_DL_INLINE_DRIVER_TYPED object is pointed
	 * to, if any such inline member exists in the meta_cb_num.
	 *
	 * This will enable us to quickly service udi_cb_alloc() calls.
	 **/
	it = metaCbInfo.begin();
	for (; it != metaCbInfo.end(); ++it)
	{
		sMetaDescriptor			*cbMetaDesc;
		udi_mei_ops_vec_template_t	*currVecTemplate;

		curr = *it;
		// Get the meta descriptor for this cb's meta_idx.
		cbMetaDesc = getMetaDescriptor(curr->metaIndex);
		if (cbMetaDesc == NULL)
		{
			printf(ERROR LZBZCORE"Failed to get metadesc for meta "
				"index %d (%s)\n",
				curr->metaIndex);

			return ERROR_NOT_FOUND;
		}

		/**	EXPLANATION:
		 * Basically want to run through each udi_mei_op_template_t
		 * until we find one that describes an op that has the same
		 * meta_cb_num as the sMetaCbInfo that we're trying to fill out
		 * the layout information for.
		 *
		 * We only use the values from the FIRST such match that we find
		 * and then exit. It is assumed that there will be no
		 * inconsistencies in the metalanguage library's visible_layout
		 * descriptors for any particular meta_cb_num.
		 *
		 **	ALTERNATIVE:
		 * The alternative is to go through every such match, and
		 * compare it to every other, and verify consistency. This would
		 * enable us to catch inconsistencies, and warn the metalanguage
		 * library's developer to fix them.
		 *
		 * But inconsistencies in visible_layouts for any meta_cb_num
		 * will not cause the kernel to be unable to run the driver;
		 * they will just make the driver's control blocks' sizes be
		 * unequal to what the driver expected. But the kernel and the
		 * Zbz UDI environment will remain unaffected, largely.
		 *
		 * Perhaps we can do these extra checks later, but for now, I'm
		 * taking the quick approach.
		 **/
		currVecTemplate = cbMetaDesc->initInfo->ops_vec_template_list;
		for (; currVecTemplate != NULL
			&& currVecTemplate->meta_ops_num != 0;
			currVecTemplate++)
		{
			udi_mei_op_template_t	*currOpTemplate;

			currOpTemplate = currVecTemplate->op_template_list;
			for (; currOpTemplate != NULL
				&& currOpTemplate->op_name != NULL;
				currOpTemplate++)
			{
				status_t		sizeTmp;

				if (currOpTemplate->meta_cb_num
					!= curr->metaCbNum)
					{ continue; };

				curr->visibleLayout = currOpTemplate
					->visible_layout;

				/* We can also precalculate the visible size
				 * here if we wish.
				 **/
				sizeTmp = fplainn::sChannelMsg
					::zudi_layout_get_size(
						currOpTemplate->visible_layout,
						1);

				if (sizeTmp < 0)
					{ return ERROR_INVALID_FORMAT; };

				curr->visibleSize = sizeTmp;
			};
		};
	};

	return ERROR_SUCCESS;
}

namespace __klzbzcore {
namespace region {
namespace channel {
namespace mgmt
{
	namespace layouts
	{
		udi_layout_t		channel_event_cb[] =
		{
			UDI_DL_UBIT8_T,
			  /* Union. We just instruct env to
			   * copy max bytes.
			   **/
			  UDI_DL_INLINE_UNTYPED,
			  UDI_DL_UBIT8_T,
			  UDI_DL_INLINE_UNTYPED,
			UDI_DL_END
		};

		udi_layout_t		*visible[] =
		{
			channel_event_cb,
			// These 4 NULLs get filled out when ZUM is initialized.
			NULL, NULL, NULL, NULL
		};

		udi_layout_t		channel_event_ind[] =
			{ UDI_DL_END };

		udi_layout_t		*marshal[] =
		{
			channel_event_ind,
			// These 4 NULLs get filled out when ZUM is initialized.
			NULL, NULL, NULL, NULL
		};
	}
}
}
}
}

void __klzbzcore::driver::sMetaCbInfo::dump(utf8Char *metaName)
{
	printf(NOTICE"meta_idx %d (%s), cb_num %d: "
		"scratch %dB, visible %dB.\n",
		metaIndex, ((metaName == NULL) ? CC"name unknown" : metaName),
		metaCbNum,
		scratchRequirement, visibleSize);

	fplainn::sChannelMsg::dumpLayout(visibleLayout);
}

void __klzbzcore::driver::main(Thread *self, mainCbFn *callerCb)
{
	fplainn::DriverInstance		*drvInst;
	CachedInfo			*cache;
	error_t				err0, err1;
	const sDriverInitEntry		*drvInitEntry;

	drvInst = self->parent->getDriverInstance();

	/**	EXPLANATION:
	 * This is essentially like a kind of udi_core lib.
	 *
	 * We will check all requirements and ensure they exist; then we'll
	 * proceed to spawn regions, etc. We need to do a message loop because
	 * we will have to do async calls to the ZUDI Index server, and possibly
	 * other subsystems.
	 **/

	/* Allocate the driver info cache. This cache will actually just be
	 * passed along as a local variable from callback to callback, and will
	 * not be a global.
	 **/
	drvInitEntry = floodplainn.zudi.findDriverInitInfo(
		drvInst->driver->shortName);

	if (drvInitEntry == NULL)
	{
		printf(ERROR LZBZCORE"driver:main %s: failed to find "
			"init_info for driver %s.\n",
			drvInst->driver->shortName, drvInst->driver->shortName);

		callerCb(self, ERROR_NOT_FOUND); return;
	};

	cache = new CachedInfo(drvInitEntry->udi_init_info);
	if (cache == NULL || cache->initialize() != ERROR_SUCCESS)
	{
		printf(ERROR LZBZCORE"driver:main %s: Failed to allocate or "
			"initialize driver info cache.\n",
			drvInst->driver->shortName);

		callerCb(self, ERROR_MEMORY_NOMEM); return;
	};

	err1 = cache->generateMetaInfoCache();
	err0 = cache->generateMetaCbCache();
	if (err0 != ERROR_SUCCESS || err1 != ERROR_SUCCESS)
	{
		printf(ERROR LZBZCORE"driver:main %s: Failed to generate "
			"meta_cb_num scratch requirement cache.\n"
			"\tOR, failed to generate metaInfo descriptor cache.\n",
			drvInst->driver->shortName);

		callerCb(self, ERROR_INITIALIZATION_FAILURE); return;
	};

	drvInst->setCachedInfo(cache);

	floodplainn.zui.loadDriverRequirementsReq(
		new MainCb(&main1, self, cache, callerCb));

	for (;FOREVER;)
	{
		MessageStream::sHeader		*iMsg;
		Callback			*callback;

		self->messageStream.pull(&iMsg);
		callback = (Callback *)iMsg->privateData;

		switch (iMsg->subsystem)
		{
		case MSGSTREAM_SUBSYSTEM_USER0:
			localService::handler(
				(MessageStream::sPostMsg *)iMsg,
				drvInst, cache, self);

			break;

		case MSGSTREAM_SUBSYSTEM_ZUDI:
			switch (iMsg->function)
			{
			case MSGSTREAM_ZUDI___KCALL:
				__kcontrol::handler(
					(fplainn::Zudi::sKernelCallMsg *)iMsg,
					cache);

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

		delete iMsg;
	};
}

void __klzbzcore::driver::main1(
	MessageStream::sHeader *iMsg, CachedInfo *cache, Thread *self,
	mainCbFn *callerCb
	)
{
	DriverProcess			*selfProcess;
	fplainn::DriverInstance		*drvInst;
	udi_ops_init_t			*curr;
	error_t				err;

	selfProcess = (DriverProcess *)self->parent;
	drvInst = self->parent->getDriverInstance();

	printf(NOTICE LZBZCORE"driverPath1: Ret from loadRequirementsReq: %s "
		"(%d).\n",
		strerror(iMsg->error), iMsg->error);

	if (iMsg->error != ERROR_SUCCESS)
		{ callerCb(self, iMsg->error); return; };

	/**	EXPLANATION:
	 * In the userspace libzbzcore, we would have to make calls to the VFS
	 * to load and link all of the metalanguage libraries, as well as the
	 * driver module itself.
	 *
	 * However, since this is the kernel syslib, we can skip all of that.
	 **/
	if (drvInst->driver->nInternalBops != drvInst->driver->nRegions - 1)
	{
		printf(NOTICE LZBZCORE"driverPath1: %s: driver has %d ibops, "
			"but %d regions.\n",
			drvInst->driver->shortName,
			drvInst->driver->nInternalBops,
			drvInst->driver->nRegions);

		callerCb(self, ERROR_INVALID_STATE); return;
	};

	// Set child bind ops vectors.
	for (uarch_t i=0; i<drvInst->driver->nChildBops; i++)
	{
		fplainn::DriverInit	drvInfoParser(cache->initInfo);
		udi_ops_init_t		*tmp;
		udi_ops_vector_t	*opsVector=NULL;

		if (drvInst->driver->childBops[i].metaIndex == 0)
		{
			opsVector = (udi_ops_vector_t *)cache->initInfo
				->primary_init_info->mgmt_ops;

			if (opsVector == NULL)
			{
				printf(ERROR LZBZCORE"driver:main1 %s: No mgmt "
					"ops vector specified in init info.\n",
					drvInst->driver->shortName);

				callerCb(self, ERROR_INVALID_STATE); return;
			};
		}
		else
		{
			tmp = drvInfoParser.getOpsInit(
				drvInst->driver->childBops[i].opsIndex);

			if (tmp == NULL)
			{
				printf(ERROR LZBZCORE"driver:main1 %s: Failed to "
					"obtain ops vector addr for child bop "
					"with meta idx %d.\n",
					drvInst->driver->shortName,
					drvInst->driver->childBops[i]
						.metaIndex);

				callerCb(self, ERROR_INVALID_STATE); return;
			};

			opsVector = tmp->ops_vector;
		};

		drvInst->setChildBopVector(
			drvInst->driver->childBops[i].metaIndex, opsVector);
	};

	// Associate ops vector indexes with meta indexes for the kernel.
	for (curr = cache->initInfo->ops_init_list;
		curr != NULL && curr->ops_idx != 0;
		curr++)
	{
		err = drvInst->driver->addOrModifyOpsInit(
			curr->ops_idx, curr->meta_idx);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR "Failed to add ops_idx %d from ops_init_t "
				"to driver's kernel metadata.\n",
				curr->ops_idx);

			callerCb(self, err); return;
		}
	}

	// Done with the spawnDriver syscall at this point.
	printf(NOTICE LZBZCORE"spawnDriver(%s, NUMA%d): done. PID %x.\n",
		drvInst->driver->shortName,
		drvInst->bankId, selfProcess->id);

	callerCb(self, ERROR_SUCCESS);
}

void __klzbzcore::driver::localService::handler(
	MessageStream::sPostMsg *iMsg, fplainn::DriverInstance *drvInst,
	__klzbzcore::driver::CachedInfo *cache, Thread *self
	)
{
	switch (iMsg->header.function)
	{
	case REGION_INIT_SYNC_REQ:
		// Just a force-sync operation. No actual data to be sent.
		self->messageStream.postMessage(
			iMsg->header.sourceId, 0, REGION_INIT_SYNC_REQ,
			iMsg->data,
			iMsg->header.privateData);

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
			iMsg->header.sourceId,
			(utf8Char *)iMsg->data)
			== ERROR_SUCCESS);

		self->messageStream.postMessage(
			iMsg->header.sourceId,
			0, GET_THREAD_DEVICE_PATH_REQ,
			iMsg->data, iMsg->header.privateData);

		return;

	case REGION_INIT_IND:
		fplainn::Zudi::sKernelCallMsg		*ctxt;

		ctxt = (fplainn::Zudi::sKernelCallMsg *)iMsg->header.privateData;
		regionInitInd(ctxt, iMsg->header.error);
		return;

	case GET_DRIVER_CACHED_INFO:
		self->messageStream.postMessage(
			iMsg->header.sourceId, 0, GET_DRIVER_CACHED_INFO,
			cache, iMsg->header.privateData);

		return;

	default:
		printf(WARNING LZBZCORE"drv: user0 Q: unknown message %d.\n",
			iMsg->header.function);
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

			if (currDev->instance->getRegionInfo(tid, &idx)
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

		return;
	};

	/* If all regions succeeded, continue executing instantiateDevice at
	 * instantiateDevice1.
	 **/
	__klzbzcore::driver::__kcontrol::instantiateDeviceReq1(ctxt);
	return;
}

void __klzbzcore::driver::__kcontrol::handler(
	fplainn::Zudi::sKernelCallMsg *msg,
	__klzbzcore::driver::CachedInfo *cache
	)
{
	fplainn::Zudi::sKernelCallMsg		*copy;

	(void)cache;

	copy = new fplainn::Zudi::sKernelCallMsg(*msg);
	if (copy != NULL) {
		copy->set(msg->path, msg->command);
	}
	else
	{
		printf(ERROR LZBZCORE"drvPath: process__kcall: "
			"__KOP_INST_DEV: failed to alloc caller "
			"context block.\n\tCaller %x, device %s.\n",
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

		// We pass the context to each thread.
		err = self->parent->spawnThread(
			&::__klzbzcore::region::main, ctxt,
			NULL, (Thread::schedPolicyE)0, 0,
//			SPAWNTHREAD_FLAGS_DORMANT,
			0,
			&newThread);

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
			drv->regions[i].index, newThread->getFullId());

		dev->instance->setThreadRegionPointer(
			newThread->getFullId());

//		taskTrib.wake(newThread);
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
printf(NOTICE LZBZCORE"instDevReq2: HERE! %p\n", ctxt);
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
