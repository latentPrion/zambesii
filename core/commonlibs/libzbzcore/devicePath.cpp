
#include <__kstdlib/callback.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/stdlib.h>
#include <__kstdlib/__kcxxlib/memory>
#include <commonlibs/libzbzcore/libzbzcore.h>
#include <commonlibs/libzbzcore/libzudi.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/process.h>


class __klzbzcore::region::MainCb
: public _Callback<__kmainCbFn>
{
	fplainn::Zudi::sKernelCallMsg		*ctxtMsg;
	Thread					*self;
	fplainn::Device				*dev;
	__klzbzcore::driver::CachedInfo		**drvInfoCache;
	lzudi::sRegion				*r;

public:
	MainCb(
		__kmainCbFn *kcb,
		fplainn::Zudi::sKernelCallMsg *ctxtMsg,
		Thread *self,
		__klzbzcore::driver::CachedInfo **drvInfoCache,
		lzudi::sRegion *r,
		fplainn::Device *dev)
	: _Callback<__kmainCbFn>(kcb),
	ctxtMsg(ctxtMsg), self(self), dev(dev), drvInfoCache(drvInfoCache), r(r)
	{}

	virtual void operator()(MessageStream::sHeader *msg)
		{ function(msg, ctxtMsg, self, drvInfoCache, r, dev); }
};

static void postRegionInitInd(
	Thread *self, fplainn::Zudi::sKernelCallMsg *ctxt, error_t err
	)
{
	self->messageStream.postMessage(
		self->parent->id, 0,
		__klzbzcore::driver::localService::REGION_INIT_IND,
		NULL, ctxt, err);
}

