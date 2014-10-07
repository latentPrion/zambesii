
#include <__kstdlib/callback.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/memory>
#include <commonlibs/libzbzcore/libzbzcore.h>
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

public:
	MainCb(
		__kmainCbFn *kcb,
		fplainn::Zudi::sKernelCallMsg *ctxtMsg,
		Thread *self,
		__klzbzcore::driver::CachedInfo **drvInfoCache,
		fplainn::Device *dev)
	: _Callback<__kmainCbFn>(kcb),
	ctxtMsg(ctxtMsg), self(self), dev(dev), drvInfoCache(drvInfoCache)
	{}

	virtual void operator()(MessageStream::sHeader *msg)
		{ function(msg, ctxtMsg, self, drvInfoCache, dev); }
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
	MessageStream::sHeader			*iMsg;
	fplainn::Device				*dev;
	udi_init_context_t			*rdata;
	ubit16					regionIndex;
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

	self->messageStream.postMessage(
		self->parent->id, 0,
		__klzbzcore::driver::localService::GET_DRIVER_CACHED_INFO,
		NULL, new MainCb(&main1, ctxt, self, &drvInfoCache, dev));

	assert_fatal(
		dev->instance->getRegionInfo(
			self->getFullId(), &regionIndex, &rdata)
			== ERROR_SUCCESS);

	printf(NOTICE LZBZCORE"Region %d, dev %s: Entering event loop.\n",
		regionIndex, ctxt->path);

	for (;FOREVER;)
	{
		self->messageStream.pull(&iMsg);
		printf(NOTICE"%s dev, region %d, got a message.\n"
			"\tsubsys %d, func %d, sourcetid 0x%x targettid 0x%x\n",
			dev->driverInstance->driver->longName, regionIndex,
			iMsg->subsystem, iMsg->function,
			iMsg->sourceId, iMsg->targetId);

		switch (iMsg->subsystem)
		{
		case MSGSTREAM_SUBSYSTEM_ZUDI:
			switch (iMsg->function)
			{
			case MSGSTREAM_ZUDI_MGMT_CALL:
				mgmt::handler(
					(fplainn::Zudi::sMgmtCallMsg *)iMsg,
					dev, regionIndex);

				break;
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
					regionIndex);

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
	fplainn::Device *dev
	)
{
	/* Force a sync operation with the main thread. We can't continue until
	 * we can guarantee that the main thread has filled out our region
	 * information in our device-instance object.
	 *
	 * Otherwise it will be filled with garbage and we will read garbage
	 * data.
	 **/
	*drvInfoCache = (__klzbzcore::driver::CachedInfo *)
		((MessageStream::sPostMsg *)msg)->data;

	self->messageStream.postMessage(
		self->parent->id,
		0, __klzbzcore::driver::localService::REGION_INIT_SYNC_REQ,
		NULL, new MainCb(&main2, ctxt, self, drvInfoCache, dev));
}

void __klzbzcore::region::main2(
	MessageStream::sHeader *,
	fplainn::Zudi::sKernelCallMsg *ctxt,
	Thread *self, __klzbzcore::driver::CachedInfo **drvInfoCache,
	fplainn::Device *dev
	)
{
	udi_init_context_t			*rdata;
	ubit16					regionIndex;
	error_t					err;

	assert_fatal(
		dev->instance->getRegionInfo(
			self->getFullId(), &regionIndex, &rdata)
			== ERROR_SUCCESS);

	printf(NOTICE LZBZCORE"Device %s, region %d running: rdata 0x%x.\n",
		ctxt->path, regionIndex, rdata);

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

		err = floodplainn.zudi.getInternalBopVectorIndexes(
			regionIndex, &opsIndex0, &opsIndex1);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZBZCORE"regionEntry %d: Failed to get "
				"internal bops vector indexes.\n",
				regionIndex);

			postRegionInitInd(self, ctxt, ERROR_INVALID_ARG_VAL);
			return;
		};

		ops_info_tmp = (*drvInfoCache)->initInfo->ops_init_list;
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

			postRegionInitInd(self, ctxt, ERROR_INVALID_ARG_VAL);
			return;
		};

		err = floodplainn.zudi.spawnInternalBindChannel(
			ctxt->path, regionIndex, opsVector0, opsVector1);

		if (err != ERROR_SUCCESS)
		{
			printf(ERROR LZBZCORE"regionEntry %d: Failed to spawn "
				"internal bind channel because %s(%d).\n",
				regionIndex, strerror(err), err);

			postRegionInitInd(self, ctxt, err);
			return;
		};
	};

	/* Send a message to the main thread, letting it know that a region
	 * thead's initialization is now done.
	 **/
	postRegionInitInd(self, ctxt, ERROR_SUCCESS);
}

struct sUdi_Mgmt_ContextBlock
{
	sUdi_Mgmt_ContextBlock(fplainn::Zudi::sMgmtCallMsg *msg)
	:
	privateData(msg->header.privateData), sourceTid(msg->header.sourceId)
	{
		strncpy8(this->devicePath, msg->path, FVFS_PATH_MAXLEN);
	}

	void		*privateData;
	processId_t	sourceTid;
	utf8Char	devicePath[FVFS_PATH_MAXLEN];
};

error_t __klzbzcore::region::mgmt::handler(
	fplainn::Zudi::sMgmtCallMsg *msg,
	fplainn::Device *dev, ubit16 regionIndex
	)
{
	udi_init_t		*initInfo;
	udi_mgmt_ops_t		*mgmtOps;
	sUdi_Mgmt_ContextBlock	*contextBlock;

	initInfo = dev->driverInstance->driver->driverInitInfo;
	mgmtOps = initInfo->primary_init_info->mgmt_ops;

	contextBlock = new sUdi_Mgmt_ContextBlock(msg);
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
	case fplainn::Zudi::sMgmtCallMsg::MGMTOP_USAGE:
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
	sUdi_Mgmt_ContextBlock		*contextBlock;

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
	contextBlock = (sUdi_Mgmt_ContextBlock *)cb->gcb.initiator_context;

	//floodplainn.zudi.udi_usage_res(
	//	contextBlock->devicePath,
	//	contextBlock->sourceTid, contextBlock->privateData);

	delete contextBlock;
}