void __klzbzcore::region::main(void *)
{
	lzudi::sRegion				r;
	MessageStream::sHeader			*iMsg;
	fplainn::Device				*dev;
	error_t					err;
	Thread					*self;
	fplainn::Zudi::sKernelCallMsg		*ctxt;
	__klzbzcore::driver::CachedInfo		*drvInfoCache=NULL;

	self = (Thread *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	ctxt = (fplainn::Zudi::sKernelCallMsg *)self->getPrivateData();


	err = floodplainn.getDevice(ctxt->path, &dev);
	if (err != ERROR_SUCCESS)
	{
		printf(ERROR LZBZCORE"regionEntry: Device path %s returned "
			"from main thread failed to resolve.\n",
			ctxt->path);

		postRegionInitInd(self, ctxt, ERROR_INVALID_RESOURCE_NAME);
		return;
	};

	if (r.initialize() != ERROR_SUCCESS)
	{
		printf(ERROR LZBZCORE"region:main: Failed to initialize local "
			"region metadata.\n");

		postRegionInitInd(self, ctxt, ERROR_INITIALIZATION_FAILURE);
		return;
	};

	/* Add the region-local data to the list to make it accessible to
	 * udi service calls.
	 **/
	dev->instance->regionLocalMetadata.insert(&r);

	self->messageStream.postMessage(
		self->parent->id, 0,
		__klzbzcore::driver::localService::GET_DRIVER_CACHED_INFO,
		NULL, new MainCb(&main1, ctxt, self, &drvInfoCache, &r, dev));

	assert_fatal(
		dev->instance->getRegionInfo(self->getFullId(), &r.index)
			== ERROR_SUCCESS);

	printf(NOTICE LZBZCORE"Region %d, dev %s: Entering event loop.\n",
		r.index, ctxt->path);

	for (;FOREVER;)
	{
		self->messageStream.pull(&iMsg);
		printf(NOTICE"%s dev, region %d, got a message.\n"
			"\tsubsys %d, func %d, sourcetid 0x%x targettid 0x%x\n",
			dev->driverInstance->driver->longName, r.index,
			iMsg->subsystem, iMsg->function,
			iMsg->sourceId, iMsg->targetId);

		switch (iMsg->subsystem)
		{
		case MSGSTREAM_SUBSYSTEM_ZUDI:
			switch (iMsg->function)
			{
			case MSGSTREAM_ZUDI_CHANNEL_SEND:
				channel::handler(
					(fplainn::sChannelMsg *)iMsg,
					self, drvInfoCache, &r);

				break;

			default:
				printf(WARNING LZBZCORE"rgn:main: Unknown ZUDI "
					"message %d.\n",
					iMsg->function);
			};

			break;
		default:
			Callback		*callback;

			callback = (Callback *)iMsg->privateData;
			if (callback == NULL)
			{
				printf(WARNING LZBZCORE"region:main %s, "
					"region %d: unknown message with no "
					"callback continuation.\n",
					dev->driverInstance->driver->longName,
					r.index);

				break;
			}

			(*callback)(iMsg);
			delete callback;
			break;
		};
	};

	taskTrib.dormant(self->getFullId());
}

void __klzbzcore::region::main1(
	MessageStream::sHeader *msg,
	fplainn::Zudi::sKernelCallMsg *ctxt,
	Thread *self, __klzbzcore::driver::CachedInfo **drvInfoCache,
	lzudi::sRegion *r,
	fplainn::Device *dev
	)
{
	// Shall always be at least udi_init_context_t bytes large.
	udi_size_t		rdataSize=sizeof(udi_init_context_t);
	sbit8			rdataSizeFound=0;

	/* Force a sync operation with the main thread. We can't continue until
	 * we can guarantee that the main thread has filled out our region
	 * information in our device-instance object.
	 *
	 * Otherwise it will be filled with garbage and we will read garbage
	 * data.
	 **/
	*drvInfoCache = (__klzbzcore::driver::CachedInfo *)
		((MessageStream::sPostMsg *)msg)->data;

	// We can allocate the region data now however.
	if (r->index == 0)
	{
		rdataSize += (*drvInfoCache)->initInfo
			->primary_init_info->rdata_size;

		rdataSizeFound = 1;
	}
	else
	{
		udi_secondary_init_t		*currRegion;

		currRegion = (*drvInfoCache)->initInfo->secondary_init_list;
		for (; currRegion != NULL && currRegion->region_idx != 0;
			currRegion++)
		{
			if (currRegion->region_idx != r->index) { continue; };

			rdataSizeFound = 1;
			rdataSize += currRegion->rdata_size;
			break;
		};
	};

	if (!rdataSizeFound)
	{
		printf(FATAL LZBZCORE"region:main1 %d: No matching "
			"udi_secondary_init_t for this region.\n",
			r->index);

		postRegionInitInd(self, ctxt, ERROR_UNINITIALIZED);
		return;
	};

	r->rdata = new (malloc(rdataSize)) udi_init_context_t;
	if (r->rdata == NULL)
	{
		printf(FATAL LZBZCORE"region:main1 %d: Failed to alloc rdata.\n",
			r->index);

		postRegionInitInd(self, ctxt, ERROR_MEMORY_NOMEM);
		return;
	};

	self->messageStream.postMessage(
		self->parent->id,
		0, __klzbzcore::driver::localService::REGION_INIT_SYNC_REQ,
		NULL, new MainCb(&main2, ctxt, self, drvInfoCache, r, dev));
}

void __klzbzcore::region::main2(
	MessageStream::sHeader *,
	fplainn::Zudi::sKernelCallMsg *ctxt,
	Thread *self, __klzbzcore::driver::CachedInfo **drvInfoCache,
	lzudi::sRegion *r,
	fplainn::Device *dev
	)
{
	error_t					err;

	assert_fatal(
		dev->instance->getRegionInfo(self->getFullId(), &r->index)
			== ERROR_SUCCESS);

	printf(NOTICE LZBZCORE"region:main2 Device %s, region %d running: rdata 0x%x.\n",
		ctxt->path, r->index, r->rdata);

	/**	EXPLANATION:
	 * Next, we spawn all internal bind channels.
	 * internal_bind_ops meta_idx region_idx ops0_idx ops1_idx1 bind_cb_idx.
	 * Small sanity check here before moving on.
	 **/

	// Now spawn this region's internal bind channel.
	if (r->index != 0)
	{
		ubit16				metaIndex, opsIndex0, opsIndex1,
						bindCbIndex;
		udi_ops_vector_t		*opsVector0=NULL,
						*opsVector1=NULL;
		udi_ops_init_t			*ops_info_tmp;
		fplainn::Endpoint		*__kibindEndp;
		lzudi::sEndpointContext		*endpContext;
		fplainn::Driver::sMetalanguage	*iBopMeta;
		const sMetaInitEntry		*iBopMetaInfo;

		err = floodplainn.zudi.getInternalBopInfo(
			r->index, &metaIndex, &opsIndex0, &opsIndex1,
			&bindCbIndex);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZBZCORE"region:main2 %d: Failed to get "
				"internal bops vector indexes.\n",
				r->index);

			postRegionInitInd(self, ctxt, ERROR_INVALID_ARG_VAL);
			return;
		};

		// Look up both opsVectors using their ops_idxes.
		ops_info_tmp = (*drvInfoCache)->initInfo->ops_init_list;
		for (; ops_info_tmp != NULL && ops_info_tmp->ops_idx != 0;
			ops_info_tmp++)
		{
			if (ops_info_tmp->ops_idx == opsIndex0)
				{ opsVector0 = ops_info_tmp->ops_vector; };

			if (ops_info_tmp->ops_idx == opsIndex1)
				{ opsVector1 = ops_info_tmp->ops_vector; };
		};

		if (opsVector0 == NULL || opsVector1 == NULL)
		{
			printf(ERROR LZBZCORE"region:main2 %d: Failed to get "
				"ops vector addresses for intern. bind chan.\n",
				r->index);

			postRegionInitInd(self, ctxt, ERROR_INVALID_ARG_VAL);
			return;
		};

		/* We are going to need the metaName and udi_meta_info for
		 * the local endpoint-context structure.
		 **/
		iBopMeta = dev->driverInstance->driver->getMetalanguage(
			metaIndex);

		if (iBopMeta == NULL)
		{
			printf(ERROR LZBZCORE"region:main2 %d: Failed to get "
				"metalanguage info for ibop.\n",
				r->index);

			postRegionInitInd(self, ctxt, ERROR_NOT_FOUND);
			return;
		};

		// Now get the udi_meta_info.
		iBopMetaInfo = floodplainn.zudi.findMetaInitInfo(
			iBopMeta->name);

		if (iBopMetaInfo == NULL)
		{
			printf(ERROR LZBZCORE"region:main2 %d: kernel has no "
				"support for meta %s used in ibop.\n",
				r->index, iBopMeta->name);

			postRegionInitInd(self, ctxt, ERROR_UNSUPPORTED);
			return;
		};

		err = floodplainn.zudi.spawnInternalBindChannel(
			ctxt->path, r->index, opsVector0, opsVector1,
			&__kibindEndp);

		endpContext = new lzudi::sEndpointContext(
			__kibindEndp, iBopMeta->name,
			iBopMetaInfo->udi_meta_info,
			opsIndex1, bindCbIndex, r->rdata);

		// Unify the error checking for both of these.
		if (err != ERROR_SUCCESS || endpContext == NULL)
		{
			printf(ERROR LZBZCORE"region:main2 %d: Failed to spawn "
				"internal bind channel because %s(%d).\n"
				"\tOR, failed to alloc local sEndpoint context "
				"for ibindChan.\n",
				r->index, strerror(err), err);

			postRegionInitInd(self, ctxt, ERROR_INITIALIZATION_FAILURE);
			return;
		};

		/* Finally, set the endpContext as the channel's channelPrivData
		 * and conclude by adding the endpContext to the local region-
		 * data's endpoint list.
		 **/
		floodplainn.zudi.setEndpointPrivateData(
			__kibindEndp, endpContext);

		r->endpoints.insert(endpContext);
	};

	/**	EXPLANATION:
	 * Now we spawn the child bind channels to the device's parents.
	 * Run through all known parents, and for each parent, if that parent's
	 * target region is us, we proceed to spawn the child-bind-chan.
	 *
	 * If the target region is not us, we ignore the parent_bind_ops
	 * directive. The correct thread will eventually pick it up.
	 *
	 **	CAVEAT:
	 * This whole sequence is UNTESTED.
	 **/
	for (uarch_t i=0; i<dev->getNParentTags(); i++)
	{
		sbit8					matchedInParent=0,
							matchedInChild=0,
							isCorrectRegion=0;
		fplainn::Device::sParentTag		*pTag;
		fplainn::Device				*parentDev;
		fplainn::Driver				*drv, *parentDrv;
		fplainn::Endpoint			*__kpbindEndp;
		lzudi::sEndpointContext			*pbindEndpContext;
		const sMetaInitEntry			*pBopMetaInfo;
		fplainn::Driver::sParentBop		*correctPBop;
		utf8Char				parentFullName[
			FVFS_TAG_NAME_MAXLEN + 1];

		drv = dev->driverInstance->driver;
		pTag = dev->indexedGetParentTag(i);
		// Should never happen, though.
		if (pTag == NULL) {
			postRegionInitInd(self, ctxt, ERROR_NOT_FOUND); return;
		};

		err = pTag->tag->getFullName(
			parentFullName, sizeof(parentFullName) - 1);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZBZCORE"reg:main2 %d: Unable to get "
				"fullname of\n\tparent %d (name %s).\n",
				r->index, pTag->id, pTag->tag->getName());

			postRegionInitInd(self, ctxt, err); return;
		};

		// Get a handle to the parent device.
		err = floodplainn.getDevice(parentFullName, &parentDev);
		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZBZCORE"reg:main2 %d: Unable to get "
				"handle to parent %d (%s).\n",
				r->index, pTag->id, parentFullName);

			postRegionInitInd(self, ctxt, err); return;
		};

		parentDrv = parentDev->driverInstance->driver;

		/**	EXPLANATION:
		 * The parent, when it enumerated this child, would have also
		 * enumerated a metalanguage to go along with it. This is the
		 * metalanguage that the parent expects the child to connect
		 * bind to it with.
		 *
		 * We must see whether or not the child exposes a
		 * parent_bind_ops that matches this metalanguage that the
		 * parent expects it to bind with.
		 *
		 * If it does not, then we cannot reasonably expect the child to
		 * bind. And in that case, the child will be cut off from I/O
		 * with its parent, so in all, this is a fatal condition.
		 *
		 **	SEQUENCE:
		 * Cycle through all parentBops and compare the meta_idx's
		 * metaName to the parent's expected bind-meta.
		 **/
		for (uarch_t j=0; j<drv->nParentBops; j++)
		{
			fplainn::Driver::sParentBop	*childPBop;
			fplainn::Driver::sMetalanguage	*childMeta;

			childPBop = &drv->parentBops[j];
			childMeta = drv->getMetalanguage(childPBop->metaIndex);

			if (!strncmp8(
				childMeta->name, pTag->udi.metaName,
				DRIVER_METALANGUAGE_MAXLEN))
			{
				matchedInChild = 1;

				/* Secondarily, we need to know whether or not
				 * the bind channel should be anchored to THIS
				 * REGION. Or else, we'll have to leave it alone
				 * for the right region-thread anyway.
				 **/
				if (childPBop->regionIndex == r->index) {
					isCorrectRegion = 1;
				};

				correctPBop = childPBop;
				break;
			};
		};

		/* If we are not the region that the parent-bind channel should
		 * be anchored to, we need to skip this parent tag and leave it
		 * up to the correct region to do it.
		 **/
		if (!isCorrectRegion) { continue; };

		/**	EXPLANATION:
		 * This next sequence is not strictly necessary. We are going to
		 * check to see if the parent exposes a childBop that matches
		 * the one that it enumerated this child with.
		 *
		 * This is really just pedantic double-checking/validation, and
		 * it's not a required step. If the check fails though, it is
		 * fatal, but we can skip the check and assume it won't fail.
		 * (It can fail, but it's unlikely that driver writers would
		 * make this mistake).
		 **/
		for (uarch_t j=0; j<parentDrv->nChildBops; j++)
		{
			fplainn::Driver::sChildBop	*parentCBop;
			fplainn::Driver::sMetalanguage	*parentMeta;

			parentCBop = &parentDrv->childBops[j];
			parentMeta = parentDrv->getMetalanguage(
				parentCBop->metaIndex);

			if (!strncmp8(
				parentMeta->name, pTag->udi.metaName,
				DRIVER_METALANGUAGE_MAXLEN))
			{
				matchedInParent = 1;
				break;
			};
		};

		if (!matchedInParent || !matchedInChild)
		{
			printf(ERROR LZBZCORE"reg:main2 %d: Failed to find the "
				"device's detecting meta in both child and "
				"parent devices.\n\tCannot establish "
				"child bind channel to parent %s.\n",
				r->index, parentFullName);

			postRegionInitInd(self, ctxt, ERROR_NOT_FOUND); return;
		};

		pBopMetaInfo = floodplainn.zudi.findMetaInitInfo(
			pTag->udi.metaName);

		if (pBopMetaInfo == NULL)
		{
			printf(ERROR LZBZCORE"reg:main2 %d: Kernel doesn't "
				"support meta %s needed to bind to parent %s.\n",
				r->index, pTag->udi.metaName,
				parentFullName);

			postRegionInitInd(self, ctxt, ERROR_INVALID_RESOURCE_NAME);
			return;
		};

		printf(NOTICE"reg:main2 %d: Binding to parent %s, meta %s.\n",
			r->index, parentFullName, pTag->udi.metaName);

		/**	FIXME:
		 * This whole child-bind-channel spawning sequence will have
		 * to be moved over to devicePath.cpp, and executed from
		 * within a region thread instead.
		 *
		 * Specifically, each region thread should cycle through all
		 * of the device's parent bops. For all those parent bops that
		 * are to be tied to that thread's region, that thread will
		 * then call spawnChildBindChannel().
		 *
		 * The reason we force the target region thread to execute the
		 * spawnChildBindChannel() call is because that way, we can take
		 * the ENDPOINT-handle that is returned by spawnChildBindChannel
		 * and immediately fill it in to the local region data that that
		 * thread maintains.
		 **/
		err = floodplainn.zudi.spawnChildBindChannel(
			parentFullName, ctxt->path,
			pTag->udi.metaName,
			(udi_ops_vector_t *)0xF00115,
			&__kpbindEndp);

		pbindEndpContext = new lzudi::sEndpointContext(
			__kpbindEndp, pTag->udi.metaName,
			pBopMetaInfo->udi_meta_info, correctPBop->opsIndex,
			correctPBop->bindCbIndex,
			r->rdata);

		if (err != ERROR_SUCCESS || pbindEndpContext == NULL)
		{
			printf(NOTICE LZBZCORE"reg:main2 %d: Failed to spawn "
				"cbind chan; err %d.\n"
				"\tOR, failed to allocate local endpoint "
				"context.\n",
				r->index, err);

			postRegionInitInd(self, ctxt, ERROR_CRITICAL); return;
		};

		floodplainn.zudi.setEndpointPrivateData(
			__kpbindEndp, pbindEndpContext);

		r->endpoints.insert(pbindEndpContext);
	};

	/* Send a message to the main thread, letting it know that a region
	 * thead's initialization is now done.
	 **/
	postRegionInitInd(self, ctxt, ERROR_SUCCESS);
}

void __klzbzcore::region::channel::handler(
	fplainn::sChannelMsg *msg,
	Thread *self,
	__klzbzcore::driver::CachedInfo *drvInfoCache,
	lzudi::sRegion *r
	)
{
	lzudi::sEndpointContext					*endpContext;

	endpContext = (lzudi::sEndpointContext *)msg->endpointPrivateData;

	if (endpContext == NULL)
	{
		/* Allocate lzudi::sEndpointContext here. Then call
		 * FloodplainnStream::setEndpointContext() to seal the deal.
		 **/
		printf(NOTICE"channel message, but no channel context.\n");

#if 0
		endpContext = new lzudi::sEndpointContext(
			msg->__kendpoint,
			META_NAME, META_INFO, OPS_IDX,
			0, CHANNEL_CONTEXT);

		if (endpContext == NULL)
		{
			printf(ERROR LZBZCORE"rgn:handler %s,rgn%d: failed to "
				"demand alloc endpointContext.\n",
				self->getRegion()->parent->device->longName,
				r->index);

			return;
		};

		floodplainn.zudi.setEndpointPrivateData(
			msg->__kendpoint, endpContext);

		// Finally, add it to the list on the region-local metadata.
		r->endpoints.insert(endpContext);
#else
		return;
#endif
	};

	/* The opsIndex 0 should only ever be called by the MA, and regardless
	 * of which channel the ZUM MA makes a call on, it will always set the
	 * metaName to be udi_mgmt. The metaName "udi_mgmt" is the signature
	 * of a channel message from the ZUM MA.
	 **/
	if (msg->opsIndex == 0
		&& strncmp8(
			msg->metaName, CC"udi_mgmt",
			DRIVER_METALANGUAGE_MAXLEN) != 0)
	{
		printf(ERROR LZBZCORE"rgn:handler %s,rgn%d: ops_idx for "
			"channel message is 0, but the metaName is not "
			"udi_mgmt.\n",
			self->getRegion()->parent->device->longName,
			r->index);

		return;
	};

	/* Now that we are guaranteed to have an sEndpointContext, we can do
	 * some double-checking. The metalanguage name from both ends of the
	 * channel must match. The only exception is where the channel message
	 * is a udi_channel_event_ind call, in which case the metaName will be
	 * udi_mgmt.
	 **/
	if (msg->opsIndex != 0
		&& strncmp8(
			endpContext->metaName, msg->metaName,
			DRIVER_METALANGUAGE_MAXLEN) != 0)
	{
		printf(ERROR LZBZCORE"rgn:handler %s,rgn%d: channel is "
			"anchored with a %s ops vector, but a %s message came "
			"in on it. Aborting.\n",
			self->getRegion()->parent->device->longName,
			r->index,
			endpContext->metaName, msg->metaName);

		return;
	};

	/* Now we can try to determine what kind of channel message this is.
	 *	* If the metaName is "udi_mgmt", then this is a MA call into the
	 *	  driver.
	 *		* If the opsIndex of the call is 0, it is a MA call
	 *		  for a udi_channel_event_ind operation.
	 *		* If the opsIndex of the call is not 0, it is a MA call
	 *		  for a udi_mgmt_ops_t operation.
	 *	* If the metaName is not "udi_mgmt", this is a standard IPC
	 *	  call.
	 **/
	if (!strncmp8(msg->metaName, CC"udi_mgmt", DRIVER_METALANGUAGE_MAXLEN)) {
		printf(NOTICE"MGMT MA call.");
	}
	else {
		printf(NOTICE"Standard IPC call.");
	};
}
